#include "app_config.h"
#include "smartbox_update.h"
#if RCSP_UPDATE_EN
//#include "msg.h"
#include "uart.h"
//#include "common/sys_timer.h"
#include "system/timer.h"
#include "update.h"
#include "custom_cfg.h"
//#include "irq_api.h"
#include "btstack/avctp_user.h"
#include "smartbox_misc_setting.h"
#include "smartbox_rcsp_manage.h"
#include "smartbox_bt_manage.h"
#include "update_loader_download.h"
#include "btstack_3th_protocol_user.h"
#include "le_smartbox_module.h"
#include "classic/tws_api.h"
#include "mic_effect.h"

#if (SMART_BOX_EN)

#define RCSP_DEBUG_EN

#ifdef  RCSP_DEBUG_EN
#define rcsp_putchar(x)                putchar(x)
#define rcsp_printf                    printf
#define rcsp_printf_buf(x,len)         printf_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_printf_buf(...)
#endif

#define DEV_UPDATE_FILE_INFO_OFFEST  0x00//0x40
#define DEV_UPDATE_FILE_INFO_LEN     0x00//(0x10 + VER_INFO_EXT_COUNT * (VER_INFO_EXT_MAX_LEN + 1))

typedef enum {
    UPDATA_START = 0x00,
    UPDATA_REV_DATA,
    UPDATA_STOP,
} UPDATA_BIT_FLAG;

#if 0
static update_file_id_t update_file_id_info = {
    .ver[0] = 0xff,
    .ver[1] = 0xff,
};
#else
static update_file_ext_id_t update_file_id_info = {
    .update_file_id_info.ver[0] = 0xff,
    .update_file_id_info.ver[1] = 0xff,
};
#endif

extern const int support_dual_bank_update_en;
extern void ble_app_disconnect(void);
extern void updata_parm_set(UPDATA_TYPE up_type, void *priv, u32 len);
extern u8 check_le_pakcet_sent_finish_flag(void);

static u8 update_flag = 0;
static u8 disconnect_flag = 0;

static void smartbox_update_prepare(void)
{
    ///升级前可以选择关闭需要关闭的模块
    extern void smartbox_function_setting_stop(void);
    smartbox_function_setting_stop();

#if (SOUNDCARD_ENABLE)
    extern void soundcard_close(void);
    soundcard_close();
    return ;
#endif

#if (TCFG_MIC_EFFECT_ENABLE && (0 == SMARTBOX_REVERBERATION_SETTING))
    mic_effect_stop();
#endif

}

static void smartbox_update_fail_and_resume(void)
{
#if (TCFG_MIC_EFFECT_ENABLE)
    mic_effect_start();
#endif

#if (SOUNDCARD_ENABLE)
    extern void soundcard_start(void);
    soundcard_start();
#endif
}

u8 get_jl_update_flag(void)
{
    printf("get_update_flag:%x\n", update_flag);
    return update_flag;
}

void set_jl_update_flag(u8 flag)
{
    update_flag = flag;
    printf("update_flag:%x\n", update_flag);
}

static u8 device_type = 0;
static void set_curr_update_type(u8 type)
{
    device_type = type;
}

u8 get_curr_device_type(void)
{
    return device_type;
}

typedef struct _update_mode_t {
    u8 opcode;
    u8 opcode_sn;

} update_mode_t;

static update_mode_t update_record_info;
u8 tws_need_update = 0;             //标志耳机是否需要强制升级

static void JL_controller_save_curr_cmd_para(u8 OpCode, u8 OpCode_SN)
{
    update_record_info.opcode = OpCode;
    update_record_info.opcode_sn = OpCode_SN;
}

static void JL_controller_get_curr_cmd_para(u8 *OpCode, u8 *OpCode_SN)
{
    *OpCode = update_record_info.opcode;
    *OpCode_SN = update_record_info.opcode_sn;
}

static void (*fw_update_block_handle)(u8 state, u8 *buf, u16 len) = NULL;
static void register_receive_fw_update_block_handle(void (*handle)(u8 state, u8 *buf, u16 len))
{
    fw_update_block_handle = handle;
}

static u16  ble_discon_timeout;
static void ble_discon_timeout_handle(void *priv)
{
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_START, NULL, 0);
}

