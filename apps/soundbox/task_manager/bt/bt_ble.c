#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_task.h"

#include "btstack/btstack_task.h"
#include "user_cfg.h"
#include "vm.h"
#include "app_power_manage.h"
#include "btcontroller_modules.h"
#include "app_chargestore.h"
#include "btstack/avctp_user.h"
#include "bt_common.h"
#include "le_common.h"


#include "bt/bt_ble.h"

#if TCFG_APP_BT_EN

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)

#define LOG_TAG_CONST       BT_BLE
#define LOG_TAG             "[BT_BLE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if 0
#define ble_puts     printf
#define ble_printf   printf
#define ble_put_buf  printf_buf

#else

#define ble_puts      log_info
#define ble_printf    log_info
#define ble_put_buf   log_debug_hexdump

#endif

#define  BLE_TIMER_SET              (100)

//停留时间，可以自定义修改
#define ADV_ST_CONN_HOLD_TIME            (20000/BLE_TIMER_SET)//phone connect device
#define ADV_ST_RECONN_HOLD_TIME          (20000/BLE_TIMER_SET)//device connect phone

//以下时间不建议修改
#define ADV_ST_DISMISS_HOLD_TIME         (2000/BLE_TIMER_SET)
#define ADV_ST_PRE1_HOLD_TIME            (200/BLE_TIMER_SET)
#define ADV_ST_FAST_DISMISS_HOLD_TIME    (1500/BLE_TIMER_SET)//to fast display reconnect

#define PHONE_ICON_ONE_BETA             0 //beta case,one ear case

//0---default,inquiry,connect,reconnect
//1---only inquiry and first connect
#define PHONE_ICON_CTRL_MODE            0 //


#define PHONE_ICON_AUTO_DISMISS         1 //auto dismiss icon

#if TCFG_CHARGESTORE_ENABLE
#define REFLASH_ICON_STATE              1 //auto reflash
#else
#define REFLASH_ICON_STATE              0 //not auto reflash,fixed
#endif

static u32 ble_timer_handle = 0;
static u16 adv_st_count = 0;
static u8  dev_battery_level[3];//master,slave,box
static volatile u8 adv_control_enable;//master
static volatile u8 adv_st_mode;
static volatile u8 adv_st_next_mode;
static volatile u8 adv_enable = 0;
static u8 adv_data[ADV_RSP_PACKET_MAX];
static u8 adv_data_len = 0;
static u8 rsp_data[ADV_RSP_PACKET_MAX];
static u8 rsp_data_len = 0;
static u8 adv_rand;
static u8 adv_open_lock;//lock open 标记

//-------------------user_tag data start----------------------------------------------------------
static u8 user_tag_data[19] = {
    0xd6, 0x05,
    0x01, 0x00,
    0x01, 0x00,
    0x00, //6
};


static const u8 user_pre1_tag_data[29] = {
    0xd6, 0x05,
    0x02, 0x00,
    0x02, 0x00,
    0x00, //6
};

static const u8 user_pre2_tag_data[29] = {
    0xd6, 0x05,
    0x03, 0x00,
    0x03, 0x00,
    0x00, //6
};

static const u8 user_conn_tag_data[29] = {
    0xd6, 0x05,
    0x04, 0x00,
    0x04, 0x00,
    0x00, //6
};

static const u8 user_reconn_tag_data[29] = {
    0xd6, 0x05,
    0x05, 0x00,
    0x05, 0x00,
    0x00, //6
};

//-----------------------data end------------------------------------------------------

extern const char *bt_get_local_name();
static void adv_rand_init(void);
static u8 update_dev_battery_level(void);

//-------------------------------------------------------

