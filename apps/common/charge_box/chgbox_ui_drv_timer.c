#include "typedef.h"
#include "asm/pwm_led.h"
#include "system/includes.h"
#include "chgbox_ctrl.h"
#include "chargeIc_manage.h"
#include "chgbox_ui.h"
#include "app_config.h"

//使用timer推灯,灭灯才能进入低功耗

#if (TCFG_CHARGE_BOX_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHGBOXUI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if (TCFG_CHARGE_BOX_UI_ENABLE && TCFG_CHGBOX_UI_TIMERLED)

//常亮、常暗、呼吸
enum {
    GHGBOX_LED_MODE_ON,
    GHGBOX_LED_MODE_OFF,
    GHGBOX_LED_MODE_BRE,
    CHGBOX_LED_MODE_FLASH,
};

//呼吸灯的步骤 渐亮--亮--渐暗--暗
enum {
    SOFT_LED_STEP_UP = 0,
    SOFT_LED_STEP_LIGHT,
    SOFT_LED_STEP_DOWN,
    SOFT_LED_STEP_DARK,
};

typedef struct _CHG_SOFT_PWM_LED {
    //初始化，亮暗接口接口
    void (*led_init)(void);
    void (*led_on_off)(u8 on_off);

    u16 bre_times;    //呼吸次数,0xffff为循环

    u16 up_times;     //渐亮次数
    u16 light_times;  //亮次数
    u16 down_times;   //渐暗次数
    u16 dark_times;   //暗次数

    u16 step_cnt;
    u8  step;         //步骤

    u8  p_cnt;        //占空比计数
    u8  cur_duty;     //当前占空比
    u8  max_duty;     //最大占空比，控制最大亮度

    u8  busy;         //忙标志，更换参数时作保护用
    u8  idle;         //LED可以进低功耗

    u8  sp_flicker;    //特殊闪烁标志（用于处理亮-->亮，暗-->暗变化时灯光无反应的现象）
    u16 sp_flicker_cnt;//特殊闪烁维持时间

    u8  mode;         //亮灯模式
} CHG_SOFT_PWM_LED;

#define MC_TIMER_UNIT_US           30   //多少us起一次中断
#define SOFT_MC_PWM_MAX            128  //pwm周期(==MC_TIMER_UNIT_US * SOFT_MC_PWM_MAX 单位us)
#define BRIGHTNESS_MAX             40   //亮度0~100

//注意时间尺度,即是mc_clk初始化出来的时间
#define UP_TIMES_DEFAULT         50
#define DOWN_TIMES_DEFAULT       50
#define SP_FLICKER_CNT_DEFAULT   30 //特殊闪烁时长计数


CHG_SOFT_PWM_LED chgbox_led[CHG_LED_MAX];

//led亮的时候不能进入低功耗
static volatile u8 is_led_active = 0;
static u8 led_idle_query(void)
{
    return (!is_led_active);
}
REGISTER_LP_TARGET(led_lp_target) = {
    .name = "chgbox_led",
    .is_idle = led_idle_query,
};

