#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "typedef.h"
#include "3th_profile_api.h"
#include "bt_tws.h"
#include "key_event_deal.h"
#include "system/timer.h"
#include "btstack/frame_queque.h"
#include "bt_common.h"


#include "btstack/third_party/baidu/dueros_dma.h"
#include "btstack/avctp_user.h"
#include "os/os_api.h"

#if (BT_FOR_APP_EN)

#if (DUEROS_DMA_EN)
#include "dma_deal.h"

extern void dueros_rand_set(u8 *rand, u8 len);
extern u8 tws_dueros_rand_get_vm(u8 *data);
#endif

#if (DUEROS_DMA_EN)
#define AI_PL_DEAL  dma_msg_deal
#elif (GMA_EN)
#include "gma.h"
#define AI_PL_DEAL  tm_gma_msg_deal
#else
#define AI_PL_DEAL 	NULL
#endif

#if (OTA_TWS_SAME_TIME_ENABLE && RCSP_ADV_EN && USER_APP_EN)
#include "rcsp_adv_tws_ota.h"
#endif

#if (OTA_TWS_SAME_TIME_ENABLE && SMART_BOX_EN)
#include "smartbox_update_tws.h"
#else
#include "update_tws.h"
#endif

extern void check_mic_busy();
extern void bt_api_all_sniff_exit(void);
extern void dueros_speech_start_from_tws(void);
extern void dueros_speech_stop_from_tws(u8 *data, u8 len);
extern u32 tws_link_read_slot_clk();
extern int mic_coder_busy_flag(void);

static u8 mic_data_type = SOURCE_TYPE;

static u8 connect_type = TYPE_NULL;
static u8 tws_ble_type = TYPE_NULL;

void mic_set_data_source(u8 data_type)
{
    mic_data_type = data_type;
}

__attribute__((weak))
u8 mic_get_data_source(void)
{
    return mic_data_type;
}

u8 get_ble_connect_type(void)
{
    return tws_ble_type;
}

void set_ble_connect_type(u8 type)
{
    tws_ble_type = type;
}

u8 get_app_connect_type(void)
{
    return connect_type;
}

void set_app_connect_type(u8 type)
{
    connect_type = type;
}


void app_tws_send_data(u8 cmd, u8 opcode, u8 *data, u8 len)
{
    struct tws_sync_info_t send_data;
    u32 master_clk = 0;
    u16 instan = 300;

    switch (opcode) {
#if (DUEROS_DMA_EN)
    case TWS_AI_DMA_RAND:
        send_data.type = cmd;
        send_data.u.data[0] = opcode;
        memcpy(&send_data.u.data[1], data, len);
        break;
#endif

    case TWS_AI_SPEECH_STOP:
        send_data.type = cmd;
        send_data.u.data[0] = opcode;
        send_data.u.data[1] = get_app_connect_type();
        master_clk = tws_link_read_slot_clk();
        send_data.u.data[2] = master_clk & 0xff;
        send_data.u.data[3] = (master_clk >> 8) & 0xff;
        send_data.u.data[4] = (master_clk >> 16) & 0xff;
        send_data.u.data[5] = (master_clk >> 24) & 0xff;
        send_data.u.data[6] = instan & 0xff;
        send_data.u.data[7] = (instan >> 8) & 0xff;
        sys_timeout_add(NULL, check_mic_busy, instan);
        printf("\n\n\n\nmasterclk %0x\n", master_clk);
        put_buf(&send_data.u.data[2], 6);
        break;
    }

    tws_api_send_data_to_sibling((u8 *)&send_data, sizeof(struct tws_sync_info_t), TWS_FUNC_ID_AI_SYNC);

}
void app_tws_get_data_from_sibling(u8 cmd, u8 *data, u8 len)
{
    switch (cmd) {
#if (DUEROS_DMA_EN)
    case TWS_APP_DATA_SEND:
        switch (data[0]) {
        case TWS_AI_DMA_RAND:
            /* printf("\n\n\n\ntws get rand\n\n\n"); */
            /* put_buf(&data[1],len-1); */
            dueros_rand_set(&data[1], len - 1);
            break;

        case TWS_AI_SPEECH_STOP:
#if (DUEROS_DMA_EN)
            if (data[1] == TYPE_BLE) {
                if (tws_api_get_role() == TWS_ROLE_SLAVE) {
                    printf("\n\n\nGET TWS_AI_SPEECH_STOP\n");
                    dueros_speech_stop_from_tws(&data[2], 6);
                }
            }
#endif
            break;
        }
        break;
#endif
    }
}

void tws_dueros_rand_send(u8 *data, u8 len)
{
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        /* printf("\n\n\n\ntws set rand\n\n\n"); */
        /* put_buf(data,len); */
        app_tws_send_data(TWS_APP_DATA_SEND, TWS_AI_DMA_RAND, data, len);
    }
}