static u16 wait_response_timeout;
static void wait_response_and_disconn_ble(void *priv)
{
    rcsp_printf("W");
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_DEV_DISCONNECT, NULL, 0);
}
extern u32 ex_cfg_fill_content_api(void);
static void rcsp_update_private_param_fill(UPDATA_PARM *p)
{
    u32 exif_addr;

    exif_addr = ex_cfg_fill_content_api();
    memcpy(p->parm_priv, (u8 *)&exif_addr, sizeof(exif_addr));
}

static void rcsp_update_before_jump_handle(int type)
{
    cpu_reset();
}

#define SMARTBOX_UPDATE_WAIT_SPP_DISCONN_TIME		(1000)
static bool check_edr_is_disconnct(void);
static void wait_response_and_disconn_spp(void *priv)
{
    static u32 wait_time = 0;
    if (check_edr_is_disconnct()) {
        rcsp_printf("b");
        if (wait_time > SMARTBOX_UPDATE_WAIT_SPP_DISCONN_TIME) {
            wait_time = 0;
            user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
        } else {
            wait_time += 100;
        }
        sys_timeout_add(NULL, wait_response_and_disconn_spp, 100);
        return;
    }

    wait_time = 0;
    if (RCSP_BLE == get_curr_device_type()) {
        rcsp_printf("BLE_APP_UPDATE\n");
        update_mode_api_v2(BLE_APP_UPDATA,
                           rcsp_update_private_param_fill,
                           rcsp_update_before_jump_handle);
    } else {
        rcsp_printf("SPP_APP_UPDATA\n");
        update_mode_api_v2(SPP_APP_UPDATA,
                           rcsp_update_private_param_fill,
                           rcsp_update_before_jump_handle);
    }
}

int JL_rcsp_update_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    int ret = 0;
    u8 msg[4];
    rcsp_printf("%s\n", __FUNCTION__);
    switch (OpCode) {
    case JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET:
        if (0 == len) {
            msg[0] = OpCode;
            msg[1] = OpCode_SN;
            rcsp_printf("JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET\n");
            JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP,
                                  MSG_JL_GET_DEV_UPDATE_FILE_INFO_OFFSET,
                                  msg,
                                  2);
        } else {
            rcsp_printf("JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET ERR\n");
        }
        break;
    case JL_OPCODE_INQUIRE_DEVICE_IF_CAN_UPDATE:
        rcsp_printf("JL_OPCODE_INQUIRE_DEVICE_IF_CAN_UPDATE:%x %x\n", len, data[0]);
        if (len) {
            set_curr_update_type(data[0]);
            msg[0] = OpCode;
            msg[1] = OpCode_SN;
            msg[2] = 0x00;
            JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP,
                                  MSG_JL_INQUIRE_DEVEICE_IF_CAN_UPDATE,
                                  msg,
                                  3);
        }
        break;
    case JL_OPCODE_EXIT_UPDATE_MODE:
        rcsp_printf("JL_OPCODE_EXIT_UPDATE_MODE\n");
        break;
    case JL_OPCODE_ENTER_UPDATE_MODE:
        rcsp_printf("JL_OPCODE_ENTER_UPDATE_MODE\n");
        msg[0] = OpCode;
        msg[1] = OpCode_SN;
        JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP,
                              MSG_JL_ENTER_UPDATE_MODE,
                              msg,
                              2);
        break;
    case JL_OPCODE_SEND_FW_UPDATE_BLOCK:
        rcsp_printf("JL_OPCODE_SEND_FW_UPDATE_BLOCK\n");
        break;
    case JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS:
        rcsp_printf("JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS\n");
        JL_controller_save_curr_cmd_para(OpCode, OpCode_SN);
        if (fw_update_block_handle) {
            fw_update_block_handle(UPDATA_STOP, NULL, 0);
        }
        break;
    case JL_OPCODE_SET_DEVICE_REBOOT:
        rcsp_printf("JL_OPCODE_SET_DEVICE_REBOOT\n");
        if (support_dual_bank_update_en) {
            cpu_reset();
        }
        break;
    default:
        ret = 1;
        break;
    }
    return ret;
}

static void JL_rcsp_resp_dev_update_file_info_offest(u8 OpCode, u8 OpCode_SN)
{
    u8 data[4 + 2];
    u16 update_file_info_offset = DEV_UPDATE_FILE_INFO_OFFEST;
    u16 update_file_info_len = DEV_UPDATE_FILE_INFO_LEN;
    WRITE_BIG_U32(data + 0, update_file_info_offset);
    WRITE_BIG_U16(data + 4, update_file_info_len);
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, data, sizeof(data));
}

