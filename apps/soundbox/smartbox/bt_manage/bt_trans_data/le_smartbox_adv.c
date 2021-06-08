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
#include "smartbox/config.h"

#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "app_power_manage.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"
#include "btstack/avctp_user.h"
#include "app_chargestore.h"

#include "btcrypt.h"
#include "custom_cfg.h"
#include "smartbox_music_info_setting.h"
#include "classic/tws_api.h"
#include "le_smartbox_module.h"
#include "smartbox_rcsp_manage.h"
#include "bt/bt_tws.h"

#include "asm/charge.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_RCSP_DEMO)


#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info          printf
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

static u8 adv_data_len;
static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 scan_rsp_data_len;
static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31
static u32 ble_timer_handle = 0;
u16 sibling_ver_info = 0;

#define BLE_TIMER_SET (500)

struct _bt_adv_handle {

    u8 seq_rand;
    u8 connect_flag;

    u8 bat_charge_L;
    u8 bat_charge_R;
    u8 bat_charge_C;
    u8 bat_percent_L;
    u8 bat_percent_R;
    u8 bat_percent_C;

    u8 modify_flag;
    u8 adv_disable_cnt;

    u8 ble_adv_notify;
} bt_adv_handle = {0};

#define __this (&bt_adv_handle)

extern u8 get_tws_sibling_bat_persent(void);

u8 get_adv_notify_state(void)
{
    return __this->ble_adv_notify;
}

void set_adv_notify_state(u8 notify)
{
    __this->ble_adv_notify  = notify;
}

static void rcsp_adv_fill_mac_addr(u8 *mac_addr_buf)
{
#if (MUTIl_CHARGING_BOX_EN)
    extern u8 *get_chargebox_adv_addr(void);
    u8 *mac_addr = get_chargebox_adv_addr();
    if (mac_addr) {
        swapX(mac_addr, mac_addr_buf, 6);
    }

    /* printf("mac_addr:"); */
    /* printf_buf(mac_addr_buf, 6); */

#else
    swapX(bt_get_mac_addr(), mac_addr_buf, 6);
#endif
}

int smartbox_make_set_adv_data(void)
{
    u8 i;
    u8 *buf = adv_data;
    buf[0] = 0x1E;
    buf[1] = 0xFF;

    buf[2] = 0xD6;	// JL ID
    buf[3] = 0x05;

#if 1
    u16 vid = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);
    buf[4] = vid & 0xFF;
    buf[5] = vid >> 8;

    u16 pid = get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
    buf[6] = pid & 0xFF;
    buf[7] = pid >> 8;
#else
    buf[4] = 0x02;	// VID
    buf[5] = 0x00;

    buf[6] = 0x33;	// PID
    buf[7] = 0x00;
#endif

#if (MUTIl_CHARGING_BOX_EN)
    buf[8] = 0x01;	//   0:音箱类型   |  protocol verson
#else // MUTIl_CHARGING_BOX_EN
#if (SOUNDCARD_ENABLE)
    buf[8] = 0x40;	//   4:声卡类型   |  protocol verson
#else
    buf[8] = 0x0;	//   0:音箱类型   |  protocol verson
#endif
    if (RCSP_SPP == get_defalut_bt_channel_sel()) {
        buf[8] |= 2;
    }
#endif // MUTIl_CHARGING_BOX_EN end

    rcsp_adv_fill_mac_addr(&buf[9]);
    buf[15] = __this->connect_flag;

    /* buf[16] = 0xC1; */
    /* buf[17] = 0x40; */
    /* buf[18] = 0x30; */
    buf[16] = __this->bat_percent_L ? (((!!__this->bat_charge_L) << 7) | (__this->bat_percent_L & 0x7F)) : 0;
    buf[17] = __this->bat_percent_R ? (((!!__this->bat_charge_R) << 7) | (__this->bat_percent_R & 0x7F)) : 0;
    buf[18] = __this->bat_percent_C ? (((!!__this->bat_charge_C) << 7) | (__this->bat_percent_C & 0x7F)) : 0;

    memset(&buf[19], 0x00, 4);							// reserve

    buf[19] = __this->seq_rand;

#if (MUTIl_CHARGING_BOX_EN)
    buf[20] = 1;
#else
    if (RCSP_SPP == get_defalut_bt_channel_sel()) {
        buf[20] = 1;
    }