const char ble_name_string[] = "693_ble";
static int make_set_adv_data(u8 *info, u8 len)
{
    u8 offset = 0;
    u8 *buf = (void *)adv_data;
    u8 *tag_buf = (void *)info;
    u8 tag_len = len;
    /*
    #if 0 //BLE广播使用EDR的蓝牙名字，防止连接时蓝牙名字变成693_ble蓝牙名字最多只能31-21=10字节。
        u8 *edr_name = bt_get_local_name();
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)edr_name, strlen(edr_name));
    #else
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)ble_name_string, strlen(ble_name_string));
    #endif
    */

    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, (void *)tag_buf, tag_len);

    if (offset > ADV_RSP_PACKET_MAX) {
        ble_puts("***adv_data overflow!!!!!!\n");
        return -1;
    }
    ble_printf("ADV data(%d):", offset);
    ble_put_buf(buf, offset);
    adv_data_len = offset;
    return 0;
}

static int make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = (void *)rsp_data;

    if (offset > ADV_RSP_PACKET_MAX) {
        ble_puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    ble_printf("RSP data(%d):,offset");
    ble_put_buf(buf, offset);
    rsp_data_len = offset;
    return 0;
}

static void adv_setup_init(void)
{
    u8 adv_type = 3;
    u8 adv_channel  = 0x07;
    u16 adv_interval;

    switch (adv_st_mode) {
    case ADV_ST_INQUIRY:
    case ADV_ST_DISMISS:
    case ADV_ST_FAST_DISMISS:
        //特殊状态,广播周期快
        adv_interval = 0x30;
        break;

    case ADV_ST_CONN:
    case ADV_ST_RECONN:
    default:
        if (get_total_connect_dev()) {
            //连上手机，广播周期慢
            adv_interval = 0x60;
        } else {
            //未连上手机，广播周期快
            adv_interval = 0x30;
        }
        break;
    }

    ble_printf("adv_interval=%d\n", adv_interval);

    ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, adv_interval, adv_type, adv_channel);

    ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, adv_data_len, adv_data);
    ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, rsp_data_len, rsp_data);
}

u8 bt_ble_get_adv_enable(void)
{
    return adv_enable;
}

void bt_ble_adv_enable(u8 enable)
{
    if (adv_enable == enable) {
        return;
    }
    adv_enable = enable;

    ble_printf("***** ble_adv_en:%d ******\n", enable);
    if (enable) {
        adv_setup_init();
    }
    ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, enable);
}

//-----------------------
extern u8 get_charge_online_flag(void);
static u8 get_device_bat_percent(void)
{
#if CONFIG_DISPLAY_DETAIL_BAT
    u8 val = get_vbat_percent();
#else
    u8 val = get_self_battery_level() * 10 + 10;
#endif

    if (val > 100) {
        val = 100;
    }
    val = (val | (get_charge_online_flag() << 7));
    /* printf("[%d]",val); */
    return val;
}

static void reflash_adv_tag_data(const u8 *tag_data, u8 len, u8 spec_val)
{
    u8 tmp_tag_data[32];
    u8 tmp8;

    memcpy(tmp_tag_data, tag_data, len);
    memcpy(&tmp_tag_data[14], dev_battery_level, 3);

#if TCFG_CHARGESTORE_ENABLE
    //位置14,15,16 对应 R,L,BOX
    if (chargestore_get_earphone_pos() == 'L') {
        //switch position
        ble_puts("is L pos\n");
        tmp8 = tmp_tag_data[14];
        tmp_tag_data[14] = tmp_tag_data[15];
        tmp_tag_data[15] = tmp8;
    } else {
        ble_puts("is R pos\n");
    }
#endif

    if (spec_val != 0xff) {
        tmp_tag_data[17] = spec_val;
        if (spec_val != 9) {
#if	TCFG_USER_TWS_ENABLE
            /* memset(&tmp_tag_data[14],0xff,3);	 */
#endif
        } else {
            memset(&tmp_tag_data[14], 0xFF, 3);//关闭广播的时候不显示电量
        }
    }

    //---
#if (!REFLASH_ICON_STATE) //
    if (len == 29) {
#if	TCFG_USER_TWS_ENABLE
        //fixed do,start,固定分开显示电量,如不分开，可以屏蔽这段代码
        ble_puts("fixed two do\n");
        /* ble_put_buf(&tmp_tag_data[14],3);  */
        if ((tmp_tag_data[14] != 0xff)
            && (tmp_tag_data[15] != 0xff)) {
            if ((tmp_tag_data[14]&BIT(7))
                && (tmp_tag_data[15]&BIT(7))) {
                tmp_tag_data[15] &= 0x7f;
                ble_puts("fixed c1\n");
            } else {
                if (((tmp_tag_data[14]&BIT(7)) == 0)
                    && ((tmp_tag_data[15]&BIT(7)) == 0)) {
                    tmp_tag_data[14] |= BIT(7);
                    ble_puts("fixed c2\n");
                }

            }

        }
        //fixed do,end
#endif
    }
#endif
    //---

    bt_ble_adv_enable(0);
    make_set_adv_data(tmp_tag_data, len);
    bt_ble_adv_enable(1);
}

