
/*************************************************************

             此文件函数主要是蓝牙模式按键处理


**************************************************************/
#include "system/includes.h"
#include "media/includes.h"

#include "app_config.h"
#include "app_task.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "btstack/btstack_error.h"
#include "btctrler/btctrler_task.h"
#include "classic/hci_lmp.h"

#include "bt/bt_tws.h"
#include "bt/bt_ble.h"
#include "bt/bt.h"
#include "bt/vol_sync.h"
#include "bt/bt_emitter.h"
#include "bt_common.h"
#include "aec_user.h"

#include "math.h"
#include "spp_user.h"


#include "app_chargestore.h"
#include "app_charge.h"
#include "app_main.h"
#include "app_power_manage.h"
#include "user_cfg.h"

#include "asm/pwm_led.h"
#include "asm/timer.h"
#include "asm/hwi.h"
#include "cpu.h"

#include "ui/ui_api.h"
#include "ui_manage.h"
#include "ui/ui_style.h"

#include "key_event_deal.h"
#include "clock_cfg.h"
#include "gSensor/gSensor_manage.h"
#include "soundcard/soundcard.h"

#include "audio_dec.h"
#include "tone_player.h"
#include "dac.h"


#define __this 	(&app_bt_hdl)

#define LOG_TAG_CONST        BT
#define LOG_TAG             "[BT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_APP_BT_EN

#if TCFG_USER_TWS_ENABLE
static void max_tone_timer(void *priv)
{
    if (!tone_get_status()) {
        __this->max_tone_timer_hdl = 0;
        __this->replay_tone_flag = 1;
    } else {
        __this->max_tone_timer_hdl = sys_timeout_add(NULL, max_tone_timer, TWS_SYNC_TIME_DO + 100);
    }
}
#endif


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式获取按键事件前的过滤
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_key_event_filter_before()
{
#if TCFG_APP_FM_EMITTER_EN
#if TCFG_UI_ENABLE
    if (ui_get_app_menu(GRT_CUR_MENU) == MENU_FM_SET_FRE) {
        return false;
    }
#endif
#endif

    if (key_is_ui_takeover()) {
        return false;
    }
    return true;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式获取按键事件后的过滤
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_key_event_filter_after(int key_event)
{

#if (TCFG_DEC2TWS_ENABLE)
    u8 is_tws_all_in_bt();
    if ((key_event != KEY_POWEROFF) && \
        (key_event != KEY_POWEROFF_HOLD) && \
        (key_event != KEY_CHANGE_MODE) && \
        (key_event != KEY_REVERB_OPEN)) {
        if (!is_tws_all_in_bt()) {
            return true;
        }
    }
#endif
    return false;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 tws 区分左右耳加音量
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void volume_up(void)
{
    u8 test_box_vol_up = 0x41;
    s8 cur_vol = 0;
    u8 call_status = get_call_status();

    if (tone_get_status()) {
        if (get_call_status() == BT_CALL_INCOMING) {
            app_audio_volume_up(1);
        }
        return;
    }

    /*打电话出去彩铃要可以调音量大小*/
    if ((call_status == BT_CALL_ACTIVE) || (call_status == BT_CALL_OUTGOING)) {
        cur_vol = app_audio_get_volume(APP_AUDIO_STATE_CALL);
    } else {
        cur_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    }
    if (get_remote_test_flag()) {
        user_send_cmd_prepare(USER_CTRL_TEST_KEY, 1, &test_box_vol_up); //音量加
    }

    if (cur_vol >= app_audio_get_max_volume()) {
#if TCFG_USER_TWS_ENABLE
        if (get_tws_sibling_connect_state()) {
            if (tws_api_get_role() == TWS_ROLE_MASTER && __this->replay_tone_flag) {
                __this->replay_tone_flag = 0;               //防止提示音被打断标志
                tws_api_sync_call_by_uuid('T', SYNC_CMD_MAX_VOL, TWS_SYNC_TIME_DO);
                __this->max_tone_timer_hdl = sys_timeout_add(NULL, max_tone_timer, TWS_SYNC_TIME_DO + 100);  //同步在TWS_SYNC_TIME_DO之后才会播放提示音，所以timer需要在这个时间之后才去检测提示音状态
            }
        } else
#endif
        {
#if TCFG_MAX_VOL_PROMPT
            tone_play_by_path(tone_table[IDEX_TONE_MAX_VOL], 0);
#endif
        }

        if (get_call_status() != BT_CALL_HANGUP) {
            /*本地音量最大，如果手机音量还没最大，继续加，以防显示不同步*/
            if (bt_user_priv_var.phone_vol < 15) {
                if (get_curr_channel_state() & HID_CH) {
                    user_send_cmd_prepare(USER_CTRL_HID_VOL_UP, 0, NULL);     //使用HID调音量
                } else {
                    user_send_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_UP, 0, NULL);
                }
            }
            return;
        }
#if BT_SUPPORT_MUSIC_VOL_SYNC
#if (TCFG_DEC2TWS_ENABLE)
        if (!is_tws_all_in_bt()) {
            log_debug(">>>>>>>tws is not all in bt mode !!!\n");
        } else
#endif
        {
            opid_play_vol_sync_fun(&app_var.music_volume, 1);
            user_send_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL);
        }
#endif/*BT_SUPPORT_MUSIC_VOL_SYNC*/
        return;
    }

#if BT_SUPPORT_MUSIC_VOL_SYNC
#if (TCFG_BD_NUM == 2)
    if ((APP_AUDIO_STATE_MUSIC == app_audio_get_state()) || (a2dp_get_status() ==  BT_MUSIC_STATUS_STARTING)) {
#else
    if (APP_AUDIO_STATE_MUSIC == app_audio_get_state()) {
#endif
        opid_play_vol_sync_fun(&app_var.music_volume, 1);
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_var.music_volume, 1);
    } else {
        app_audio_volume_up(1);
    }
#else
    app_audio_volume_up(1);
#endif/*BT_SUPPORT_MUSIC_VOL_SYNC*/
    log_info("vol+: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    if (get_call_status() != BT_CALL_HANGUP) {
        if (get_curr_channel_state() & HID_CH) {
            user_send_cmd_prepare(USER_CTRL_HID_VOL_UP, 0, NULL);     //使用HID调音量
        } else {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_UP, 0, NULL);
        }
    } else {
#if BT_SUPPORT_MUSIC_VOL_SYNC
#if (TCFG_DEC2TWS_ENABLE)
        if (!is_tws_all_in_bt()) {
            log_debug(">>>>>>>tws is not all in bt mode !!!\n");
        } else
#endif
        {
            /* opid_play_vol_sync_fun(&app_var.music_volume, 0); */

#if TCFG_USER_TWS_ENABLE
            user_send_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL);     //使用HID调音量
            //user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL, 0, NULL);
#else
            user_send_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL);
