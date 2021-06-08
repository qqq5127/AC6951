#include "smartbox/config.h"
#include "smartbox/event.h"
#include "smartbox/smartbox.h"
#include "btstack/btstack_task.h"
#include "btstack/avctp_user.h"
#include "smartbox/function.h"

#include "fm_emitter/fm_emitter_manage.h"
#include "app_action.h"
#include "key_event_deal.h"
#include "audio_config.h"
#include "smartbox_setting_opt.h"
#include "le_smartbox_module.h"

#if (SMART_BOX_EN)

extern void bt_search_device(void);

extern void emitter_search_stop(u8 result);

static void smartbox_common_event_deal(int msg, int argc, int *argv)
{
    int err = 0;
    struct smartbox *smart = (struct smartbox *)argv[0];
    if (smart == NULL) {
        printf("smart hdl NULL err!!\n");
        return ;
    }

    switch (msg) {
    case USER_MSG_SMARTBOX_DISCONNECT_EDR:
        printf("USER_MSG_SMARTBOX_DISCONNECT_EDR\n");
        if (get_curr_channel_state() != 0) {
            user_send_cmd_prepare(USER_CTRL_A2DP_CMD_CLOSE, 0, NULL);
            user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        }
        break;
#if TCFG_USER_EMITTER_ENABLE
    case USER_MSG_SMARTBOX_BT_SCAN_OPEN:
        printf("USER_MSG_SMARTBOX_BT_SCAN_OPEN\n");
        bt_search_device();
        break;
    case USER_MSG_SMARTBOX_BT_SCAN_STOP:
        printf("USER_MSG_SMARTBOX_BT_SCAN_STOP\n");
        emitter_search_stop(0);
        break;

    case USER_MSG_SMARTBOX_BT_CONNECT_SPEC_ADDR:
        printf("USER_MSG_SMARTBOX_BT_CONNECT_SPEC_ADDR\n");
        put_buf(smart->emitter_con_addr, 6);
        user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, smart->emitter_con_addr);
        break;
#endif//TCFG_USER_EMITTER_ENABLE

    case USER_MSG_SMARTBOX_SET_VOL:
        printf("USER_MSG_SMARTBOX_SET_VOL\n");
        smartbox_function_update(COMMON_FUNCTION, (u32)argv[1]);
        break;
    case USER_MSG_SMARTBOX_SET_EQ_PARAM:
        printf("USER_MSG_SMARTBOX_SET_EQ_PARAM\n");
        smartbox_function_update(COMMON_FUNCTION, (u32)argv[1]);
        break;
    case USER_MSG_SMARTBOX_SET_FMTX_FREQ:
        printf("USER_MSG_SMARTBOX_SET_FMTX_FREQ\n");
#if (TCFG_APP_FM_EMITTER_EN && TCFG_FM_EMITTER_INSIDE_ENABLE)
        u16 freq = (u16)argv[1];
        printf("freq %d\n", freq);
        fm_emitter_manage_set_fre(freq);
#endif
        /* #if TCFG_UI_ENABLE */
        /* 			ui_set_menu_ioctl(MENU_FM_DISP_FRE, freq); */
        /* #endif */
        break;
#if TCFG_USER_EMITTER_ENABLE
    case USER_MSG_SMARTBOX_SET_BT_EMITTER_SW:
        printf("USER_MSG_SMARTBOX_SET_BT_EMITTER_SW\n");
        break;
    case USER_MSG_SMARTBOX_SET_BT_EMITTER_CONNECT_STATES:
        printf("USER_MSG_SMARTBOX_SET_BT_EMITTER_CONNECT_STATES, state = %d\n", argv[1]);
        if (argv[1]) {
            emitter_search_stop(0);
            put_buf(smart->emitter_con_addr, 6);
            user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, smart->emitter_con_addr);
        } else {
            if (get_curr_channel_state() != 0) {
                printf("-----------------11111111111111111\n");
                user_send_cmd_prepare(USER_CTRL_A2DP_CMD_CLOSE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
            }
        }
        break;
#endif//TCFG_USER_EMITTER_ENABLE

#if (RCSP_FILE_OPT && TCFG_APP_MUSIC_EN)
    case USER_MSG_SMARTBOX_BS_END:
        printf("USER_MSG_SMARTBOX_BS_END\n");
        u8 reason = (u8)argv[1];
        char *dev_logo = (char *)argv[2];
        char *cur_dev_logo = music_player_get_dev_cur();
        u32 sclust = (u32)argv[3];
        smartbox_browser_stop();
        printf("reason = %d, dev_loop = %s, sclust = %x\n", reason, dev_logo, sclust);
        if (2 == reason) {
            u8 app = app_get_curr_task();
            if (app == APP_MUSIC_TASK) {
                printf("is music mode \n");
                if ((music_player_get_file_sclust() == sclust) //簇号相同
                    && (cur_dev_logo && (strcmp(cur_dev_logo, dev_logo) == 0)) //设备相同
                    && (music_player_get_play_status() == FILE_DEC_STATUS_PLAY) //正在播放
                   ) {
                    //同一个设备的同一首歌曲，在播放的情况，浏览选中不重新播放
                    printf("the same music file!!\n");
                } else {
                    dev_manager_set_active_by_logo(dev_logo);
                    app_task_put_key_msg(KEY_MUSIC_PLAYE_BY_DEV_SCLUST, sclust);
                }
            } else {
                printf("is not music mode\n");
                ///设定音乐模式初次播放参数为按照簇号播放
                music_task_set_parm(MUSIC_TASK_START_BY_SCLUST, sclust);
                ///将选定的设备设置为活动设备
                dev_manager_set_active_by_logo(dev_logo);
                ///切换模式
                app_task_switch_to(APP_MUSIC_TASK);
            }
        }
        JL_CMD_send(JL_OPCODE_FILE_BROWSE_REQUEST_STOP, &reason, 1, JL_NEED_RESPOND);
        break;
#endif
    case USER_MSG_SMARTBOX_MODE_SWITCH:
        printf("USER_MSG_SMARTBOX_MODE_SWITCH\n");
        bool ret = true;
        u8 mode = (u8)argv[1];
        switch (mode) {
        case FM_FUNCTION_MASK:
#if TCFG_APP_FM_EN
            ret = app_task_switch_to(APP_FM_TASK);
#endif
            break;
        case BT_FUNCTION_MASK:
#if TCFG_APP_BT_EN
            ret = app_task_switch_to(APP_BT_TASK);
#endif
            break;
        case MUSIC_FUNCTION_MASK:
#if TCFG_APP_MUSIC_EN
            ret = app_task_switch_to(APP_MUSIC_TASK);
#endif
            break;
        case RTC_FUNCTION_MASK:
#if TCFG_APP_RTC_EN
            ret = app_task_switch_to(APP_RTC_TASK);
#endif
            break;
        case LINEIN_FUNCTION_MASK:
#if TCFG_APP_LINEIN_EN
            ret = app_task_switch_to(APP_LINEIN_TASK);
#endif
            break;
        case FMTX_FUNCTION_MASK:
            break;
        }
        if (false == ret) {
            extern void function_change_inform(u8 app_mode, u8 ret);
            function_change_inform(app_get_curr_task(), ret);
        }
        break;
    case USER_MSG_SMARTBOX_FM_UPDATE_STATE:
        printf("USER_MSG_SMARTBOX_FM_UPDATE_STATE\n");
        extern void smartbot_fm_msg_deal(int msg);
        smartbot_fm_msg_deal(-1);
        break;
    case USER_MSG_SMARTBOX_RTC_UPDATE_STATE:
        /* printf("USER_MSG_SMARTBOX_RTC_UPDATE_STATE\n"); */
        /* extern void smartbot_rtc_msg_deal(int msg); */
        /* smartbot_rtc_msg_deal(-1); */
        break;
    default:
        break;
    }
}