static bool check_adv_open_lock(void)
{
    if (get_total_connect_dev()) {
        //已连上手机
        adv_open_lock = 1;
    }
    return adv_open_lock;
}

static void change_adv_st_mode(u8 mode)
{
    if (mode == adv_st_mode) {
        return;
    }

    ble_printf("ble_adv_st:old= %d,new= %d \n", adv_st_mode, mode);

    adv_st_mode = mode;
    adv_st_count = 0;

    switch (mode) {
    case ADV_ST_CONN:
        check_adv_open_lock();
        reflash_adv_tag_data(user_conn_tag_data, sizeof(user_conn_tag_data), 0xff);
        break;

    case ADV_ST_RECONN:
        check_adv_open_lock();
        reflash_adv_tag_data(user_reconn_tag_data, sizeof(user_reconn_tag_data), 0xff);
        break;

    case ADV_ST_DISMISS:
        reflash_adv_tag_data(user_tag_data, sizeof(user_tag_data), 0x9);
        break;

    case ADV_ST_INQUIRY:
        reflash_adv_tag_data(user_tag_data, sizeof(user_tag_data), adv_rand);
        break;

    case ADV_ST_PRE1:
        reflash_adv_tag_data(user_pre1_tag_data, sizeof(user_pre1_tag_data), 0xff);
        break;

    case ADV_ST_PRE2:
        reflash_adv_tag_data(user_pre2_tag_data, sizeof(user_pre2_tag_data), 0xff);
        break;

    case ADV_ST_FAST_DISMISS:
        reflash_adv_tag_data(user_tag_data, sizeof(user_tag_data), 0x9);
        break;

    case ADV_ST_END:
        bt_ble_adv_enable(0);
        break;

    default:
        break;
    }

}

static void check_dev_pos_changed(u8 cur_adv_mode)
{
#if REFLASH_ICON_STATE
    if (update_dev_battery_level()) {
        ble_puts("---reflash_dev_state\n");
        /* adv_st_next_mode = cur_adv_mode; */
        /* change_adv_st_mode(ADV_ST_FAST_DISMISS); */

        change_adv_st_mode(ADV_ST_END);
        change_adv_st_mode(cur_adv_mode);
    }
#endif
}

