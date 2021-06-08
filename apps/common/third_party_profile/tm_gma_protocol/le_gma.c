/*********************************************************************************************
    *   Filename        : le_server_module.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2017-01-17 11:14

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/

// *****************************************************************************
/* EXAMPLE_START(le_counter): LE Peripheral - Heartbeat Counter over GATT
 *
 * @text All newer operating systems provide GATT Client functionality.
 * The LE Counter examples demonstrates how to specify a minimal GATT Database
 * with a custom GATT Service and a custom Characteristic that sends periodic
 * notifications.
 */
// *****************************************************************************
#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"

#include "le_common.h"
#include "le_gma.h"
#include "gma.h"
#include "bt_tws.h"
#include "ble_user.h"
#include "3th_profile_api.h"
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_GMA)

#define TEST_SEND_DATA_RATE          0  //测试 send_data
#define TEST_SEND_HANDLE_VAL         ATT_CHARACTERISTIC_fed8_01_VALUE_HANDLE
/* #define TEST_SEND_HANDLE_VAL         ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE */

#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info          printf
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif


static int app_send_user_audio_data_do(u8 *data, u16 len);
void ble_ibeacon_timer_close(void);
/* #define LOG_TAG_CONST       BT_BLE */
/* #define LOG_TAG             "[LE_S_DEMO]" */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
/* #define LOG_CLI_ENABLE */
/* #include "debug.h" */

//------
/* #define ATT_CTRL_BLOCK_SIZE       (80)                    //note: fixed */
#define ATT_LOCAL_PAYLOAD_SIZE    (180)                   //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (2*512)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));
//---------------
#define BLE_ADV_TYPE_NORMAL		     0
#define BLE_ADV_TYPE_IBEACON	     1
static u8 ble_adv_type = BLE_ADV_TYPE_NORMAL;

#if TEST_SEND_DATA_RATE
static u32 test_data_count;
static u32 server_timer_handle = 0;
static u8 test_data_start;
#endif

//-----------------
#define GMA_OTA					BIT(2)
#define GMA_ENCRYPT_EN 			BIT(3)
#define GMA_MACHINE_SECRET		BIT(4)
#define JL_APP_EN				0

#if GMA_TWS_SUPPORTED
u8 gma_tws_support = 1;
#endif
typedef struct {
    u16 cid;
    u8 vid: 4;
    u8 subtype: 4;
    u8 fask;
    u8 pid[4];
    u8 mac[6];
#if GMA_TWS_SUPPORTED
    u16 ext;
#endif
} tm_info_t;

const static tm_info_t gma_manufacturer = {
    /* .len = 0x0c,	 */
    /* .type = 0xff,	 */
    .cid = 0x01a8,
    .vid = 5,
    .subtype = 0x0a,
#if (GMA_OTA_EN)
    .fask = 1 | GMA_OTA | GMA_ENCRYPT_EN | GMA_MACHINE_SECRET,
#else
    .fask = 1 | GMA_ENCRYPT_EN | GMA_MACHINE_SECRET,
#endif
    /*bt pid set by gma para pid*/
    .pid = {0x00, 0x00, 0x00, 0x00},
#if GMA_TWS_SUPPORTED
    .ext = BIT(0) | BIT(2) | BIT(4),
#endif
};

typedef struct {
    u8 pre_amble[4];

    u16 cid;
    u8 vid;
    u8 pid[4];
    u8 mac[6];
    u8 rfu[3];

#if 0
    u8 major_value[2];
    u8 minor_value[2];
    u8 measured_power;
#endif // 0
} __attribute__((packed)) tm_ibeacon_t;

const static tm_ibeacon_t gma_ibeacon_manufacturer = {
    .pre_amble = {0x4c, 0x00, 0x02, 0x15},

    .cid = 0x01a8,
    .vid = 0x01,
    .pid = {0, 0, 0, 0},
    .mac = {0, 0, 0, 0, 0, 0},
    .rfu = {0, 0, 0},
#if 0
    .major_value = {0x00, 0x01},
    .minor_value = {0x00, 0x02},
    .measured_power = 0xcd,
#endif // 0
};
//---------------
#define ADV_INTERVAL_MIN                  (800)
#define ADV_INTERVAL_MIN_IBEACON          (32)

