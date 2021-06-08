#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "typedef.h"
#include "btstack/third_party/baidu/dueros_dma.h"
#include "btstack/third_party/baidu/dueros_dma_pb.h"
#include "btstack/third_party/baidu/dueros_platform.h"
/* #include "dueros_spp_user.h" */
#include "system/task.h"
#include "os/os_api.h"
#include "app_config.h"
#include "app_action.h"
#include "generic/lbuf.h"
#include "user_cfg.h"
#include "btstack/avctp_user.h"
#include "tone_player.h"
#include "key_event_deal.h"
#include "event.h"
#include "key_event_deal.h"
#include "dma_deal.h"
#include "classic/tws_local_media_sync.h"
#include "bt_tws.h"
#include "bt_common.h"
#include "3th_profile_api.h"
#include "system/timer.h"
#include "spp_user.h"
#include "btstack/frame_queque.h"
#include "app_task.h"

#define log_info printf


#if (DUEROS_DMA_EN)

#define  DUEROS_DEBUG_CLOSE	  1
#define APP_CMD_LEN (0x10)
struct app_cmd_ctrl {
    u8 pos_read;
    u8 pos_write;
    u8 app_rand[8];
    u8 speech_data[6];
    u8 app_cmd_buf[APP_CMD_LEN];
};
struct app_cmd_ctrl app_user_cmd;

enum {
    PAIR_STEPS_INIT = 0,
    PAIR_STEPS_GET_DEV_INFO,
    PAIR_STEPS_SUCESS,
};

extern void mic_set_data_source(u8 data_type);
extern void stack_run_loop_resume();
extern u32 tws_link_read_slot_clk();
extern void app_tws_send_data(u8 cmd, u8 opcode, u8 *data, u8 len);
extern void tws_ai_send_speech_stop();
extern void tws_dueros_rand_set_vm(u8 *data, u8 len);
extern void ai_tws_send_data_to_sibling(u8 cmd, u8 type);
u8 mic_get_data_source(void);
_WEAK_ int ai_mic_rec_start(void)
{
    return 0;
}
_WEAK_ int ai_mic_rec_close(void)
{
    return 0;
}
void tws_ble_dueros_handle_create();

static u8 dueros_pair_state;
char dma_serial_number[] = "12345667";
int  mic_coder_busy = 0;
u8 tws_dma_data[8] = {0};

#define DMA_FRAME_MEM_LEN  512+256+128//1024
#define DMA_SYS_MEM_LEN    1024
static u8 dma_frame_mem_pool[DMA_FRAME_MEM_LEN] ALIGNED(0x4);
static u8 dma_sys_mem_pool[DMA_SYS_MEM_LEN] ALIGNED(0x4);

static volatile u8 chl_lock_flag = 0;

#define DMA_TWS_MEM_LEN 1024*2
/* u8 dma_tws_mic_pool[DMA_TWS_MEM_LEN] ALIGNED(0x4); */
u8 *dma_tws_mic_pool = NULL;


char bt_address[18] = {0,};
u8 dma_classic_bluetooth_mac[6] = {0x09, 0x0B, 0x01, 0x0F, 0x03, 0x11};
Transport dma_supported_transports[2] =  {TRANSPORT__BLUETOOTH_LOW_ENERGY, TRANSPORT__BLUETOOTH_RFCOMM};
#if (AUDIO_FORMAT_TYPE == FORMAT_OPUS)
AudioFormat supported_audio_formats = AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS;
#else
AudioFormat supported_audio_formats = AUDIO_FORMAT__SPEEX_16KHZ_16KBPS;//AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS;
#endif

static PROTOCOL_VER protocol_v = {
    .magic = DUEROS_DMA_MAGIC,
    .hVersion = DUEROS_DMA_HIGH_VERSION,
    .lVersion = DUEROS_DMA_LOW_VERSION,
};

static void (*dueros_speech_start)(void) = NULL;
static void (*dueros_speech_stop)(void) = NULL;
static DUEROS_DMA *dueros_dma_api;

extern int  dueros_dma_send_no_pair_ack(char *request_id);
static bool set_dueros_pair_state(u8 new_state, u8 init_flag)
{
    if (!init_flag) {
        if (new_state != dueros_pair_state + 1) {
            log_info("!!!---pair_steps fail: old= %d,new= %d\n", dueros_pair_state, new_state);
            return FALSE;
        }
    }
    dueros_pair_state = new_state;
    return TRUE;
}