void send_conn_sta(u8 cmd, u8 type)
{
    struct tws_sync_info_t conn_sta;

    conn_sta.type = cmd;
    conn_sta.u.conn_type = type;

    tws_api_send_data_to_sibling((u8 *)&conn_sta, sizeof(struct tws_sync_info_t), TWS_FUNC_ID_AI_SYNC);
}

void ai_tws_send_data_to_sibling(u8 cmd, u8 type)
{
    switch (cmd) {
    case TWS_AI_SPEECH_START:
    case TWS_AI_CONN_STA:
    case TWS_AI_DISCONN_STA:
        send_conn_sta(cmd, type);
        break;
    }
}

void tws_sync_dueros_info(void)
{
#if (DUEROS_DMA_EN)
    u8 rand_data[8];

    if (get_app_connect_type()) {
        if (tws_dueros_rand_get_vm(rand_data)) {
            printf("\n\n\n\ntws send rand\n\n\n");
            tws_dueros_rand_send(rand_data, 8);
        }

        if (get_app_connect_type() == TYPE_BLE) {
            printf("\n\n\n\ntws send conn\n\n\n");
            set_ble_connect_type(TYPE_MASTER_BLE);
            ai_tws_send_data_to_sibling(TWS_AI_CONN_STA, TYPE_MASTER_BLE);
        }
    }

#endif
}

void tws_data_to_sibling_send(u8 opcode, u8 *data, u8 len)
{
    struct tws_sync_info_t send_data;

    if (len > (sizeof(send_data.u.data_large) - 2)) {
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>send data length errror !!! \n");
        return;
    }

    printf(">>>>>>>>>>send data to sibling \n");
    send_data.type = TWS_DATA_SEND;
    send_data.u.data_large[0] = len + 1;
    send_data.u.data_large[1] = opcode;
    memcpy(&(send_data.u.data_large[2]), data, len);

    tws_api_send_data_to_sibling((u8 *)&send_data, sizeof(struct tws_sync_info_t), TWS_FUNC_ID_AI_SYNC);
}

void ai_event_post(u32 type, u8 event)
{
    struct sys_event e;
    e.type = SYS_BT_EVENT;
    e.arg  = (void *)type;
    e.u.bt.event = event;
    sys_event_notify(&e);
}

void a2dp_drop_frame_start();
void a2dp_drop_frame_stop();
int a2dp_decoder_pause(void);
int a2dp_decoder_start(void);
void tws_slave_a2dp_drop_frame_ctl(u8 en)
{
    tws_data_to_sibling_send(TWS_AI_A2DP_DROP_FRAME_CTL, &en, 1);
}

void tws_slave_a2dp_drop_frame_process(u8 en)
{
    if (en) {
        printf("a2dp drop frame begin \n");
        ai_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, KEY_AI_DEC_SUSPEND);
    } else {
        printf("a2dp drop frame end \n");
        ai_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, KEY_AI_DEC_RESUME);
    }
}

const u8 *bt_get_mac_addr();
void ai_tws_large_data_get_from_sibling(u8 *data, u8 len)
{
    u8 opcode =  data[0];
    printf(">>>>>>>>>>>>>>>>tws get data from sibling: ");
    put_buf(data, len);

#if (OTA_TWS_SAME_TIME_ENABLE && (RCSP_BTMATE_EN || RCSP_ADV_EN || GMA_EN || SMART_BOX_EN || TME_EN))
    tws_ota_get_data_from_sibling(opcode, data + 1, len - 1);
#endif

    switch (opcode) {
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    case TWS_AI_GMA_START_SYNC_LIC:
        gma_sync_remote_addr(&data[1]);

        break;
#endif
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    case TWS_AI_GMA_SYNC_LIC:
        printf(">>> TWS_AI_GMA_START_SYNC_LIC \n");
        gma_slave_sync_remote_addr(&data[1]);
        gma_send_secret_sync();
        break;
#endif
    case TWS_AI_A2DP_DROP_FRAME_CTL:
        tws_slave_a2dp_drop_frame_process(data[1]);
        break;
    default:
        break;
    }
}

void ai_tws_get_data_from_sibling(u8 cmd, u8 type)
{
    switch (cmd) {

    case TWS_AI_SPEECH_START:
        printf("\n\n\nTWS_AI_SPEECH_START\n");
#if (DUEROS_DMA_EN)
        if (type == TYPE_BLE) {
            if (get_ble_connect_type() == TYPE_SLAVE_BLE) {
                mic_set_data_source(SINK_TYPE_SLAVE);
                dueros_speech_start_from_tws();
            }
        }
#endif
        break;

    case TWS_AI_CONN_STA:
        printf("\n\n\nTWS_AI_CONN_STA\n");

#if (DUEROS_DMA_EN)
        if (type == TYPE_MASTER_BLE) {
            dueros_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, APP_TWS_BLE_DUEROS_CONNECT);
        }
#endif
        break;

    case TWS_AI_DISCONN_STA:
        printf("\n\n\nTWS_AI_DISCONN_STA\n");

#if (DUEROS_DMA_EN)
        if (type == TYPE_MASTER_BLE) {
            dueros_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, APP_TWS_BLE_DUEROS_DISCONNECT);
        }
