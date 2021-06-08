#include "init.h"
#include "app_config.h"
#include "system/includes.h"
#include "asm/charge.h"
#include "asm/chargestore.h"
#include "user_cfg.h"
#include "app_chargestore.h"
#include "device/vm.h"
#include "btstack/avctp_user.h"
#include "app_power_manage.h"
#include "app_action.h"
#include "app_main.h"
#include "app_charge.h"
#include "classic/tws_api.h"
#include "update.h"
#include "bt_ble.h"
#include "bt_tws.h"
#include "app_action.h"
#include "bt_common.h"
#include "le_rcsp_adv_module.h"
#include "app_task.h"
#include "update_loader_download.h"

#define LOG_TAG_CONST       APP_CHARGESTORE
#define LOG_TAG             "[APP_CHARGESTORE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define CMD_TWS_CHANNEL_SET         0x01
#define CMD_TWS_REMOTE_ADDR         0x02
#define CMD_TWS_ADDR_DELETE         0x03
#define CMD_BOX_TWS_CHANNEL_SEL     0x04//测试盒获取地址
#define CMD_BOX_TWS_REMOTE_ADDR     0x05//测试盒交换地址
#define CMD_POWER_LEVEL_OPEN        0x06//开盖充电舱报告/获取电量
#define CMD_POWER_LEVEL_CLOSE       0x07//合盖充电舱报告/获取电量
#define CMD_RESTORE_SYS             0x08//恢复出厂设置
#define CMD_ENTER_DUT               0x09//进入测试模式
#define CMD_EX_FIRST_READ_INFO      0x0A//F95读取数据首包信息
#define CMD_EX_CONTINUE_READ_INFO   0x0B//F95读取数据后续包信息
#define CMD_EX_FIRST_WRITE_INFO     0x0C//F95写入数据首包信息
#define CMD_EX_CONTINUE_WRITE_INFO  0x0D//F95写入数据后续包信息
#define CMD_EX_INFO_COMPLETE        0x0E//F95完成信息交换
#define CMD_TWS_SET_CHANNEL         0x0F//F95设置左右声道信息
#define CMD_BOX_UPDATE              0x20//测试盒升级
#define CMD_BOX_MODULE              0x21//测试盒一级命令

#define CMD_SHUT_DOWN               0x80//充电舱关机,充满电关机,或者是低电关机
#define CMD_CLOSE_CID               0x81//充电舱盒盖
#define CMD_ANC_MODULE              0x90//ANC一级命令
#define CMD_UNDEFINE                0xff//未知命令回复

//testbox sub cmd
#define CMD_BOX_BT_NAME_INFO		0x01 //获取蓝牙名
#define CMD_BOX_SDK_VERSION			0x02 //获取sdk版本信息
#define CMD_BOX_BATTERY_VOL			0x03 //获取电量
#define CMD_BOX_ENTER_DUT			0x04 //进入dut模式

struct chargestore_info {
    int timer;
    int shutdown_timer;
    u8 version;
    u8 chip_type;
    u8 max_packet_size;//充电舱端一包的最大值
    u8 bt_init_ok;//蓝牙协议栈初始化成功
    u8 power_level;//本机记录的充电舱电量百分比
    u8 pre_power_lvl;
    u8 sibling_chg_lev;//对耳同步的充电舱电量
    u8 power_status;//充电舱供电状态 0:断电 5V不在线 1:升压 5V在线
    u8 cover_status;//充电舱盖子状态 0:合盖 1:开盖
    u8 testbox_status;//测试盒在线状态
    u8 connect_status;//通信成功
    u8 ear_number;//盒盖时耳机在线数
    u8 channel;//左右
    u8 tws_power;//对耳的电量
    u8 power_sync;//第一次获取到充电舱电量时,同步到对耳
    u8 pair_flag;//配对标记
    u8 close_ing;//等待合窗超时
    u8 active_disconnect;//主动断开连接
    u8 event_hdl_flag;
    u8 switch2bt;
};

extern void sys_enter_soft_poweroff(void *priv);
//extern void bt_tws_cancle_wait_pair();
extern int tws_api_cancle_wait_pair_internal();
extern int tws_cancle_create_connection_internal();
extern void tws_api_set_connect_aa_allways(u32 connect_aa);
extern u32 tws_le_get_pair_aa(void);
extern u32 tws_le_get_connect_aa(void);
extern u32 tws_le_get_search_aa(void);
extern void bt_page_scan_for_test();
extern u8 get_jl_chip_id(void);
extern u8 get_jl_chip_id2(void);
extern void bt_get_vm_mac_addr(u8 *addr);
extern bool get_tws_sibling_connect_state(void);
extern bool get_tws_phone_connect_state(void);
extern void local_irq_disable();
extern void cpu_reset();
extern u32 dual_bank_update_exist_flag_get(void);
extern u32 classic_update_task_exist_flag_get(void);
extern void chargestore_api_set_timeout(u8 timeout);