static volatile hci_con_handle_t con_handle;
///adv flag
static volatile u8 adv_is_en = 0;

//加密设置
static const uint8_t sm_min_key_size = 7;

//连接参数设置
static const uint8_t connection_update_enable = 1; ///0--disable, 1--enable
static uint8_t connection_update_cnt = 0; //
static const struct conn_update_param_t connection_param_table[] = {
    {16, 24, 16,  600},//11
    {12, 28, 14,  600},//3.7
    {8,  20, 20,  600},

};

static const struct conn_update_param_t connection_param_table_call[] = {
    {120,  160, 0,  600},
    {60,  80, 0,  600},
    {60,  80, 0,  600},
};

static u8 conn_update_param_index = 0;
static u8 conn_param_len = sizeof(connection_param_table) / sizeof(struct conn_update_param_t);
#define CONN_PARAM_TABLE_CNT      (sizeof(connection_param_table)/sizeof(struct conn_update_param_t))

// 广播包内容
/* static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31 */
// scan_rsp 内容
/* static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31 */

#if (ATT_RAM_BUFSIZE < 64)
#error "adv_data & rsp_data buffer error!!!!!!!!!!!!"
#endif

#define adv_data       &att_ram_buffer[0]
#define scan_rsp_data  &att_ram_buffer[32]

static char gap_device_name[BT_NAME_LEN_MAX] = "br22_ble_test";
static u8 gap_device_name_len = 0;
static u8 ble_work_state = 0;
static u8 test_read_write_buf[4];
static u8 adv_ctrl_en;

static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;

static int app_send_user_data_check(u16 len);
static int app_send_user_data_do(void *priv, u8 *data, u16 len);
static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);
static int gma_send_user_data_do(u8 *data, u16 len);

// Complete Local Name  默认的蓝牙名字

//------------------------------------------------------
//广播参数设置
static void advertisements_setup_init();
extern const char *bt_get_local_name();
static int set_adv_enable(void *priv, u32 en);
static int get_buffer_vaild_len(void *priv);
extern void wdt_clear(void);
//------------------------------------------------------
static void send_request_connect_parameter(u8 table_index)
{
    struct conn_update_param_t *param = NULL; //static ram
    switch (conn_update_param_index) {
    case 0:
        param = (void *)&connection_param_table[table_index];
        break;
    case 1:
        param = (void *)&connection_param_table_call[table_index];
        break;
    default:
        break;
    }

    log_info("update_request:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    if (con_handle) {
        ble_user_cmd_prepare(BLE_CMD_REQ_CONN_PARAM_UPDATE, 2, con_handle, param);
    }
}

static void check_connetion_updata_deal(void)
{
    if (connection_update_enable) {
        if (connection_update_cnt < conn_param_len) {
            send_request_connect_parameter(connection_update_cnt);
        }
    }
}

static void connection_update_complete_success(u8 *packet)
{
    int con_handle, conn_interval, conn_latency, conn_timeout;

    con_handle = hci_subevent_le_connection_update_complete_get_connection_handle(packet);
    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);
}


_INLINE_
static void set_ble_work_state(ble_state_e state)
{
    if (state != ble_work_state) {
        //log_info("ble_work_st:%x->%x\n", ble_work_state, state);
        ble_work_state = state;
        if (app_ble_state_callback) {
            app_ble_state_callback((void *)channel_priv, state);
        }
    }
}

_INLINE_
static ble_state_e get_ble_work_state(void)
{
    return ble_work_state;
}

static void cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            log_info("Just Works Confirmed.\n");
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("Passkey display: %06u.\n", tmp32);
            break;
        }
        break;
    }
}


