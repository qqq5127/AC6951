#include "gpio.h"
#include "app_config.h"
#include "system/includes.h"
#include "chgbox_ctrl.h"
#include "asm/adc_api.h"
#include "chgbox_box.h"
#include "device/chargebox.h"
#include "chgbox_handshake.h"

#if (TCFG_CHARGE_BOX_ENABLE && TCFG_HANDSHAKE_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHG_HS]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

void chgbox_timer_delay_us(u8 us);
static u8  hs_cur_sys_clk = 0;   //当前时钟匹配值
static u32 hs_small_clk = 0;     //小于48m的系统时钟,记录，用来恢复
static u8 chgbox_hs_repeat_times = 0;  //重复次数
static int hs_timer;

static struct _hs_hdl hs_ctrl = {
    .port = TCFG_HANDSHAKE_IO,                //初始化IO
    .send_delay_us = chgbox_timer_delay_us,   //注册延时函数
};

/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手初始化
   @param    无
   @return   无
   @note     初始化io及注册延时
*/
/*------------------------------------------------------------------------------------*/
void chgbox_handshake_init(void)
{
    handshake_ctrl_init(&hs_ctrl);
}

static u16 delay_tap[HS_DELAY_240M + 1][HS_DELAY_16US + 1] = {
    19, 39, 52, 141, 158, 309, 355,//48m
    23, 45, 59, 146, 165, 315, 359,//60m
    27, 51, 66, 148, 170, 318, 367,//80m
    30, 55, 73, 151, 173, 320, 369,//96m
    32, 58, 75, 154, 177, 323, 370,//120m
    37, 60, 79, 157, 178, 325, 374,//160m
    38, 62, 82, 158, 179, 327, 376,//192m
    40, 64, 84, 159, 184, 328, 378,//240m
};

/*------------------------------------------------------------------------------------*/
/**@brief    获取当前的时钟,设置对应的表值
   @param    无
   @return   无
   @note     该值匹配delay_tap
*/
/*------------------------------------------------------------------------------------*/
static void set_hs_cur_sys_clk(void)
{
    u32 cur_clock = 0;
    cur_clock = clk_get("sys");
#if (TCFG_CLOCK_SYS_SRC == SYS_CLOCK_INPUT_PLL_RCL)
    if (cur_clock != 48000000) { //转成48m，结束后恢复原来的时钟
        hs_small_clk = cur_clock;
        clk_set("sys", 48 * 1000000L);
        hs_cur_sys_clk = HS_DELAY_48M;
        puts("rest 48m\n");
        if (clk_get("lsb") != 48000000L) {
            ASSERT(0, "clock err: %d", clk_get("lsb"));
        }
    }
#else
    if (cur_clock < 48000000) { //小于48m时，转成48m，结束后恢复原来的时钟
        hs_small_clk = cur_clock;
        clk_set("sys", 48 * 1000000L);
        hs_cur_sys_clk = HS_DELAY_48M;
        puts("rest 48m\n");
    } else {
        switch (cur_clock) {
        case 48000000:
            hs_cur_sys_clk = HS_DELAY_48M;
            break;
        case 60000000:
            hs_cur_sys_clk = HS_DELAY_60M;
            break;
        case 80000000:
            hs_cur_sys_clk = HS_DELAY_80M;
            break;
        case 96000000:
            hs_cur_sys_clk = HS_DELAY_96M;
            break;
        case 120000000:
            hs_cur_sys_clk = HS_DELAY_120M;
            break;
        case 160000000:
            hs_cur_sys_clk = HS_DELAY_160M;
            break;
        case 192000000:
            hs_cur_sys_clk = HS_DELAY_192M;
            break;
        case 240000000:
            hs_cur_sys_clk = HS_DELAY_240M;
            break;
        }
    }
#endif
}

