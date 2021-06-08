#include "app_config.h"
#include "key_event_deal.h"
#include "system/includes.h"
#include "tone_player.h"
#include "app_task.h"
#include "tone_player.h"
#include "media/includes.h"
#include "system/sys_time.h"
#include "ui/ui_api.h"
#include "rtc/alarm.h"
#include "rtc/rtc_ui.h"
#include "clock_cfg.h"
#include "ui/ui_style.h"
#include "ui_manage.h"

/*************************************************************
   此文件函数主要是rtc模式按键处理和事件处理

**************************************************************/


#if TCFG_APP_RTC_EN



#define LOG_TAG_CONST       APP_RTC
#define LOG_TAG             "[APP_RTC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"



#define RTC_SET_MODE  0x55
#define ALM_SET_MODE  0xAA

#define RTC_POS_DEFAULT      RTC_POS_YEAR
#define RTC_ALM_POS_DEFAULT  ALM_POS_HOUR
#define RTC_MODE_DEFAULT     RTC_SET_MODE

#define MAX_YEAR          2099
#define MIN_YEAR          2000

enum {
    RTC_POS_NULL = 0,
    RTC_POS_YEAR,
    RTC_POS_MONTH,
    RTC_POS_DAY,
    RTC_POS_HOUR,
    RTC_POS_MIN,
    /* RTC_POS_SEC, */
    RTC_POS_MAX,
    ALM_POS_HOUR,
    ALM_POS_MIN,
    ALM_POS_ENABLE,
    ALM_POS_MAX,
};

struct rtc_opr {
    void *dev_handle;
    u8  rtc_set_mode;
    u8  rtc_pos;
    u8  alm_enable;
    u8  alm_num;
    struct sys_time set_time;
};

static struct rtc_opr *__this = NULL;



const char *alm_string[] =  {" AL ", " ON ", " OFF"};
const char *alm_select[] =  {"AL-1", "AL-2", "AL-3", "AL-4", "AL-5"};




static void ui_set_rtc_timeout(u8 menu)
{
    if (!__this) {
        return ;
    }
    __this->rtc_set_mode =  RTC_SET_MODE;
    __this->rtc_pos = RTC_POS_NULL;
}

struct ui_rtc_display *__attribute__((weak)) rtc_ui_get_display_buf()
{
    return NULL;
}