static void JL_resp_inquire_device_if_can_update(u8 OpCode, u8 OpCode_SN, u8 update_sta)
{
    u8 data[1];
    data[0] = update_sta;
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, data, sizeof(data));
}

enum {
    UPDATE_FLAG_OK,
    UPDATE_FLAG_LOW_POWER,
    UPDATE_FLAG_FW_INFO_ERR,
    UPDATE_FLAG_FW_INFO_CONSISTENT,
    UPDATE_FLAG_TWS_DISCONNECT,
};

static u8 judge_remote_version_can_update(void)
{
    //extern u16 ex_cfg_get_local_version_info(void);
    u16 remote_file_ver = READ_BIG_U16(update_file_id_info.update_file_id_info.ver);
    //u16 local_ver = ex_cfg_get_local_version_info();
    u16 local_ver = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);

#if (0 == VER_INFO_EXT_COUNT)
    //extern u16 ex_cfg_get_local_pid_info(void);
    //extern u16 ex_cfg_get_local_vid_info(void);

    //u16 local_pid = ex_cfg_get_local_pid_info();
    //u16 local_vid = ex_cfg_get_local_vid_info();
    u16 local_pid = get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
    u16 local_vid = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);

    u16 remote_file_pid = READ_BIG_U16(update_file_id_info.update_file_id_info.pid);
    u16 remote_file_vid = READ_BIG_U16(update_file_id_info.update_file_id_info.vid);

    if (remote_file_ver > local_ver || remote_file_pid != local_pid || remote_file_vid != local_vid) {
        return UPDATE_FLAG_FW_INFO_ERR;
    }
#else
    //extern u32 ex_cfg_get_local_authkey_info(u8 * authkey_data[], u8 * authkey_len);
    //extern u32 ex_cfg_get_local_procode_info(u8 * procode_data[], u8 * procode_len);

    u8 authkey_len = 0;
    u8 *local_authkey_data = NULL;
    get_authkey_procode_from_cfg_file(&local_authkey_data, &authkey_len, GET_AUTH_KEY_FROM_EX_CFG);

    u8 procode_len = 0;
    u8 *local_procode_data = NULL;
    get_authkey_procode_from_cfg_file(&local_procode_data, &procode_len, GET_PRO_CODE_FROM_EX_CFG);

    //ex_cfg_get_local_authkey_info(&local_authkey_data, &authkey_len);
    //ex_cfg_get_local_procode_info(&local_procode_data, &procode_len);

    u8 *remote_authkey_data = update_file_id_info.ext;
    u8 *remote_procode_data = update_file_id_info.ext + authkey_len + 1;

    if (remote_file_ver < local_ver
        || 0 != memcmp(remote_authkey_data, local_authkey_data, authkey_len)
        || 0 != memcmp(remote_procode_data, local_procode_data, procode_len)) {
        return UPDATE_FLAG_FW_INFO_ERR;
    }
#endif

    if (remote_file_ver == local_ver) {
        rcsp_printf("remote_file_ver is %x, local_ver is %x, remote_file_ver is similar to local_ver\n", remote_file_ver, local_ver);
        return UPDATE_FLAG_FW_INFO_CONSISTENT;
    }

    return UPDATE_FLAG_OK;
}

static bool check_edr_is_disconnct(void)
{
    if (get_curr_channel_state()) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static bool check_ble_all_packet_sent(void)
{
    if (check_le_pakcet_sent_finish_flag()) {
        return TRUE;
    } else {
        return FALSE;
    }
}


static u32 rcsp_update_data_read(void *priv, u32 offset_addr, u16 len)
{
    u32 err;
    u8 data[4 + 2];
    WRITE_BIG_U32(data, offset_addr);
    WRITE_BIG_U16(data + 4, len);
    err = JL_CMD_send(JL_OPCODE_SEND_FW_UPDATE_BLOCK, data, sizeof(data), JL_NEED_RESPOND);

    return err;
}

static JL_ERR JL_controller_resp_get_dev_refresh_fw_status(u8 OpCode, u8 OpCode_SN, u8 result)
{
    JL_ERR send_err = JL_ERR_NONE;
    u8 data[1];

    data[0] = result;	//0:sucess 1:fail;

    send_err = JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, data, sizeof(data));

    return send_err;
}

