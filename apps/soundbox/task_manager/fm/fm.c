#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "app_action.h"
#include "tone_player.h"
#include "app_main.h"
#include "ui_manage.h"
#include "vm.h"
#include "key_event_deal.h"
#include "asm/pwm_led.h"
#include "user_cfg.h"
#include "ui/ui_api.h"
#include "clock_cfg.h"
#include "includes.h"
#include "fm/fm_api.h"
#include "fm/fm_manage.h"
#include "fm/fm_rw.h"

#define LOG_TAG_CONST       APP_FM
#define LOG_TAG             "[APP_FM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

/*************************************************************
   此文件函数主要是fm模式按键处理和事件处�?

	void app_fm_task()
   fm模式主函�?

	static int fm_event_handler(struct sys_event *event)
   fm系统事件所有处理入�?

	static void fm_app_unint(void)
	fm模式退�?

**************************************************************/



#if TCFG_APP_FM_EN
static u8 fm_idle_flag = 1;
//*----------------------------------------------------------------------------*/
/**@brief    fm按键消息入口
   @param    �?
   @return   1、消息已经处理，不需要发送到common  0、消息发送到common处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int fm_key_event_opr(struct sys_event *event)
{
    int ret = true;
    struct key_event *key = &event->u.key;

    int key_event = event->u.key.event;
    int key_value = event->u.key.value;//

    r_printf("key value:%d, event:%d \n", key->value, key->event);

#if (TCFG_SPI_LCD_ENABLE)
    extern int key_is_ui_takeover();
    if (key_is_ui_takeover()) {
        return false;
    }
#endif

    switch (key_event) {
    case KEY_TEST_DEMO_0:
        log_info("KEY_TEST_DEMO_0 = %d \n", key_value);
        app_task_put_key_msg(KEY_TEST_DEMO_1, 5678); //test demo
        break;
    case KEY_TEST_DEMO_1:
        log_info("KEY_TEST_DEMO_1 = %d \n", key_value); //test demo
        break;

    case KEY_MUSIC_PP://暂停播放
        /* app_task_put_key_msg(KEY_TEST_DEMO_0,1234);  //test demo// */
        fm_volume_pp();
        break;
    case  KEY_FM_SCAN_ALL://全自动搜�?
    case  KEY_FM_SCAN_ALL_DOWN://全自动搜�?
    case  KEY_FM_SCAN_ALL_UP://全自动搜�?
        fm_scan_all();
        break;
    case  KEY_FM_SCAN_DOWN:
        fm_scan_down();//半自动搜�?
        break;
    case  KEY_FM_SCAN_UP:
        fm_scan_up();//半自动搜�?
        break;
    case  KEY_FM_PREV_STATION://下一�?
        fm_prev_station();
        break;
    case  KEY_FM_NEXT_STATION:
        fm_next_station();
        break;
    case  KEY_FM_PREV_FREQ://下一个频�?
        fm_prev_freq();
        break;
    case  KEY_FM_NEXT_FREQ:
        fm_next_freq();
        break;
    case KEY_VOL_UP:
        fm_volume_up();
        break;
    case KEY_VOL_DOWN:
        fm_volume_down();
        break;

    default:
        ret = false;
        break;
    }
#if (SMART_BOX_EN)
    extern void smartbot_fm_msg_deal(int msg);
    smartbot_fm_msg_deal(key_event);
#endif

    return ret;
}



//*----------------------------------------------------------------------------*/
/**@brief    fm 模式活跃状�?所有消息入�?
   @param    �?
   @return   1、当前消息已经处理，不需要发送comomon 0、当前消息不是fm处理的，发送到common统一处理
   @note
*/
/*----------------------------------------------------------------------------*/

static int fm_event_handler(struct sys_event *event)
{
    int err = 0;
    switch (event->type) {
    case SYS_KEY_EVENT:
        return fm_key_event_opr(event);
        break;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_FM) {
            if (event->u.dev.event == DEVICE_EVENT_IN) {
                log_info("fm online \n");
            } else if (event->u.dev.event == DEVICE_EVENT_OUT) {
                log_info("fm offline \n");
                app_task_switch_next();
            }
            return true;
        }
        return false;

    default:
        return false;
    }
}

static void fm_app_init(void)
{
    fm_idle_flag = 0;
    sys_key_event_enable();
    ui_update_status(STATUS_FM_MODE);
    clock_idle(FM_IDLE_CLOCK);
    fm_manage_init();//
    fm_api_init();//设置频率信息
}


static void fm_app_start(void)
{
    fm_manage_start();//收音出声
}

static void fm_app_uninit(void)
{
    fm_api_release();
    fm_manage_close();
    /* tone_play_stop(); */
    tone_play_stop_by_path(tone_table[IDEX_TONE_FM]);
    fm_idle_flag = 1;
}


static void  fm_tone_play_end_callback(void *priv, int flag)
{
    u32 index = (u32)priv;

    if (APP_FM_TASK != app_get_curr_task()) {
        log_error("tone callback task out \n");
        return;
    }

    switch (index) {
    case IDEX_TONE_FM:
        ///提示音播放结束， 启动播放器播�?
        fm_app_start();
        break;
    default:
        break;
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    fm主任�?
   @param    �?
   @return   �?
   @note
*/
/*----------------------------------------------------------------------------*/
void app_fm_task()
{
    int msg[32];
    fm_app_init();

		
		set_pa_mode(1);
#if TCFG_DEC2TWS_ENABLE
    extern void set_tws_background_connected_flag(u8 flag);
    extern u8 get_tws_background_connected_flag();
    if (get_tws_background_connected_flag()) { //不播放提示音
        fm_app_start();
        set_tws_background_connected_flag(0);
    } else
#endif

    {
        tone_play_with_callback_by_name(tone_table[IDEX_TONE_FM], 1, fm_tone_play_end_callback, (void *)IDEX_TONE_FM);
    }
//    if (err) {
//        ///提示音播放失败，直接启动播放
//        fm_app_start();
//    }

    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (fm_event_handler((struct sys_event *)(&msg[1])) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));    //由common统一处理
            }
            break;
        default:
            break;
        }

        if (app_task_exitting()) {
            fm_app_uninit();
            return;
        }
    }
}

static u8 fm_idle_query(void)
{
    return fm_idle_flag;
}
REGISTER_LP_TARGET(fm_lp_target) = {
    .name = "fm",
    .is_idle = fm_idle_query,
};

#else


void app_fm_task()
{

}


#endif