//*----------------------------------------------------------------------------*/
/**@brief    rtc 闹钟设置 \ 时间设置转换
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void set_rtc_sw()
{
    if ((!__this) || (!__this->dev_handle)) {
        return;
    }


    struct ui_rtc_display *rtc = rtc_ui_get_display_buf();
    if (!rtc) {
        return;
    }
    switch (__this->rtc_set_mode) {
    case RTC_SET_MODE:
        __this->rtc_set_mode = ALM_SET_MODE;
        __this->rtc_pos = RTC_POS_NULL;
        __this->alm_num = 0;
        rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
        rtc->str = alm_select[__this->alm_num];
        UI_SHOW_MENU(MENU_RTC_SET, 10 * 1000, 0, ui_set_rtc_timeout);
        break;

    case ALM_SET_MODE:
        __this->alm_num++;
        __this->rtc_pos = RTC_POS_NULL;
        if (__this->alm_num >= sizeof(alm_select) / sizeof(alm_select[0])) {
            __this->rtc_set_mode = RTC_SET_MODE;
            __this->alm_num = 0;
            UI_REFLASH_WINDOW(true);
            break;
        }
        rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
        rtc->str = alm_select[__this->alm_num];
        UI_SHOW_MENU(MENU_RTC_SET, 10 * 1000, 0, ui_set_rtc_timeout);
        break;

    }
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 设置调整时钟的位置
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void set_rtc_pos()
{
    T_ALARM alarm = {0};
    if ((!__this) || (!__this->dev_handle)) {
        return;
    }

    struct ui_rtc_display *rtc = rtc_ui_get_display_buf();

    if (!rtc) {
        return;
    }

    switch (__this->rtc_set_mode) {
    case RTC_SET_MODE:

        if (__this->rtc_pos == RTC_POS_NULL) {
            __this->rtc_pos = RTC_POS_DEFAULT;
            dev_ioctl(__this->dev_handle, IOCTL_GET_SYS_TIME, (u32)&__this->set_time);
        } else {
            __this->rtc_pos++;
            if (__this->rtc_pos == RTC_POS_MAX) {
                __this->rtc_pos = RTC_POS_NULL;
                rtc_update_time_api(&__this->set_time);
                UI_REFLASH_WINDOW(true);
                break;
            }
        }

        rtc->rtc_menu = UI_RTC_ACTION_YEAR_SET + (__this->rtc_pos - RTC_POS_YEAR);

        rtc->time.Year = __this->set_time.year;
        rtc->time.Month = __this->set_time.month;
        rtc->time.Day = __this->set_time.day;
        rtc->time.Hour = __this->set_time.hour;
        rtc->time.Min = __this->set_time.min;
        rtc->time.Sec = __this->set_time.sec;

        UI_SHOW_MENU(MENU_RTC_SET, 10 * 1000, 0, ui_set_rtc_timeout);
        break;

    case ALM_SET_MODE:
        if (__this->rtc_pos == RTC_POS_NULL) {
            __this->rtc_pos = RTC_ALM_POS_DEFAULT;
            if (alarm_get_info(&alarm, __this->alm_num) != 0) {
                log_error("alarm_get_info \n");
            }

            __this->set_time.hour = alarm.time.hour;
            __this->set_time.min = alarm.time.min;
            __this->alm_enable = alarm.sw;

        } else {
            __this->rtc_pos++;
            if (__this->rtc_pos == ALM_POS_MAX) {
                __this->rtc_pos = RTC_POS_NULL;
                alarm.time.hour = __this->set_time.hour;
                alarm.time.min  = __this->set_time.min;
                alarm.time.sec  = 0;
                alarm.sw = __this->alm_enable;
                alarm.index = __this->alm_num;
                alarm.mode  = 0;
                alarm_add(&alarm, __this->alm_num);
                __this->alm_num++;
                if (__this->alm_num >= sizeof(alm_select) / sizeof(alm_select[0])) {
                    __this->rtc_set_mode = RTC_SET_MODE;
                    __this->alm_num = 0;
                    UI_REFLASH_WINDOW(true);
                } else {
                    rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
                    rtc->str = alm_select[__this->alm_num];
                    UI_SHOW_MENU(MENU_RTC_SET, 10 * 1000, 0, ui_set_rtc_timeout);
                }

                break;
            }
        }

        if (ALM_POS_ENABLE == __this->rtc_pos) {
            rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
            if (__this->alm_enable) {
                rtc->str = " ON ";
            } else {
                rtc->str = " OFF";
            }
        } else {
            rtc->rtc_menu = UI_RTC_ACTION_HOUR_SET + (__this->rtc_pos - ALM_POS_HOUR);
            rtc->time.Year = __this->set_time.year;
            rtc->time.Month = __this->set_time.month;
            rtc->time.Day = __this->set_time.day;
            rtc->time.Hour = __this->set_time.hour;
            rtc->time.Min = __this->set_time.min;
            rtc->time.Sec = __this->set_time.sec;
        }

        UI_SHOW_MENU(MENU_RTC_SET, 10 * 1000, 0, ui_set_rtc_timeout);
        break;

    }
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 调整时钟 加时间
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void set_rtc_up()
{

    if ((!__this) || (!__this->dev_handle)) {
        return;
    }

    struct ui_rtc_display *rtc = rtc_ui_get_display_buf();

    if (!rtc) {
        return;
    }

    if (__this->rtc_pos == RTC_POS_NULL) {
        return ;
    }


    switch (__this->rtc_set_mode) {
    case RTC_SET_MODE:
        switch (__this->rtc_pos) {
        case RTC_POS_YEAR:
            __this->set_time.year++;
            if (__this->set_time.year > MAX_YEAR) {
                __this->set_time.year = MIN_YEAR;
            }
            break;
        case RTC_POS_MONTH:
            if (++__this->set_time.month > 12) {
                __this->set_time.month = 1;
            }
            break;
        case RTC_POS_DAY:
            if (++__this->set_time.day > month_for_day(__this->set_time.month, __this->set_time.year)) {
                __this->set_time.day = 1;
            }
            break;
        case RTC_POS_HOUR:
            if (++__this->set_time.hour >= 24) {
                __this->set_time.hour = 0;
            }
            break;

        case RTC_POS_MIN:
            if (++__this->set_time.min >= 60) {
                __this->set_time.min = 0;
            }

            break;
            /* case RTC_POS_SEC: */

            /* break; */
        }


        rtc->rtc_menu = UI_RTC_ACTION_YEAR_SET + (__this->rtc_pos - RTC_POS_YEAR);
        rtc->time.Year = __this->set_time.year;
        rtc->time.Month = __this->set_time.month;
        rtc->time.Day = __this->set_time.day;
        rtc->time.Hour = __this->set_time.hour;
        rtc->time.Min = __this->set_time.min;
        rtc->time.Sec = __this->set_time.sec;
        UI_SHOW_MENU(MENU_RTC_SET, 10 * 1000, 0, ui_set_rtc_timeout);
        break;
    case ALM_SET_MODE:

        switch (__this->rtc_pos) {
        case ALM_POS_HOUR:
            if (++__this->set_time.hour >= 24) {
                __this->set_time.hour = 0;
            }
            break;

        case ALM_POS_MIN:
            if (++__this->set_time.min >= 60) {
                __this->set_time.min = 0;
            }
            break;
        case ALM_POS_ENABLE:
            __this->alm_enable = !__this->alm_enable;
            break;
        }

        if (ALM_POS_ENABLE == __this->rtc_pos) {
            rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
            if (__this->alm_enable) {
                rtc->str = " ON ";
            } else {
                rtc->str = " OFF";
            }
        } else {
            rtc->rtc_menu = UI_RTC_ACTION_HOUR_SET + (__this->rtc_pos - ALM_POS_HOUR);
            rtc->time.Year = __this->set_time.year;
            rtc->time.Month = __this->set_time.month;
            rtc->time.Day = __this->set_time.day;
            rtc->time.Hour = __this->set_time.hour;
            rtc->time.Min = __this->set_time.min;
            rtc->time.Sec = __this->set_time.sec;
        }

        UI_SHOW_MENU(MENU_RTC_SET, 10 * 1000, 0, ui_set_rtc_timeout);
        break;

    default:
        break;
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 调整时钟 减时间
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void set_rtc_down()
{

    if ((!__this) || (!__this->dev_handle)) {
        return;
    }
    struct ui_rtc_display *rtc = rtc_ui_get_display_buf();

    if (!rtc) {
        return;
    }

    if (__this->rtc_pos == RTC_POS_NULL) {
        return ;
    }

    switch (__this->rtc_set_mode) {
    case RTC_SET_MODE:
        switch (__this->rtc_pos) {
        case RTC_POS_YEAR:
            __this->set_time.year--;
            if (__this->set_time.year < MIN_YEAR) {
                __this->set_time.year = MAX_YEAR;
            }
            break;
        case RTC_POS_MONTH:

            if (__this->set_time.month == 1) {
                __this->set_time.month = 12;
            } else {
                __this->set_time.month--;
            }

            break;
        case RTC_POS_DAY:

            if (__this->set_time.day == 1) {
                __this->set_time.day = month_for_day(__this->set_time.month, __this->set_time.year);
            } else {
                __this->set_time.day --;
            }

            break;
        case RTC_POS_HOUR:
            if (__this->set_time.hour == 0) {
                __this->set_time.hour = 23;
            } else {
                __this->set_time.hour--;
            }
            break;
        case RTC_POS_MIN:
            if (__this->set_time.min == 0) {
                __this->set_time.min = 59;
            } else {
                __this->set_time.min--;
            }
            break;
        }

        rtc->rtc_menu = UI_RTC_ACTION_YEAR_SET + (__this->rtc_pos - RTC_POS_YEAR);
        rtc->time.Year = __this->set_time.year;
        rtc->time.Month = __this->set_time.month;
        rtc->time.Day = __this->set_time.day;
        rtc->time.Hour = __this->set_time.hour;
        rtc->time.Min = __this->set_time.min;
        rtc->time.Sec = __this->set_time.sec;
        UI_SHOW_MENU(MENU_RTC_SET, 10 * 1000, 0, ui_set_rtc_timeout);
        break;

    case ALM_SET_MODE:
        switch (__this->rtc_pos) {
        case ALM_POS_HOUR:
            if (__this->set_time.hour == 0) {
                __this->set_time.hour = 23;
            } else {
                __this->set_time.hour--;
            }
            break;

        case ALM_POS_MIN:
            if (__this->set_time.min == 0) {
                __this->set_time.min = 59;
            } else {
                __this->set_time.min--;
            }
            break;

        case ALM_POS_ENABLE:
            __this->alm_enable = !__this->alm_enable;
            break;
        }

        if (ALM_POS_ENABLE == __this->rtc_pos) {
            rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
            if (__this->alm_enable) {
                rtc->str = " ON ";
            } else {
                rtc->str = " OFF";
            }
        } else {
            rtc->rtc_menu = UI_RTC_ACTION_HOUR_SET + (__this->rtc_pos - ALM_POS_HOUR);
            rtc->time.Year = __this->set_time.year;
            rtc->time.Month = __this->set_time.month;
            rtc->time.Day = __this->set_time.day;
            rtc->time.Hour = __this->set_time.hour;
            rtc->time.Min = __this->set_time.min;
            rtc->time.Sec = __this->set_time.sec;
        }
        UI_SHOW_MENU(MENU_RTC_SET, 10 * 1000, 0, ui_set_rtc_timeout);

        break;

    default:

        break;
    }



}