static u32 rcsp_update_status_response(void *priv, u8 status)
{
    u8 OpCode;
    u8 OpCode_SN;

    JL_ERR send_err = JL_ERR_NONE;

    JL_controller_get_curr_cmd_para(&OpCode, &OpCode_SN);

    //log_info("get cmd para:%x %x\n", OpCode, OpCode_SN);

    if (JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS == OpCode) {
        send_err = JL_controller_resp_get_dev_refresh_fw_status(OpCode, OpCode_SN, status);
    }

    return send_err;
}



int JL_rcsp_update_cmd_receive_resp(void *priv, u8 OpCode, u8 status, u8 *data, u16 len)
{
    int ret = 0;
    switch (OpCode) {
    case JL_OPCODE_SEND_FW_UPDATE_BLOCK:
        if (fw_update_block_handle) {
            fw_update_block_handle(UPDATA_REV_DATA, data, len);
        }
        break;
    default:
        ret = 1;
        break;
    }
    return ret;
}

static void rcsp_loader_download_result_handle(void *priv, u8 type, u8 cmd)
{
    if (UPDATE_LOADER_OK == cmd) {
        //JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP,MSG_JL_UPDATE_START,NULL,0);
        set_jl_update_flag(1);
        if (support_dual_bank_update_en) {
            rcsp_printf(">>>rcsp update succ\n");
            update_result_set(UPDATA_SUCC);
        }
    } else {
        rcsp_printf(">>>update loader err\n");
        smartbox_update_fail_and_resume();
        /* #if OTA_TWS_SAME_TIME_ENABLE */
        /*         if((tws_ota_control(OTA_TYPE_GET) == OTA_TWS)) { */
        /*             tws_ota_stop(OTA_STOP_UPDATE_OVER_ERR); */
        /*         } */
        /* #endif */
        //rcsp_db_update_fail_deal();
    }
}



