
/*************************************************************
   此文件函数主要是idle模式按键处理和事件处理

	void app_idle_task()
   idle模式主函数

	static int idle_sys_event_handler(struct sys_event *event)
   idle模式系统事件所有处理入口

	static void idle_task_close(void)
	idle模式退出

	该模式只是作为系统空跑的，可以做家关机，把没有用的模块关闭

**************************************************************/
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "tone_player.h"
#include "asm/charge.h"
#include "app_charge.h"
#include "app_main.h"
#include "ui_manage.h"
#include "vm.h"
#include "app_chargestore.h"
#include "user_cfg.h"
#include "ui/ui_api.h"
#include "key_event_deal.h"

#define LOG_TAG_CONST       APP_IDLE
#define LOG_TAG             "[APP_IDLE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


static int timer_printf_1sec = 0;
static u8 is_idle_flag = 0;
static u8 goto_poweron_cnt = 0;
static u8 goto_poweron_flag = 0;

extern u8 get_power_on_status(void);

static void idle_key_poweron_deal(u8 step);
static void idle_app_open_module();

#define POWER_ON_CNT       10
/// idle 是否关闭不用的模块，减少功耗
#define LOW_POWER_IN_IDLE    0


#if LOW_POWER_IN_IDLE
///*----------------------------------------------------------------------------*/
/**@brief  下面处理是为了关闭不需要用的模块，减少系统功耗
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
#include "usb/otg.h"
#include "usb/host/usb_host.h"
#include "asm/power/p33.h"
struct timer_hdl {
    u32 ticks;
    int index;
    int prd;
    u32 fine_cnt;
    void *power_ctrl;
    struct list_head head;
};

static struct timer_hdl hdl;
static u8 suspend_flag = 0;

#define __this  (&hdl)

static const u32 timer_div[] = {
    /*0000*/    1,
    /*0001*/    4,
    /*0010*/    16,
    /*0011*/    64,
    /*0100*/    2,
    /*0101*/    8,
    /*0110*/    32,
    /*0111*/    128,
    /*1000*/    256,
    /*1001*/    4 * 256,
    /*1010*/    16 * 256,
    /*1011*/    64 * 256,
    /*1100*/    2 * 256,
    /*1101*/    8 * 256,
    /*1110*/    32 * 256,
    /*1111*/    128 * 256,
};

#define APP_TIMER_CLK           clk_get("timer")
#define MAX_TIME_CNT            0x7fff
#define MIN_TIME_CNT            0x100

#define TIMER_UNIT_MS           2
#define MAX_TIMER_PERIOD_MS     (1000/TIMER_UNIT_MS)

extern void adc_scan(void *priv);
___interrupt
static void idle_timer2_isr()
{
    static u32 cnt = 0;
    JL_TIMER2->CON |= BIT(14);

    ++cnt;
    if ((cnt % 5) == 0) {
    }

    if (cnt == 500) {
        cnt = 0;
    }
    /* adc_scan(NULL); */
}

static int idle_timer2_init()
{
    u32 prd_cnt;
    u8 index;

    for (index = 0; index < (sizeof(timer_div) / sizeof(timer_div[0])); index++) {
        prd_cnt = TIMER_UNIT_MS * (APP_TIMER_CLK / 1000) / timer_div[index];
        if (prd_cnt > MIN_TIME_CNT && prd_cnt < MAX_TIME_CNT) {
            break;
        }
    }
    __this->index   = index;
    __this->prd     = prd_cnt;

    JL_TIMER2->CNT = 0;
    JL_TIMER2->PRD = prd_cnt; //2ms
    request_irq(IRQ_TIME2_IDX, 3, idle_timer2_isr, 0);
    JL_TIMER2->CON = (index << 4) | BIT(0) | BIT(3);

    log_info("PRD : 0x%x / %d", JL_TIMER2->PRD, clk_get("timer"));

    return 0;
}

static void idle_timer2_uninit()
{
    JL_TIMER2->CON &= ~(BIT(1) | BIT(0));
}

static void idle_timer1_open()
{
    JL_TIMER1->CON |= BIT(0);
}

static void idle_timer1_close()
{
    JL_TIMER1->CON &= ~(BIT(1) | BIT(0));
}

u32 regs_buf[11] = {0};
void resume_some_peripheral()
{
    u32 *regs_ptr = regs_buf;

    if (!suspend_flag) {
        return;
    }

    clk_set("sys", 24 * 1000000L);

    JL_ANA->DAA_CON0 = *regs_ptr++ ;
    JL_ANA->DAA_CON1 = *regs_ptr++ ;
    JL_ANA->DAA_CON2 = *regs_ptr++ ;
    JL_ANA->DAA_CON3 = *regs_ptr++ ;
    JL_ANA->DAA_CON7 = *regs_ptr++ ;

    JL_ANA->ADA_CON0 = *regs_ptr++ ;
    JL_ANA->ADA_CON1 = *regs_ptr++ ;
    JL_ANA->ADA_CON2 = *regs_ptr++ ;
    JL_ANA->ADA_CON3 = *regs_ptr++ ;
    JL_ANA->ADA_CON4 = *regs_ptr++ ;

    suspend_flag = 0;
}