#endif

    u8 t_buf[16];
    btcon_hash(&buf[2], 16, &buf[15], 4, t_buf);		// hash

    for (i = 0; i < 8; i++) {								// single byte
        buf[23 + i] = t_buf[2 * i + 1];
    }

    __this->modify_flag = 0;
    adv_data_len = 31;
    ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, 31, buf);
    r_printf("ADV data():");
    put_buf(buf, 31);
    return 0;
}

int smartbox_make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = scan_rsp_data;
    u8 *edr_name = bt_get_local_name();

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)edr_name, strlen(edr_name));
    /* ble_printf("RSP data(%d):,offset"); */
    /* ble_put_buf(buf, offset); */

    scan_rsp_data_len = 31;
    ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, 31, buf);
    return 0;
}


static int update_adv_data(u8 *buf)
{
    u8 i;
    buf[1] = 0xD6;	// JL ID
    buf[0] = 0x05;

#if 1
    u16 vid = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);
    buf[3] = vid & 0xFF;
    buf[2] = vid >> 8;

    u16 pid = get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
    buf[5] = pid & 0xFF;
    buf[4] = pid >> 8;
#else
    buf[3] = 0x02;	// VID
    buf[2] = 0x00;

    buf[5] = 0x33;	// PID
    buf[4] = 0x00;
#endif

#if (SOUNDCARD_ENABLE)
    buf[6] = 0x40;	//   4:声卡类型   |  protocol verson
#else
    buf[6] = 0x0;	//   0:音箱类型   |  protocol verson
#endif
    if (RCSP_SPP == get_defalut_bt_channel_sel()) {
        buf[6] |= 2;
    }

    swapX(bt_get_mac_addr(), &buf[7], 6);

    buf[13] = __this->connect_flag;

    buf[14] = __this->bat_percent_L ? (((!!__this->bat_charge_L) << 7) | (__this->bat_percent_L & 0x7F)) : 0;
    buf[15] = __this->bat_percent_R ? (((!!__this->bat_charge_R) << 7) | (__this->bat_percent_R & 0x7F)) : 0;
    buf[16] = __this->bat_percent_C ? (((!!__this->bat_charge_C) << 7) | (__this->bat_percent_C & 0x7F)) : 0;

    buf[17] = __this->seq_rand;

    return 0;
}