/*------------------------------------------------------------------------------------*/
/**@brief    设置灯为常亮、常暗模式
   @param    led_type:  灯序号
             on_off:    1-->常亮 0-->常暗
             sp_flicker:是否闪烁标志 1-->在亮--亮 暗--暗过程中取反一下
             fade:      1-->快速响应 0-->渐变响应
   @return   无
   @note     把灯设置为 常亮/常暗 模式(默认同时设置其他灯为常暗)
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_stu(u8 led_type, u8 on_off, u8 sp_flicker, u8 fade)
{
    //只开关一个灯,其他关掉
    u8 i;
    for (i = 0; i < CHG_LED_MAX; i++) {
        chgbox_led[i].busy = 1;

        if (led_type == i) {

            //设置特殊闪烁
            chgbox_led[i].sp_flicker = sp_flicker;
            chgbox_led[i].sp_flicker_cnt = SP_FLICKER_CNT_DEFAULT;

            if (on_off) {
                if (fade) {
                    //要根据当前占空比，把cnt计算好，避免亮度突变
                    chgbox_led[i].step_cnt = chgbox_led[i].up_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
                } else {
                    chgbox_led[i].step_cnt = 0;
                    chgbox_led[i].cur_duty = chgbox_led[i].max_duty;
                    chgbox_led[i].sp_flicker = 0;
                }
                chgbox_led[i].mode = GHGBOX_LED_MODE_ON; //常亮
                chgbox_led[i].up_times = UP_TIMES_DEFAULT;
            } else {
                if (fade) {
                    //要根据当前占空比，把cnt计算好
                    chgbox_led[i].step_cnt = chgbox_led[i].down_times - chgbox_led[i].down_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
                } else {
                    chgbox_led[i].step_cnt = 0;
                    chgbox_led[i].cur_duty = 0;
                    chgbox_led[i].sp_flicker = 0;
                }
                chgbox_led[i].mode = GHGBOX_LED_MODE_OFF; //常暗
                chgbox_led[i].down_times = DOWN_TIMES_DEFAULT;
            }
        } else {
            if (fade) {
                chgbox_led[i].step_cnt = chgbox_led[i].down_times - chgbox_led[i].down_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
            } else {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].cur_duty = 0;
            }
            chgbox_led[i].mode = GHGBOX_LED_MODE_OFF; //其他灯常暗
            chgbox_led[i].sp_flicker = 0;
            chgbox_led[i].down_times = 20;
        }
        is_led_active = 1;
        chgbox_led[i].idle = 0;
        chgbox_led[i].bre_times = 0;
        chgbox_led[i].busy = 0;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    设置灯为呼吸模式
   @param    led_type:  灯序号
             speed_mode:闪烁参数选择，这里默认为快、慢两种
             is_bre:是否为呼吸模式
             time:呼吸/闪烁多少次
   @return   无
   @note     把灯设置为呼吸模式(默认同时其他灯为常暗)
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_bre(u8 led_type, u8 speed_mode, u8 is_bre, u16 time)
{
    //只开关一个灯,其他关掉
    u8 i;
    for (i = 0; i < CHG_LED_MAX; i++) {
        chgbox_led[i].busy = 1;

        if (led_type == i) {
            if (is_bre) {
                chgbox_led[i].mode = GHGBOX_LED_MODE_BRE; //呼吸
            } else {
                chgbox_led[i].mode = CHGBOX_LED_MODE_FLASH; //闪烁
            }
            chgbox_led[i].bre_times = time;         //循环
            if (speed_mode == LED_FLASH_FAST) {
                chgbox_led[i].up_times = 50;
                chgbox_led[i].light_times = 10;
                chgbox_led[i].down_times = 50;
                chgbox_led[i].dark_times = 10;
            } else if (speed_mode == LED_FLASH_SLOW) {
                chgbox_led[i].up_times = 200;
                chgbox_led[i].light_times = 40;
                chgbox_led[i].down_times = 200;
                chgbox_led[i].dark_times = 40;
            }
            //要根据当前占空比，把cnt计算好,
            chgbox_led[i].step_cnt = chgbox_led[i].up_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
        } else {
            chgbox_led[i].mode = GHGBOX_LED_MODE_OFF; //常暗
            chgbox_led[i].down_times = 20;
            chgbox_led[i].step_cnt = chgbox_led[i].down_times - chgbox_led[i].down_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
            chgbox_led[i].bre_times = 0;
        }
        is_led_active = 1;
        chgbox_led[i].idle = 0;
        chgbox_led[i].busy = 0;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    呼吸灯全暗
   @param    fade:是否淡入
   @return   无
   @note     把所有的灯设置为常暗模式
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_all_off(u8 fade)
{
    u8 i;
    for (i = 0; i < CHG_LED_MAX; i++) {
        chgbox_led[i].busy = 1;

        if (fade) {
            chgbox_led[i].step_cnt = chgbox_led[i].down_times - chgbox_led[i].down_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
        } else {
            chgbox_led[i].step_cnt = 0;
            chgbox_led[i].sp_flicker = 0;
            chgbox_led[i].cur_duty = 0;
        }
        chgbox_led[i].mode = GHGBOX_LED_MODE_OFF; //常暗
        chgbox_led[i].down_times = DOWN_TIMES_DEFAULT;
        chgbox_led[i].bre_times = 0;
        chgbox_led[i].idle = 0;
        is_led_active = 1;
        chgbox_led[i].busy = 0;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    呼吸灯全亮
   @param    fade:是否淡入
   @return   无
   @note     把所有的灯设置为常亮模式
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_all_on(u8 fade)
{
    u8 i;
    for (i = 0; i < CHG_LED_MAX; i++) {
        chgbox_led[i].busy = 1;
        is_led_active = 1;
        if (fade) {
            chgbox_led[i].step_cnt = chgbox_led[i].up_times * chgbox_led[i].cur_duty / chgbox_led[i].max_duty;
        } else {
            chgbox_led[i].step_cnt = 0;
            chgbox_led[i].sp_flicker = 0;
            chgbox_led[i].cur_duty = chgbox_led[i].max_duty;
        }
        chgbox_led[i].mode = GHGBOX_LED_MODE_ON; //常亮
        chgbox_led[i].up_times = UP_TIMES_DEFAULT;
        chgbox_led[i].bre_times = 0;
        chgbox_led[i].idle = 0;
        chgbox_led[i].busy = 0;
    }
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//硬件、软pwm驱动部分(时钟、占空比控制等)
//蓝灯初始化
static void chg_red_led_init()
{
#if (CHG_RED_LED_IO != NO_CONFIG_PORT)
    gpio_set_die(CHG_RED_LED_IO, 1);
    gpio_set_pull_down(CHG_RED_LED_IO, 0);
    gpio_set_pull_up(CHG_RED_LED_IO, 0);
    gpio_set_die(CHG_RED_LED_IO, 0);
    gpio_set_dieh(CHG_RED_LED_IO, 0);
    //初始化为暗态
    gpio_direction_output(CHG_RED_LED_IO, !CHG_LED_MODE);
#endif
}

//蓝灯高低电平控制
//on_off:1:输出高平电  0:输出低电平
SEC(.chargebox_code)//频繁调用的放ram里
static void chg_set_red_led(u8 on_off)
{
#if CHG_RED_LED_IO != NO_CONFIG_PORT
    u8 io_num;
    io_num = CHG_RED_LED_IO % 16;

#if (CHG_LED_MODE == 0)
    on_off = !on_off;
#endif

    if (on_off) {
#if(CHG_RED_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT |= BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT |= BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT |= BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTD_07)
        JL_PORTD->OUT |= BIT(io_num);
#elif(CHG_RED_LED_IO == IO_PORT_DP)
        JL_USB_IO->CON0 |= BIT(0);
#elif(CHG_RED_LED_IO == IO_PORT_DM)
        JL_USB_IO->CON0 |= BIT(1);
#endif
    } else {
#if(CHG_RED_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT &= ~BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT &= ~BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT &= ~BIT(io_num);
#elif(CHG_RED_LED_IO <= IO_PORTD_07)
        JL_PORTD->OUT &= ~BIT(io_num);
#elif(CHG_RED_LED_IO == IO_PORT_DP)
        JL_USB_IO->CON0 &= ~BIT(0);
#elif(CHG_RED_LED_IO == IO_PORT_DM)
        JL_USB_IO->CON0 &= ~BIT(1);
#endif
    }
#endif
}

//绿灯初始化
static void chg_green_led_init()
{
#if (CHG_GREEN_LED_IO != NO_CONFIG_PORT)
    gpio_set_die(CHG_GREEN_LED_IO, 1);
    gpio_set_pull_down(CHG_GREEN_LED_IO, 0);
    gpio_set_pull_up(CHG_GREEN_LED_IO, 0);
    gpio_set_die(CHG_GREEN_LED_IO, 0);
    gpio_set_dieh(CHG_GREEN_LED_IO, 0);
    //初始化为暗态
    gpio_direction_output(CHG_GREEN_LED_IO, !CHG_LED_MODE);
#endif
}

//绿灯高低电平控制
//on_off:1:输出高平电  0:输出低电平
SEC(.chargebox_code)//频繁调用的放ram里
static void chg_set_green_led(u8 on_off)
{
#if (CHG_GREEN_LED_IO != NO_CONFIG_PORT)
    u8 io_num;
    io_num = CHG_GREEN_LED_IO % 16;

#if (CHG_LED_MODE == 0)
    on_off = !on_off;
#endif

    if (on_off) {
#if(CHG_GREEN_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT |= BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT |= BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT |= BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTD_07)
        JL_PORTD->OUT |= BIT(io_num);
#elif(CHG_GREEN_LED_IO == IO_PORT_DP)
        JL_USB_IO->CON0 |= BIT(0);
#elif(CHG_GREEN_LED_IO == IO_PORT_DM)
        JL_USB_IO->CON0 |= BIT(1);
#endif

    } else {
#if(CHG_GREEN_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT &= ~BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT &= ~BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT &= ~BIT(io_num);
#elif(CHG_GREEN_LED_IO <= IO_PORTD_07)
        JL_PORTD->OUT &= ~BIT(io_num);
#elif(CHG_GREEN_LED_IO == IO_PORT_DP)
        JL_USB_IO->CON0 &= ~BIT(0);
#elif(CHG_GREEN_LED_IO == IO_PORT_DM)
        JL_USB_IO->CON0 &= ~BIT(1);
#endif
    }
#endif
}

//蓝灯初始化
static void chg_blue_led_init()
{
#if (CHG_BLUE_LED_IO != NO_CONFIG_PORT)
    gpio_set_die(CHG_BLUE_LED_IO, 1);
    gpio_set_pull_down(CHG_BLUE_LED_IO, 0);
    gpio_set_pull_up(CHG_BLUE_LED_IO, 0);
    gpio_direction_output(CHG_BLUE_LED_IO, 0);
    gpio_set_die(CHG_BLUE_LED_IO, 0);
    gpio_set_dieh(CHG_BLUE_LED_IO, 0);
    //初始化为暗态
    gpio_direction_output(CHG_BLUE_LED_IO, !CHG_LED_MODE);
#endif
}

//蓝灯高低电平控制
//on_off:1:输出高平电  0:输出低电平
SEC(.chargebox_code)
static void chg_set_blue_led(u8 on_off)
{
#if (CHG_BLUE_LED_IO != NO_CONFIG_PORT)
    u8 io_num;
    io_num = CHG_BLUE_LED_IO % 16;

#if (CHG_LED_MODE == 0)
    on_off = !on_off;
#endif

    if (on_off) {
#if(CHG_BLUE_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT |= BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT |= BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT |= BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTD_07)
        JL_PORTD->OUT |= BIT(io_num);
#elif(CHG_BLUE_LED_IO == IO_PORT_DP)
        JL_USB_IO->CON0 |= BIT(0);
#elif(CHG_BLUE_LED_IO == IO_PORT_DM)
        JL_USB_IO->CON0 |= BIT(1);
#endif
    } else {
#if(CHG_BLUE_LED_IO <= IO_PORTA_15)
        JL_PORTA->OUT &= ~BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTB_15)
        JL_PORTB->OUT &= ~BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTC_15)
        JL_PORTC->OUT &= ~BIT(io_num);
#elif(CHG_BLUE_LED_IO <= IO_PORTD_07)
        JL_PORTD->OUT &= ~BIT(io_num);
#elif(CHG_BLUE_LED_IO == IO_PORT_DP)
        JL_USB_IO->CON0 &= ~BIT(0);
#elif(CHG_BLUE_LED_IO == IO_PORT_DM)
        JL_USB_IO->CON0 &= ~BIT(1);
#endif
    }
#endif
}

/*------------------------------------------------------------------------------------*/
/**@brief    呼吸灯占空比控制
   @param    i:灯序号
   @return   无
   @note     根据常亮、常暗、呼吸等模式控制占空比
*/
/*------------------------------------------------------------------------------------*/
SEC(.chargebox_code)
static u8 soft_pwm_led_ctrl(u8 i)
{
    u8 ret = 0;
    switch (chgbox_led[i].mode) {
    case GHGBOX_LED_MODE_ON: //常亮
        if (chgbox_led[i].sp_flicker) { //暗一下(包括了渐暗+暗 两个过程)
            if (chgbox_led[i].cur_duty > 0) {
                chgbox_led[i].step_cnt++;
                if (chgbox_led[i].step_cnt >= chgbox_led[i].down_times) {
                    chgbox_led[i].step_cnt = 0;
                    chgbox_led[i].cur_duty  = 0;
                } else {
                    chgbox_led[i].cur_duty = (chgbox_led[i].down_times - chgbox_led[i].step_cnt) * chgbox_led[i].max_duty / chgbox_led[i].down_times;
                }
            } else {
                if (chgbox_led[i].sp_flicker_cnt) { //持续暗的时间
                    chgbox_led[i].sp_flicker_cnt--;
                    if (chgbox_led[i].sp_flicker_cnt == 0) {
                        chgbox_led[i].sp_flicker = 0; //结束暗一下流程
                    }
                }
            }
        } else if (chgbox_led[i].cur_duty < chgbox_led[i].max_duty) {
            chgbox_led[i].step_cnt++;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].up_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].cur_duty  = chgbox_led[i].max_duty;
            } else {
                //这里为了避免灯光突变，根据改变前的亮度来计算了cnt
                chgbox_led[i].cur_duty = chgbox_led[i].step_cnt * chgbox_led[i].max_duty / chgbox_led[i].up_times;
            }
        }
        break;
    case GHGBOX_LED_MODE_OFF://常暗
        if (chgbox_led[i].sp_flicker) { //亮一下
            if (chgbox_led[i].cur_duty < chgbox_led[i].max_duty) {
                chgbox_led[i].step_cnt++;
                if (chgbox_led[i].step_cnt >= chgbox_led[i].up_times) {
                    chgbox_led[i].step_cnt = 0;
                    chgbox_led[i].cur_duty  = chgbox_led[i].max_duty;
                } else {
                    //这里为了避免灯光突变，根据改变前的亮度来计算了cnt
                    chgbox_led[i].cur_duty = chgbox_led[i].step_cnt * chgbox_led[i].max_duty / chgbox_led[i].up_times;
                }
            } else {
                if (chgbox_led[i].sp_flicker_cnt) { //持续亮的时间
                    chgbox_led[i].sp_flicker_cnt--;
                    if (chgbox_led[i].sp_flicker_cnt == 0) {
                        chgbox_led[i].sp_flicker = 0; //结束亮一下流程
                    }
                }
            }
        } else if (chgbox_led[i].cur_duty > 0) {
            chgbox_led[i].step_cnt++;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].down_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].cur_duty  = 0;
            } else {
                //这里为了避免灯光突变，根据改变前的亮度来计算了cnt
                chgbox_led[i].cur_duty = (chgbox_led[i].down_times - chgbox_led[i].step_cnt) * chgbox_led[i].max_duty / chgbox_led[i].down_times;
            }
        } else {
            ret = 1;
        }
        break;
    case GHGBOX_LED_MODE_BRE://呼吸灯模式
        if (chgbox_led[i].bre_times == 0) {
            ret = 1;
            break;
        }

        if (chgbox_led[i].step == SOFT_LED_STEP_UP) {
            chgbox_led[i].step_cnt++;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].up_times) { //当前段结束
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].step++; //进入下一个步骤
            } else {
                chgbox_led[i].cur_duty = chgbox_led[i].step_cnt * chgbox_led[i].max_duty / chgbox_led[i].up_times;
            }
        } else if (chgbox_led[i].step == SOFT_LED_STEP_LIGHT) {
            chgbox_led[i].step_cnt++;
            chgbox_led[i].cur_duty = chgbox_led[i].max_duty;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].light_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].step++;
            }
        } else if (chgbox_led[i].step == SOFT_LED_STEP_DOWN) {
            chgbox_led[i].step_cnt++;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].down_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].step++;
            } else {
                chgbox_led[i].cur_duty = (chgbox_led[i].down_times - chgbox_led[i].step_cnt) * chgbox_led[i].max_duty / chgbox_led[i].down_times;
            }
        } else if (chgbox_led[i].step == SOFT_LED_STEP_DARK) {
            chgbox_led[i].step_cnt++;
            chgbox_led[i].cur_duty = 0;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].dark_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].step = 0;    //重新开始下一次呼吸
                if (chgbox_led[i].bre_times != 0xffff) { //非循环
                    chgbox_led[i].bre_times--; //呼吸次数递减
                }
            }
        }
        break;
    case CHGBOX_LED_MODE_FLASH:
        if (chgbox_led[i].bre_times == 0) {
            ret = 1;
            break;
        }
        if (chgbox_led[i].step == SOFT_LED_STEP_UP) {
            chgbox_led[i].cur_duty = chgbox_led[i].max_duty;
            chgbox_led[i].step_cnt++;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].up_times + chgbox_led[i].light_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].step = SOFT_LED_STEP_DOWN;
            }
        } else if (chgbox_led[i].step == SOFT_LED_STEP_DOWN) {
            chgbox_led[i].cur_duty = 0;
            chgbox_led[i].step_cnt++;
            if (chgbox_led[i].step_cnt >= chgbox_led[i].down_times + chgbox_led[i].dark_times) {
                chgbox_led[i].step_cnt = 0;
                chgbox_led[i].step = SOFT_LED_STEP_UP;
                if (chgbox_led[i].bre_times != 0xffff) { //非循环
                    chgbox_led[i].bre_times--; //次数递减
                }
            }
        } else {
            chgbox_led[i].bre_times = 0;
            chgbox_led[i].cur_duty = 0;
        }
        break;
    }
    return ret;
}