int dueros_process_cmd_from_phone(unsigned char *buf, int len)
{
    ControlEnvelope *envelope = NULL ;
    Dialog *p_dialog = NULL;
    InitiatorType initiatorTye ;
    unsigned char  _1BYTE = 0;
    unsigned char  _2BYTE = 0;

    log_info(">>>>>>>dueros_process_cmd_from_phone<<<<<<<<\n");

    DUEROS_DMA_HEADER *p_header  = (DUEROS_DMA_HEADER *)buf;

    u8 *payload_buf = NULL;
    WORD32 payload_len = 0, process_len = 0;

    if (get_dma_state() == DUEROS_DMA_CLOSE) {
        log_info("dueros_dma.inited != DUEROS_DMA_INITED\n");
        return -1;
    }


    _1BYTE = *buf;
    _2BYTE = *(buf + 1);

    p_header->version = ((_1BYTE >> 4)& _4_BITS);
    p_header->streamID = (((_1BYTE & _4_BITS) << 1) | (_2BYTE >> 7));
    p_header->reserve = ((_2BYTE >> 1)&_6_BITS);
    p_header->length  = (_2BYTE & _1_BITS);


    if (p_header->length) {
        payload_buf = buf + 4;
        payload_len = buf[2] << 8 | payload_buf[3];
        process_len = payload_len + 4;
    } else {
        payload_buf = buf + 3;
        payload_len = buf[2];
        process_len = payload_len + 3;
    }


    envelope = control_envelope__unpack(NULL, payload_len, payload_buf);
    if (envelope == NULL) {
        log_info("control_envelopy__unpack error !!! \n");
        return -1;
    }
    log_info("------envelope->command= %d\n", envelope->command);

    switch (envelope->command) {

    case COMMAND__PROVIDE_SPEECH :

        log_info("------COMMAND__PROVIDE_SPEECH id= %s\n", envelope->request_id);

        if (dueros_dma_check_summary(envelope) != 0) {
            dueros_dma_send_check_sum_fail(COMMAND__PROVIDE_SPEECH_ACK, envelope->request_id);
            break;
        }

        dueros_dma_send_prov_speech_ack(envelope->providespeech->dialog->id, envelope->request_id);

        /* dueros_dma.dma_oper.start_speech(); */
        /* dueros_dma.dialog=(WORD32)envelope->providespeech->dialog->id; */
        /* dueros_dma_send_prov_speech_ack(dueros_dma.dialog,envelope->request_id); */
        break;

    case COMMAND__START_SPEECH_ACK:
        log_info("------COMMAND__START_SPEECH_ACK  %s\n", envelope->request_id);

        //do nothing
        break;;

    case COMMAND__STOP_SPEECH:
        log_info("------COMMAND__STOP_SPEECH id=%s\n", envelope->request_id);
        /* dueros_dma_api->dma_oper.stop_speech(); */
        /* dueros_dma_api->dma_oper.stop_clear(); */
        dueros_stop_send();
        dueros_dma_stop_speech_ack(dueros_dma_api->dialog, envelope->request_id);
        break;

    case COMMAND__NOTIFY_SPEECH_STATE:
        log_info("------COMMAND__NOTIFY_SPEECH_STATE id=%s\n", envelope->request_id);
        dueros_dma_api->dma_oper.notify_speech_state(envelope->notifyspeechstate);
        dueros_dma_send_notify_speech_state_ack(envelope->request_id);
        break;

    case COMMAND__GET_DEVICE_INFORMATION:
        log_info("------COMMAND__GET_DEVICE_INFORMATION id=%s\n", envelope->request_id);
        initiatorTye = dueros_dma_send_get_dev_info_ack(envelope->request_id);

        set_dueros_pair_state(PAIR_STEPS_GET_DEV_INFO, 0);
        /*如果唤醒在手机上做，不需要手机端start*/
        if (initiatorTye == INITIATOR_TYPE__PHONE_WAKEUP || initiatorTye == INITIATOR_TYPE__TAP) {
            break;
        }

        /*如果唤醒是在端上做*/
        dueros_dma_start_speech();
        break;

    case COMMAND__GET_DEVICE_CONFIGURATION:
        log_info("------COMMAND__GET_DEVICE_CONFIGURATION %s \n", envelope->request_id);
        dueros_dma_sent_get_dev_conf_ack(envelope->request_id);
        break;

    case COMMAND__NOTIFY_DEVICE_CONFIGURATION_ACK:
        log_info("------COMMAND__NOTIFY_DEVICE_CONFIGURATION_ACK id=%s\n", envelope->request_id);

        //提供给应用一个接口。
        //do nothing
        break;

    case COMMAND__GET_STATE:
        log_info("------COMMAND__GET_STATE id=%s\n", envelope->request_id);


        log_info("feature %d\n", envelope->getstate->feature);

        if (envelope->getstate->feature == 0x02) {
            if (dueros_dma_api->rand != NULL) {
                if (dueros_dma_check_summary(envelope) != 0) {
                    dueros_dma_send_get_state_check_sum_fail(COMMAND__GET_STATE_ACK, envelope->request_id);
                    break;
                }
            } else {
                dueros_dma_send_get_state_check_sum_fail(COMMAND__GET_STATE_ACK, envelope->request_id);
                break;
            }
        }
        switch (envelope->getstate->feature) {
        // supported
        case DMA_ALIVE:
        case DMA_SIGN_VERIFY:
        case AUX_CONNECTED:
        case A2DP_ENABLED:
        case HFP_ENABLED:
        case A2DP_CONNECTED:
        case HFP_CONNECTED:
        case BT_CLASSIC_DISCOVERABLE:
            //dueros_dma_send_get_state_ack(envelope->getstate->feature, envelope->request_id);
            dueros_dma_send_getstate_ack(envelope->getstate->feature, envelope->request_id, ERROR_CODE__SUCCESS);
            break;

        // unsupported
        case DEVICE_CALIBRATION_REQUIRED:
        case DEVICE_DND_ENABLED:
        case DHF_infrared_Detected:
        case DHF_OTA_MODE:

        case DEVICE_THEME:
        case CELLULAR_CONNECTIVITY_STATUS:
        case MESSAGE_NOTIFICATION:
        case CALL_NOTIFICATION:
        case REMOTE_NOTIFICATION:
        case DHF_FM_Frequency:
        case DCS_scene_Mode:
            dueros_dma_send_getstate_ack(envelope->getstate->feature, envelope->request_id, ERROR_CODE__UNSUPPORTED);
            break;
        default:
            log_info("------------unknow feature:%d \n", envelope->getstate->feature);
            dueros_dma_send_getstate_ack(envelope->getstate->feature, envelope->request_id, ERROR_CODE__UNSUPPORTED);
            break;
        }

        /* dueros_dma_send_notify_dev_conf(); */
        /* dueros_dma_send_sync_state(1); */
        break;

    case COMMAND__SET_STATE:
        log_info("------COMMAND__SET_STATE id=%s\n", envelope->request_id);

        if (envelope->setstate->state->feature == 0xf002) {
            if (dueros_dma_check_summary(envelope) != 0) {
                dueros_dma_send_check_sum_fail(COMMAND__SET_STATE, envelope->request_id);
                break;
            }

        }

        dueros_dma_send_set_state_ack(envelope->setstate, envelope->request_id);
        break;

    case COMMAND__SYNCHRONIZE_STATE_ACK:
        //什么时候同步,提供给应用一个接口
        //do nothing
        dueros_dma_send_synchronize_state_ack(envelope->request_id);
        log_info("------COMMAND__SYNCHRONIZE_STATE_ACK id=%s\n", envelope->request_id);
        break;

    case COMMAND__FORWARD_AT_COMMAND:

        log_info("------COMMAND__FORWARD_AT_COMMAND id=%s\n", envelope->request_id);
        dueros_dma_send_at_command_ack(envelope->request_id, envelope->forwardatcommand->command);
        break;

    case COMMAND__END_POINT_SPEECH:
        log_info("------COMMAND__END_POINT_SPEECH id=%s\n", envelope->request_id);
//			dueros_dma_stop_speech();
        dueros_dma_send_endpoint_speech_ack(envelope->request_id);
        break;

    case COMMAND__PAIR:
        log_info("------ COMMAND__PAIR id=%s\n", envelope->request_id);
        if (set_dueros_pair_state(PAIR_STEPS_SUCESS, 0)) {
            dueros_dma_send_pair_ack(envelope->request_id);
            //连接成功后的提示音
            dueros_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, KEY_DUEROS_CONNECTED);
        } else {
            log_info("------not pair_ack----!!!!!!\n");
            dueros_dma_send_no_pair_ack(envelope->request_id);
        }
        break;

    default: {

        //dueros_dma_send_getstate_ack(envelope->getstate->feature, envelope->request_id, ERROR_CODE__UNSUPPORTED);
        log_info("-----default %d\n", envelope->command);
    }
    }


    control_envelope__free_unpacked(envelope, NULL); //ray fix
    //resume_data_send_thread();//ray fix
    return 0;
}
#if 1
int mic_coder_busy_flag(void)
{
    return mic_coder_busy;
}
_WEAK_
int dueros_start_speech(void)
{

    // g_f_printf("dueros start speech\n");

#if !DUEROS_DEBUG_CLOSE
    ///mqc mark
    if (ai_mic_rec_start() == 0) {
        mic_coder_busy = 1;

#if TCFG_USER_TWS_ENABLE
        if (mic_get_data_source()) {
            //log_info("\n\n\n\ntws mic data init\n");

            if (dma_tws_mic_pool == 0) {
                dma_tws_mic_pool = malloc(DMA_TWS_MEM_LEN);

                if (dma_tws_mic_pool == NULL) {
                    ASSERT(0, "dam_tws_mic pool is err\n");
                }

                tws_api_local_media_trans_start();
                tws_api_local_media_trans_set_buf(dma_tws_mic_pool, DMA_TWS_MEM_LEN);
            }

        }
#endif

    }

#endif
    return 0;
}