u8 *ble_get_scan_rsp_ptr(u16 *len)
{
    if (len) {
        *len = scan_rsp_data_len;
    }
#if 0//RCSP_UPDATE_EN
    u8 offset = 0;
    offset = make_eir_packet_val(&scan_rsp_data[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x02, 1);
#endif
    return scan_rsp_data;
}

u8 *ble_get_adv_data_ptr(u16 *len)
{
    if (len) {
        *len = adv_data_len;
    }
#if RCSP_UPDATE_EN
    adv_data[15] = 1;
    adv_data[20] = 0;
    u8 t_buf[16];
    btcon_hash(&adv_data[2], 16, &adv_data[15], 4, t_buf);
    for (u8 i = 0; i < 8; i++) {
        adv_data[23 + i] = t_buf[2 * i + 1];
    }
#endif
    return adv_data;
}





static u8 update_dev_battery_level(void)
{

    u8 master_bat = 0;
    u8 master_charge = 0;
    u8 slave_bat = 0;
    u8 slave_charge = 0;
    u8 box_bat = 0;
    u8 box_charge = 0;

//	Master bat
#if CONFIG_DISPLAY_DETAIL_BAT
    master_bat = get_vbat_percent();
#else
    master_bat = get_self_battery_level() * 10 + 10;
#endif
    if (master_bat > 100) {
        master_bat = 100;
    }
    master_charge = get_charge_online_flag();


// Slave bat

#if	TCFG_USER_TWS_ENABLE
    slave_bat = get_tws_sibling_bat_persent();
#if TCFG_CHARGESTORE_ENABLE
    if (slave_bat == 0xff) {
        /* log_info("--update_bat_01\n"); */
        if (get_bt_tws_connect_status()) {
            slave_bat = chargestore_get_sibling_power_level();
        }
    }
#endif

    if (slave_bat == 0xff) {
        /*  log_info("--update_bat_02\n"); */
        slave_bat = 0x00;
    }

    slave_charge = !!(slave_bat & 0x80);
    slave_bat &= 0x7F;
#else
    slave_charge = 0;
    slave_bat = 0;
#endif

// Box bat
    if ((master_charge && (master_bat != 0))
        || (slave_charge && (slave_bat != 0))) {
        //earphone in charge
#if TCFG_CHARGESTORE_ENABLE
        box_bat = chargestore_get_power_level();
        if (box_bat == 0xFF) {
            box_bat = 0;
        }
#else
        box_bat = 0;
#endif
    } else {
        //all not in charge
        box_bat = 0;
    }
    box_charge = !!(box_bat & 0x80);
    box_bat &= 0x7F;
// check if master is L

    u8 tbat_charge_L = 0;
    u8 tbat_charge_R = 0;
    u8 tbat_percent_L = 0;
    u8 tbat_percent_R = 0;
    u8 tbat_percent_C = box_bat;
    u8 tbat_charge_C = box_charge;
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        if ('L' == tws_api_get_local_channel()) {
            tbat_charge_L = master_charge;
            tbat_charge_R = slave_charge;
            tbat_percent_L = master_bat;
            tbat_percent_R = slave_bat;
        } else if ('R' == tws_api_get_local_channel()) {
            tbat_charge_L = slave_charge;
            tbat_charge_R = master_charge;
            tbat_percent_L = slave_bat;
            tbat_percent_R = master_bat;
        }
    } else {
        switch (tws_api_get_local_channel()) {
        case 'R':
            tbat_charge_R = master_charge;
            tbat_percent_R = master_bat;
            break;
        case 'L':
        default:
            tbat_charge_L = master_charge;
            tbat_percent_L = master_bat;
            break;
        }
    }
#else
    tbat_charge_L = master_charge;
    tbat_percent_L = master_bat;
#endif

#if 0//TCFG_CHARGESTORE_ENABLE
    u8 tpercent = 0;
    u8 tcharge = 0;
    if (chargestore_get_earphone_pos() == 'L') {
        //switch position
        log_info("is L pos\n");
        tpercent = tbat_percent_R;
        tcharge = tbat_charge_R;
        tbat_percent_R = tbat_percent_L;
        tbat_charge_R = tbat_charge_L;
        tbat_percent_L = tpercent;
        tbat_charge_L = tcharge;
    } else {
        log_info("is R pos\n");
    }
#endif

    if ((!!__this->bat_charge_L) || (!!__this->bat_charge_R) || (!!__this->bat_charge_C)) {
        if (((__this->bat_percent_C + 1) & 0x7F) <= 0x2) {
            __this->bat_percent_C = 0x2;
        }
    }
// check if change
    u8 tchange = 0;
    if ((tbat_charge_L  != __this->bat_charge_L)
        || (tbat_charge_R  != __this->bat_charge_R)
        || (tbat_charge_C  != __this->bat_charge_C)
        || (tbat_percent_L != __this->bat_percent_L)
        || (tbat_percent_R != __this->bat_percent_R)
        || (tbat_percent_C != __this->bat_percent_C)) {
        tchange = 1;
    }

    __this->bat_charge_L  = tbat_charge_L;
    __this->bat_charge_R  = tbat_charge_R;
    __this->bat_charge_C  = tbat_charge_C;
    __this->bat_percent_L = tbat_percent_L;
    __this->bat_percent_R = tbat_percent_R;
    __this->bat_percent_C = tbat_percent_C;

    /* u8 *buf = adv_data; */
    /* buf[16] = __this->bat_percent_L ? (((!!__this->bat_charge_L) << 7) | (__this->bat_percent_L & 0x7F)):0; */
    /* buf[17] = __this->bat_percent_R ? (((!!__this->bat_charge_R) << 7) | (__this->bat_percent_R & 0x7F)):0; */
    /* buf[18] = __this->bat_percent_C ? (((!!__this->bat_charge_C) << 7) | (__this->bat_percent_C & 0x7F)):0; */

    return tchange;
}


static u8 update_dev_connect_flag(void)
{
    u8 old_flag = __this->connect_flag;


    extern u8 get_bt_connect_status(void);
    if (get_bt_connect_status() ==  BT_STATUS_WAITINT_CONN) {
        __this->connect_flag = 1;
    } else {
        __this->connect_flag = 2;
    }

    if (old_flag != __this->connect_flag) {
        return 1;
    }
    return 0;
}