#if TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE

static struct chargestore_info info;
#define __this  (&info)
static u8 send_buf[36];
static u8 local_packet[36];
static CHARGE_STORE_INFO read_info, write_info;
static u8 read_index, write_index;

#if TCFG_CHARGESTORE_ENABLE
void chargestore_clean_tws_conn_info(u8 type)
{
    CHARGE_STORE_INFO charge_store_info;
    log_info("chargestore_clean_tws_conn_info=%d\n", type);
    if (type == TWS_DEL_TWS_ADDR) {
        log_info("TWS_DEL_TWS_ADDR\n");
    } else if (type == TWS_DEL_PHONE_ADDR) {
        log_info("TWS_DEL_PHONE_ADDR\n");
    } else if (type == TWS_DEL_ALL_ADDR) {
        log_info("TWS_DEL_ALL_ADDR\n");
    }
    memset(&charge_store_info, 0xff, sizeof(CHARGE_STORE_INFO));
    syscfg_write(CFG_TWS_REMOTE_ADDR, charge_store_info.tws_remote_addr, sizeof(charge_store_info.tws_remote_addr));
}
#endif

#if TCFG_USER_TWS_ENABLE

extern void bt_tws_remove_pairs();
bool chargestore_set_tws_remote_info(u8 *data, u8 len)
{
    u8 i;
    bool ret = true;
    u8 remote_addr[6];
    u8 common_addr[6];
    u8 local_addr[6];
    CHARGE_STORE_INFO *charge_store_info = (CHARGE_STORE_INFO *)data;
    if (len > sizeof(CHARGE_STORE_INFO)) {
        log_error("len err\n");
        return false;
    }
    //set remote addr
    syscfg_read(CFG_TWS_REMOTE_ADDR, remote_addr, sizeof(remote_addr));
    if (memcmp(remote_addr, charge_store_info->tws_local_addr, sizeof(remote_addr))) {
        ret = false;
    }
    syscfg_write(CFG_TWS_REMOTE_ADDR, charge_store_info->tws_local_addr, sizeof(charge_store_info->tws_remote_addr));

#if (CONFIG_TWS_COMMON_ADDR_SELECT != CONFIG_TWS_COMMON_ADDR_USED_LEFT)
    //set common addr
    bt_get_tws_local_addr(local_addr);
    syscfg_read(CFG_TWS_COMMON_ADDR, common_addr, sizeof(common_addr));

    for (i = 0; i < sizeof(common_addr); i++) {
        if (common_addr[i] != (u8)(local_addr[i] + charge_store_info->tws_local_addr[i])) {
            ret = false;
        }
        common_addr[i] = local_addr[i] + charge_store_info->tws_local_addr[i];
    }
    syscfg_write(CFG_TWS_COMMON_ADDR, common_addr, sizeof(common_addr));
#endif

#ifndef CONFIG_NEW_BREDR_ENABLE
    if (__this->channel == 'L') {
        if (tws_le_get_connect_aa() != tws_le_get_pair_aa()) {
            ret = false;
        }
        tws_api_set_connect_aa_allways(tws_le_get_pair_aa());
    } else {
        if (tws_le_get_connect_aa() != charge_store_info->pair_aa) {
            ret = false;
        }
        tws_api_set_connect_aa_allways(charge_store_info->pair_aa);
    }
#endif
    if (__this->testbox_status) {
        u8 cmd = CMD_BOX_TWS_REMOTE_ADDR;
        chargestore_api_write(&cmd, 1);

    }
    return ret;
}
#endif

