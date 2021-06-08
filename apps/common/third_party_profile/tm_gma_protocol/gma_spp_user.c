
#include "app_config.h"
#include "app_action.h"

#include "system/includes.h"
#include "spp_user.h"
#include "string.h"
#include "circular_buf.h"
#include "bt_common.h"
#include "gma.h"
#include "bt_tws.h"
#include "3th_profile_api.h"
#include "app_config.h"

#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info          printf
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define TEST_SPP_DATA_RATE      0

/* #define DEBUG_ENABLE */
/* #include "debug_log.h" */
#include "gma_spp_user.h"

#if GMA_EN

#if TEST_SPP_DATA_RATE
#define SPP_TIMER_MS            (100)
static u32 test_data_count;
static u32 spp_timer_handle = 0;
static u32 spp_test_start;
#endif


static struct spp_operation_t *spp_api = NULL;
static u8 spp_state;

int gma_spp_send_data(u8 *data, u16 len)
{
    if (spp_api) {
        //log_info("spp_api_tx(%d) \n", len);
        /* log_info_hexdump(data, len); */
        return spp_api->send_data(NULL, data, len);
    }
    return SPP_USER_ERR_SEND_FAIL;
}

int gma_spp_send_data_check(u16 len)
{
    if (spp_api) {
        if (spp_api->busy_state()) {
            return 0;
        }
    }
    return 1;
}

static void gma_spp_state_cbk(u8 state)
{
    spp_state = state;
    switch (state) {
    case SPP_USER_ST_CONNECT:
        log_info("SPP_USER_ST_CONNECT ~~~\n");

        set_app_connect_type(TYPE_SPP);
        gma_init(gma_spp_send_data_check, gma_spp_send_data, gma_spp_send_data);
#if TCFG_USER_BLE_ENABLE
        bt_ble_adv_enable(0);
#endif
        break;

    case SPP_USER_ST_DISCONN:
        log_info("SPP_USER_ST_DISCONN ~~~\n");

        set_app_connect_type(TYPE_NULL);
        gma_exit();

#if TCFG_USER_BLE_ENABLE
        bt_ble_adv_enable(1);
#endif
        break;

    default:
        break;
    }

}

static void gma_spp_send_wakeup(void)
{
    putchar('W');
    tm_send_process_resume();
}

static void gma_spp_recieve_cbk(void *priv, u8 *buf, u16 len)
{
    log_info("spp_api_rx(%d) \n", len);
    log_info_hexdump(buf, (len > 16) ? 16 : len);
#if(TCFG_USER_TWS_ENABLE)
    if (get_tws_sibling_connect_state()) { //对耳已连接，断开对耳
        if (tws_api_get_role() == TWS_ROLE_MASTER) {///slave do not need to analysis cmd
            gma_recv_proc(buf, len);
        }
    } else {
        gma_recv_proc(buf, len);
    }
#else  //(TCFG_USER_TWS_ENABLE)

    gma_recv_proc(buf, len);
#endif //(TCFG_USER_TWS_ENABLE)

#if TEST_SPP_DATA_RATE
    if ((buf[0] == 'A') && (buf[1] == 'F')) {
        spp_test_start = 1;
    }
#endif
}


#if TEST_SPP_DATA_RATE

static void test_spp_send_data(void)
{
    u16 send_len = 250;
    if (gma_spp_send_data_check(send_len)) {
        test_data_count += send_len;
        gma_spp_send_data(&test_data_count, send_len);
    }
}

static void test_timer_handler(void)
{
    static u32 t_count = 0;

    if (SPP_USER_ST_CONNECT != spp_state) {
        test_data_count = 0;
        spp_test_start = 0;
        return;
    }

    if (spp_test_start) {
        test_spp_send_data();
    }

    if (++t_count < (1000 / SPP_TIMER_MS)) {
        return;
    }
    t_count = 0;

    if (test_data_count) {
        log_info("\n-spp_data_rate: %d bps-\n", test_data_count * 8);
        test_data_count = 0;
    }
}
#endif

void gma_spp_disconnect(void)
{
    if (spp_api) {
        printf(">>>>>>>>>gma spp disconnect \n");
        spp_api->disconnect(NULL);
    }
}

void gma_spp_init(void)
{
    log_info("\n-%s-\n", __FUNCTION__);
    spp_state = 0;
    spp_get_operation_table(&spp_api);
    spp_api->regist_recieve_cbk(0, gma_spp_recieve_cbk);
    spp_api->regist_state_cbk(0, gma_spp_state_cbk);
    spp_api->regist_wakeup_send(NULL, gma_spp_send_wakeup);

#if TEST_SPP_DATA_RATE
    spp_timer_handle  = sys_timer_add(NULL, test_timer_handler, SPP_TIMER_MS);
#endif

}
#endif