void suspend_some_peripheral()
{
    u32 *regs_ptr = regs_buf;

    if (suspend_flag) {
        return;
    }

    clk_set("sys", 6 * 1000000L);
    SYSVDD_VOL_SEL(SYSVDD_VOL_SEL_102V);
    VDC13_VOL_SEL(VDC13_VOL_SEL_105V);

    *regs_ptr++ = JL_ANA->DAA_CON0;
    *regs_ptr++ = JL_ANA->DAA_CON1;
    *regs_ptr++ = JL_ANA->DAA_CON2;
    *regs_ptr++ = JL_ANA->DAA_CON3;
    *regs_ptr++ = JL_ANA->DAA_CON7;

    *regs_ptr++ = JL_ANA->ADA_CON0;
    *regs_ptr++ = JL_ANA->ADA_CON1;
    *regs_ptr++ = JL_ANA->ADA_CON2;
    *regs_ptr++ = JL_ANA->ADA_CON3;
    *regs_ptr++ = JL_ANA->ADA_CON4;

    JL_ANA->DAA_CON0 = 0;
    JL_ANA->DAA_CON1 = 0;
    JL_ANA->DAA_CON2 = 0;
    JL_ANA->DAA_CON3 = 0;
    JL_ANA->DAA_CON7 = 0;

    JL_ANA->ADA_CON0 = 0;
    JL_ANA->ADA_CON1 = 0;
    JL_ANA->ADA_CON2 = 0;
    JL_ANA->ADA_CON3 = 0;
    JL_ANA->ADA_CON4 = 0;

    suspend_flag = 1;
}

static u8 is_idle_query(void)
{
    return is_idle_flag;
}
#if 0
REGISTER_LP_TARGET(idle_lp_target) = {
    .name = "not_idle",
    .is_idle = is_idle_query,
};
#endif

#endif

static void idle_key_poweron_deal(u8 step)
{
    switch (step) {
    case 0:
        goto_poweron_cnt = 0;
        goto_poweron_flag = 1;
        break;
    case 1:
        log_info("poweron flag:%d cnt:%d\n", goto_poweron_flag, goto_poweron_cnt);
        if (goto_poweron_flag) {
            goto_poweron_cnt++;
            if (goto_poweron_cnt >= POWER_ON_CNT) {
                goto_poweron_cnt = 0;
                goto_poweron_flag = 0;
                app_var.goto_poweroff_flag = 0;
#if LOW_POWER_IN_IDLE
                idle_app_open_module();
#endif

#if TWFG_APP_POWERON_IGNORE_DEV
                app_task_switch_to(APP_POWERON_TASK);
#endif
            }
        }
        break;
    }
}

#if (TWFG_APP_POWERON_IGNORE_DEV == 0)
static u16 ignore_dev_timeout = 0;
static void poweron_task_switch_to_bt(void *priv) //超时还没有设备挂载，则切到蓝牙模式
{
    app_task_switch_to(APP_BT_TASK);
}
#endif

//*----------------------------------------------------------------------------*/
/**@brief    idle 按键消息入口
   @param    无
   @return   1、消息已经处理，不需要发送到common  0、消息发送到common处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int idle_key_event_opr(struct sys_event *event)
{
    int ret = false;
    int key_event = event->u.key.event;
    int key_value = event->u.key.value;

    log_info("key_event:%d \n", key_event);

    switch (key_event) {
    case KEY_POWER_ON:
    case KEY_POWER_ON_HOLD:
        idle_key_poweron_deal(key_event - KEY_POWER_ON);
        ret = true;
        break;
    }
    return ret;
}

//*----------------------------------------------------------------------------*/
/**@brief    idle 模式活跃状态 所有消息入口
   @param    无
   @return   1、当前消息已经处理，不需要发送comomon 0、当前消息不是idle处理的，发送到common统一处理
   @note
*/
/*----------------------------------------------------------------------------*/
extern int app_common_otg_devcie_event(struct sys_event *event);
static int idle_sys_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return idle_key_event_opr(event);

    case SYS_BT_EVENT:
        /* return 0; */
        return true;
    case SYS_DEVICE_EVENT:
#if TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE
        if ((u32)event->arg == DEVICE_EVENT_CHARGE_STORE) {
            app_chargestore_event_handler(&event->u.chargestore);
        }
#endif
        return 0;
    default:
        return true;
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    idle 退出
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void idle_task_close()
{
    UI_HIDE_CURR_WINDOW();
}

