#include "smartbox/config.h"
#include "le_smartbox_module.h"
#include "smartbox_adv_bluetooth.h"
#include "smartbox_setting_opt.h"
#include "smartbox_setting_sync.h"
#include "custom_cfg.h"
#include "btstack/avctp_user.h"
#include "smartbox_rcsp_manage.h"
#include "smartbox_music_info_setting.h"

//#if (SMART_BOX_EN && RCSP_SMARTBOX_ADV_EN)
#if (SMART_BOX_EN)

#define RCSP_DEBUG_EN
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                putchar(x)
#define rcsp_printf                    printf
#define rcsp_printf_buf(x,len)         put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_printf_buf(...)
#endif

#define JL_OPCODE_SET_DEVICE_REBOOT		0xE7

extern int get_bt_tws_connect_status();

static u8 adv_setting_result = 0;
static u8 adv_set_deal_one_attr(u8 *buf, u8 size, u8 offset)
{
    u8 rlen = buf[offset];
    if ((offset + rlen + 1) > (size - offset)) {
        rcsp_printf("\n\ndeal attr end!\n\n");
        return rlen;
    }
    u8 type = buf[offset + 1];
    u8 *pbuf = &buf[offset + 2];
    u8 dlen = rlen - 1;
    adv_setting_result = 0;

    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(type);
    if (setting_opt_hdl) {
        adv_setting_result = set_setting_extra_handle(setting_opt_hdl, pbuf, &dlen);
    }
    if (!adv_setting_result) {
        set_smartbox_opt_setting(setting_opt_hdl, NULL);
    }

    return rlen + 1;
}

static u32 JL_opcode_set_adv_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    rcsp_printf("JL_opcode_set_adv_info:\n");
    rcsp_printf_buf(data, len);
    u8 offset = 0;
    while (offset < len) {
        offset += adv_set_deal_one_attr(data, len, offset);
    }
    u8 ret = 0;
    if (adv_setting_result) {
        ret = adv_setting_result;
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &ret, 1);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &ret, 1);
    }
    return 0;
}

static u8 smartbox_adv_get_and_fill_adv_info(u8 *buf, u16 max_len, u8 offset, u8 type)
{
    u8 *adv_data = NULL;
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(type);
    u8 size = get_setting_extra_handle(setting_opt_hdl, &adv_data, NULL);
    if (size && adv_data) {
        size = add_one_attr(buf, max_len, offset, type, (void *)adv_data, size);
    }
    return size;
}

extern void bt_adv_get_bat(u8 *buf);
static u32 JL_opcode_get_adv_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u8 buf[256];
    u8 offset = 0;

    u32 ret = 0;
    u32 mask = READ_BIG_U32(data);
    rcsp_printf("FEATURE MASK : %x\n", mask);
    /* #define ATTR_TYPE_BAT_VALUE      (0) */
    /* #define ATTR_TYPE_EDR_NAME       (1) */
    //get version
    if (mask & BIT(ATTR_TYPE_BAT_VALUE)) {
        rcsp_printf("ATTR_TYPE_BAT_VALUE\n");
        u8 bat[3];
        bt_adv_get_bat(bat);
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_BAT_VALUE, bat, 3);
    }

    if (mask & BIT(ATTR_TYPE_EDR_NAME)) {
        rcsp_printf("ATTR_TYPE_EDR_NAME\n");
        offset += smartbox_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_EDR_NAME);
    }

    if (mask & BIT(ATTR_TYPE_KEY_SETTING)) {
        rcsp_printf("ATTR_TYPE_KEY_SETTING\n");
        offset += smartbox_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_KEY_SETTING);
    }

    if (mask & BIT(ATTR_TYPE_LED_SETTING)) {
        rcsp_printf("ATTR_TYPE_LED_SETTING\n");
        offset += smartbox_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_LED_SETTING);
    }


    if (mask & BIT(ATTR_TYPE_MIC_SETTING)) {
        rcsp_printf("ATTR_TYPE_MIC_SETTING\n");
        offset += smartbox_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_MIC_SETTING);
    }

    if (mask & BIT(ATTR_TYPE_WORK_MODE)) {
        rcsp_printf("ATTR_TYPE_WORK_MODE\n");
        offset += smartbox_adv_get_and_fill_adv_info(buf, sizeof(buf), offset, ATTR_TYPE_WORK_MODE);
    }

#if RCSP_ADV_PRODUCT_MSG_ENABLE
    if (mask & BIT(ATTR_TYPE_PRODUCT_MESSAGE)) {
        rcsp_printf("ATTR_TYPE_PRODUCT_MESSAGE\n");
        u16 vid = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);
        u16 pid = get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
        u8 tversion[6];
        tversion[0] = 0x05;
        tversion[1] = 0xD6;
        tversion[3] = vid & 0xFF;
        tversion[2] = vid >> 8;
        tversion[5] = pid & 0xFF;
        tversion[4] = pid >> 8;
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_PRODUCT_MESSAGE, (void *)tversion, 6);

    }
#endif
    rcsp_printf_buf(buf, offset);

    ret = JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, buf, offset);

    return ret;
}

u8 adv_info_notify(u8 *buf, u16 len)
{
    return JL_CMD_send(JL_OPCODE_ADV_DEVICE_NOTIFY, buf, len, JL_NOT_NEED_RESPOND);
}