#define TIMER_USE_MC_PWM        0
#if TIMER_USE_MC_PWM == 0
#define JL_TIMERx       JL_TIMER5
#define IRQ_TIMEX_IDX   IRQ_TIME5_IDX
#define TIMERX_IOS_BIT  21
#define TIMERX_IOS_CLK  12000000
#endif

/*------------------------------------------------------------------------------------*/
/**@brief    mc_clk中断回调
   @param    无
   @return   无
   @note     用于循环所有的呼吸灯，包括亮暗控制,占空比设置等
*/
/*------------------------------------------------------------------------------------*/
SEC(.chargebox_code)
___interrupt
void soft_pwm_led_isr(void)
{
#if TIMER_USE_MC_PWM
    JL_MCPWM->TMR0_CON |= BIT(10); //清pending
#else
    JL_TIMERx->CON |= BIT(14);
#endif
    u8 i, led_idle = 0;

    for (i = 0; i < CHG_LED_MAX; i++) { //循环所有的灯
        if (!chgbox_led[i].busy) {
            if (chgbox_led[i].p_cnt < (chgbox_led[i].cur_duty)) { //占空比
                chgbox_led[i].led_on_off(1); //亮
            } else {
                chgbox_led[i].led_on_off(0);
            }
            chgbox_led[i].p_cnt++;
            if (chgbox_led[i].p_cnt >= SOFT_MC_PWM_MAX) { //完成一个PWM周期
                chgbox_led[i].p_cnt = 0;
                chgbox_led[i].idle = soft_pwm_led_ctrl(i);//占空比控制
            }
        }
        led_idle += chgbox_led[i].idle;
    }
    if (led_idle == CHG_LED_MAX) {
        is_led_active = 0;
    }
}