static void rtc_app_init()
{
    if (!__this) {
        __this = zalloc(sizeof(struct rtc_opr));
        ASSERT(__this, "%s %di \n", __func__, __LINE__);
        __this->dev_handle = dev_open("rtc", NULL);
        if (!__this->dev_handle) {
            ASSERT(0, "%s %d \n", __func__, __LINE__);
        }
    }
    __this->rtc_set_mode =  RTC_SET_MODE;
    __this->rtc_pos = RTC_POS_NULL;

    ui_update_status(STATUS_RTC_MODE);
    clock_idle(RTC_IDLE_CLOCK);
    UI_SHOW_WINDOW(ID_WINDOW_CLOCK);
    sys_key_event_enable();

}


//*----------------------------------------------------------------------------*/
/**@brief    rtc 退出
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void rtc_task_close()
{
#if SMART_BOX_EN
    extern void smartbox_rtc_mode_exit(void);
    smartbox_rtc_mode_exit();
#endif
    UI_HIDE_CURR_WINDOW();
    if (__this) {
        if (__this->dev_handle) {
            dev_close(__this->dev_handle);
            __this->dev_handle = NULL;
        }
        free(__this);
        __this = NULL;
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 按键消息入口
   @param    无
   @return   1、消息已经处理，不需要发送到common  0、消息发送到common处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int rtc_key_event_opr(struct sys_event *event)
{
    int ret = true;
    int err = 0;

#if (TCFG_SPI_LCD_ENABLE)
    extern int key_is_ui_takeover();
    if (key_is_ui_takeover()) {
        return false;
    }
#endif

    int key_event = event->u.key.event;
    int key_value = event->u.key.value;//

    log_info("key_event:%d \n", key_event);

    if (__this && __this->dev_handle) {
        switch (key_event) {
        case KEY_RTC_UP:
            log_info("KEY_RTC_UP \n");
            set_rtc_up();
            break;

        case KEY_RTC_DOWN:
            log_info("KEY_RTC_DOWN \n");
            set_rtc_down();
            break;

        case KEY_RTC_SW:
            log_info("KEY_RTC_SW \n");
            set_rtc_sw();
            break;

        case KEY_RTC_SW_POS:
            log_info("KEY_RTC_SW_POS \n");
            set_rtc_pos();
            break;

        default :
            ret = false;
            break;
        }
    }
    return ret;
}


//*----------------------------------------------------------------------------*/
/**@brief    rtc 按键消息入口
   @param    无
   @return   1、消息已经处理，不需要发送到common  0、消息发送到common处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int rtc_sys_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return rtc_key_event_opr(event);
    case SYS_DEVICE_EVENT:
        return false;
    default:
        return false;
    }
    return false;
}


//*----------------------------------------------------------------------------*/
/**@brief   rtc 启动
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void rtc_task_start()
{
    rtc_app_init();
    if (alarm_active_flag_get()) {
        alarm_ring_start();
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 模式提示音播放结束处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void  rtc_tone_play_end_callback(void *priv, int flag)
{
    u32 index = (u32)priv;

    if (APP_RTC_TASK != app_get_curr_task()) {
        log_error("tone callback task out \n");
        return;
    }

    switch (index) {
    case IDEX_TONE_RTC:
        ///提示音播放结束， 启动播放器播放
        break;
    default:
        break;
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 主任务
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void app_rtc_task()
{
    int res;
    int msg[32];
#if (SMART_BOX_EN)
    extern u8 smartbox_rtc_ring_tone(void);
    if (smartbox_rtc_ring_tone()) {
        tone_play_with_callback_by_name(tone_table[IDEX_TONE_RTC], 1, rtc_tone_play_end_callback, (void *)IDEX_TONE_RTC);
    }
#else
    tone_play_with_callback_by_name(tone_table[IDEX_TONE_RTC], 1, rtc_tone_play_end_callback, (void *)IDEX_TONE_RTC);
#endif
    rtc_task_start();

    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (rtc_sys_event_handler((struct sys_event *)(&msg[1])) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));    //由common统一处理
            }
            break;
        default:
            break;
        }

        if (app_task_exitting()) {
            rtc_task_close();
            return;
        }
    }
}

#else


void app_rtc_task()
{

}

#endif