#define WRITE_LIT_U16(a,src)   {*((u8*)(a)+1) = (u8)(src>>8); *((u8*)(a)+0) = (u8)(src&0xff); }
#define EXTEND_TWS_REMOTE_ADDR_SIZE (1 + sizeof(u16) + sizeof(u16))
extern char tws_api_get_local_channel();
extern u16 bt_get_tws_device_indicate(u8 *tws_device_indicate);
u16 chargestore_get_tws_remote_info(u8 *data)
{
    u16 ret_len = 0;
    u16 device_ind;
    u16 reserved_data = 0;
    CHARGE_STORE_INFO *charge_store_info = (CHARGE_STORE_INFO *)data;
    bt_get_tws_local_addr(charge_store_info->tws_local_addr);
    bt_get_vm_mac_addr(charge_store_info->tws_mac_addr);
#ifndef CONFIG_NEW_BREDR_ENABLE
    charge_store_info->search_aa = tws_le_get_search_aa();
    charge_store_info->pair_aa = tws_le_get_pair_aa();
#endif
    if (1 == __this->testbox_status) {
#if (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_AS_LEFT_CHANNEL) \
	 ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_AS_RIGHT_CHANNEL) \
	 ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_DOWN_AS_LEFT) \
	 ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_UP_AS_LEFT)
        data[sizeof(CHARGE_STORE_INFO)] = tws_api_get_local_channel();
#else
        data[sizeof(CHARGE_STORE_INFO)] = 'U';
#endif
        device_ind = bt_get_tws_device_indicate(NULL);
        WRITE_LIT_U16(&data[sizeof(CHARGE_STORE_INFO) + 1], device_ind);
        WRITE_LIT_U16(&data[sizeof(CHARGE_STORE_INFO) + 1 + 2], reserved_data);
        //printf("tws_ch:%c %x\n", data[sizeof(CHARGE_STORE_INFO)], device_ind);
        ret_len = sizeof(CHARGE_STORE_INFO) + EXTEND_TWS_REMOTE_ADDR_SIZE;
    } else {
        ret_len = sizeof(CHARGE_STORE_INFO);
    }

    return ret_len;
}

#if TCFG_CHARGESTORE_ENABLE
u16 chargestore_f95_read_tws_remote_info(u8 *data, u8 flag)
{
    u8 read_len;
    u8 *pbuf = (u8 *)&read_info;
    if (flag) {//first packet
        read_index = 0;
        chargestore_get_tws_remote_info((u8 *)&read_info);
    }
    read_len = sizeof(read_info) - read_index;
    read_len = (read_len > (__this->max_packet_size - 1)) ? (__this->max_packet_size - 1) : read_len;
    memcpy(data, pbuf + read_index, read_len);
    read_index += read_len;
    return read_len;
}

u16 chargestore_f95_write_tws_remote_info(u8 *data, u8 len, u8 flag)
{
    u8 write_len;
    u8 *pbuf = (u8 *)&write_info;
    if (flag) {
        write_index = 0;
        memset(&write_info, 0, sizeof(write_info));
    }
    write_len = sizeof(write_info) - write_index;
    write_len = (write_len >= len) ? len : write_len;
    memcpy(pbuf + write_index, data, write_len);
    write_index += write_len;
    return write_len;
}
#endif

void chargestore_set_tws_channel_info(void)
{
    if ((__this->channel == 'L') || (__this->channel == 'R')) {
        syscfg_write(CFG_CHARGESTORE_TWS_CHANNEL, &__this->channel, 1);
    }
}

#if TCFG_CHARGESTORE_ENABLE
void chargestore_timeout_deal(void *priv)
{
    __this->timer = 0;
    __this->close_ing = 0;
    if ((!__this->cover_status) || __this->active_disconnect) {//当前为合盖或者主动断开连接
        u8 app = app_get_curr_task();
        if (app == APP_IDLE_TASK) {
            sys_enter_soft_poweroff((void *)1);
        }
    } else {

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)

        /* if ((!get_total_connect_dev()) && (tws_api_get_role() == TWS_ROLE_MASTER) && (get_bt_tws_connect_status())) { */
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            printf("charge_icon_ctl...\n");
            bt_ble_icon_reset();
#if CONFIG_NO_DISPLAY_BUTTON_ICON
            if (get_total_connect_dev()) {
                //蓝牙未真正断开;重新广播
                bt_ble_icon_open(ICON_TYPE_RECONNECT);
            } else {
                //蓝牙未连接，重新开可见性
                bt_ble_icon_open(ICON_TYPE_INQUIRY);
            }
#else
            //不管蓝牙是否连接，默认打开
            bt_ble_icon_open(ICON_TYPE_RECONNECT);
#endif

        }
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV_RCSP)
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            printf("charge_icon_ctl...\n");
#if CONFIG_NO_DISPLAY_BUTTON_ICON
            if (get_total_connect_dev()) {
                //蓝牙未真正断开;重新广播
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTED, 1);
            } else {
                //蓝牙未连接，重新开可见性
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
            }
#else
            //不管蓝牙是否连接，默认打开
            bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTED, 1);
#endif

        }
#endif

    }
    __this->ear_number = 1;
}