static void bt_ble_timer_handler(void)
{
    u8 bat_percent;

    if (!adv_enable) {
        return;
    }

    adv_st_count++;
    switch (adv_st_mode) {
    case ADV_ST_PRE1:
        if (adv_st_count > ADV_ST_PRE1_HOLD_TIME) {
            change_adv_st_mode(ADV_ST_PRE2);
        }
        break;

    case ADV_ST_CONN:
        if (adv_st_count > ADV_ST_CONN_HOLD_TIME) {
#if PHONE_ICON_AUTO_DISMISS
            change_adv_st_mode(ADV_ST_DISMISS);
#else
            change_adv_st_mode(ADV_ST_END);
#endif
        } else {
            check_dev_pos_changed(adv_st_mode);
        }
        break;

    case ADV_ST_RECONN:
        if (adv_st_count > ADV_ST_RECONN_HOLD_TIME) {
#if PHONE_ICON_AUTO_DISMISS
            change_adv_st_mode(ADV_ST_DISMISS);
#else
            change_adv_st_mode(ADV_ST_END);
#endif
        } else {
            check_dev_pos_changed(adv_st_mode);
        }
        break;

    case ADV_ST_DISMISS:
        if (adv_st_count > ADV_ST_DISMISS_HOLD_TIME) {
            change_adv_st_mode(ADV_ST_END);
        }
        break;

    case ADV_ST_FAST_DISMISS:
        if (adv_st_count > ADV_ST_FAST_DISMISS_HOLD_TIME) {
            change_adv_st_mode(adv_st_next_mode);
        }
        break;

    case ADV_ST_INQUIRY:
        check_dev_pos_changed(adv_st_mode);
        break;

    case ADV_ST_NULL:
    case ADV_ST_PRE2:
        break;

    default:
        break;
    }

}

static void bt_ble_timer_init(void)
{
    ble_timer_handle = sys_timer_add(NULL, bt_ble_timer_handler, BLE_TIMER_SET);
}


extern const u8 *bt_get_mac_addr();
extern const u8 *ble_get_mac_addr(void);
extern void swapX(const uint8_t *src, uint8_t *dst, int len);
void bt_ble_test_tag_do(void)
{
    ble_puts("bt_ble_test_tag_do\n");
    //初始化三个电量值
    memset(dev_battery_level, 0xff, sizeof(dev_battery_level));
    dev_battery_level[0] = get_device_bat_percent();
    swapX(bt_get_mac_addr(), &user_tag_data[7], 6);
    ble_puts("\n------master address------\n");
    ble_put_buf(&user_tag_data[7], 6);
    /* make_set_adv_data(user_tag_data, sizeof(user_tag_data)); */
}

void bt_ble_icon_set_comm_address(u8 *addr)
{
    swapX(addr, &user_tag_data[7], 6);
    le_controller_set_mac(addr);//将ble广播地址改成公共地址

    ble_puts("\n------reset master address------\n");
    ble_put_buf(&user_tag_data[7], 6);
}

void bt_ble_icon_reset(void)
{
    ble_puts("bt_ble_icon_reset\n");
    if (!adv_control_enable) {
        return;
    }
    //重置状态，允许再广播
    ble_puts("reset_do\n");
    adv_rand_init();
    adv_st_mode = ADV_ST_NULL;
    adv_open_lock = 0;
    bt_ble_adv_enable(0);
}

u8 bt_ble_icon_get_adv_state(void)
{
    return adv_st_mode;
}

//return change_flag
static u8 update_dev_battery_level(void)
{

    u8 old_dev_battery[3];

    memcpy(old_dev_battery, dev_battery_level, 3);

    //read bat_level
    dev_battery_level[0] = get_device_bat_percent();

    u8 sibling_bat_level = dev_battery_level[1];
    if (sibling_bat_level == 0xff) {
        /* ble_puts("--update_bat_00\n"); */
        goto update_dev2;
    }


#if	TCFG_USER_TWS_ENABLE
    sibling_bat_level = get_tws_sibling_bat_persent();
#endif

#if TCFG_CHARGESTORE_ENABLE
    if (sibling_bat_level == 0xff) {
        /* ble_puts("--update_bat_01\n"); */
        sibling_bat_level  = chargestore_get_sibling_power_level();
    }
#endif

    if (sibling_bat_level == 0xff) {
        /* ble_puts("--update_bat_02\n"); */
        dev_battery_level[1] = dev_battery_level[0];
    } else {
        dev_battery_level[1] = sibling_bat_level;
    }

update_dev2:
    if (((dev_battery_level[0] & BIT(7)) && (dev_battery_level[0] != 0xff))
        || ((dev_battery_level[1] & BIT(7)) && (dev_battery_level[1] != 0xff))) {
        //earphone in charge
#if TCFG_CHARGESTORE_ENABLE
        dev_battery_level[2] = chargestore_get_power_level();
#else
        dev_battery_level[2] = 0xff;
#endif
    } else {
        //all not in charge
        dev_battery_level[2] = 0xff;
    }

    u8 i;
    u8 change_flag = 0;

    for (i = 0; i < 3; i++) {
        if ((dev_battery_level[i]&BIT(7)) != (old_dev_battery[i]&BIT(7))) {
            break;
        }

        if ((dev_battery_level[i] == 0xff) && (old_dev_battery[i] != 0xff)) {
            break;
        }

        if ((dev_battery_level[i] != 0xff) && (old_dev_battery[i] == 0xff)) {
            break;
        }

    }

    if (i < 3) {
        ble_puts("dev_pos_state have changed:");
        ble_put_buf(old_dev_battery, 3);
        ble_put_buf(dev_battery_level, 3);
        change_flag = 1;
    }
    return change_flag;
}



