#include "gma_app_main.h"
#include "3th_profile_api.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack_3th_protocol_user.h"
#include "key_event_deal.h"
#include "soundbox.h"
#include "bt_tws.h"
#include "bt_common.h"
#include "gma.h"
#include "tone_player.h"
#include "app_main.h"
#include "clock_cfg.h"

#define GMA_APP_DEBUG_ENABLE
#ifdef GMA_APP_DEBUG_ENABLE
#define gma_app_printf printf
#else
#define gma_app_printf(...)
#endif//GMA_APP_DEBUG_ENABLE

#if (GMA_EN)

#define GMA_APP_SPEECH_TONE_EN			1

enum {
    GMA_APP_SPEECH_STATUS_IDLE = 0x0,
    GMA_APP_SPEECH_STATUS_START_REPORT,
    GMA_APP_SPEECH_STATUS_REC,
};

static volatile u8 app_speech_start_status = 0;
static volatile u16 gma_app_speech_timer = 0;
extern int ai_mic_rec_start(void);
extern int ai_mic_rec_close(void);
extern int ai_mic_is_busy(void);
extern u8 get_remote_dev_company(void);
extern void phone_call_close_ble_adv(void);
extern void phone_call_resume_ble_adv(void);
extern u8 get_app_connect_type(void);
extern void ble_ibeacon_adv(void);
extern void phone_call_change_ble_conn_param(u8 param_index);
extern void phone_call_begin_ai(void);

void mic_rec_clock_set(void)
{
    clock_add_set(AI_SPEECH_CLK);
}

void mic_rec_clock_recover(void)
{
    clock_remove_set(AI_SPEECH_CLK);
}

int gma_hw_stop_speech(void)
{
    int err = app_task_put_key_msg(KEY_SEND_SPEECH_STOP, 0);
    if (err) {
        gma_app_printf("speech stop err !!");
        return -1;
    }
    printf("gma_hw_stop_speech >>>>>>>>>>\n");
    return 0;
}

int gma_hw_start_speech(void)
{
    if (app_speech_start_status) {
        printf("gma_hw_start_speech aready!!\n");
        return 0;
    }
    int err = app_task_put_key_msg(KEY_APP_SEND_SPEECH_START, 0);
    if (err) {
        gma_app_printf("speech start err !!");
        return -1;
    }
    printf("gma_hw_start_speech >>>>>>>>>>\n");
    return 0;
}

static void gma_speech_stop(void)
{
    if (gma_app_speech_timer) {
        sys_timeout_del(gma_app_speech_timer);
        gma_app_speech_timer = 0;
    }
    app_speech_start_status = GMA_APP_SPEECH_STATUS_IDLE;
    gma_app_printf("gma_speech_stop\n");
    ///mqc mark
    ai_mic_rec_close();
}

static void gma_speech_timeout(void *priv)
{
    gma_app_printf("gma speech timeout !!! \n");
    gma_speech_stop();
}
static int gma_speech_start(void)
{
    if (ai_mic_is_busy()) {
        if (gma_app_speech_timer) {
            sys_timer_modify(gma_app_speech_timer, 8000);
        }
        return -1;
    }

    if (ai_mic_rec_start() == 0) {
        gma_app_speech_timer = sys_timeout_add(NULL, gma_speech_timeout, 8000);
        app_speech_start_status = GMA_APP_SPEECH_STATUS_REC;
        return 0;
    }
    gma_app_printf("speech start fail\n");
    return -1;
}

static int gma_key_speech_start(void)
{
    if (BT_STATUS_TAKEING_PHONE == get_bt_connect_status()) {
        gma_app_printf("phone ing...\n");
        return -1;
    }

    if (gma_connect_success() && (get_curr_channel_state()&A2DP_CH)) {
    } else {
        if (get_curr_channel_state()&A2DP_CH) {
            //<GMA未连接， 但是A2DP已连接， 点击唤醒键， 提示TTS【请打开小度APP】
        } else {
            //<蓝牙完全关闭状态， 用户按唤醒键， 提示TTS【蓝牙未连接， 请用手机蓝牙和我连接吧】
        }
        gma_app_printf("A2dp or gma no connect err!!!\n");
        return -1;
    }

    if (app_var.siri_stu) {
        gma_app_printf("siri activing...\n");
        return -1;
    }

    ///按键启动需要report, app启动， 直接启动语音即可
    gma_app_printf("gma start mic status report \n");
    if (gma_mic_status_report(START_MIC)) {
        gma_app_printf("gma_mic_status_report fail\n");
        return -1;
    }

    app_speech_start_status = GMA_APP_SPEECH_STATUS_START_REPORT;
    return 0;
}


