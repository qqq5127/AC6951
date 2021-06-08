
/*************************************************************
    此文件函数主要是pc模式按键处理和事件处理

	void app_pc_task()
    pc模式主函数

	static int pc_sys_event_handler(struct sys_event *event)
	pc模式系统事件所有处理入口

	static void pc_task_close()
	pc模式退出


**************************************************************/

#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "tone_player.h"
#include "asm/charge.h"
#include "asm/usb.h"
#include "app_charge.h"
#include "app_main.h"
#include "app_online_cfg.h"
#include "app_power_manage.h"
#include "ui_manage.h"
#include "app_chargestore.h"
#include "key_event_deal.h"
#include "os/os_api.h"
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"
#include "usb/device/hid.h"
#include "usb/device/msd.h"
#include "uac_stream.h"
#include "pc/pc.h"
#include "user_cfg.h"
#include "device/sdmmc.h"
#include "pc/usb_msd.h"
#include "ui/ui_api.h"
#include "clock_cfg.h"
#include "dev_multiplex_api.h"
#include "usb/otg.h"
#include "bt_tws.h"
#include "clock_cfg.h"
#include "user_api/app_status_api.h"


#if TCFG_APP_PC_EN

#define LOG_TAG_CONST        APP_PC
#define LOG_TAG             "[APP_PC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


struct pc_opr {
    u8 volume;
    u8 onoff : 1;
};

static struct pc_opr pc_hdl = {0};
#define __this 	(&pc_hdl)
static u8 pc_idle_flag = 1;



static void pc_start()
{
    if (__this->onoff) {
        log_info("PC is start ");
        return ;
    }
    __this->onoff = 1;
    log_info("App Start - PC");

    app_status_handler(APP_STATUS_PC);
#if TCFG_PC_ENABLE
    usb_start();
#endif
}

static void pc_stop()
{
    if (!__this->onoff) {
        log_info("PC is stop ");
        return ;
    }
    __this->onoff = 0;

    log_info("App Stop - PC");
#if TCFG_PC_ENABLE
    usb_stop();
#endif
}

static void pc_hold()
{
    if (!__this->onoff) {
        log_info("PC is hold");
        return ;
    }
    __this->onoff = 0;

    log_info("App Hold- PC");
#if TCFG_PC_ENABLE
    usb_pause();
#endif
}



//*----------------------------------------------------------------------------*/
/**@brief    pc 按键消息入口
   @param    无
   @return   1、消息已经处理，不需要发送到common  0、消息发送到common处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int pc_key_event_opr(struct sys_event *event)
{
    int ret = true;
    int err = 0;

    int key_event = event->u.key.event;
    int key_value = event->u.key.value;

    log_debug("key value:%d, event:%d \n", key_value, key_event);

    switch (key_event) {
#if TCFG_USB_SLAVE_HID_ENABLE
    case  KEY_MUSIC_PP:
        log_info("KEY_MUSIC_PP\n");
        hid_key_handler(0, USB_AUDIO_PP);
        break;
    case  KEY_MUSIC_PREV:
        log_info("KEY_MUSIC_PREV\n");
        hid_key_handler(0, USB_AUDIO_PREFILE);
        break;
    case  KEY_MUSIC_NEXT:
        log_info("KEY_MUSIC_NEXT\n");
        hid_key_handler(0, USB_AUDIO_NEXTFILE);
        break;
    case  KEY_VOL_UP:
        log_info("pc KEY_VOL_UP\n");
        hid_key_handler(0, USB_AUDIO_VOLUP);
        printf(">>>pc vol+: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
        break;
    case  KEY_VOL_DOWN:
        log_info("pc KEY_VOL_DOWN\n");
        hid_key_handler(0, USB_AUDIO_VOLDOWN);
        printf(">>>pc vol-: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
        break;
#endif
    default:
        ret = false;
        break;
    }

    return ret;
}

//*----------------------------------------------------------------------------*/
/**@brief    pc 入口
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void pc_task_start(void)
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    pc_dm_multiplex_init();
#endif

    __this->volume =  app_audio_get_volume(APP_AUDIO_STATE_MUSIC);

    pc_start();
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    usb_otg_resume(0);
#endif
}

//*----------------------------------------------------------------------------*/
/**@brief    pc 关闭
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void pc_app_uninit(void)
{
    if (pc_app_check() == false) {
        pc_stop();
    } else {
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
        usb_otg_suspend(0, 0);
#endif
        pc_hold();
    }

    /* tone_play_stop(); */
    tone_play_stop_by_path(tone_table[IDEX_TONE_PC]);//停止播放提示音
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    pc_dm_multiplex_exit();
#endif