#endif
        break;

    }
}

static void __ai_tws_sync_call(struct tws_sync_info_t *hdl)
{
    switch (hdl->type) {
    case TWS_APP_DATA_SEND:
        app_tws_get_data_from_sibling(hdl->type, hdl->u.data, sizeof(hdl->u.data));
        break;
    case TWS_DATA_SEND:
        ai_tws_large_data_get_from_sibling(&(hdl->u.data_large[1]), hdl->u.data_large[0]);
        break;
    default:
        ai_tws_get_data_from_sibling(hdl->type, hdl->u.conn_type);
        break;
    }

    free(hdl);
}

static void ai_tws_sync(void *_data, u16 len, bool rx)
{
    if (rx) {
        printf(">>>ai_tws_sync \n");
        /*put_buf(_data, len);*/
        struct tws_sync_info_t *hdl = malloc(sizeof(struct tws_sync_info_t));
        memcpy(hdl, _data, sizeof(struct tws_sync_info_t));

        int msg[4];
        msg[0] = (int)__ai_tws_sync_call;
        msg[1] = 1;
        msg[2] = (int)hdl;
        os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
    }
}


REGISTER_TWS_FUNC_STUB(app_vol_sync_stub) = {
    .func_id = TWS_FUNC_ID_AI_SYNC,
    .func    = ai_tws_sync,
};

static bool ai_pl_msg_deal(u32 param)
{
    if (AI_PL_DEAL) {
        return ((bool (*)(u32))AI_PL_DEAL)(param);
    }

    return false;
}

void tws_mic_speech_msg_deal(u8 event, u32 event_type)
{
    switch (event) {
    case KEY_SEND_SPEECH_START:

        printf(">>>>>>>>>>>>>>>>>>>>func:%s line:%d \n", __func__, __LINE__);
        bt_api_all_sniff_exit();

        if (mic_coder_busy_flag()) {
            printf("mic is busy!\n");
            break;
        }
        printf(">>>>>>>>>>>>>>>>>>>>func:%s line:%d \n", __func__, __LINE__);
        /*TWS 已连接*/
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            if (get_app_connect_type() == TYPE_SPP) {
                if (event_type == KEY_EVENT_FROM_TWS) {
                    printf("\n\n\n\n !!!!!!!!!!!!key_event master\n");
                    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
                        mic_set_data_source(SOURCE_TYPE);
                    } else {
                        mic_set_data_source(SINK_TYPE_MASTER);
                    }

                    ai_pl_msg_deal((u32)event);
                    break;
                } else {
                    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
                        {
                            printf("\n\n\n\n !!!!!!!!!key_event slave\n");
                            /* ai_tws_send_data_to_sibling(TWS_AI_SPEECH_START, TYPE_SPP); */
                            mic_set_data_source(SINK_TYPE_SLAVE);
                            ai_pl_msg_deal((u32)event);
                            break;
                        }
                    }
                }
            } else if (get_ble_connect_type()) {
                if (event_type == KEY_EVENT_FROM_TWS) {
                    printf("\n\n\n\n !!!!!!!!!!!!key_event master\n");
                    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
                        mic_set_data_source(SOURCE_TYPE);
                        ai_pl_msg_deal((u32)APP_TWS_BLE_SLAVE_SPEECH_START);
                        break;
                    } else {

                        ai_tws_send_data_to_sibling(TWS_AI_SPEECH_START, TYPE_BLE);

                        mic_set_data_source(SINK_TYPE_MASTER);
                        ai_pl_msg_deal((u32)event);
                    }
                    break;
                } else {
                    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
                        {
                            printf("\n\n\n\n !!!!!!!!!key_event slave\n");
                            /* ai_tws_send_data_to_sibling(TWS_AI_SPEECH_START, TYPE_SPP); */
                            break;
                        }
                    }
                }
            }

            printf("\n\n\n\n speech start\n");
            mic_set_data_source(SOURCE_TYPE);
            ai_pl_msg_deal((u32)event);
            break;
        } else {
            printf("\n\n\n\n speech without tws\n");
            mic_set_data_source(SOURCE_TYPE);
            ai_pl_msg_deal((u32)event);
            break;
        }
        break;
    }
}
#endif