//open 不同类型广播
void bt_ble_icon_open(u8 type)
{
    ble_printf("bt_ble_icon_open: %d\n", type);

    if (!adv_control_enable) {
        return;
    }

    if (adv_open_lock) {
        ble_puts("open_lock\n");
        return;
    }

    ble_puts("open_do\n");

    update_dev_battery_level();

    switch (adv_st_mode) {
    case ADV_ST_NULL:

#if PHONE_ICON_ONE_BETA
        if (type == ICON_TYPE_INQUIRY) {
            change_adv_st_mode(ADV_ST_INQUIRY);
        } else if (type == ICON_TYPE_CONNECTED || type == ICON_TYPE_RECONNECT) {
            //record,and not display
            change_adv_st_mode(ADV_ST_END);
        }
        break;
#endif

#if (PHONE_ICON_CTRL_MODE == 1)
        if (type == ICON_TYPE_INQUIRY) {
            change_adv_st_mode(ADV_ST_INQUIRY);
        } else if (type == ICON_TYPE_CONNECTED || type == ICON_TYPE_RECONNECT) {
            //record,and not display
            change_adv_st_mode(ADV_ST_END);
        }
        break;
#endif


        if (type == ICON_TYPE_CONNECTED || type == ICON_TYPE_RECONNECT) {
            change_adv_st_mode(ADV_ST_RECONN);
        } else if (type == ICON_TYPE_PRE_DEV) {
            change_adv_st_mode(ADV_ST_PRE1);
        } else {
            change_adv_st_mode(ADV_ST_INQUIRY);
        }
        break;

    case ADV_ST_INQUIRY:

#if PHONE_ICON_ONE_BETA
        if (type == ICON_TYPE_CONNECTED || type == ICON_TYPE_RECONNECT) {
            change_adv_st_mode(ADV_ST_DISMISS);
        }
        break;
#endif

#if (PHONE_ICON_CTRL_MODE == 1)
        if (type == ICON_TYPE_CONNECTED || type == ICON_TYPE_RECONNECT) {
            change_adv_st_mode(ADV_ST_DISMISS);
        }
        break;
#endif


        switch (type) {
        case ICON_TYPE_CONNECTED:
#if	TCFG_USER_TWS_ENABLE
            change_adv_st_mode(ADV_ST_CONN);
#else
            change_adv_st_mode(ADV_ST_DISMISS);
#endif
            break;

        case ICON_TYPE_RECONNECT:
            change_adv_st_mode(ADV_ST_FAST_DISMISS);
            adv_st_next_mode = ADV_ST_RECONN;
            break;

        default:
            break;
        }
        break;

    case ADV_ST_PRE1:
    case ADV_ST_PRE2:
        if (type == ICON_TYPE_INQUIRY) {
            //开inquiry
            change_adv_st_mode(ADV_ST_INQUIRY);
        } else if (type == ICON_TYPE_CONNECTED) {
            change_adv_st_mode(ADV_ST_CONN);
        } else {
            ;
        }
        break;


    case ADV_ST_RECONN:
        if (type == ICON_TYPE_INQUIRY) {
            change_adv_st_mode(ADV_ST_FAST_DISMISS);
            adv_st_next_mode = ADV_ST_INQUIRY;
        } else if (type == ICON_TYPE_CONNECTED) {
            change_adv_st_mode(ADV_ST_FAST_DISMISS);
            adv_st_next_mode = ADV_ST_CONN;
        } else {
            ;
        }
        break;

    default:
        break;
    }
}