void chargestore_set_phone_disconnect(void)
{
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
#if (!CONFIG_NO_DISPLAY_BUTTON_ICON)
    if (__this->pair_flag && get_tws_sibling_connect_state()) {
        //check ble inquiry
        //printf("get box log_key...con_dev=%d\n",get_tws_phone_connect_state());
        if ((bt_ble_icon_get_adv_state() == ADV_ST_RECONN)
            || (bt_ble_icon_get_adv_state() == ADV_ST_DISMISS)
            || (bt_ble_icon_get_adv_state() == ADV_ST_END)) {
            bt_ble_icon_reset();
            bt_ble_icon_open(ICON_TYPE_INQUIRY);
        }
    }
#endif
    __this->pair_flag = 0;
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV_RCSP)
#if (!CONFIG_NO_DISPLAY_BUTTON_ICON)
    if (__this->pair_flag && get_tws_sibling_connect_state()) {
        //check ble inquiry
        bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
    }
#endif
    __this->pair_flag = 0;
#endif
}

void chargestore_set_phone_connect(void)
{
    __this->active_disconnect = 0;
}

#endif

int app_chargestore_event_handler(struct chargestore_event *chargestore_dev)
{
    int ret = false;

    u8 app = app_get_curr_task();

#if defined(TCFG_CHARGE_KEEP_UPDATA) && TCFG_CHARGE_KEEP_UPDATA
    //在升级过程中,不响应智能充电舱app层消息
    if (dual_bank_update_exist_flag_get() || classic_update_task_exist_flag_get()) {
        return ret;
    }
#endif

    switch (chargestore_dev->event) {
    case CMD_RESTORE_SYS:
#if (TCFG_AUDIO_ANC_ENABLE == ANC_RUN_MODE)

#if TCFG_USER_TWS_ENABLE
        bt_tws_remove_pairs();
#endif
        user_send_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
        cpu_reset();
#endif
        break;
#if TCFG_TEST_BOX_ENABLE
    case CMD_BOX_TWS_CHANNEL_SEL:
        __this->event_hdl_flag = 0;
        chargestore_set_tws_channel_info();
#if TCFG_SYS_LVD_EN
        if (get_vbat_need_shutdown() == TRUE) {
            //电压过低,不进行测试
            break;
        }
#endif
#if TCFG_APP_BT_EN
        if (app != APP_BT_TASK) {
            if (!app_var.goto_poweroff_flag) {
                app_var.play_poweron_tone = 0;
                task_switch_to_bt();
            }
        } else {
            if ((!__this->connect_status) && __this->bt_init_ok) {
                log_info("\n\nbt_page_scan_for_test\n\n");
                __this->connect_status = 1;
#if	TCFG_USER_TWS_ENABLE
                bt_page_scan_for_test();
#endif
            }
        }
#endif
        break;
#if TCFG_USER_TWS_ENABLE
    case CMD_BOX_TWS_REMOTE_ADDR:
        log_info("event_CMD_BOX_TWS_REMOTE_ADDR \n");
        chargestore_set_tws_remote_info(chargestore_dev->packet, chargestore_dev->size);
        break;
#endif
#endif
#if TCFG_CHARGESTORE_ENABLE
    case CMD_TWS_CHANNEL_SET:
        chargestore_set_tws_channel_info();
        break;
#if TCFG_USER_TWS_ENABLE
    case CMD_TWS_REMOTE_ADDR:
        log_info("event_CMD_TWS_REMOTE_ADDR\n");
        if (chargestore_set_tws_remote_info(chargestore_dev->packet, chargestore_dev->size) == false) {
            //交换地址后,断开与手机连接,并删除所有连过的手机地址
            user_send_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
            __this->ear_number = 2;
            sys_enter_soft_poweroff((void *)1);
        } else {
            __this->pair_flag = 1;
            if (get_tws_phone_connect_state() == TRUE) {
                __this->active_disconnect = 1;
                user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
            } else {
                chargestore_set_phone_disconnect();
            }
        }
        break;
    case CMD_TWS_ADDR_DELETE:
        log_info("event_CMD_TWS_ADDR_DELETE\n");
        chargestore_clean_tws_conn_info(chargestore_dev->packet[0]);
        break;
#endif
    case CMD_POWER_LEVEL_OPEN:
        log_info("event_CMD_POWER_LEVEL_OPEN\n");

        //电压过低,不进响应开盖命令
#if TCFG_SYS_LVD_EN
        if (get_vbat_need_shutdown() == TRUE) {
            log_info(" lowpower deal!\n");
            break;
        }
#endif

        if (__this->cover_status) {//当前为开盖
            if (__this->power_sync) {
                if (chargestore_sync_chg_level() == 0) {
                    __this->power_sync = 0;
                }
            }

            if ((app == APP_IDLE_TASK) && (app_var.goto_poweroff_flag == 0)) {
                /* app_var.play_poweron_tone = 1; */
                app_var.play_poweron_tone = 0;
                power_set_mode(TCFG_LOWPOWER_POWER_SEL);
                __this->switch2bt = 1;
                task_switch_to_bt();
                __this->switch2bt = 0;
            }
        }
        break;
    case CMD_POWER_LEVEL_CLOSE:
        log_info("event_CMD_POWER_LEVEL_CLOSE\n");
        if (!__this->cover_status) {//当前为合盖
            if (app != APP_IDLE_TASK) {
                sys_enter_soft_poweroff((void *)1);
            }
        }
        break;
    case CMD_CLOSE_CID:
        log_info("event_CMD_CLOSE_CID\n");
        if (!__this->cover_status) {//当前为合盖
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
            if ((__this->bt_init_ok) && (tws_api_get_role() == TWS_ROLE_MASTER))  {
                bt_ble_icon_close(1);
            }
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV_RCSP)
            if ((__this->bt_init_ok) && (tws_api_get_role() == TWS_ROLE_MASTER))  {
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_DISMISS, 1);
            }
#endif
            if (!__this->timer) {
                __this->timer = sys_timeout_add(NULL, chargestore_timeout_deal, 2000);
                if (!__this->timer) {
                    log_error("timer alloc err!\n");
                } else {
                    __this->close_ing = 1;
                }
            } else {
                sys_timer_modify(__this->timer, 2000);
                __this->close_ing = 1;
            }
        } else {
            __this->ear_number = 1;
            __this->close_ing = 0;
        }
        break;