void check_mic_busy(void)
{
    mic_coder_busy = 0;
    printf("mic to %0x\n", tws_link_read_slot_clk());
    log_info("mic c3\n");
}

void set_mic_busy_timeout(u8 *data, u8 len)
{
    u32 cur_master_clk = 0;
    u32 get_master_clk = 0;
    u32 get_instan = 0;

    if (data && len) {
        get_master_clk = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        get_instan = data[4] | (data[5] << 8);

        cur_master_clk = tws_link_read_slot_clk();
        if (cur_master_clk < (get_master_clk + (get_instan * 1000) / 625)) {
            get_instan = get_instan - ((cur_master_clk - get_master_clk) * 625 / 1000);
            if (get_instan < 1000) {
                sys_timeout_add(NULL, check_mic_busy, get_instan);
                printf("\n\n\n\nget stop instan timeout %0x,%0x,%0x\n", cur_master_clk, get_master_clk, get_instan);
            } else {
                mic_coder_busy = 0;
                log_info("mic timeout err\n");
                printf("get stop instan timeout %0x,%0x,%0x\n", cur_master_clk, get_master_clk, get_instan);
            }
        } else {
            mic_coder_busy = 0;
            log_info("mic c2\n");
            printf("get stop instan timeout %0x,%0x,%0x\n", cur_master_clk, get_master_clk, get_instan);
        }
    }
}