#if TEST_SEND_DATA_RATE
static void server_timer_handler(void)
{
    if (!con_handle) {
        test_data_count = 0;
        test_data_start = 0;
        return;
    }

    if (test_data_count) {
        log_info("\n-ble_data_rate: %d bps-\n", test_data_count * 8);
        test_data_count = 0;
    }
}

static void server_timer_start(void)
{
    server_timer_handle  = sys_timer_add(NULL, server_timer_handler, 1000);
}

static void server_timer_stop(void)
{
    if (server_timer_handle) {
        sys_timeout_del(server_timer_handle);
        server_timer_handle = 0;
    }
}

void test_data_send_packet(void)
{
    u32 vaild_len = get_buffer_vaild_len(0);

    if (!test_data_start) {
        return;
    }

    if (vaild_len) {
        /* printf("\n---test_data_len = %d---\n",vaild_len); */
        app_send_user_data(TEST_SEND_HANDLE_VAL, &test_data_send_packet, vaild_len, ATT_OP_AUTO_READ_CCC);
        /* app_send_user_data(ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, &test_data_send_packet, vaild_len,ATT_OP_AUTO_READ_CCC); */
        test_data_count += vaild_len;
    }
    wdt_clear();
}
#endif


static void can_send_now_wakeup(void)
{
    putchar('E');
    tm_send_process_resume();
#if 0
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }

#if TEST_SEND_DATA_RATE
    test_data_send_packet();
#endif
#endif
}

extern void sys_auto_shut_down_disable(void);
extern void sys_auto_shut_down_enable(void);
extern u8 get_total_connect_dev(void);
static void ble_auto_shut_down_enable(u8 enable)
{
#if TCFG_AUTO_SHUT_DOWN_TIME
    if (enable) {
        if (get_total_connect_dev() == 0) {    //已经没有设备连接
            sys_auto_shut_down_enable();
        }
    } else {
        sys_auto_shut_down_disable();
    }
#endif
}

_WEAK_
u8 ble_update_get_ready_jump_flag(void)
{
    return 0;
}
/*
 * @section Packet Handler
 *
 * @text The packet handler is used to:
 *        - stop the counter after a disconnect
 *        - send a notification when the requested ATT_EVENT_CAN_SEND_NOW is received
 */

/* LISTING_START(packetHandler): Packet Handler */
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        /* case DAEMON_EVENT_HCI_PACKET_SENT: */
        /* break; */
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE\n");
        case ATT_EVENT_CAN_SEND_NOW:
            can_send_now_wakeup();
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                con_handle = little_endian_read_16(packet, 4);
                ble_ibeacon_timer_close();
                log_info(">>>>>HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", con_handle);
                connection_update_complete_success(packet + 8);
                ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_PAYLOAD_SIZE);
                set_ble_work_state(BLE_ST_CONNECT);
                ble_auto_shut_down_enable(0);
                set_app_connect_type(TYPE_BLE);
                gma_init(app_send_user_data_check, gma_send_user_data_do, app_send_user_audio_data_do);
            }
            break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                connection_update_complete_success(packet);
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info(">>>>>HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
            gma_exit();
            set_app_connect_type(TYPE_NULL);
            con_handle = 0;
            ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, 0, 0, 0);
            set_ble_work_state(BLE_ST_DISCONN);

            if (!ble_update_get_ready_jump_flag()) {
                bt_ble_adv_enable(1);
            }
            connection_update_cnt = 0;
            ble_auto_shut_down_enable(1);
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU = %u\n", mtu);
            ble_user_cmd_prepare(BLE_CMD_ATT_MTU_SIZE, 1, mtu);
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("-update_rsp: %02x\n", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!\n");
                check_connetion_updata_deal();
            } else {
                connection_update_cnt = conn_param_len;
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
            break;
        }
        break;
    }
}


/* LISTING_END */