//dismiss_flag
void bt_ble_icon_close(u8 dismiss_flag)
{
    ble_printf("bt_ble_close: %d\n", dismiss_flag);

    if ((!dismiss_flag) || (!adv_control_enable)) {
        ble_puts("close_adv\n");
        change_adv_st_mode(ADV_ST_END);
        return;
    }

    switch (adv_st_mode) {
    case ADV_ST_CONN:
    case ADV_ST_RECONN:
    case ADV_ST_INQUIRY:
    case ADV_ST_PRE1:
    case ADV_ST_PRE2:
    case ADV_ST_FAST_DISMISS:
    case ADV_ST_DISMISS:
    case ADV_ST_END://也要做
        change_adv_st_mode(ADV_ST_DISMISS);
        break;

    default:
        break;
    }
}

//tws,need set en
extern bool get_tws_sibling_connect_state(void);
extern int tws_api_get_role(void);
void bt_ble_icon_slave_en(u8 en)
{
    ble_printf("------icon_slave_enable = %d------\n", en);
    if (en) {
        dev_battery_level[1] = 0xfc;//set flag
    } else {
        dev_battery_level[1] = 0xff;
    }

#if	TCFG_USER_TWS_ENABLE
    if (get_tws_sibling_connect_state()) {
        if (tws_api_get_role()) {
            //已配对连接过的从机，记录以后不允许再广播
            adv_open_lock = 1;//lock open 接口
            bt_ble_icon_close(0);
        }
    }
#endif
}


//tws,master need set en;使能ICON_OPEN 接口控制
void bt_ble_set_control_en(u8 en)
{
    ble_printf("------adv_control_enable = %d------\n", en);
    adv_control_enable = en;//set master flag
}
//-----------------------
static void adv_rand_init(void)
{
    int ret = syscfg_read(CFG_BLE_MODE_INFO, (u8 *)&adv_rand, 1);
    if (!ret) {
        adv_rand = (rand32() % 7) + 1;
    } else {
        adv_rand++;
    }

    if (adv_rand > 7) {
        adv_rand = 1;
    }
    syscfg_write(CFG_BLE_MODE_INFO, (u8 *)&adv_rand, 1);
    ble_printf("------adv_rand = %d------\n", adv_rand);

}

extern void dut_le_hw_open(void);
extern void ble_bqb_test_thread_init(void);
__BANK_INIT_ENTRY
void bt_ble_init(void)
{
    ble_printf("***** ble_init******\n");
    adv_rand_init();

#if	TCFG_USER_TWS_ENABLE
    adv_control_enable = 0;//tws,default off
#else
    adv_control_enable = 1;//one earphone,default on
#endif
    adv_st_mode = ADV_ST_NULL;
    adv_open_lock = 0;
    bt_ble_test_tag_do();
    bt_ble_timer_init();
}

/*
   void bt_ble_icon_set(u8 *info, u8 len)
   {
   ble_printf("***** ble_icon_set******\n");

   bt_ble_adv_enable(0);
   update_user_tag_data(info, len);
   bt_ble_adv_enable(1);
   }
 */

void bt_ble_exit(void)
{
    ble_printf("***** ble_exit******\n");

    bt_ble_adv_enable(0);
    adv_st_mode = ADV_ST_NULL;
    adv_control_enable = 0;
    if (ble_timer_handle) {
        sys_timeout_del(ble_timer_handle);
        ble_timer_handle = 0;;
    }
}


#endif
#endif