#endif
    default:
        break;
    }

    return ret;
}

#if TCFG_CHARGESTORE_ENABLE

u8 chargestore_get_vbat_percent(void)
{
    u8 power;
#if CONFIG_DISPLAY_DETAIL_BAT
    power = get_vbat_percent();//显示个位数的电量
#else
    power = get_self_battery_level() * 10 + 10; //显示10%~100%
#endif

#if TCFG_CHARGE_ENABLE
    if (get_charge_full_flag()) {
        power = 100;
    } else if (power == 100) {
        power = 99;
    }
    if (get_charge_online_flag()) {
        power |= BIT(7);
    }
#endif
    return power;
}

void chargestore_set_power_status(u8 *buf, u8 len)
{
    __this->version = buf[0] & 0x0f;
    __this->chip_type = (buf[0] >> 4) & 0x0f;
    //f95可能传一个大于100的电量
    if ((buf[1] & 0x7f) > 100) {
        __this->power_level = (buf[1] & 0x80) | 100;
    } else {
        __this->power_level = buf[1];
    }
    if (len > 2) {
        __this->max_packet_size = buf[2];
        if (len > 3) {
            __this->tws_power = buf[3];
        }
    }
}

void chargestore_shutdown_do(void *priv)
{
    log_info("chargestore shutdown!\n");
    power_set_soft_poweroff();
}

#endif


void chargestore_event_to_user(u8 *packet, u32 type, u8 event, u8 size)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    if (packet != NULL) {
        if (size > sizeof(local_packet)) {
            return;
        }
        e.u.chargestore.packet = local_packet;
        memcpy(e.u.chargestore.packet, packet, size);
    }
    e.arg  = (void *)type;
    e.u.chargestore.event = event;
    e.u.chargestore.size = size;
    sys_event_notify(&e);
}