static void  gma_app_tone_play_end_callback(void *priv, int flag)
{
    u32 index = (u32)priv;
    switch (index) {
    case IDEX_TONE_SPEECH_START:
        gma_app_printf("IDEX_TONE_SPEECH_START\n");
        gma_speech_start();
        break;
    default:
        break;
    }
}

void gma_app_key_event_deal(struct sys_event *event)
{
    int ret = false;
    struct key_event *key = &event->u.key;
    int key_event = event->u.key.event;
    int key_value = event->u.key.value;
    switch (key_event) {
    case KEY_APP_SEND_SPEECH_START:
        gma_app_printf("KEY_APP_SEND_SPEECH_START \n");
        tone_play_with_callback_by_name(tone_table[IDEX_TONE_SPEECH_START], 1, gma_app_tone_play_end_callback, (void *)IDEX_TONE_SPEECH_START);
        break;
    case KEY_SEND_SPEECH_START:
        gma_app_printf("KEY_SEND_SPEECH_START \n");
        if (gma_key_speech_start() == 0) {
            tone_play_with_callback_by_name(tone_table[IDEX_TONE_SPEECH_START], 1, gma_app_tone_play_end_callback, (void *)IDEX_TONE_SPEECH_START);
        }
        break;
    case KEY_SEND_SPEECH_STOP:
        gma_app_printf("KEY_SEND_SPEECH_STOP \n");
        gma_speech_stop();
        break;
    }
}


static void gma_ble_adv_ctl(void)
{
#if (GMA_BLE_ADV_MODE == 1)
#if TCFG_USER_TWS_ENABLE
    void ble_module_enable(u8 en);
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        gma_active_para_by_hci_para(1);
    } else {
        ble_module_enable(0);
    }
#else
    gma_active_para_by_hci_para(1);
#endif
#endif
}

static bool is_ios_system(void)
{
#define REMOTE_DEV_ANDROID			1
#define REMOTE_DEV_IOS				2
    gma_app_printf("remote_dev_company :%d \n", get_remote_dev_company());
    return (get_remote_dev_company() == REMOTE_DEV_IOS);
}

static int gma_app_bt_status_event_handler(struct bt_event *bt)
{
    switch (bt->event) {
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
        if (bt->value != 0xff) {
        } else {
        }
        break;
    case BT_STATUS_PHONE_INCOME:
        phone_call_close_ble_adv();
    case BT_STATUS_PHONE_OUT:
    case BT_STATUS_PHONE_ACTIVE:
        break;
    case BT_STATUS_PHONE_HANGUP:
        phone_call_resume_ble_adv();
        break;

    case BT_STATUS_PHONE_MANUFACTURER:
        if (!get_app_connect_type()) {
            gma_ble_adv_ctl();
            if (is_ios_system()) {
                ble_ibeacon_adv();
            }

            if (get_call_status() != BT_CALL_HANGUP) {
                phone_call_close_ble_adv();
            }
        }
        break;
    case BT_STATUS_VOICE_RECOGNITION:
        if ((app_var.siri_stu == 1) || (app_var.siri_stu == 2)) {
            phone_call_begin_ai();
            phone_call_close_ble_adv();
        } else if (app_var.siri_stu == 0) {
            phone_call_resume_ble_adv();
        }
        break;
    }
    return 0;
}

static int gma_app_hci_event_handler(struct bt_event *bt)
{
    switch (bt->event) {
    case HCI_EVENT_CONNECTION_COMPLETE:
        switch (bt->value) {
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            break;
        }
        break;
    }

    return 0;
}