#endif/*TCFG_USER_TWS_ENABLE*/
        }
#endif/*BT_SUPPORT_MUSIC_VOL_SYNC*/
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 tws 区分左右耳减音量
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void volume_down(void)
{
    u8 test_box_vol_down = 0x42;

    if (tone_get_status()) {
        if (get_call_status() == BT_CALL_INCOMING) {
            app_audio_volume_down(1);
        }
        return;
    }
    if (get_remote_test_flag()) {
        user_send_cmd_prepare(USER_CTRL_TEST_KEY, 1, &test_box_vol_down); //音量减
    }

    if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) <= 0) {
        if (get_call_status() != BT_CALL_HANGUP) {
            /*
             *本地音量最小，如果手机音量还没最小，继续减
             *注意：有些手机通话最小音量是1(GREE G0245D)
             */
            if (bt_user_priv_var.phone_vol > 1) {
                if (get_curr_channel_state() & HID_CH) {
                    user_send_cmd_prepare(USER_CTRL_HID_VOL_DOWN, 0, NULL);     //使用HID调音量
                } else {
                    user_send_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);
                }
            }
            return;
        }
#if BT_SUPPORT_MUSIC_VOL_SYNC
#if (TCFG_DEC2TWS_ENABLE)
        if (!is_tws_all_in_bt()) {
            log_debug(">>>>>>>tws is not all in bt mode !!!\n");
        } else