static void bt_ble_rcsp_adv_enable_do(void)
{
#if(TCFG_CHARGE_BOX_ENABLE)
    extern u8 get_tws_ear_status(void);
    if (!get_tws_ear_status()) {
        return;
    }
#endif

#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        if (!(get_rcsp_connect_status() && RCSP_SPP == bt_3th_get_cur_bt_channel_sel())) {
            return;
        }
    }
#endif


    ble_state_e cur_state =  smartbox_get_ble_work_state();
    // battery
    if (update_dev_battery_level()) {
        __this->modify_flag = 1;
    }

    // connect flag
    /* if (update_dev_connect_flag()) { */
    /* __this->modify_flag = 1; */
    /* } */

    extern u8 adv_info_notify(u8 * buf, u8 len);
    extern u8 JL_rcsp_get_auth_flag(void);
#if (!MUTIl_CHARGING_BOX_EN) && (RCSP_ADV_EN) && (TCFG_USER_TWS_ENABLE)
    if ((smartbox_get_con_handle() || get_rcsp_connect_status()) && JL_rcsp_get_auth_flag()) {
        /* extern void deal_adv_setting_gain_time_stamp(void); */
        /* deal_adv_setting_gain_time_stamp(); */
    }
#endif
    if ((smartbox_get_con_handle() || get_rcsp_connect_status()) && JL_rcsp_get_auth_flag() && (__this->ble_adv_notify || __this->modify_flag)) {
        printf("adv con_handle %d\n", smartbox_get_con_handle());
        /* u8 data[5]; */
        /* data[0] = __this->seq_rand; */
        /* data[1] = __this->connect_flag; */
        /* data[2] = __this->bat_percent_L ? (((!!__this->bat_charge_L) << 7) | (__this->bat_percent_L & 0x7F)):0; */
        /* data[3] = __this->bat_percent_R ? (((!!__this->bat_charge_R) << 7) | (__this->bat_percent_R & 0x7F)):0; */
        /* data[4] = __this->bat_percent_C ? (((!!__this->bat_charge_C) << 7) | (__this->bat_percent_C & 0x7F)):0; */
        /* adv_info_notify(data, 5); */
        u8 data[18];
        update_adv_data(data);
        adv_info_notify(data, 18);
        __this->modify_flag = 0;
        return;
    }

#if (!MUTIl_CHARGING_BOX_EN)
    if (cur_state != BLE_ST_ADV) {
        return;
    }
#endif

    if (__this->modify_flag == 0) {
        return;
    }

    printf("adv modify!!!!!!\n");
    bt_ble_adv_enable(0);
    smartbox_make_set_adv_data();
    smartbox_make_set_rsp_data();
    bt_ble_adv_enable(1);
}

void bt_ble_rcsp_adv_enable(void)
{
#if (MUTIl_CHARGING_BOX_EN)
#if (TCFG_CHARGE_BOX_ENABLE)
    extern u8 get_chgbox_lid_status(void);
    if (0 == get_chgbox_lid_status()) {
        return;
    }
#endif
    __this->modify_flag = 1;
    bt_ble_rcsp_adv_enable_do();
#endif
    ble_timer_handle = sys_timer_add(NULL, bt_ble_rcsp_adv_enable_do, BLE_TIMER_SET);
}


void bt_ble_rcsp_adv_disable(void)
{
    if (ble_timer_handle) {
        sys_timer_del(ble_timer_handle);
        ble_timer_handle = 0;
    }
    ble_app_disconnect();
    smartbox_set_adv_enable(0, 0);
    bt_adv_seq_change();
}

