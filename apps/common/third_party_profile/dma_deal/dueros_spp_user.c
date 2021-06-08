
#include "app_config.h"
#include "app_action.h"
#include "btstack/avctp_user.h"

#include "system/includes.h"
#include "spp_user.h"
#include "string.h"
#include "circular_buf.h"
#include "dma_deal.h"
#include "key_event_deal.h"
#include "classic/tws_api.h"

#include "dueros_dma.h"
#include "bt_common.h"
#include "3th_profile_api.h"

//#include "dueros_spp_user.h"
//#include "dueros_dma.h"
//#include "dma_deal.h"

#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info          printf
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

extern void bt_ble_adv_enable(u8 enable);
extern int tws_api_get_tws_state();

/* #define DEBUG_ENABLE */
/* #include "debug_log.h" */

#if (DUEROS_DMA_EN)

static struct spp_operation_t *spp_api = NULL;
static u8 spp_state;

int dueros_spp_send_data(void *priv, u8 *data, u16 len)
{
    if (spp_api) {
        return spp_api->send_data(NULL, data, len);
    }
    return SPP_USER_ERR_SEND_FAIL;
}

int dueros_spp_send_data_check(u16 len)
{
    if (spp_api) {
        if (spp_api->busy_state()) {
            return 0;
        }
    }
    return 1;
}

bool dma_msg_deal(u32 param);
static void dueros_spp_state_cbk(u8 state)
{
    printf("dueros_spp_state_cbk");
    spp_state = state;
    switch (state) {
    case SPP_USER_ST_CONNECT:

        r_printf("dma_spp conn\n");

#if TCFG_USER_BLE_ENABLE
        bt_ble_adv_enable(0);
#endif
        if (dueros_lock_connection_chl(1) == true) {
            server_register_dueros_should_send_callbak(dueros_spp_send_data_check);
            set_app_connect_type(TYPE_SPP);
            server_register_dueros_send_callbak(dueros_spp_send_data);
            smart_dueros_dma_init(NULL, NULL);
            dueros_send_ver();
            /* task_post_msg(NULL, 1, MSG_DUEROS_VER); */
        }
        break;

    case SPP_USER_ST_DISCONN:
        smart_dueros_dma_close();
        server_register_dueros_send_callbak(NULL);
        server_register_dueros_should_send_callbak(NULL);
        set_app_connect_type(TYPE_NULL);
        if (dueros_unlock_connection_chl(1) == true) {
            dueros_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, KEY_DUEROS_DISCONNECTED);
        }

#if TCFG_USER_BLE_ENABLE
#if TCFG_USER_TWS_ENABLE
        if (!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
            bt_ble_adv_enable(1);
        } else {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                bt_ble_adv_enable(1);
            }
        }
#else
        bt_ble_adv_enable(1);
#endif
#endif
        break;

    default:
        break;
    }

}

static void dueros_spp_send_wakeup(void)
{
    putchar('W');
#if (DUEROS_DMA_EN)
    void dueros_send_resume(void);
    dueros_send_resume();
#endif
}

static void dueros_spp_recieve_cbk(void *priv, u8 *buf, u16 len)
{
    /* log_info("spp_api_rx ~~~\n"); */
    /* log_info_hexdump(buf, len); */
    dueros_process_cmd_from_phone(buf, len);
}
//void dma_app_bredr_handle_reg(void)
//{
//    spp_data_deal_handle_register(user_spp_data_handler);
//}
//int dma_app_bt_state_init(void)
//{
//    dma_spp_init();
//    return 0;
//}
void dma_spp_init(void)
{
    spp_state = 0;
    spp_get_operation_table(&spp_api);
    spp_api->regist_recieve_cbk(0, dueros_spp_recieve_cbk);
    spp_api->regist_state_cbk(0, dueros_spp_state_cbk);
    spp_api->regist_wakeup_send(NULL, dueros_spp_send_wakeup);
}

#endif