#if TIMER_USE_MC_PWM
static const u32 timer_div_mc[] = {
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768
};
#else
static const u32 timer_div_mc[] = {
    1, 4, 16, 64, 1 * 2, 4 * 2, 16 * 2, 64 * 2, 1 * 256, 4 * 256, 16 * 256, 64 * 256, 1 * 2 * 256, 4 * 2 * 256, 16 * 2 * 256, 64 * 2 * 256
};
#endif
#define MC_MAX_TIME_CNT            0x7fff
#define MC_MIN_TIME_CNT            0x10
/*------------------------------------------------------------------------------------*/
/**@brief    mc_clk的初始化
   @param    无
   @return   无
   @note     初始clk,注册中断,MC_TIMER_UNIT_US 为起中断的时间,单位us
*/
/*------------------------------------------------------------------------------------*/
void mc_clk_init(void)
{
#if TIMER_USE_MC_PWM
    //br25 没有mcpwm
    u32 prd_cnt;
    u8 index;

    JL_MCPWM->TMR0_CON = BIT(10); //清pending,清其他bit
    JL_MCPWM->MCPWM_CON0 = 0;

    for (index = 0; index < (sizeof(timer_div_mc) / sizeof(timer_div_mc[0])); index++) {
        prd_cnt = MC_TIMER_UNIT_US * (clk_get("lsb") / 1000000) / timer_div_mc[index];
        if (prd_cnt > MC_MIN_TIME_CNT && prd_cnt < MC_MAX_TIME_CNT) {
            break;
        }
    }

    JL_MCPWM->TMR0_CNT = 0;
    JL_MCPWM->TMR0_PR = prd_cnt;
    JL_MCPWM->TMR0_CON |= index << 3; //分频系数

    request_irq(IRQ_MCTMRX_IDX, 3, soft_pwm_led_isr, 0);

    JL_MCPWM->TMR0_CON |= BIT(8);  //允许定时溢出中断
    JL_MCPWM->TMR0_CON |= BIT(0);  //递增模式

    JL_MCPWM->MCPWM_CON0 |= BIT(8); //只开mc timer 0
    log_info("prd_cnt:%d,index:%d,t0:%x,MCP:%x\n", prd_cnt, index, JL_MCPWM->TMR0_CON, JL_MCPWM->MCPWM_CON0);
    log_info("lsb:%d\n", clk_get("lsb"));
#else
    u32 timer_clk;
    u32 prd_cnt;
    u8 index;
    JL_TIMERx->CON = BIT(14);
    JL_TIMERx->CNT = 0;

#if (TCFG_CLOCK_SYS_SRC == SYS_CLOCK_INPUT_PLL_RCL)
    timer_clk = TIMERX_IOS_CLK;
#else
    timer_clk = clk_get("timer");
#endif

    for (index = 0; index < (sizeof(timer_div_mc) / sizeof(timer_div_mc[0])); index++) {
        prd_cnt = MC_TIMER_UNIT_US * (timer_clk / 1000000) / timer_div_mc[index];
        if (prd_cnt > MC_MIN_TIME_CNT && prd_cnt < MC_MAX_TIME_CNT) {
            break;
        }
    }
    //初始化timer
    request_irq(IRQ_TIMEX_IDX, 3, soft_pwm_led_isr, 0);
    JL_TIMERx->PRD = prd_cnt;
#if (TCFG_CLOCK_SYS_SRC == SYS_CLOCK_INPUT_PLL_RCL)
    JL_IOMAP->CON0 |= BIT(TIMERX_IOS_BIT);
    JL_TIMERx->CON = (index << 4) | BIT(0) | BIT(2);//省晶振使用pll12m为时钟源
#else
    JL_TIMERx->CON = (index << 4) | (0b10 << 2) | (0b01 << 0);//OSC时钟
#endif
#endif
}