_WEAK_
int dueros_stop_speech(void)
{
#if !DUEROS_DEBUG_CLOSE
    ///mqc mark
    ai_mic_rec_close();

#if TCFG_USER_TWS_ENABLE
    if (mic_coder_busy) {
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {

            if (get_app_connect_type() == TYPE_SPP) {
                mic_coder_busy = 0;
                log_info("mic clear spp\n");
            } else {
                printf("\ntws connect mic busy will delay\n");
            }

        } else {
            mic_coder_busy = 0;
            log_info("mic c1\n");
        }

        if (mic_get_data_source()) {
            log_info("\n\n\ntws mic data end\n");
            if (get_ble_connect_type() == TYPE_MASTER_BLE) {
                log_info("\n\n\n slave start & master stop\n");
                /* app_tws_send_data(TWS_APP_DATA_SEND, TWS_AI_SPEECH_STOP, NULL, 0); */
                /* tws_ai_send_speech_stop(); */
            }
            if (dma_tws_mic_pool) {
                g_printf("tws_api_local_media_trans_stop\n");
                tws_api_local_media_trans_stop();
                free(dma_tws_mic_pool);
                dma_tws_mic_pool = NULL;
            }
        } else {
            if (get_ble_connect_type() == TYPE_MASTER_BLE) {
                log_info("\n\n\nmastar start & send stop\n");
                /* app_tws_send_data(TWS_APP_DATA_SEND, TWS_AI_SPEECH_STOP, NULL, 0); */
                /* tws_ai_send_speech_stop(); */
            }
        }

        mic_coder_busy = 0;
        mic_set_data_source(SOURCE_TYPE);
    }

#else
    mic_coder_busy = 0;
#endif
#endif

    return 0;
}
#endif