u8 adv_info_device_request(u8 *buf, u16 len)
{
    printf("JL_OPCODE_ADV_DEVICE_REQUEST\n");
    return JL_CMD_send(JL_OPCODE_ADV_DEVICE_REQUEST, buf, len, JL_NEED_RESPOND);
}

int JL_smartbox_adv_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    rcsp_printf("JL_rcsp_adv_cmd_resp\n");
    switch (OpCode) {
    case JL_OPCODE_SET_ADV:
        rcsp_printf(" JL_OPCODE_SET_ADV\n");
        JL_opcode_set_adv_info(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_GET_ADV:
        rcsp_printf(" JL_OPCODE_GET_ADV\n");
        JL_opcode_get_adv_info(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_ADV_NOTIFY_SETTING:
        rcsp_printf(" JL_OPCODE_ADV_NOTIFY_SETTING\n");
        bt_ble_adv_ioctl(BT_ADV_SET_NOTIFY_EN, *((u8 *)data), 1);
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
        break;
    case JL_OPCODE_ADV_DEVICE_REQUEST:
        rcsp_printf("JL_OPCODE_ADV_DEVICE_REQUEST\n");
        break;
    case JL_OPCODE_SET_DEVICE_REBOOT:
        rcsp_printf("JL_OPCODE_SET_DEVICE_REBOOT\n");
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
        JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_REBOOT_DEV, NULL, 0);
        break;
    default:
        return 1;
    }
    return 0;
}

static void smartbox_wait_reboot_dev(void *priv)
{
    if (NULL == priv && !rcsp_send_list_is_empty()) {
        return;
    }
    user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    extern void ble_module_enable(u8 en);
    ble_module_enable(0);
    cpu_reset();
}

static void smartbox_rcsp_reboot_dev(void)
{
    extern void adv_edr_name_change_now(void);
    adv_edr_name_change_now();
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        modify_bt_name_and_reset(500);
    } else {
        if (sys_timer_add(NULL, smartbox_wait_reboot_dev, 500) == 0) {
            smartbox_wait_reboot_dev((void *)1);
        }
    }
#else
    if (sys_timer_add(NULL, smartbox_wait_reboot_dev, 500) == 0) {
        smartbox_wait_reboot_dev((void *)1);
    }
#endif

}

int JL_smartbox_adv_event_handler(struct rcsp_event *rcsp)
{
    switch (rcsp->event) {
    case MSG_JL_REBOOT_DEV:
        smartbox_rcsp_reboot_dev();
        break;
    case MSG_JL_UPDATE_SEQ:
        /* printf("MSG_JL_UPDATE_SEQ\n"); */
        /* extern void adv_seq_vaule_sync(void); */
        /* adv_seq_vaule_sync(); */
        break;
    case MSG_JL_UPDAET_ADV_STATE_INFO:
        /* if (get_rcsp_connect_status()) { */
        /* 	JL_rcsp_set_auth_flag(rcsp->args[0]);	 */
        /* 	bt_ble_adv_ioctl(BT_ADV_SET_NOTIFY_EN, rcsp->args[1], 1); */
        /* } */
        break;
    default:
        return 1;
    }
    return 0;
}

// 下面是弹窗的其他设置
int app_core_data_for_send(u8 *packet, u16 size)
{
    //printf("for app send size %d\n", size);

    if (JL_rcsp_get_auth_flag()) {
        *packet = 1;
    } else {
        *packet = 0;
    }

    if (RCSP_SPP == bt_3th_get_cur_bt_channel_sel()) {
        if (get_ble_adv_notify()) {
            *(packet + 1) = 1;
        } else {
            *(packet + 1) = 0;
        }

#if RCSP_ADV_MUSIC_INFO_ENABLE
        if (get_player_time_en()) {
            *(packet + 2) = 1;
        } else {
            *(packet + 2) = 0;
        }
#else
        *(packet + 2) = 0;
#endif

        *(packet + 3) = get_connect_flag();
    }

    return 4;
}

void app_core_data_for_set(u8 *packet, u16 size)
{
    u8 rcsp_auth_flag =  *packet;
    u8 ble_adv_notify =  *(packet + 1);
    u8 player_time_en =  *(packet + 2);
    u8 connect_flag   =  *(packet + 3);

    if (RCSP_SPP == bt_3th_get_cur_bt_channel_sel()) {
        JL_rcsp_set_auth_flag(rcsp_auth_flag);
        set_ble_adv_notify(ble_adv_notify);
#if RCSP_ADV_MUSIC_INFO_ENABLE
        set_player_time_en(player_time_en);
#endif
        set_connect_flag(connect_flag);
    }

    return;
}

u8 smart_auth_res_deal(void)
{
    printf("smart_auth_res_deal\n");
    if ((tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
        if (RCSP_SPP == bt_3th_get_cur_bt_channel_sel()) {
            tws_api_sync_call_by_uuid(TWS_FUNC_APP_OPT_UUID, APP_OPT_SYNC_CMD_RCSP_AUTH_RES, 300);
            return 0;
        } else {
            return 1;
        }
    } else {
        return 1;
    }
}


void rcsp_tws_auth_sync_deal(void)
{
    smart_auth_res_pass();
}
#endif