/*
 * @section ATT Read
 *
 * @text The ATT Server handles all reads to constant data. For dynamic data like the custom characteristic, the registered
 * att_read_callback is called. To handle long characteristics and long reads, the att_read_callback is first called
 * with buffer == NULL, to request the total value length. Then it will be called again requesting a chunk of the value.
 * See Listing attRead.
 */

/* LISTING_START(attRead): ATT Read */

// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{

    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback, handle= 0x%04x,buffer= %08x\n", handle, (u32)buffer);

    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        att_value_len = gap_device_name_len;

        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &gap_device_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("\n------read gap_name: %s \n", gap_device_name);
        }
        break;

    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_fed6_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_fed8_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;

    default:
        break;
    }

    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;
}


/* LISTING_END */
/*
 * @section ATT Write
 *
 * @text The only valid ATT write in this example is to the Client Characteristic Configuration, which configures notification
 * and indication. If the ATT handle matches the client configuration handle, the new configuration value is stored and used
 * in the heartbeat handler to decide if a new value should be sent. See Listing attWrite.
 */

/* LISTING_START(attWrite): ATT Write */
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;

    /* log_info("write_callback, handle= 0x%04x,size = %d\n", handle, buffer_size); */

    switch (handle) {

    case ATT_CHARACTERISTIC_fed5_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_fed7_01_VALUE_HANDLE:
        log_info("\n-gma_write %04x :\n", buffer_size);
        log_info_hexdump(buffer, buffer_size > 16 ? 16 : buffer_size);
#if(TCFG_USER_TWS_ENABLE)
        if (get_tws_sibling_connect_state()) { //对耳已连接，断开对耳
            if (tws_api_get_role() == TWS_ROLE_MASTER) {///slave do not need to analysis cmd
                gma_recv_proc(buffer, buffer_size);
            }
        } else {
            gma_recv_proc(buffer, buffer_size);
        }
#else
        gma_recv_proc(buffer, buffer_size);
#endif
        break;

    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_fed6_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_fed8_01_CLIENT_CONFIGURATION_HANDLE:
        set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
        check_connetion_updata_deal();
        log_info("\n------write ccc:%04x, %02x\n", handle, buffer[0]);
        att_set_ccc_config(handle, buffer[0]);
        can_send_now_wakeup();

#if TEST_SEND_DATA_RATE
        if (buffer[0]) {
            test_data_send_packet();
        }
#endif
        break;

    default:
        break;
    }
    return 0;
}

static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type)
{
    u32 ret = APP_BLE_NO_ERROR;

    if (!con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }


    ret = ble_user_cmd_prepare(BLE_CMD_ATT_SEND_DATA, 4, handle, data, len, handle_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }

    if (ret) {
        log_info("app_send_fail:%d !!!!!!\n", ret);
    }
    return ret;
}

static void adv_bt_mac_reverse(u8 *mac_addr)
{
    ///debug.reverse mac address
    for (int i = 0, temp = 0; i < 6 / 2; i++) {
        temp = mac_addr[i];
        mac_addr[i] = mac_addr[5 - i];
        mac_addr[5 - i] = temp;
    }

}

__attribute__((weak)) u8 *gma_pid_get(void)
{
    return (u8 *)gma_pid_get;//for test
}

extern const u8 *bt_get_mac_addr();
void gma_bt_mac_addr_get(uint8_t *buf);
//------------------------------------------------------

static int make_set_adv_data_ibeacon(void)
{
    u8 offset = 0;
    u8 *buf = adv_data;
    tm_ibeacon_t  manufacturer_data;

    //printf("ibeacon ble adv \n ");
    memcpy(&manufacturer_data, &gma_ibeacon_manufacturer, sizeof(tm_ibeacon_t));
    /* memcpy(manufacturer_data.mac, bt_get_mac_addr(), 6); */
    gma_bt_mac_addr_get(manufacturer_data.mac);

    le_controller_set_mac(manufacturer_data.mac);
    //adv_bt_mac_reverse(manufacturer_data.mac);
    //printf(">>>>>>>>ble addr: ");
    //printf_buf(manufacturer_data.mac, 6);
    memcpy(manufacturer_data.pid, gma_pid_get(), 4);

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 6, 1);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, 0xFEB3, 2);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, \
                                   (void *)&manufacturer_data, sizeof(tm_ibeacon_t));

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***adv_data overflow!!!!!!\n");
        return -1;
    }
    //r_printf("adv_data(%d):", offset);
    //put_buf(buf, offset);
    ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, offset, buf);
    return 0;
}