static int (*dueros_send_cmd_data)(void *priv, u8 *buf, u16 len) = NULL;
static int (*dueros_should_send_check_cbk)(u16 len) = NULL;
int dueros_should_send_check(uint32_t len);

void server_register_dueros_send_callbak(int (*send_callbak)(void *priv, u8 *buf, u16 len))
{
    dueros_send_cmd_data = send_callbak;
}


int server_get_dueros_send_func(void)
{
    return (int)dueros_send_cmd_data;
}

void server_register_dueros_should_send_callbak(int (*cbk)(u16 len))
{
    dueros_should_send_check_cbk = cbk;
}

int dueros_sent_raw_audio_data(uint8_t *data, uint32_t len)
{
    if (!dueros_send_cmd_data) {
        return 1;
    }

    return dueros_send_cmd_data(0, data, len);
}

/*
 *send data once if available
 * **/
int dueros_send_once(void *priv)
{
//	return 0;
    return dueros_sent_raw_audio_data(0, 0);
}


int dueros_sent_raw_cmd(uint8_t *data,  uint32_t len)
{
    if (!dueros_send_cmd_data) {
        return 1;
    }
    return dueros_send_cmd_data(0, data, len);
}

int dueros_should_send_check(uint32_t len)
{
    if (dueros_should_send_check_cbk) {
        return 	dueros_should_send_check_cbk(len);
    }
    return 1;
}

void dueros_get_setting(SpeechSettings *setting)
{
    setting->audio_profile = AUDIO_PROFILE__NEAR_FIELD;
    setting->audio_format = supported_audio_formats;//AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS;
    setting->audio_source = AUDIO_SOURCE__STREAM;
    return;
}

const u8 *bt_get_mac_addr();
void get_classic_bt_mac(u8 *addr, u8 exchange_en)
{
    u8 addr_buf[6];

    memcpy(addr_buf, bt_get_mac_addr(), 6);

    printf("class bt mac\n");
    put_buf(addr_buf, 6);

    if (exchange_en) {
        addr[0] = addr_buf[5];
        addr[1] = addr_buf[4];
        addr[2] = addr_buf[3];
        addr[3] = addr_buf[2];
        addr[4] = addr_buf[1];
        addr[5] = addr_buf[0];
    } else {

        addr[0] = addr_buf[0];
        addr[1] = addr_buf[1];
        addr[2] = addr_buf[2];
        addr[3] = addr_buf[3];
        addr[4] = addr_buf[4];
        addr[5] = addr_buf[5];
    }
}
void  bt_address_convert_to_char(uint8_t *bt_addr)
{
    int i, j = 0;
    for (i = 5 ; i >= 0; i--) {
        bt_address[ j++] = hex_to_char((bt_addr[i] >> 4) & 0xf);
        bt_address[ j++] = hex_to_char(bt_addr[i] & 0xf);
        bt_address[ j++] = ':';
    }

    bt_address[17] = '\0';
}

void serial_number_init()
{
    memset(dma_serial_number, 0, sizeof(dma_serial_number));
    u8 addr_buf[6];

    memcpy(addr_buf, bt_get_mac_addr(), 6);
    for (u8 i = 0; i < 4; i++) {
        sprintf(&dma_serial_number[2 * i], "%02x", addr_buf[i]);
    }

    printf("serial_number = %s\n", dma_serial_number);
}

void dueros_get_dev_info(DeviceInformation *info)
{
    info->serial_number =  dma_serial_number;
    info->name  = "JL_Dueros";
    info->n_supported_transports = 2;
    info->supported_transports = dma_supported_transports;

    info->device_type = "SPEAKER";
    info->n_supported_audio_formats = 1;
    info->supported_audio_formats = &supported_audio_formats;
    info->manufacturer = "JL";

    info->model = "dureOs";
    info->firmware_version = "B.1.0";
    info->software_version = "J.1.0";

    info->initiator_type = INITIATOR_TYPE__TAP;


    get_classic_bt_mac(dma_classic_bluetooth_mac, 1);
    bt_address_convert_to_char(dma_classic_bluetooth_mac);

    /* printf_buf((u8 *)bt_address, 6); */

    info->classic_bluetooth_mac = bt_address;

    info->disable_heart_beat = true;
    info->enable_advanced_security = false;
}

void dueros_get_dev_config(DeviceConfiguration *config)
{
    //do nothing
    return;
}