//*----------------------------------------------------------------------------*/
/**@brief    idle 重新打开需要的模块
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void idle_app_open_module()
{
#if LOW_POWER_IN_IDLE
    is_idle_flag = 0;
#if (TCFG_LOWPOWER_LOWPOWER_SEL == 0)
    resume_some_peripheral();
#endif

#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
    extern void sdx_dev_detect_timer_add();
    sdx_dev_detect_timer_add();
#endif

#if (TCFG_PC_ENABLE || TCFG_UDISK_ENABLE)
    extern void usb_detect_timer_add();
    usb_detect_timer_add();
#endif

#if TCFG_LINEIN_ENABLE
    extern void linein_detect_timer_add();
    linein_detect_timer_add();
#endif
    if (timer_printf_1sec) {
        sys_timer_del(timer_printf_1sec);
    }
#endif //LOW_POWER_IN_IDLE
}

//*----------------------------------------------------------------------------*/
/**@brief    idle 关闭不需要的模块
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void idle_app_close_module()
{
#if LOW_POWER_IN_IDLE
    int ret = false;
    is_idle_flag = 1;

#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
    extern void sdx_dev_detect_timer_del();
    sdx_dev_detect_timer_del();
#endif

#if (((TCFG_PC_ENABLE) && (!TCFG_USB_PORT_CHARGE)) || ((TCFG_UDISK_ENABLE) && (!TCFG_PC_ENABLE)))
    extern void usb_detect_timer_del();
    extern u32 usb_otg_online(const usb_dev usb_id);
    /* extern int usb_mount_offline(usb_dev usb_id); */
    extern void usb_pause();
    extern int dev_manager_del(char *logo);
    usb_detect_timer_del();
    os_time_dly((TCFG_OTG_DET_INTERVAL + 9) / 10);
    if (usb_otg_online(0) == HOST_MODE) {
#if TCFG_UDISK_ENABLE
        dev_manager_del("udisk0");
        usb_host_unmount(0);
        usb_h_sie_close(0);

        /*
         经过测试发现，有相当一部分在DP/DM设成高阻状态下U盘的电流仍维持在
         20 ~ 30mA，需要把DP设成上拉，DM设成下拉，这些U盘的电流才能降到2mA
         以下。即有部分U盘需要主机维持在空闲时J状态才能进入suspend。
         */
        usb_iomode(1);
        gpio_set_die(IO_PORT_DP, 1);
        gpio_set_pull_up(IO_PORT_DP, 1);
        gpio_set_pull_down(IO_PORT_DP, 0);
        gpio_set_direction(IO_PORT_DP, 1);

        gpio_set_die(IO_PORT_DM, 1);
        gpio_set_pull_up(IO_PORT_DM, 0);
        gpio_set_pull_down(IO_PORT_DM, 1);
        gpio_set_direction(IO_PORT_DM, 1);

        /* usb_mount_offline(0); */
#endif
    } else if (usb_otg_online(0) == SLAVE_MODE) {
#if TCFG_PC_ENABLE
        usb_pause();

        usb_iomode(1);
        gpio_set_die(IO_PORT_DP, 0);
        gpio_set_pull_up(IO_PORT_DP, 0);
        gpio_set_pull_down(IO_PORT_DP, 0);
        gpio_set_direction(IO_PORT_DP, 1);

        gpio_set_die(IO_PORT_DM, 0);
        gpio_set_pull_up(IO_PORT_DM, 0);
        gpio_set_pull_down(IO_PORT_DM, 0);
        gpio_set_direction(IO_PORT_DM, 1);
#endif
    }
#endif

#if TCFG_LINEIN_ENABLE
    extern void linein_detect_timer_del();
    linein_detect_timer_del();
#endif

#if (TCFG_LOWPOWER_LOWPOWER_SEL == 0)
    suspend_some_peripheral();
#endif
#endif
}
//*----------------------------------------------------------------------------*/
/**@brief    idle 启动
   @param    无
   @return
   @note     关闭SD卡 、 usb 、假关机还是软关机
*/
/*----------------------------------------------------------------------------*/
static void idle_app_start()
{
#if LOW_POWER_IN_IDLE
    idle_app_close_module();
#endif

    UI_SHOW_WINDOW(ID_WINDOW_IDLE);

#if (TCFG_CHARGE_ENABLE && !TCFG_CHARGE_POWERON_ENABLE)

#else
    sys_key_event_enable();
#endif
}


//*----------------------------------------------------------------------------*/
/**@brief    idle 主任务
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
void app_idle_task()
{
    int res;
    int msg[32];

    idle_app_start();

    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (idle_sys_event_handler((struct sys_event *)(&msg[1])) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));    //由common统一处理
            }
            break;
        default:
            break;
        }

        if (app_task_exitting()) {
            idle_task_close();
            return;
        }
    }
}