extern const char *bt_get_local_name();
extern u16 get_vbat_value(void);
extern const int config_btctler_eir_version_info_len;
extern const char *sdk_version_info_get(void);
void app_chargestore_testbox_sub_cmd_handle(u8 *buf, u8 len)
{
    u8 name_len;
    u8 send_len = 0;

    send_buf[0] = buf[0];
    send_buf[1] = buf[1];

    log_info("sub_cmd:%x\n", buf[1]);

    switch (buf[1]) {
    case CMD_BOX_BT_NAME_INFO:
        name_len = strlen(bt_get_local_name());
        if (name_len < (sizeof(send_buf) - 2)) {
            memcpy(&send_buf[2], bt_get_local_name(), name_len);
            send_len = name_len + 2;
            chargestore_api_write(send_buf, send_len);
        } else {
            log_error("bt name buf len err\n");
        }
        break;

    case CMD_BOX_BATTERY_VOL:
        send_buf[2] = get_vbat_value();
        send_buf[3] = get_vbat_value() >> 8;
        send_len = sizeof(u16) + 2;
        log_info("bat_val:%x\n", get_vbat_value());
        chargestore_api_write(send_buf, send_len);
        break;

    case CMD_BOX_SDK_VERSION:
        if (config_btctler_eir_version_info_len) {
            log_info("version:%s\n", sdk_version_info_get());
            memcpy(send_buf + 2, sdk_version_info_get(), config_btctler_eir_version_info_len);
            send_len = config_btctler_eir_version_info_len + 2;
            chargestore_api_write(send_buf, send_len);
        }
        break;

    case CMD_BOX_ENTER_DUT:
        log_info("enter dut\n");
        if (!get_total_connect_dev()) { //未连接手机
            extern void bredr_set_dut_enble(u8 en, u8 phone);
            bredr_set_dut_enble(1, 1);
        }
        chargestore_api_write(send_buf, 2);
        break;

    default:
        send_buf[0] = CMD_UNDEFINE;
        send_len = 1;
        chargestore_api_write(send_buf, send_len);
        break;
    }

    log_info_hexdump(send_buf, send_len);
}