void dueros_set_state(State *state)
{
    switch (state->feature) {
    case DMA_SIGN_VERIFY :
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = true;
        break;

    case AUX_CONNECTED:
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = false;
        break;

    case A2DP_ENABLED:
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = true;
        break;

    case HFP_ENABLED:
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = true;
        break;

    case A2DP_CONNECTED:
        state->value_case = STATE__VALUE_BOOLEAN;
        if (get_curr_channel_state() & A2DP_CH) {
            state->boolean = true;
        } else {
            state->boolean = false;
        }
        break;

    case HFP_CONNECTED:
        state->value_case = STATE__VALUE_BOOLEAN;
        if (get_curr_channel_state() & HFP_CH) {
            state->boolean = true;
        } else {
            state->boolean = false;
        }
        break;

    case BT_CLASSIC_DISCOVERABLE:
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = true;
        break;

    default:
        log_info("\nstatus err\n");
        log_info("state %d\n", state->feature);
        break;
    }
}

void dueros_get_state(uint32_t feature, State *state)
{
    switch (feature) {
    case DMA_ALIVE:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = true;
        break;


    case DMA_SIGN_VERIFY :
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = true;
        //播放提示音
        dueros_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, KEY_DUEROS_CONNECTED);
        break;

    case AUX_CONNECTED:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = false;
#if 0
        extern u8   get_aux_sta(void);
        if (get_aux_sta()) {
            state->boolean = true;
        } else {
            state->boolean = false;
        }
#endif
        break;

    case A2DP_ENABLED:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = true;
        break;

    case HFP_ENABLED:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = true;
        break;

    case A2DP_CONNECTED:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        if (get_curr_channel_state() & A2DP_CH) {
            state->boolean = true;
        } else {
            state->boolean = false;
        }
        break;

    case HFP_CONNECTED:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        if (get_curr_channel_state() & HFP_CH) {
            state->boolean = true;
        } else {
            state->boolean = false;
        }
        break;

    case BT_CLASSIC_DISCOVERABLE:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = true;
        break;

    case DEVICE_CALIBRATION_REQUIRED:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = false;
        break;
    case DEVICE_DND_ENABLED:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = false;
        break;
    case DHF_infrared_Detected:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = false;
        break;
    case DHF_OTA_MODE:
        state->feature = feature;
        state->value_case = STATE__VALUE_BOOLEAN;
        state->boolean = false;
        break;

    //FeatureIntegerType处理
    case DEVICE_THEME:
        state->feature = feature;
        state->value_case = STATE__VALUE_INTEGER;
        state->integer = 2;
        break;
    case CELLULAR_CONNECTIVITY_STATUS:
        state->feature = feature;
        state->value_case = STATE__VALUE_INTEGER;
        state->integer = 2;
        break;
    case MESSAGE_NOTIFICATION:
        state->feature = feature;
        state->value_case = STATE__VALUE_INTEGER;
        state->integer = 0;
        break;
    case CALL_NOTIFICATION:
        state->feature = feature;
        state->value_case = STATE__VALUE_INTEGER;
        state->integer = 0;
        break;
    case REMOTE_NOTIFICATION:
        state->feature = feature;
        state->value_case = STATE__VALUE_INTEGER;
        state->integer = 0;
        break;
    case DHF_FM_Frequency:
        state->feature = feature;
        state->value_case = STATE__VALUE_INTEGER;
        state->integer = 0;
        break;
    case DCS_scene_Mode:
        state->feature = feature;
        state->value_case = STATE__VALUE_INTEGER;
        state->integer = 0;
        break;
    default:
        log_info("status err\n");
        log_info("%d\n", feature);
        break;
    }
}

void dueros_notify_speech_state(NotifySpeechState *speechstate)
{
    switch (speechstate->state) {
    case SPEECH_STATE__LISTENING:
        log_info("dueros listening!\n");
        break;
    case SPEECH_STATE__PROCESSING:
        log_info("dueros processing!\n");
        break;
    case SPEECH_STATE__SPEAKING:
        log_info("dueros speaking!\n");
        break;
    default:
        log_info("dueros idle!\n");
        /* dueros_stop_speech(); */
        break;
    }
}

void dueros_event_post(u32 type, u8 event)
{
    struct sys_event e;
    e.type = SYS_BT_EVENT;
    e.arg  = (void *)type;
    e.u.bt.event = event;
    sys_event_notify(&e);
}