/*------------------------------------------------------------------------------------*/
/**@brief    握手后重设置时钟
   @param    无
   @return   无
   @note     有改过时钟才重设置
*/
/*------------------------------------------------------------------------------------*/
static void after_handshake_resume_small_clk(void)
{
    u32 cur_clock = 0;
    if (hs_small_clk) {
        clk_set("sys", hs_small_clk);
        hs_small_clk = 0;//清0
        cur_clock = clk_get("sys");
        printf("after handshake app :%d\n", cur_clock);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手延时
   @param    无
   @return   无
   @note     提供不同的ms级延时
*/
/*------------------------------------------------------------------------------------*/
static void chgbox_timer_delay_ms(u8 ms)
{
    //delay 值要根据不同的频率去调整，小于48m的要先设置48m，方便延时
    JL_TIMER2->CNT = 0;
#if (TCFG_CLOCK_SYS_SRC == SYS_CLOCK_INPUT_PLL_RCL)
    JL_TIMER2->PRD = ms * clk_get("lsb") / (2 * 1000);
    JL_TIMER2->CON = BIT(0) | BIT(6) | BIT(14); //系统时钟,lsb, div 2
#else
    JL_TIMER2->PRD = ms * clk_get("timer") / 1000;
    JL_TIMER2->CON = BIT(0) | BIT(3) | BIT(14); //1分频,osc时钟,24m，24次就1us
#endif
    while (!(JL_TIMER2->CON & BIT(15))); //等pending
    JL_TIMER2->CON = 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手延时
   @param    无
   @return   无
   @note     提供不同的us级延时
*/
/*------------------------------------------------------------------------------------*/
SEC(.chargebox_code)
void chgbox_timer_delay_us(u8 us)
{
    //delay 值要根据不同的频率去调整，小于48m的要先设置48m，方便延时
    JL_TIMER2->CNT = 0;
#if (TCFG_CLOCK_SYS_SRC == SYS_CLOCK_INPUT_PLL_RCL)
    JL_TIMER2->PRD = delay_tap[hs_cur_sys_clk][us];//这里不允许做乘除法
    JL_TIMER2->CON = BIT(0) | BIT(6) | BIT(14); //系统时钟,lsb, div 2, 必须为24M
#else
    JL_TIMER2->PRD = delay_tap[hs_cur_sys_clk][us];
    JL_TIMER2->CON = BIT(0) | BIT(3) | BIT(14); //1分频,osc时钟,24m，24次就1us
#endif
    while (!(JL_TIMER2->CON & BIT(15))); //等pending
    JL_TIMER2->CON = 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手
   @param    无
   @return   无
   @note     注意：为了精确的时间，会关闭其他中断
*/
/*------------------------------------------------------------------------------------*/
void chgbox_handshake_run_app(void)
{
    set_hs_cur_sys_clk();

    local_irq_disable();
    chgbox_timer_delay_ms(2);
    handshake_send_app(HS_CMD0);
    chgbox_timer_delay_ms(2);
    handshake_send_app(HS_CMD1);
    chgbox_timer_delay_ms(2);
    handshake_send_app(HS_CMD2);
    chgbox_timer_delay_ms(2);
    handshake_send_app(HS_CMD3);
    local_irq_enable();

    after_handshake_resume_small_clk();
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting重复握手
   @param    null
   @return   无
   @note     用定时器实现重复握手增加握手成功几率
*/
/*------------------------------------------------------------------------------------*/
static void chgbox_handshake_deal(void *priv)
{
    chgbox_handshake_run_app();
    chgbox_hs_repeat_times--;
    if (chgbox_hs_repeat_times == 0) {
        sys_timer_del(hs_timer);
        hs_timer = 0;
        app_chargebox_event_to_user(CHGBOX_EVENT_HANDSHAKE_OK);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting重复握手初始化
   @param    times:重复次数
   @return   无
   @note     初始化多次握手的次数
*/
/*------------------------------------------------------------------------------------*/
void chgbox_handshake_set_repeat(u8 times)
{
    chgbox_hs_repeat_times = times;
    if (chgbox_hs_repeat_times) {
        if (hs_timer == 0) {
            hs_timer = sys_timer_add(NULL, chgbox_handshake_deal, 200);
        }
    } else {
        if (hs_timer) {
            sys_timer_del(hs_timer);
            hs_timer = 0;
        }
    }
}

#endif