void app_chargestore_data_deal(u8 *buf, u8 len)
{
    //u8 send_buf[30];
    /* log_info_hexdump(buf, len); */
#if TCFG_CHARGESTORE_ENABLE//有通信则关机定时器删掉
    chargestore_shutdown_reset();
#endif
    send_buf[0] = buf[0];
#ifdef CONFIG_CHARGESTORE_REMAP_ENABLE
    if (remap_app_chargestore_data_deal(buf, len)) {
        return;
    }
#endif
    switch (buf[0]) {
#if TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE
#if TCFG_TEST_BOX_ENABLE
    //case CMD_ENTER_DUT:
    //	__this->testbox_status = 1;
    //log_info("enter_dut\n");
    //break;

    case CMD_BOX_MODULE:
        app_chargestore_testbox_sub_cmd_handle(buf, len);
        break;

    case CMD_BOX_UPDATE:
        if (UPDATE_MODULE_IS_SUPPORT(UPDATE_UART_EN)) {
            __this->testbox_status = 1;
            if (buf[13] == get_jl_chip_id() || buf[13] == get_jl_chip_id2()) {
                chargestore_set_update_ram();
                cpu_reset();
            } else {
                send_buf[1] = 0x01;//chip id err
                chargestore_api_write(send_buf, 2);
            }
        }
        break;
    case CMD_BOX_TWS_CHANNEL_SEL:
        __this->testbox_status = 1;
#endif
#if TCFG_CHARGESTORE_ENABLE
    case CMD_TWS_CHANNEL_SET:
#endif
        __this->channel = (buf[1] == TWS_CHANNEL_LEFT) ? 'L' : 'R';
        if (0 == __this->event_hdl_flag) {
            chargestore_event_to_user(NULL, DEVICE_EVENT_CHARGE_STORE, buf[0], 0);
            __this->event_hdl_flag = 1;
        }
#if TCFG_APP_BT_EN
        if (__this->bt_init_ok) {
            len = chargestore_get_tws_remote_info(&send_buf[1]);
            chargestore_api_write(send_buf, len + 1);
        } else {
            send_buf[0] = CMD_UNDEFINE;
            chargestore_api_write(send_buf, 1);
        }
#else
        len = chargestore_get_tws_remote_info(&send_buf[1]);
        chargestore_api_write(send_buf, len + 1);
#endif
        break;
#endif
#if TCFG_CHARGESTORE_ENABLE
    case CMD_TWS_SET_CHANNEL:
        __this->channel = (buf[1] == TWS_CHANNEL_LEFT) ? 'L' : 'R';
        log_info("f95 set channel = %c\n", __this->channel);
        chargestore_event_to_user(NULL, DEVICE_EVENT_CHARGE_STORE, CMD_TWS_CHANNEL_SET, 0);
        if (__this->bt_init_ok) {
            chargestore_api_write(send_buf, 1);
        } else {
            send_buf[0] = CMD_UNDEFINE;
            chargestore_api_write(send_buf, 1);
        }
        break;
#endif

#if TCFG_USER_TWS_ENABLE
#if TCFG_TEST_BOX_ENABLE
    case CMD_BOX_TWS_REMOTE_ADDR:
        len -= EXTEND_TWS_REMOTE_ADDR_SIZE;
        __this->testbox_status = 1;
        __this->close_ing = 0;
        chargestore_event_to_user((u8 *)&buf[1], DEVICE_EVENT_CHARGE_STORE, buf[0], len - 1);
        chargestore_api_set_timeout(100);
        break;
#endif
#if TCFG_CHARGESTORE_ENABLE
    case CMD_TWS_REMOTE_ADDR:
        __this->close_ing = 0;
        chargestore_event_to_user((u8 *)&buf[1], DEVICE_EVENT_CHARGE_STORE, buf[0], len - 1);
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_EX_FIRST_READ_INFO:
        log_info("read first!\n");
        __this->close_ing = 0;
        len = chargestore_f95_read_tws_remote_info(&send_buf[1], 1);
        chargestore_api_write(send_buf, len + 1);
        break;
    case CMD_EX_CONTINUE_READ_INFO:
        log_info("read continue!\n");
        __this->close_ing = 0;
        len = chargestore_f95_read_tws_remote_info(&send_buf[1], 0);
        chargestore_api_write(send_buf, len + 1);
        break;
    case CMD_EX_FIRST_WRITE_INFO:
        log_info("write first!\n");
        __this->close_ing = 0;
        chargestore_f95_write_tws_remote_info(&buf[1], len - 1, 1);
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_EX_CONTINUE_WRITE_INFO:
        log_info("write continue!\n");
        __this->close_ing = 0;
        chargestore_f95_write_tws_remote_info(&buf[1], len - 1, 0);
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_EX_INFO_COMPLETE:
        log_info("ex complete!\n");
        chargestore_event_to_user((u8 *)&write_info, DEVICE_EVENT_CHARGE_STORE, CMD_TWS_REMOTE_ADDR, sizeof(write_info));
        chargestore_api_write(send_buf, 1);
        break;
#endif
#endif

#if TCFG_CHARGESTORE_ENABLE
    case CMD_TWS_ADDR_DELETE:
        __this->close_ing = 0;
        chargestore_event_to_user(&buf[1], DEVICE_EVENT_CHARGE_STORE, CMD_TWS_ADDR_DELETE, len - 1);
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_POWER_LEVEL_OPEN:
        __this->power_status = 1;
        __this->cover_status = 1;
        __this->close_ing = 0;
        if (__this->power_level == 0xff) {
            __this->power_sync = 1;
        }
        chargestore_set_power_status(&buf[1], len - 1);
        if (__this->power_level != __this->pre_power_lvl) {
            __this->power_sync = 1;
        }
        __this->pre_power_lvl = __this->power_level;
        send_buf[1] = chargestore_get_vbat_percent();
        send_buf[2] = chargestore_get_det_level(__this->chip_type);
        chargestore_api_write(send_buf, 3);
        //切模式过程中不发送消息,防止堆满消息
        if (__this->switch2bt == 0) {
            chargestore_event_to_user(NULL, DEVICE_EVENT_CHARGE_STORE, CMD_POWER_LEVEL_OPEN, 0);
        }
        break;
    case CMD_POWER_LEVEL_CLOSE:
        __this->power_status = 1;
        __this->cover_status = 0;
        __this->close_ing = 0;
        chargestore_set_power_status(&buf[1], len - 1);
        send_buf[1] = chargestore_get_vbat_percent();
        send_buf[2] = chargestore_get_det_level(__this->chip_type);
        chargestore_api_write(send_buf, 3);
        chargestore_event_to_user(NULL, DEVICE_EVENT_CHARGE_STORE, CMD_POWER_LEVEL_CLOSE, 0);
        break;
    case CMD_SHUT_DOWN:
        log_info("shut down\n");
        __this->power_status = 0;
        __this->cover_status = 0;
        __this->close_ing = 0;
        chargestore_api_write(send_buf, 1);
        __this->shutdown_timer = sys_hi_timer_add(NULL, chargestore_shutdown_do, 1000);
        break;
    case CMD_CLOSE_CID:
        log_info("close cid\n");
        __this->power_status = 1;
        __this->cover_status = 0;
        __this->ear_number = buf[1];
        chargestore_api_write(send_buf, 1);
        chargestore_event_to_user(NULL, DEVICE_EVENT_CHARGE_STORE, CMD_CLOSE_CID, 0);
        break;

    case CMD_RESTORE_SYS:
        r_printf("restore sys\n");
        __this->power_status = 1;
        __this->cover_status = 1;
        __this->close_ing = 0;
        chargestore_api_write(send_buf, 1);
        chargestore_event_to_user(NULL, DEVICE_EVENT_CHARGE_STORE, CMD_RESTORE_SYS, 0);
        break;
#endif

#if TCFG_ANC_BOX_ENABLE
    case CMD_ANC_MODULE:
        app_ancbox_module_deal(buf, len);
        break;
#endif
    default:
        send_buf[0] = CMD_UNDEFINE;
        chargestore_api_write(send_buf, 1);
        break;
    }
}