extern void register_receive_fw_update_block_handle(void (*handle)(u8 state, u8 *buf, u16 len));
extern void rcsp_update_data_api_register(u32(*data_send_hdl)(void *priv, u32 offset, u16 len), u32(*send_update_handl)(void *priv, u8 state));
extern void rcsp_update_handle(void *buf, int len);
extern void bt_set_low_latency_mode(int enable);
extern void bt_adv_seq_change(void);
extern int bt_tws_poweroff();
extern void bt_wait_phone_connect_control(u8 enable);
extern int tws_api_get_role(void);
int JL_rcsp_update_msg_deal(void *hdl, u8 event, u8 *msg)
{
    int ret = 0;
    u16 remote_file_version;
    u8 can_update_flag = UPDATE_FLAG_FW_INFO_ERR;

    printf("---%s --- %d   %d\n", __func__, __LINE__, event);
    switch (event) {
    case MSG_JL_GET_DEV_UPDATE_FILE_INFO_OFFSET:
        rcsp_printf("MSG_JL_GET_DEV_UPDATE_FILE_INFO_OFFSET\n");

        /* extern void linein_mutex_stop(void *priv); */
        /* linein_mutex_stop(NULL); */
        /* extern void linein_mute(u8 mute); */
        /* linein_mute(1); */

        JL_rcsp_resp_dev_update_file_info_offest((u8)msg[0], (u8)msg[1]);
        break;

    case MSG_JL_INQUIRE_DEVEICE_IF_CAN_UPDATE:
        rcsp_printf("MSG_JL_INQUIRE_DEVEICE_IF_CAN_UPDATE\n");
#if 0
        remote_file_version = READ_BIG_U16(update_file_id_info.update_file_id_info.ver);
        rcsp_printf("remote_file_ver:V%d.%d.%d.%d\n",
                    (remote_file_version & 0xf000) >> 12,
                    (remote_file_version & 0x0f00) >> 8,
                    (remote_file_version & 0x00f0) >> 4,
                    (remote_file_version & 0x000f));

        if (0 == remote_file_version) {
            can_update_flag = UPDATE_FLAG_OK;
        } else {
            can_update_flag = judge_remote_version_can_update();
        }

        if (UPDATE_FLAG_OK == can_update_flag) {
            smartbox_update_prepare();
            set_jl_update_flag(1);
        }
#else
#if (TCFG_USER_TWS_ENABLE && OTA_TWS_SAME_TIME_ENABLE)
        int get_bt_tws_connect_status();
        if (get_bt_tws_connect_status() || (!support_dual_bank_update_en)) {    //单备份返回成功
            can_update_flag = UPDATE_FLAG_OK;
        } else {
            can_update_flag = UPDATE_FLAG_TWS_DISCONNECT;
        }
#else
        can_update_flag = UPDATE_FLAG_OK;
#endif      //endif OTA_TWS_SAME_TIME_ENABLE
#endif
        //todo;judge voltage
        JL_resp_inquire_device_if_can_update((u8)msg[0], (u8)msg[1], can_update_flag);

        if (UPDATE_FLAG_OK == can_update_flag) {
            smartbox_update_prepare();
        }

        if (0 == support_dual_bank_update_en) {
#if (TCFG_USER_TWS_ENABLE == 1)
            if (!tws_api_get_role()) {
#else
            if (1) {
#endif
                JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_LOADER_DOWNLOAD_START, NULL, 0);
            } else {
                tws_need_update = 1;                //置上该标志位APP重新连接的时候强制进入升级
            }
            //bt_tws_poweroff();          //单备份升级断开tws
            //bt_adv_seq_change();
        }

        break;

    case MSG_JL_DEV_DISCONNECT:
        if (rcsp_send_list_is_empty() && check_ble_all_packet_sent()) {
            rcsp_printf("MSG_JL_DEV_DISCONNECT\n");
            ble_app_disconnect();
            if (check_edr_is_disconnct()) {
                puts("-need discon edr\n");
                user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
            }
            ble_discon_timeout = sys_timeout_add(NULL, ble_discon_timeout_handle, 1000);
        } else {
            wait_response_timeout = sys_timeout_add(NULL, wait_response_and_disconn_ble, 100);
        }
        break;

    case MSG_JL_LOADER_DOWNLOAD_START:
        printf("---%s --- %d\n", __func__, __LINE__);
        rcsp_update_data_api_register(rcsp_update_data_read, rcsp_update_status_response);
        register_receive_fw_update_block_handle(rcsp_update_handle);
        if (RCSP_BLE == get_curr_device_type()) {
            rcsp_update_loader_download_init(BLE_APP_UPDATA, rcsp_loader_download_result_handle);
        } else if (RCSP_SPP == get_curr_device_type()) {
            rcsp_update_loader_download_init(SPP_APP_UPDATA, rcsp_loader_download_result_handle);
        }
        break;

    case MSG_JL_UPDATE_START:
        sys_timeout_add(NULL, wait_response_and_disconn_spp, 100);
        break;

    case MSG_JL_ENTER_UPDATE_MODE:
        rcsp_printf("MSG_JL_ENTER_UPDATE_MODE:%x %x\n", msg[0], msg[1]);
        bt_set_low_latency_mode(0);
#if TCFG_USER_TWS_ENABLE
        if (support_dual_bank_update_en && !tws_api_get_role()) {
#else
        if (support_dual_bank_update_en) {
#endif
            u8 status = 0;
            JL_CMD_response_send(msg[0], JL_PRO_STATUS_SUCCESS, msg[1], &status, 1);
            rcsp_update_data_api_register(rcsp_update_data_read, rcsp_update_status_response);
            register_receive_fw_update_block_handle(rcsp_update_handle);
            rcsp_update_loader_download_init(DUAL_BANK_UPDATA, rcsp_loader_download_result_handle);
        }
        break;

    default:
        ret = 1;
        break;
    }
    return ret;
}


static u8 rcsp_update_flag = 0;
void set_rcsp_db_update_status(u8 value)
{
    rcsp_update_flag = value;
}

u8 get_rcsp_db_update_status()
{
    return rcsp_update_flag;
}

void rcsp_before_enter_db_update_mode() //进入双备份升级前
{
    r_printf("%s", __func__);
    rcsp_update_flag = 1;
    void sys_auto_shut_down_disable(void);
    if (get_total_connect_dev() == 0) {
        sys_auto_shut_down_disable();
    }
#if TCFG_USER_TWS_ENABLE
    int tws_api_get_role(void);
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
#endif
    if (get_total_connect_dev()) { //断开蓝牙连接，断开之后不能让他重新开
        /* user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL); */
    }
#if TCFG_USER_TWS_ENABLE
    extern 	void tws_cancle_all_noconn();
    /* tws_cancle_all_noconn() ; */
#else
    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
#endif
}

extern void sys_auto_shut_down_enable(void);
void rcsp_db_update_fail_deal() //双备份升级失败处理
{
    r_printf("%s", __func__);
    if (rcsp_update_flag) {
        rcsp_update_flag = 0;
        if (get_total_connect_dev() == 0) {
            sys_auto_shut_down_enable();
        }
    }
    /* cpu_reset(); //升级失败直接复位 */
}

#endif
#endif