#endif
        {
            opid_play_vol_sync_fun(&app_var.music_volume, 0);
            user_send_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_DEC, 0, NULL);
        }
#endif
        return;
    }

#if BT_SUPPORT_MUSIC_VOL_SYNC
#if (TCFG_BD_NUM == 2)
    if ((APP_AUDIO_STATE_MUSIC == app_audio_get_state()) || (a2dp_get_status() ==  BT_MUSIC_STATUS_STARTING)) {
#else
    if (APP_AUDIO_STATE_MUSIC == app_audio_get_state()) {
#endif
        opid_play_vol_sync_fun(&app_var.music_volume, 0);
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_var.music_volume, 1);
    } else {
        app_audio_volume_down(1);
    }
#else
    app_audio_volume_down(1);
#endif/*BT_SUPPORT_MUSIC_VOL_SYNC*/
    log_info("vol-: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    if (get_call_status() != BT_CALL_HANGUP) {
        if (get_curr_channel_state() & HID_CH) {
            user_send_cmd_prepare(USER_CTRL_HID_VOL_DOWN, 0, NULL);     //使用HID调音量
        } else {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);
        }
    } else {
#if BT_SUPPORT_MUSIC_VOL_SYNC
#if (TCFG_DEC2TWS_ENABLE)
        if (!is_tws_all_in_bt()) {
            log_debug(">>>>>>>tws is not all in bt mode !!!\n");
        } else
#endif
        {
            /* opid_play_vol_sync_fun(&app_var.music_volume, 0); */
            if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) == 0) {
                app_audio_volume_down(0);
            }

#if TCFG_USER_TWS_ENABLE
            user_send_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_DEC, 0, NULL);
            //user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL, 0, NULL);
#else
            user_send_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_DEC, 0, NULL);
#endif
        }
#endif
    }
}



#if ONE_KEY_CTL_DIFF_FUNC
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 tws 左右耳区分按键功能
   @param
   @return
   @note     左耳:下一曲、音量加
   			 右耳：上一曲、音量减