static void gma_app_opt_tws_event_handler(struct bt_event *bt)
{
}

static void gma_app_bt_tws_event_handler(struct bt_event *bt)
{
    int role = bt->args[0];
    int phone_link_connection = bt->args[1];
    int reason = bt->args[2];

    switch (bt->event) {
    case TWS_EVENT_CONNECTED:
        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        /*
         * 跟手机的链路LMP层已完全断开, 只有tws在连接状态才会收到此事件
         */
        if (reason == 0x0b) {
            //CONNECTION ALREADY EXISTS
        } else {
        }
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        /*
         * TWS连接断开
         */
        break;
    case TWS_EVENT_ROLE_SWITCH:
        break;
    }
}

static int gma_ai_event_handler(struct bt_event *bt)
{
#if 0
    switch (bt->event) {
    case  KEY_CALL_LAST_NO:
        log_info("KEY_CALL_LAST_NO \n");
        if (bt_user_priv_var.last_call_type ==  BT_STATUS_PHONE_INCOME) {
            user_send_cmd_prepare(USER_CTRL_DIAL_NUMBER, bt_user_priv_var.income_phone_len,
                                  bt_user_priv_var.income_phone_num);
        } else {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_LAST_NO, 0, NULL);
        }
        break;
    case  KEY_CALL_HANG_UP:
        log_info("KEY_CALL_HANG_UP \n");
        if ((get_call_status() == BT_CALL_ACTIVE) ||
            (get_call_status() == BT_CALL_OUTGOING) ||
            (get_call_status() == BT_CALL_ALERT) ||
            (get_call_status() == BT_CALL_INCOMING)) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        break;
    case  KEY_CALL_ANSWER:
        log_info("KEY_CALL_ANSWER \n");
        if (get_call_status() == BT_CALL_INCOMING) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
        }
        break;

    case KEY_AI_DEC_SUSPEND:
        a2dp_tws_dec_suspend(NULL);
        break;

    case KEY_AI_DEC_RESUME:
        a2dp_tws_dec_resume();
        break;

    case KEY_GMA_SYNC_TRIPLE_SECRET:
        gma_send_secret_to_sibling();
        break;
    default:
        break;
    }
#endif
    return 0;
}

int gma_app_bt_state_set_page_scan_enable(void)
{
    return 0;
}

int gma_app_bt_state_get_connect_mac_addr(void)
{
    return 0;
}

int gma_app_bt_state_cancel_page_scan(void)
{
    return 0;
}

int gma_app_bt_state_tws_init(int paired)
{
    return 0;
}

int gma_app_bt_state_tws_connected(int first_pair, u8 *comm_addr)
{
    return 0;
}

int gma_app_bt_state_enter_soft_poweroff(void)
{
    return 0;
}

void gma_app_bredr_handle_reg(void)
{
    spp_data_deal_handle_register(user_spp_data_handler);
}

int gma_app_sys_event_handler_specific(struct sys_event *event)
{
    switch (event->type) {
    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            gma_app_bt_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            gma_app_hci_event_handler(&event->u.bt);
        }
#if TCFG_USER_TWS_ENABLE
        else if (((u32)event->arg == SYS_BT_EVENT_FROM_TWS)) {
            gma_app_bt_tws_event_handler(&event->u.bt);
        } else if (((u32)event->arg == SYS_BT_EVENT_FROM_APP_OPT_TWS)) {
            gma_app_opt_tws_event_handler(&event->u.bt);
        }
#endif
#if OTA_TWS_SAME_TIME_ENABLE
        else if (((u32)event->arg == SYS_BT_OTA_EVENT_TYPE_STATUS)) {
        }
#endif
        else if (((u32)event->arg == SYS_BT_AI_EVENT_TYPE_STATUS)) {
            gma_ai_event_handler(&event->u.bt);
        }
        break;
    case SYS_DEVICE_EVENT:
        break;

    case SYS_KEY_EVENT:
        gma_app_key_event_deal(event);
        break;

    case SYS_BT_AI_EVENT:
        break;

    }

    return 0;
}

int gma_app_bt_state_init(void)
{
    gma_spp_init();
    return 0;
}
#endif//GMA_EN