_INLINE_
static int make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = adv_data;
    tm_info_t manufacturer_data;

    u8 *gma_pid_get(void);
    //printf("mormal ble adv \n ");
    memcpy(&manufacturer_data, &gma_manufacturer, sizeof(tm_info_t));
    /* memcpy(manufacturer_data.mac, bt_get_mac_addr(), 6); */
    gma_bt_mac_addr_get(manufacturer_data.mac);
    le_controller_set_mac(manufacturer_data.mac);

    //adv_bt_mac_reverse(manufacturer_data.mac);
    //printf(">>>>>>>>ble addr: ");
    //printf_buf(manufacturer_data.mac, 6);
    memcpy(manufacturer_data.pid, gma_pid_get(), 4);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 6, 1);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, 0xFEB3, 2);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, (void *)&manufacturer_data, sizeof(tm_info_t));

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***adv_data overflow!!!!!!\n");
        return -1;
    }
    //r_printf("adv_data(%d):", offset);
    //put_buf(buf, offset);
    ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, offset, buf);
    return 0;
}

_INLINE_
static int make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = scan_rsp_data;

    u8 name_len = gap_device_name_len;
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len > vaild_len) {
        name_len = vaild_len;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

//    log_info("rsp_data(%d):", offset);
//    log_info_hexdump(buf, offset);
    ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, offset, buf);
    return 0;
}

//广播参数设置
_INLINE_
static void advertisements_setup_init()
{
    uint8_t adv_type = 0;
    uint8_t adv_channel = 7;
    int   ret = 0;

    if (ble_adv_type == BLE_ADV_TYPE_IBEACON) {
        //adv_type = 3;
//        putchar('E');
    }

    bool ble_ibeacon_is_active(void);
    if (ble_ibeacon_is_active()) {
        ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, ADV_INTERVAL_MIN_IBEACON, adv_type, adv_channel);
    } else {
        ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, ADV_INTERVAL_MIN, adv_type, adv_channel);
    }


    if (ble_adv_type == BLE_ADV_TYPE_NORMAL) {
        ret |= make_set_adv_data();
    } else {

        ret |= make_set_adv_data_ibeacon();
    }

    ret |= make_set_rsp_data();

    if (ret) {
        puts("advertisements_setup_init fail !!!!!!\n");
        return;
    }

}

#define PASSKEY_ENTER_ENABLE      0 //输入passkey使能，可修改passkey
//重设passkey回调函数，在这里可以重新设置passkey
//passkey为6个数字组成，十万位、万位。。。。个位 各表示一个数字 高位不够为0
static void reset_passkey_cb(u32 *key)
{
#if 1
    u32 newkey = rand32();//获取随机数

    newkey &= 0xfffff;
    if (newkey > 999999) {
        newkey = newkey - 999999; //不能大于999999
    }
    *key = newkey; //小于或等于六位数
    printf("set new_key= %06u\n", *key);
#else
    *key = 123456; //for debug
#endif
}

extern void reset_PK_cb_register(void (*reset_pk)(u32 *));
void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
{
    //setup SM: Display only
    sm_init();
    sm_set_io_capabilities(io_type);
    sm_set_authentication_requirements(auth_req);
    sm_set_encryption_key_size_range(min_key_size, 16);
    sm_set_request_security(security_en);
    sm_event_callback_set(&cbk_sm_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        reset_PK_cb_register(reset_passkey_cb);
    }
}