// 同步seq到对端
#define TWS_FUNC_ID_SEQ_RAND_SYNC	(('S' << (3 * 8)) | ('E' << (2 * 8)) | ('Q' << (1 * 8)) | ('\0'))
void tws_conn_switch_role();
static void deal_sibling_seq_rand_sync(void *data, u16 len, bool rx)
{
    put_buf(data, len);
    if (rx) {
        switch (((u8 *)data)[0]) {
        case TWS_ADV_SEQ_CHANGE:
            __this->seq_rand = ((u8 *) data)[1];
            JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_SEQ, NULL, 0);
            break;
        case TWS_VERSON_INFO:
            printf("get sibling version:0x%x\n", ((u8 *)data)[1] | ((u8 *)data)[2] << 8);
            sibling_ver_info  = ((u8 *)data)[1] | ((u8 *)data)[2] << 8;
            u16 cur_ver_info  = get_vid_pid_ver_from_cfg_file(GET_VER_FROM_EX_CFG);
            if ((cur_ver_info > sibling_ver_info) && (tws_api_get_role() == TWS_ROLE_MASTER)) { //如果主机版本号比从机高需要进行role_switch
                //tws_conn_switch_role();
            }
            break;

        }
    }
}
REGISTER_TWS_FUNC_STUB(adv_seq_rand_sync) = {
    .func_id = TWS_FUNC_ID_SEQ_RAND_SYNC,
    .func    = deal_sibling_seq_rand_sync,
};
void adv_seq_vaule_sync(void)
{
    syscfg_write(ADV_SEQ_RAND, &__this->seq_rand, sizeof(__this->seq_rand));
}
void bt_adv_seq_change(void)
{
    u8 trand;
    u8 data[2];
    syscfg_read(ADV_SEQ_RAND, &trand, 1);
    log_info("adv seq read: %x\n", trand);
    trand++;
    syscfg_write(ADV_SEQ_RAND, &trand, 1);
    log_info("adv seq write: %x\n", trand);
    __this->seq_rand = trand;
    data[0] = TWS_ADV_SEQ_CHANGE;
    data[1] = __this->seq_rand;
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status() && tws_api_get_role() == TWS_ROLE_MASTER) {
        tws_api_send_data_to_sibling(data, sizeof(data), TWS_FUNC_ID_SEQ_RAND_SYNC);
    }
#endif
}

u8 get_ble_adv_notify(void)
{
    return __this->ble_adv_notify;
}

void set_ble_adv_notify(u8 en)
{

    __this->ble_adv_notify = en;
}

u8 get_connect_flag(void)
{
    return __this->connect_flag;
}

void set_connect_flag(u8 value)
{
    __this->connect_flag = 	value;
}

void bt_ble_init_do(void)
{
    log_info("bt_ble_test_tag_do\n");
    //初始化三个电量值
    /* swapX(bt_get_mac_addr(), &user_tag_data[7], 6); */

    __this->connect_flag = 0;
    __this->bat_charge_L = 1;
    __this->bat_charge_R = 1;
    __this->bat_charge_C = 1;
    __this->bat_percent_L = 100;
    __this->bat_percent_R = 100;
    __this->bat_percent_C = 100;
    __this->modify_flag = 0;
    __this->ble_adv_notify = 0;
    update_dev_battery_level();
    bt_adv_seq_change();
    //update_dev_connect_flag();
}

int bt_ble_adv_ioctl(u32 cmd, u32 priv, u8 mode)
{
    printf(" bt_ble_adv_ioctl %d %d %d\n", cmd, priv, mode);
    ble_state_e cur_state =  smartbox_get_ble_work_state();
    if (mode) {			// set
        switch (cmd) {
        case BT_ADV_ENABLE:
            break;
        case BT_ADV_DISABLE:
            if (cur_state == BLE_ST_ADV) {
                printf("ADV DISABLE !!!\n");
                bt_ble_adv_enable(0);
                return 0;
            }
            __this->connect_flag = SECNE_DISMISS;
            break;
        case BT_ADV_SET_EDR_CON_FLAG:
            __this->connect_flag = priv;
            if (priv == SECNE_UNCONNECTED) {
                bt_adv_seq_change();
            }
#if (RCSP_ADV_MUSIC_INFO_ENABLE)
            extern void rcsp_adv_music_info_set_state(u8 state, u32 time);
            if (priv == SECNE_UNCONNECTED) {
                rcsp_adv_music_info_set_state(0, 1);
            } else if (priv == SECNE_CONNECTED) {
                rcsp_adv_music_info_set_state(1, 1000);
            }
#endif
            break;
        case BT_ADV_SET_BAT_CHARGE_L:
            __this->bat_charge_L = priv;
            break;
        case BT_ADV_SET_BAT_CHARGE_R:
            __this->bat_charge_R = priv;
            break;
        case BT_ADV_SET_BAT_CHARGE_C:
            __this->bat_charge_C = priv;
            break;
        case BT_ADV_SET_BAT_PERCENT_L:
            __this->bat_percent_L = priv;
            break;
        case BT_ADV_SET_BAT_PERCENT_R:
            __this->bat_percent_R = priv;
            break;
        case BT_ADV_SET_BAT_PERCENT_C:
            __this->bat_percent_C = priv;
            break;
        case BT_ADV_SET_NOTIFY_EN:
            __this->ble_adv_notify = priv;
            break;
        default:
            return -1;
        }

        __this->modify_flag = 1;
    } else {			// get
        switch (cmd) {
        case BT_ADV_ENABLE:
        case BT_ADV_DISABLE:
            break;
        case BT_ADV_SET_EDR_CON_FLAG:
            break;
        case BT_ADV_SET_BAT_CHARGE_L:
            break;
        case BT_ADV_SET_BAT_CHARGE_R:
            break;
        case BT_ADV_SET_BAT_CHARGE_C:
            break;
        case BT_ADV_SET_BAT_PERCENT_L:
            break;
        case BT_ADV_SET_BAT_PERCENT_R:
            break;
        case BT_ADV_SET_BAT_PERCENT_C:
            break;
        default:
            break;
        }
    }
    return 0;
}