extern int task_post_msg(char *name, int argc, ...);
int dueros_at_command(const char *cmd)
{
    u8 len;
    char num_buf[20];

    ///mqc mark
    if (!memcmp(cmd, "ATA", 3)) {
        app_task_put_key_msg(KEY_CALL_ANSWER, 0);
        //dueros_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, KEY_CALL_ANSWER);
    } else if (!memcmp(cmd, "ATD", 3)) {
        len = strlen(cmd + 3);
        if (len < 20) {
            memcpy(num_buf, cmd + 3, len);
            /* put_buf((u8 *)num_buf,len); */
            user_send_cmd_prepare(USER_CTRL_DIAL_NUMBER, len, (u8 *)num_buf);
        }
    } else if (!memcmp(cmd, "CHUP", 4)) {
        app_task_put_key_msg(KEY_CALL_HANG_UP, 0);
        //dueros_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, KEY_CALL_HANG_UP);
    } else if (!memcmp(cmd, "BLDN", 4)) {
        app_task_put_key_msg(KEY_CALL_LAST_NO, 0);
        //dueros_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, KEY_CALL_LAST_NO);
    } else if (!memcmp(cmd, "CHLD=0", 6)) {
        user_send_cmd_prepare(USER_CTRL_HFP_THREE_WAY_REJECT, 0, NULL);
    } else if (!memcmp(cmd, "CHLD=1", 6)) {
        user_send_cmd_prepare(USER_CTRL_HFP_THREE_WAY_ANSWER1, 0, NULL);
    } else if (!memcmp(cmd, "CHLD=2", 6)) {
        user_send_cmd_prepare(USER_CTRL_HFP_THREE_WAY_ANSWER2, 0, NULL);
    } else if (!memcmp(cmd, "CHLD=3", 6)) {
        user_send_cmd_prepare(USER_CTRL_HFP_THREE_WAY_ANSWER3, 0, NULL);
    }
    return 0;
}

int dueros_stop_clear(void)
{
    return 0;
}

char *dueros_get_key(void)
{
    return NULL;
}


DUEROS_DMA_OPER  dueros_dma_op = {
    .sent_raw_audio_data   =  dueros_sent_raw_audio_data,
    .sent_raw_cmd          =  dueros_sent_raw_cmd,
    .should_send		   =  dueros_should_send_check,
    .start_speech          =  dueros_start_speech,
    .stop_speech           =  dueros_stop_speech,
    .get_setting           =  dueros_get_setting,
    .get_dev_info          =  dueros_get_dev_info,
    .get_dev_config        =  dueros_get_dev_config,
    .set_state             =  dueros_set_state,
    .get_state             =  dueros_get_state,
    .notify_speech_state   =  dueros_notify_speech_state,
    .at_command            =  dueros_at_command,
    .stop_clear            =  dueros_stop_clear,
    .get_key               =  dueros_get_key,
    .set_pair_info         =  dueros_set_pair_info,
};


u32 app_send_cmd_prepare(APP_CMD_TYPE cmd, u16 param_len, u8 *param)
{
    u32 ret = 0;

    switch (cmd) {
    case APP_TWS_DUEROS_RAND_SET:
        /* tws_dueros_rand_set_vm(&app_user_cmd.app_rand, 8); */
        memcpy(app_user_cmd.app_rand, param, param_len);
        break;

    case APP_SPEECH_STOP_FROM_TWS:
        memcpy(app_user_cmd.speech_data, param, param_len);
        break;
    }

    app_user_cmd.app_cmd_buf[app_user_cmd.pos_write++] = cmd;
    if (app_user_cmd.pos_write >= APP_CMD_LEN) {
        app_user_cmd.pos_write = 0;
    }

    stack_run_loop_resume();

    return 0;

}

void dueros_send_resume(void)
{
    /* os_taskq_post_type("dueros", APP_DUEROS_SEND, 0, NULL); */
    app_send_cmd_prepare(APP_DUEROS_SEND, 0, NULL);
}

void dueros_rand_set(u8 *rand, u8 len)
{
    if (len < 10) {
        /* r_f_printf("app dueros set\n"); */
        memcpy(tws_dma_data, rand, len);
        /* os_taskq_post_type("dueros", APP_TWS_DUEROS_RAND_SET, len, tws_dma_data); */
        app_send_cmd_prepare(APP_TWS_DUEROS_RAND_SET, len, tws_dma_data);
    }
}