extern void le_device_db_init(void);
void ble_profile_init(void)
{
    printf("ble profile init\n");
    le_device_db_init();

#if PASSKEY_ENTER_ENABLE
    ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_MITM_PROTECTION, 7, TCFG_BLE_SECURITY_EN);
#else
    ble_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_BONDING, 7, TCFG_BLE_SECURITY_EN);
#endif

    /* setup ATT server */
    att_server_init(profile_data, att_read_callback, att_write_callback);
    att_server_register_packet_handler(cbk_packet_handler);
    /* gatt_client_register_packet_handler(packet_cbk); */

    // register for HCI events
    hci_event_callback_set(&cbk_packet_handler);
    /* ble_l2cap_register_packet_handler(packet_cbk); */
    /* sm_event_packet_handler_register(packet_cbk); */
    le_l2cap_register_packet_handler(&cbk_packet_handler);
}



_INLINE_
static int set_adv_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    if (!adv_ctrl_en) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (en) {
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state =  get_ble_work_state();
    switch (cur_state) {
    case BLE_ST_ADV:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
        break;
    default:
        return APP_BLE_OPERATION_ERROR;
        break;
    }

    if (cur_state == next_state) {
        return APP_BLE_NO_ERROR;
    }
    //log_info("adv_en:%d\n", en);
    set_ble_work_state(next_state);
    if (en) {
        advertisements_setup_init();
    }
    ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, en);
    return APP_BLE_NO_ERROR;
}


static int ble_disconnect(void *priv)
{
    if (con_handle) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            log_info(">>>ble send disconnect\n");
            set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_user_cmd_prepare(BLE_CMD_DISCONNECT, 1, con_handle);
        } else {
            log_info(">>>ble wait disconnect...\n");
        }
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}

static int get_buffer_vaild_len(void *priv)
{
    u32 vaild_len = 0;
    ble_user_cmd_prepare(BLE_CMD_ATT_VAILD_LEN, 1, &vaild_len);
    return vaild_len;
}

