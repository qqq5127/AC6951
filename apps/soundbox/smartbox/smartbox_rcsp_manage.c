#include "smartbox/config.h"

#include "app_main.h"
#include "smartbox_update_tws.h"
#include "3th_profile_api.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_tws.h"
#include "smartbox_rcsp_manage.h"
#include "le_smartbox_module.h"

#include "smartbox_setting_opt.h"
#include "smartbox_adv_bluetooth.h"
#include "smartbox/function.h"
#include "smartbox_music_info_setting.h"
#include "smartbox_update.h"
#include "JL_rcsp_protocol.h"
#include "file_transfer.h"

#if SMART_BOX_EN

#define LOG_TAG             "[RCSP-ADV]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

extern u8 check_le_pakcet_sent_finish_flag(void);
static void smartbox_user_state_handler(u8 *param, u8 param_len);

u8 get_rcsp_connect_status(void)
{
#if 1//RCSP_UPDATE_EN
    if (RCSP_BLE == bt_3th_get_cur_bt_channel_sel()) {
        if (bt_3th_get_jl_ble_status() == BLE_ST_CONNECT || bt_3th_get_jl_ble_status() == BLE_ST_NOTIFY_IDICATE) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return bt_3th_get_jl_spp_status();
    }
#else
    return 0;
#endif
}

void JL_rcsp_event_to_user(u32 type, u8 event, u8 *msg, u8 size)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;

    if (size > sizeof(e.u.rcsp.args)) {
        printf("rcsp event size overflow:%x %x\n", size, sizeof(e.u.rcsp.args));
    }

    e.arg  = (void *)type;
    e.u.rcsp.event = event;

    if (size) {
        memcpy(e.u.rcsp.args, msg, size);
    }

    e.u.rcsp.size = size;

    sys_event_notify(&e);
}

static void wait_response_and_disconn_ble(void *priv)
{
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_SWITCH_DEVICE, NULL, 0);
}

int JL_rcsp_event_handler(struct rcsp_event *rcsp)
{
    int ret = 0;
    switch (rcsp->event) {
    case MSG_JL_ADV_SETTING_SYNC:
        update_smartbox_setting(-1);
        break;
    case MSG_JL_ADV_SETTING_UPDATE:
        update_info_from_vm_info();
        break;
    case MSG_JL_SWITCH_DEVICE:
        printf("MSG_JL_SWITCH_DEVICE\n");
        static u16 wait_response_timeout = 0;
        static u8 wait_cnt = 0;
        // ble切换spp：前一个条件开始不满足，之后满足; 后一个条件开始满足，之后不满足; 超时10次后自动断开ble
        if (wait_response_timeout || (RCSP_BLE ==  bt_3th_get_cur_bt_channel_sel() && RCSP_SPP == rcsp->args[0])) {
            if ((10 == wait_cnt) || (rcsp_send_list_is_empty() && check_le_pakcet_sent_finish_flag())) {
                if (wait_response_timeout) {
                    sys_timeout_del(wait_response_timeout);
                    wait_response_timeout = 0;
                }
                wait_cnt = 0;
                ble_app_disconnect();
            } else {
                wait_cnt++;
                wait_response_timeout = sys_timeout_add(NULL, wait_response_and_disconn_ble, 100);
            }
        }
        break;
    case MSG_JL_USER_SPP_BLE_STATE:
        smartbox_user_state_handler(rcsp->args, rcsp->size);
        break;
#if RCSP_ADV_MUSIC_INFO_ENABLE
    case MSG_JL_UPDATE_PLAYER_TIME:
        if (JL_rcsp_get_auth_flag()) {
            smartbox_function_update(BT_FUNCTION_MASK, 0x100);
        }
        break;
    case MSG_JL_UPDATE_PLAYER_STATE:
        if (JL_rcsp_get_auth_flag()) {
            smartbox_function_update(BT_FUNCTION_MASK, 0x80);
        }
        break;
    case MSG_JL_UPDATE_MUSIC_INFO:
        if (JL_rcsp_get_auth_flag()) {
            /* printf("rcsp type %x\n",rcsp->args[0]); */
            smartbox_function_update(BT_FUNCTION_MASK, BIT(rcsp->args[0] - 1));
        }
        break;
    case  MSG_JL_UPDATE_MUSIC_PLAYER_TIME_TEMER:
        if (JL_rcsp_get_auth_flag()) {
            music_player_time_timer_deal(rcsp->args[0]);
        }
        break;
#endif

    default:
#if 1//RCSP_SMARTBOX_ADV_EN
        if (0 == JL_smartbox_adv_event_handler(rcsp)) {
            break;
        }
#endif
#if RCSP_UPDATE_EN
        if (0 == JL_rcsp_update_msg_deal(NULL, rcsp->event, rcsp->args)) {
            break;
        }
#endif
        break;
    }

    return ret;
}