#define APP_SMART_BOX_MSG_VAL_MAX	8
bool smartbox_msg_post(int msg, int argc, ...)
{
    int argv[APP_SMART_BOX_MSG_VAL_MAX] = {0};
    bool ret = true;
    va_list argptr;
    va_start(argptr, argc);

    if (argc > APP_SMART_BOX_MSG_VAL_MAX) {
        printf("%s, msg argc err\n", __FUNCTION__);
        ret = false;
    } else {
        argv[0] = (int)	smartbox_common_event_deal;
        argv[2] = msg;
        for (int i = 0; i < argc; i++) {
            argv[i + 3] = va_arg(argptr, int);
        }

        if (argc >= 2) {
            argv[1] = argc + 1;
        } else {
            argv[1] = 3;
            argc = 3;
        }
        int r = os_taskq_post_type("app_core", Q_CALLBACK, argc + 3, argv);
        if (r) {
            printf("app_next post msg err %x\n", r);
            ret = false;
        }
    }
    return ret;
}

int smartbox_bt_key_event_deal(int key_event, int ret)
{
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL) {
        return ret;
    }
    if (BT_CALL_HANGUP != get_call_status()) {
        return ret;
    }
    switch (key_event) {
    case  KEY_VOL_DOWN:
    case  KEY_VOL_UP:
        smartbox_function_update(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_VOL));
        break;
    }
    return ret;
}

int smartbox_common_key_event_deal(int key_event, int ret)
{
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL) {
        return ret;
    }
    if (BT_CALL_HANGUP != get_call_status()) {
        return ret;
    }
    switch (key_event) {
    case KEY_MINOR_OPT:
#if RCSP_ADV_FIND_DEVICE_ENABLE
        printf("smartbox_find_dev\n");
        extern void smartbox_find_device(void);
        smartbox_find_device();
        ret = true;
#endif
        break;
    case  KEY_VOL_DOWN:
    case  KEY_VOL_UP:
        smartbox_function_update(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_VOL));
        break;
    default:
        if (smartbox_opt_key_event_update(key_event, NULL)) {
            break;
        }
        break;
    }
    return ret;
}

bool smartbox_key_event_filter_before(int key_event)
{
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL) {
        return false;
    }
    if (0 == smart->dev_vol_sync) {
        return false;
    }
    bool ret = false;
    switch (key_event) {
    case KEY_VOL_UP:
        printf("COMMON KEY_VOL_UP\n");
        extern void volume_up(void);
        volume_up();
        UI_SHOW_MENU(MENU_MAIN_VOL, 1000, app_audio_get_volume(APP_AUDIO_CURRENT_STATE), NULL);
        ret = true;
        break;
    case KEY_VOL_DOWN:
        printf("COMMON KEY_VOL_DOWN\n");
        extern void volume_down(void);
        volume_down();
        UI_SHOW_MENU(MENU_MAIN_VOL, 1000, app_audio_get_volume(APP_AUDIO_CURRENT_STATE), NULL);
        ret = true;
        break;
    }
    if (ret) {
        smartbox_function_update(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_VOL));
    }
    return ret;
}

#endif