void bt_adv_get_bat(u8 *buf)
{
    buf[0] = __this->bat_percent_L ? (((!!__this->bat_charge_L) << 7) | (__this->bat_percent_L & 0x7F)) : 0;
    buf[1] = __this->bat_percent_R ? (((!!__this->bat_charge_R) << 7) | (__this->bat_percent_R & 0x7F)) : 0;
    buf[2] = __this->bat_percent_C ? (((!!__this->bat_charge_C) << 7) | (__this->bat_percent_C & 0x7F)) : 0;
}

extern u8 jl_app_init_setting(void);
u8 btstck_app_init_setting(void)
{
    jl_app_init_setting();
    return 1;
}

#define TWS_FUNC_ID_ADV_STATE_INFO_SYNC	(('A' << (3 * 8)) | ('S' << (2 * 8)) | ('S' << (1 * 8)) | ('Y'))
static void deal_sibling_adv_state_info_sync(void *data, u16 len, bool rx)
{
    if (rx) {
        JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDAET_ADV_STATE_INFO, data, len);
    }
}
REGISTER_TWS_FUNC_STUB(adv_state_sync) = {
    .func_id = TWS_FUNC_ID_ADV_STATE_INFO_SYNC,
    .func    = deal_sibling_adv_state_info_sync,
};
static void adv_state_info_sync(void)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        u8 param[2] = {0};
        param[0] = JL_rcsp_get_auth_flag();
        param[1] = __this->ble_adv_notify;
        tws_api_send_data_to_sibling(param, sizeof(param), TWS_FUNC_ID_ADV_STATE_INFO_SYNC);
    }
#endif
}

// type : 0 - 正常关机检查		// 1 - 低电关机检测
u8 adv_tws_both_in_charge_box(u8 type)
{
    if (0 == ((!!__this->bat_percent_L) & (!!__this->bat_percent_R))) {
        return 0;
    }
    static u8 ret = 0;
    if (ret) {
        goto check_changes;
    }

    if (type) {
#if TCFG_USER_TWS_ENABLE
        if (TWS_ROLE_MASTER == tws_api_get_role()) {
            ret = 1;
        }
#endif
        return ret;
    }

    ret = ((!!__this->bat_charge_L) ^ (!!__this->bat_charge_R));
check_changes:
    if (smartbox_get_con_handle()) {
        return ret;
    }
    if (get_rcsp_connect_status() && ret) {
        adv_state_info_sync();
    }
    return 0;
}

// 切换后触发
void adv_role_switch_handle(void)
{
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_tws_state()) {
        if (RCSP_BLE == bt_3th_get_cur_bt_channel_sel()) {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                ble_module_enable(1);
            } else {
                u8 adv_cmd = 0x3;
                extern u8 adv_info_device_request(u8 * buf, u16 len);
                adv_info_device_request(&adv_cmd, sizeof(adv_cmd));
                ble_module_enable(0);
            }
        }
    }
#endif
}

void send_version_to_sibling(void)
{
    u8 data[3] = {0};
    u16 ver = 0;
    ver =  get_vid_pid_ver_from_cfg_file(GET_VER_FROM_EX_CFG);
    printf("%s---verson:%x\n", ver);
    data[0] = TWS_VERSON_INFO;
    data[1] = ver;
    data[2] = ver >> 8;
    tws_api_send_data_to_sibling(data, sizeof(data), TWS_FUNC_ID_SEQ_RAND_SYNC);
}
#endif