static void smartbox_rcsp_disconnect(void)
{
    file_transfer_close();
}

static void smartbox_rcsp_connect(void)
{

}

static void smartbox_user_state_event(u8 opcode, u8 state)
{
    u8 data[2] = {0};
    data[0]	= opcode;
    data[1]	= state;
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_USER_SPP_BLE_STATE, data, sizeof(data));
}

static void rcsp_user_event_ble_handler(ble_state_e ble_status, u8 flag)
{
    switch (ble_status) {
    case BLE_ST_IDLE:
#if RCSP_UPDATE_EN
        if (get_jl_update_flag()) {
            JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_START, NULL, 0);
        }
#endif
        break;
    case BLE_ST_CONNECT:
        if (flag) {
            smartbox_user_state_event(BT_3TH_EVENT_COMMON_BLE_STATUS, BLE_ST_CONNECT);
            break;
        }
        smartbox_rcsp_connect();
#if (RCSP_ADV_FIND_DEVICE_ENABLE)
        printf("smartbox_find_device_reset\n");
        extern void smartbox_find_device_reset(void);
        smartbox_find_device_reset();
#endif
#if (MUTIl_CHARGING_BOX_EN)
        ble_app_disconnect();
#endif
        break;
    case BLE_ST_DISCONN:
        if (flag) {
            smartbox_user_state_event(BT_3TH_EVENT_COMMON_BLE_STATUS, BLE_ST_DISCONN);
            break;
        }
        smartbox_rcsp_disconnect();
#if RCSP_UPDATE_EN
        if (get_jl_update_flag()) {
            bt_ble_adv_enable(0);
        }
#endif
        break;
    }
}

static void rcsp_user_event_spp_handler(u8 spp_status, u8 flag)
{
    switch (spp_status) {
    case 0:
        break;
    case 1:
        if (flag) {
            smartbox_user_state_event(BT_3TH_EVENT_COMMON_SPP_STATUS, spp_status);
            break;
        }
        smartbox_rcsp_connect();
#if (0 == BT_CONNECTION_VERIFY)
        JL_rcsp_auth_reset();
#endif
#if (RCSP_ADV_FIND_DEVICE_ENABLE)
        printf("smartbox_find_device_reset\n");
        extern void smartbox_find_device_reset(void);
        smartbox_find_device_reset();
#endif
        break;
    default:
        if (flag) {
            smartbox_user_state_event(BT_3TH_EVENT_COMMON_SPP_STATUS, spp_status);
            break;
        }
        smartbox_rcsp_disconnect();
#if RCSP_ADV_MUSIC_INFO_ENABLE
        stop_get_music_timer(1);
#endif
#if (0 == BT_CONNECTION_VERIFY)
        JL_rcsp_auth_reset();
#endif
#if RCSP_UPDATE_EN
        if (get_jl_update_flag()) {
            JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_START, NULL, 0);
        }
#endif  //rcsp_update_en
        break;
    }
}

static void rcsp_user_event_select_handler(void)
{
#if RCSP_UPDATE_EN
    set_jl_update_flag(0);
#endif
}

static void smartbox_user_state_handler(u8 *param, u8 param_len)
{
    u8 opcode = param[0];
    u8 state = param[1];
    switch (opcode) {
    case BT_3TH_EVENT_COMMON_BLE_STATUS:
        rcsp_user_event_ble_handler((ble_state_e)state, 0);
        break;
    case BT_3TH_EVENT_COMMON_SPP_STATUS:
        rcsp_user_event_spp_handler(state, 0);
        break;
    }
}

void rcsp_user_event_handler(u16 opcode, u8 *data, int size)
{
    ble_state_e ble_status;
    u8 spp_status;

    switch (opcode) {
    case BT_3TH_EVENT_COMMON_BLE_STATUS:
        ble_status = data[0];
        rcsp_user_event_ble_handler(ble_status, 1);
        break;

    case BT_3TH_EVENT_COMMON_SPP_STATUS:
        spp_status = data[0];
        rcsp_user_event_spp_handler(spp_status, 1);
        break;

    case BT_3TH_EVENT_RCSP_DEV_SELECT:
        rcsp_user_event_select_handler();
        break;
    }
}

#endif