*/
/*----------------------------------------------------------------------------*/
static void lr_diff_otp_deal(u8 opt, char channel)
{
    /* log_info("lr_diff_otp_deal:%d", opt); */
    switch (opt) {
    case ONE_KEY_CTL_NEXT_PREV:
        if (channel == 'L') {
            user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
        } else if (channel == 'R') {
            user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
        } else {
            user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
        }
        break;
    case ONE_KEY_CTL_VOL_UP_DOWN:
        if (channel == 'L') {
            volume_up();
        } else if (channel == 'R') {
            volume_down();
        }
        break;
    default:
        break;
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 tws 左右耳区分按键功能
   @param
   @return
   @note     左耳:下一曲、音量加
   			 右耳：上一曲、音量减
*/
/*----------------------------------------------------------------------------*/
void key_tws_lr_diff_deal(struct sys_event *event, u8 opt)
{
    u8 channel = 'U';
    if (get_bt_tws_connect_status()) {
        channel = tws_api_get_local_channel();
        if ('L' == channel) {
            channel = (u32)event->arg == KEY_EVENT_FROM_TWS ? 'R' : 'L';
        } else {
            channel = (u32)event->arg == KEY_EVENT_FROM_TWS ? 'L' : 'R';
        }
    }
    lr_diff_otp_deal(opt, channel);
}
#else
void key_tws_lr_diff_deal(struct sys_event *event, u8 opt)
{
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式切换设备类型
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
#if (USER_SUPPORT_PROFILE_HID==1)
void user_change_profile_mode(u8 flag)
{
    user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    while (hci_standard_connect_check() != 0) {
        //wait disconnect;
        os_time_dly(10);
    }
    __bt_set_hid_independent_flag(flag);
    if (flag) {
        __change_hci_class_type(0x002570);
    } else {
        __change_hci_class_type(BD_CLASS_WEARABLE_HEADSET);
    }
    user_send_cmd_prepare(USER_CTRL_CMD_CHANGE_PROFILE_MODE, 0, NULL);
    if (connect_last_device_from_vm()) {
        /* log_debug("start connect vm addr phone \n"); */
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    }
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 pp 按键处理
   @param
   @return
   @note     播放音乐、通话接听挂断
*/
/*----------------------------------------------------------------------------*/
void bt_key_music_pp()
{
    if ((get_call_status() == BT_CALL_OUTGOING) ||
        (get_call_status() == BT_CALL_ALERT)) {
        user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
    } else if (get_call_status() == BT_CALL_INCOMING) {
        user_send_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
    } else if (get_call_status() == BT_CALL_ACTIVE) {
        user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
    } else {
        user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 prev 按键处理
   @param
   @return
   @note     播放音乐上一曲
*/
/*----------------------------------------------------------------------------*/
void bt_key_music_prev()
{
    user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 next 按键处理
   @param
   @return
   @note     播放音乐下一曲
*/
/*----------------------------------------------------------------------------*/
void bt_key_music_next()
{
#ifdef CONFIG_BOARD_AC6933B_LIGHTING
    if (get_call_status() == BT_CALL_INCOMING) {
        user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        break;
    }
#endif
    user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 vol up 按键处理
   @param
   @return
   @note     加音量
*/
/*----------------------------------------------------------------------------*/
void bt_key_vol_up()
{

    u8 vol;
    if (get_call_status() == BT_CALL_ACTIVE && bt_sco_state() == 0) {
        return;
    }
    volume_up();
    vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
    UI_SHOW_MENU(MENU_MAIN_VOL, 1000, vol, NULL);
    UI_MSG_POST("music_vol:vol=%4", vol);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 vol down 按键处理
   @param
   @return
   @note     减音量
*/
/*----------------------------------------------------------------------------*/
void bt_key_vol_down()
{
    u8 vol;
    if (get_call_status() == BT_CALL_ACTIVE && bt_sco_state() == 0) {
        return;
    }
    volume_down();
    vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
    UI_SHOW_MENU(MENU_MAIN_VOL, 1000, vol, NULL);
    UI_MSG_POST("music_vol:vol=%4", vol);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 回拨最后一个号码  来电拒听
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_key_call_last_on()
{
    if (get_call_status() == BT_CALL_INCOMING) {
        user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        return;
    }

    if ((get_call_status() == BT_CALL_ACTIVE) ||
        (get_call_status() == BT_CALL_OUTGOING) ||
        (get_call_status() == BT_CALL_ALERT) ||
        (get_call_status() == BT_CALL_INCOMING)) {
        return;//通话过程不允许回拨
    }

    if (bt_user_priv_var.last_call_type ==  BT_STATUS_PHONE_INCOME) {
        user_send_cmd_prepare(USER_CTRL_DIAL_NUMBER, bt_user_priv_var.income_phone_len,
                              bt_user_priv_var.income_phone_num);
    } else {
        user_send_cmd_prepare(USER_CTRL_HFP_CALL_LAST_NO, 0, NULL);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 通话挂断
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_key_call_hand_up()
{
    user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_key_call_answer()
{

}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式  siri开启
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_key_call_siri()
{
    user_send_cmd_prepare(USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式  hid 发起拍照命令
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_key_hid_control()
{
    /* log_info("get_curr_channel_state:%x\n", get_curr_channel_state()); */
    if (get_curr_channel_state() & HID_CH) {
        log_info("KEY_HID_CONTROL\n");
        user_send_cmd_prepare(USER_CTRL_HID_IOS, 0, NULL);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式  tws 分开左右耳的按键功能
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_key_third_click(struct sys_event *event)
{
    key_tws_lr_diff_deal(event, ONE_KEY_CTL_NEXT_PREV);
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙tws 低电处理
   @param
   @return
   @note    电量高的做主机，电量低的做从机
**/
/*----------------------------------------------------------------------------*/
void bt_key_low_lantecy()
{
    bt_set_low_latency_mode(!bt_get_low_latency_mode());
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式  混响按键不处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_key_reverb_open()
{
    if ((get_call_status() == BT_CALL_ACTIVE) ||
        (get_call_status() == BT_CALL_OUTGOING) ||
        (get_call_status() == BT_CALL_ALERT) ||
        (get_call_status() == BT_CALL_INCOMING)) {
        //通话过程不允许开/关混响
        return 1;
    }
    return 0;
}
#endif