/*------------------------------------------------------------------------------------*/
/**@brief    led呼吸灯初始化
   @param    无
   @return   无
   @note     初始化每个led:渐亮、亮、渐暗、暗，最大亮度，对应IO的初始化.mc_clk的初始化，用于
             控制pwm
*/
/*-----------------------------------------------------------------------------------*/
void chgbox_led_init(void)
{
    u8 i;
    for (i = 0; i < CHG_LED_MAX; i++) { //循环所有的灯
        memset(&chgbox_led[i], 0x0, sizeof(CHG_SOFT_PWM_LED));
        chgbox_led[i].up_times = UP_TIMES_DEFAULT;
        chgbox_led[i].light_times = 100;
        chgbox_led[i].down_times = DOWN_TIMES_DEFAULT;
        chgbox_led[i].dark_times = 10;
        chgbox_led[i].bre_times = 0;

        //可根据需要修改初始化，但要把初始化与亮灭注册进来
        if (i == CHG_LED_RED) {
            chgbox_led[i].led_on_off = chg_set_red_led;
            chgbox_led[i].led_init = chg_red_led_init;
            chgbox_led[i].max_duty = SOFT_MC_PWM_MAX * BRIGHTNESS_MAX / 100;
            chgbox_led[i].mode = GHGBOX_LED_MODE_OFF;
        } else if (i == CHG_LED_GREEN) {
            chgbox_led[i].led_on_off = chg_set_green_led;
            chgbox_led[i].led_init = chg_green_led_init;
            chgbox_led[i].max_duty = SOFT_MC_PWM_MAX * BRIGHTNESS_MAX / 100;
            chgbox_led[i].mode = GHGBOX_LED_MODE_OFF;
        } else if (i == CHG_LED_BLUE) {
            chgbox_led[i].led_on_off = chg_set_blue_led;
            chgbox_led[i].led_init = chg_blue_led_init;
            chgbox_led[i].max_duty = SOFT_MC_PWM_MAX * BRIGHTNESS_MAX / 100;
            chgbox_led[i].mode = GHGBOX_LED_MODE_OFF;
        }
        chgbox_led[i].led_init();
    }
    mc_clk_init();
}

#endif

#endif