#if (TCFG_DEV_MANAGER_ENABLE)
    dev_manager_list_check_mount();
#endif/*TCFG_DEV_MANAGER_ENABLE*/

    UI_HIDE_CURR_WINDOW();
}


#if ((defined TCFG_PC_BACKMODE_ENABLE) && (TCFG_PC_BACKMODE_ENABLE))
bool pc_backmode_check(struct sys_event *event)
{
    switch (event->type) {
    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_OTG) {
            const char *usb_msg = (const char *)event->u.dev.value;
            if (usb_msg[0] == 's') {
                if (event->u.dev.event == DEVICE_EVENT_IN) {
                    pc_task_start();
                } else if (event->u.dev.event == DEVICE_EVENT_OUT) {
                    pc_app_uninit();
                }
                return true;
            }
        }
        break;
    }
    return false;
}
#endif//TCFG_PC_BACKMODE_ENABLE

//*----------------------------------------------------------------------------*/
/**@brief    pc模式 退出
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void pc_task_close()
{
    pc_app_uninit();
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, __this->volume, 1);
#if (TCFG_DEC2TWS_ENABLE)
    bt_tws_sync_volume();
#endif
    pc_idle_flag = 1;
}

//*----------------------------------------------------------------------------*/
/**@brief    pc 模式活跃状态 所有消息入口
   @param    无
   @return   1、当前消息已经处理，不需要发送comomon 0、当前消息不是linein处理的，发送到common统一处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int pc_sys_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return pc_key_event_opr(event);

    case SYS_DEVICE_EVENT:
#if TCFG_PC_ENABLE
        if (pc_device_event_handler(event) == 2) {
            /* pc_stop(); */
            app_task_switch_next();
        }
#endif
        return false;

    default:
        return false;
    }
    return false;
}

//*----------------------------------------------------------------------------*/
/**@brief    pc 在线检测  切换模式判断使用
   @param    无
   @return   1 linein设备在线 0 设备不在线
   @note
*/
/*----------------------------------------------------------------------------*/
int pc_app_check(void)
{
#if ((defined TCFG_PC_BACKMODE_ENABLE) && (TCFG_PC_BACKMODE_ENABLE))
    return false;
#endif//TCFG_PC_BACKMODE_ENABLE

    u32 r = usb_otg_online(0);
    log_info("pc_app_check %d", r);
    if ((r == SLAVE_MODE) ||
        (r == SLAVE_MODE_WAIT_CONFIRMATION)) {
        return true;
    }
    return false;
}
//*----------------------------------------------------------------------------*/
/**@brief    PC 模式提示音播放结束处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void  pc_tone_play_end_callback(void *priv, int flag)
{
    u32 index = (u32)priv;

    if (APP_PC_TASK != app_get_curr_task()) {
        log_error("tone callback task out \n");
        return;
    }

    switch (index) {
    case IDEX_TONE_PC:
        ///提示音播放结束， 启动播放器播放
        pc_task_start();
        break;
    default:
        break;
    }
}

void pc_app_init()
{
    /* 按键消息使能 */
    pc_idle_flag = 0;
    sys_key_event_enable();

    clock_idle(PC_IDLE_CLOCK);

    UI_SHOW_WINDOW(ID_WINDOW_PC);
    UI_SHOW_MENU(MENU_PC, 1000, 0, NULL);
    ui_update_status(STATUS_PC_MODE);
    __this->volume =  app_audio_get_volume(APP_AUDIO_STATE_MUSIC);//记录下当前音量
}
//*----------------------------------------------------------------------------*/
/**@brief    pc 主任务
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void app_pc_task()
{
    int res;
    int msg[32];

    pc_app_init();

    tone_play_with_callback_by_name(tone_table[IDEX_TONE_PC], 1, pc_tone_play_end_callback, (void *)IDEX_TONE_PC);

    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (pc_sys_event_handler((struct sys_event *)(msg + 1)) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));
            }
            break;
        default:
            break;
        }

        if (app_task_exitting()) {
            pc_task_close();
            return;
        }
    }
}

static u8 pc_idle_query(void)
{
    return pc_idle_flag;
}
REGISTER_LP_TARGET(pc_lp_target) = {
    .name = "pc",
    .is_idle = pc_idle_query,
};

#else

int pc_app_check(void)
{
    return false;
}

void app_pc_task()
{

}
#endif