#if TCFG_CHARGESTORE_ENABLE
u8 chargestore_get_power_level(void)
{
    if ((__this->power_level == 0xff) ||
        ((!get_charge_online_flag()) &&
         (__this->sibling_chg_lev != 0xff))) {
        return __this->sibling_chg_lev;
    }
    return __this->power_level;
}

u8 chargestore_get_power_status(void)
{
    return __this->power_status;
}

u8 chargestore_get_cover_status(void)
{
    return __this->cover_status;
}

u8 chargestore_get_earphone_online(void)
{
    return __this->ear_number;
}

void chargestore_set_earphone_online(u8 ear_number)
{
    __this->ear_number = ear_number;
}

void chargestore_set_pair_flag(u8 pair_flag)
{
    __this->pair_flag = pair_flag;
}

void chargestore_set_active_disconnect(u8 active_disconnect)
{
    __this->active_disconnect = active_disconnect;
}

u8 chargestore_get_earphone_pos(void)
{
    log_info("get_ear_channel = %c\n", __this->channel);
    return __this->channel;
}

u8 chargestore_get_sibling_power_level(void)
{
    return __this->tws_power;
}


#if TCFG_USER_TWS_ENABLE


static void set_tws_sibling_charge_level(void *_data, u16 len, bool rx)
{
    if (rx) {
        u8 *data = (u8 *)_data;
        chargestore_set_sibling_chg_lev(data[0]);
    }
}

REGISTER_TWS_FUNC_STUB(charge_level_stub) = {
    .func_id = TWS_FUNC_ID_CHARGE_SYNC,
    .func    = set_tws_sibling_charge_level,
};

#endif

int chargestore_sync_chg_level(void)
{
#if TCFG_USER_TWS_ENABLE
    int err = -1;
    u8 app = app_get_curr_task();
    if (app == APP_BT_TASK) && ((!app_var.goto_poweroff_flag)) {
        err = tws_api_send_data_to_sibling((u8 *)&__this->power_level, 1,
                                           TWS_FUNC_ID_CHARGE_SYNC);
    }
    return err;
#else
    return 0;
#endif
}

void chargestore_set_sibling_chg_lev(u8 chg_lev)
{
    __this->sibling_chg_lev = chg_lev;
}

void chargestore_set_power_level(u8 power)
{
    __this->power_level = power;
}

u8 chargestore_check_going_to_poweroff(void)
{
    return __this->close_ing;
}

void chargestore_shutdown_reset(void)
{
    if (__this->shutdown_timer) {
        sys_hi_timer_del(__this->shutdown_timer);
        __this->shutdown_timer = 0;
    }
}

#endif

void chargestore_set_bt_init_ok(u8 flag)
{
    __this->bt_init_ok = flag;
}

#if TCFG_TEST_BOX_ENABLE
u8 chargestore_get_connect_status(void)
{
    return __this->connect_status;
}

void chargestore_clear_connect_status(void)
{
    __this->connect_status = 0;
}

u8 chargestore_get_testbox_status(void)
{
    return __this->testbox_status;
}

void chargestore_clear_testbox_status(void)
{
    __this->testbox_status = 0;
    chargestore_clear_connect_status();
}

#endif

CHARGESTORE_PLATFORM_DATA_BEGIN(chargestore_data)
.uart_irq                = TCFG_CHARGESTORE_UART_ID,
 .io_port                = TCFG_CHARGESTORE_PORT,
  CHARGESTORE_PLATFORM_DATA_END()

  __BANK_INIT
  int app_chargestore_init(void)
{
    __this->power_status = 1;
    __this->cover_status = 0;
    __this->bt_init_ok = 0;
    __this->testbox_status = 0;
    __this->connect_status = 0;
    __this->ear_number = 1;
    __this->tws_power = 0xff;
    __this->power_level = 0xff;
    __this->sibling_chg_lev = 0xff;
    __this->max_packet_size = 32;
    __this->channel = 'U';
    __this->close_ing = 0;
    __this->event_hdl_flag = 0;
    __this->switch2bt = 0;
    chargestore_api_init(&chargestore_data);
    syscfg_read(CFG_CHARGESTORE_TWS_CHANNEL, &__this->channel, 1);
    log_info("channel = %c\n", __this->channel);
    return 0;
}
__initcall(app_chargestore_init);

#endif