void dueros_speech_start_from_tws(void)
{
    /* os_taskq_post_type("dueros", APP_SPEECH_START_FROM_TWS, 0, NULL); */
    app_send_cmd_prepare(APP_SPEECH_START_FROM_TWS, 0, NULL);
}

void dueros_speech_stop_from_tws(u8 *data, u8 len)
{
    if (len < 8) {
        memcpy(tws_dma_data, data, len);
        /* os_taskq_post_type("dueros", APP_SPEECH_STOP_FROM_TWS, len, tws_dma_data); */
        app_send_cmd_prepare(APP_SPEECH_STOP_FROM_TWS, len, tws_dma_data);
    }
}

static void dueros_process_handle(struct run_loop *loop)
{
    APP_CMD_TYPE cmd;

    if (app_user_cmd.pos_read == app_user_cmd.pos_write) {
        return ;
    }

    cmd = app_user_cmd.app_cmd_buf[app_user_cmd.pos_read];

    /* r_printf("app handle cmd %d\n",cmd); */

    switch (cmd) {
    case APP_DUEROS_VER:
        dueros_dma_send_ver();
        set_dueros_pair_state(PAIR_STEPS_INIT, 1);
        break;

    case APP_DUEROS_SEND:
        data_send_process_thread();
        break;

    case APP_TWS_DUEROS_RAND_SET:
        tws_dueros_rand_set_vm(&app_user_cmd.app_rand, 8);
        break;

    case APP_SPEECH_START_FROM_TWS:
        //dma_msg_deal((u32)APP_TWS_BLE_SLAVE_SPEECH_START);
        break;

    case APP_SPEECH_STOP_FROM_TWS:
        dueros_stop_speech();
        dueros_stop_send();
        put_buf(&app_user_cmd.speech_data, 6);
        set_mic_busy_timeout(&app_user_cmd.speech_data, 6);
        break;

    default:
        break;
    }

    if (++app_user_cmd.pos_read >= APP_CMD_LEN) {
        app_user_cmd.pos_read = 0;
    }
}

void dueros_send_ver(void)
{
    /* os_taskq_post_type("dueros", APP_DUEROS_VER, 0, NULL); */
    app_send_cmd_prepare(APP_DUEROS_VER, 0, NULL);
}

void smart_dueros_dma_init(void (*start_speech)(void), void (*stop_speech)(void))
{
    mic_coder_busy = 0;

    dueros_speech_start = start_speech;
    dueros_speech_stop = stop_speech;

    tws_ble_dueros_handle_create();

    serial_number_init();
    dma_spp_init();

    jl_dueros_dma_init(&dueros_dma_op, &protocol_v, \
                       dma_frame_mem_pool, sizeof(dma_frame_mem_pool), \
                       dma_sys_mem_pool, sizeof(dma_sys_mem_pool));
    /// mqc mark
//    att_regist_thread_deal_cbk(data_send_process_thread);
    server_register_dueros_resume_callbak(dueros_send_resume);
    //    dueros_dma_register_file_read_callback((void *)cache, fs_open_file_bypath);
    dueros_dma_register_file_read_callback();
    dueros_dma_api = jl_dueros_dma_get_api();



#if	!DUEROS_DEBUG_CLOSE
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        if (get_app_connect_type() == TYPE_BLE) {
            set_ble_connect_type(TYPE_MASTER_BLE);
            ai_tws_send_data_to_sibling(TWS_AI_CONN_STA, TYPE_MASTER_BLE);
        }
    }
#endif
#endif

}

void smart_dueros_dma_close(void)
{
    mic_coder_busy = 0;
    dueros_stop_speech();
    jl_dueros_dma_close();
    app_remove_stack_run();
}

void tws_ble_dueros_handle_create()
{
    memset(&app_user_cmd, 0, sizeof(struct app_cmd_ctrl));
    app_user_cmd.pos_read = 0;
    app_user_cmd.pos_write = 0;
    run_loop_register_for_app(dueros_process_handle);
}

void tws_ble_slave_dueros_task_del()
{
    app_remove_stack_run();
}


bool dueros_lock_connection_chl(u8 chl)
{
    if (chl_lock_flag) {
        return false;
    }
    chl_lock_flag |= BIT(chl);
    return true;
}

bool dueros_unlock_connection_chl(u8 chl)
{
    if (chl_lock_flag & BIT(chl)) {
        chl_lock_flag &= ~BIT(chl);
        return true;
    }
    return false;
}


#endif