static int app_send_user_data_do(void *priv, u8 *data, u16 len)
{
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-le_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif

#if (JL_APP_EN)
    int res = app_send_user_data(ATT_CHARACTERISTIC_fed8_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
#else
    int res = app_send_user_data(ATT_CHARACTERISTIC_fed6_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
#endif

    return res;

}

static int gma_send_user_data_do(u8 *data, u16 len)
{
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-le_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif

#if (JL_APP_EN)
    int res = app_send_user_data(ATT_CHARACTERISTIC_fed8_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
#else
    int res = app_send_user_data(ATT_CHARACTERISTIC_fed6_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
#endif

    return res;

}

static int app_send_user_audio_data_do(u8 *data, u16 len)
{
    int res = app_send_user_data(ATT_CHARACTERISTIC_fed8_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
    return res;
}


static int app_send_user_data_check(u16 len)
{
    u32 buf_space = get_buffer_vaild_len(0);
    if (len <= buf_space) {
        return 1;
    }
    return 0;
}


static int regiest_wakeup_send(void *priv, void *cbk)
{
    ble_resume_send_wakeup = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_recieve_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_recieve_callback = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_state_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_ble_state_callback = cbk;
    return APP_BLE_NO_ERROR;
}

u8 bt_ble_adv_is_en(void)
{
    return adv_is_en;
}

_INLINE_
void bt_ble_adv_enable(u8 enable)
{
    adv_is_en = enable;
    //r_printf(">>>>>>>>>>>>>>>>>>>>>>bt enalbe:%d \n", adv_is_en);
    set_adv_enable(0, enable);
}

u16 bt_ble_is_connected(void)
{
    return con_handle;
}

void ble_module_enable(u8 en)
{
    r_printf("---------------------------ble mode_en:%d\n", en);
    if (en) {
        adv_ctrl_en = 1;
        bt_ble_adv_enable(1);
    } else {
        if (con_handle) {
            adv_ctrl_en = 0;
            ble_disconnect(NULL);
        } else {
            bt_ble_adv_enable(0);
            adv_ctrl_en = 0;
        }
    }
}

void ble_mac_reset(void)
{

    ///close ble module
    ble_module_enable(0);
    ///reset ble mac address
    u8 buf[6];
    gma_bt_mac_addr_get(buf);

    le_controller_set_mac(buf);
    ///enable ble module
    ble_module_enable(1);
    printf("ble mac reset success \n");

}

void phone_call_close_ble_adv(void)
{
    int role = tws_api_get_role();
    printf("call, role: %d, state: %d", role, ble_work_state);
    if ((role == TWS_ROLE_MASTER) && (BLE_ST_ADV == ble_work_state)) { //BLE正在广播
        //关闭BLE
        bt_ble_adv_enable(0);
    }
}

void phone_call_resume_ble_adv(void)
{
    int role = tws_api_get_role();
    printf("call, role: %d, con_hd: %d\n", role, con_handle);

    if ((get_call_status() == BT_CALL_HANGUP) && (role == TWS_ROLE_MASTER) && (0 == con_handle)) { //BLE未连接，且需要广播
        //开启广播
        bt_ble_adv_enable(1);
    }
}


void phone_call_change_ble_conn_param(u8 param_index)
{
    if (!con_handle) {
        return;
    }
    connection_update_cnt = 0;
    conn_update_param_index = param_index;
    switch (param_index) {
    case 0:
        conn_param_len = sizeof(connection_param_table) / sizeof(struct conn_update_param_t);
        break;
    case 1:
        conn_param_len = sizeof(connection_param_table_call) / sizeof(struct conn_update_param_t);
        break;
    default:
        break;
    }
    check_connetion_updata_deal();
}

/*
 *switch ibeacon mode & normal mode
 * */
static u16 timeout_id = 0;
static void ble_normal_adv(void *priv)
{
    timeout_id = 0;
    ble_adv_type = BLE_ADV_TYPE_NORMAL;
    if (bt_ble_adv_is_en()) {
        ble_adv_type = BLE_ADV_TYPE_NORMAL;
        ble_mac_reset();
    }
}

static u16 ble_ibeacon_adv_timer_hdl = 0;
static u16 ble_ibeacon_adv_timerout_hdl = 0;
void ble_ibeacon_adv_timeout(void *priv)
{
    ble_ibeacon_timer_close();

    printf(">>>>>>>ble_ibeacon_adv_timeout \n");
    ///close ble adv
    bt_ble_adv_enable(0);
    ///switch adv data packet
    ble_adv_type = BLE_ADV_TYPE_NORMAL;
    ///open ble adv
    bt_ble_adv_enable(1);
}

void ble_ibeacon_adv_timer(void *priv)
{
    static int adv_type = 0;
    if ((get_app_connect_type() != TYPE_NULL) || !bt_ble_adv_is_en() \
        || (BT_STATUS_TAKEING_PHONE == get_bt_connect_status())) {
        ///ble connect or adv close
        printf(">>>>>>>>>close ibeacon timer \n");
        ble_ibeacon_timer_close();

        if ((get_app_connect_type() != TYPE_NULL) && bt_ble_adv_is_en()) {
            ble_adv_type = BLE_ADV_TYPE_NORMAL;
            bt_ble_adv_enable(0);
        }
        return;
    }

    adv_type ++;


    wdt_clear();
    ///close ble adv
    bt_ble_adv_enable(0);

    ///switch adv data packet
    if (adv_type >= 2) {
        adv_type = 0;
        //putchar('A');
        ble_adv_type = BLE_ADV_TYPE_NORMAL;
    } else {
        //putchar('B');
        ble_adv_type = BLE_ADV_TYPE_IBEACON;
    }
    ///open ble adv
    bt_ble_adv_enable(1);

    ///reset status
    ble_adv_type = BLE_ADV_TYPE_NORMAL;
}

void ble_ibeacon_timer_init(void)
{
    ///active timer,250ms
    if (ble_ibeacon_adv_timer_hdl != 0) {
        return;
    }

    ble_ibeacon_adv_timer_hdl = sys_timer_add(NULL, ble_ibeacon_adv_timer, 20);
    ble_ibeacon_adv_timerout_hdl = sys_timeout_add(NULL, ble_ibeacon_adv_timeout, 30000);
}

void ble_ibeacon_timer_close(void)
{
    if (ble_ibeacon_adv_timer_hdl) {
        sys_timer_del(ble_ibeacon_adv_timer_hdl);
        ble_ibeacon_adv_timer_hdl = 0;

        sys_timeout_del(ble_ibeacon_adv_timerout_hdl);
        ble_ibeacon_adv_timerout_hdl = 0;
    }
}

void ble_ibeacon_adv(void)
{

    if (BT_STATUS_TAKEING_PHONE == get_bt_connect_status()) {
        printf("phone ing...\n");
        return;
    }

    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }

    ble_ibeacon_timer_init();
}

_INLINE_
bool ble_ibeacon_is_active(void)
{
    return (ble_ibeacon_adv_timer_hdl != 0);
}

//SYS_HI_TIMER_ADD(ble_ibeacon_adv_timer, NULL, 20);

#define BLE_EXT_NAME_EN         0 //添加后缀名
#if BLE_EXT_NAME_EN
static const char ble_ext_name[] = "(BLE)";
#endif

void bt_ble_init(void)
{
    log_info("***** ble_init******\n");
    char *name_p;
#if BLE_EXT_NAME_EN
    u8 ext_name_len = sizeof(ble_ext_name) - 1;
#else
    u8 ext_name_len = 0;
#endif

    name_p = (char *)bt_get_local_name();
    gap_device_name_len = strlen(name_p);
    if (gap_device_name_len > BT_NAME_LEN_MAX - ext_name_len) {
        gap_device_name_len = BT_NAME_LEN_MAX - ext_name_len;
    }

    //增加后缀，区分名字
    memcpy(gap_device_name, name_p, gap_device_name_len);
#if BLE_EXT_NAME_EN
    memcpy(&gap_device_name[gap_device_name_len], "(BLE)", ext_name_len);
    gap_device_name_len += ext_name_len;
#endif

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);

    set_ble_work_state(BLE_ST_INIT_OK);
#if (GMA_BLE_ADV_MODE == 0)
    ble_module_enable(1);
#endif

#if TEST_SEND_DATA_RATE
    server_timer_start();
#endif
}

void bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");

    ble_module_enable(0);

#if TEST_SEND_DATA_RATE
    server_timer_stop();
#endif

}

void ble_app_disconnect(void)
{
    ble_disconnect(NULL);
}

static const struct ble_server_operation_t mi_ble_operation = {
    .adv_enable = set_adv_enable,
    .disconnect = ble_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .send_data = (void *)app_send_user_data_do,
    .regist_wakeup_send = regiest_wakeup_send,
    .regist_recieve_cbk = regiest_recieve_cbk,
    .regist_state_cbk = regiest_state_cbk,
};


void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&mi_ble_operation;
}

extern bool get_remote_test_flag();
void ble_server_send_test_key_num(u8 key_num)
{
    if (con_handle) {
        if (get_remote_test_flag()) {
            ble_user_cmd_prepare(BLE_CMD_SEND_TEST_KEY_NUM, 2, con_handle, key_num);
        } else {
            log_info("-not conn testbox\n");
        }
    }
}


bool ble_msg_deal(u32 param)
{
    struct ble_server_operation_t *test_opt;
    ble_get_server_operation_table(&test_opt);

#if 0//for test key
    switch (param) {
    case MSG_BT_PP:
        break;

    case MSG_BT_NEXT_FILE:
        break;

    case MSG_HALF_SECOND:
        /* putchar('H'); */
        break;

    default:
        break;
    }
#endif

    return FALSE;
}

void input_key_handler(u8 key_status, u8 key_number)
{
    ble_server_send_test_key_num(key_number);
}

#endif


