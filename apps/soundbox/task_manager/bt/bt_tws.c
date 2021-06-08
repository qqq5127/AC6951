
/*************************************************************

             此文件函数主要是蓝牙tws 功能处理


tws已有配对，启动链接,对方要在被链接状态
int bt_open_tws_conn(u16 timeout)

tws已经连接上，只断开tws链接
void bt_disconnect_tws_conn()

tws链接上，取消tws配对信息，并且断开链接
void  bt_tws_remove_tws_pair()

tws发起搜索链接，对方要在被链接状态
int bt_tws_start_search_and_pair()

tws 回链手机、tws被搜索链接、被手机搜索链接、tws链接等状态切换
客户可以在这个函数里面根据自己链接需求修改
static void connect_and_connectable_switch(void *_sw)

客户的链接需求可以根据各种情况来处理
根据 gtws.state 状态来判断需要做那种操作


**************************************************************/



#include "system/includes.h"
#include "media/includes.h"
#include "device/vm.h"
#include "tone_player.h"

#include "app_config.h"
#include "app_task.h"

#include "user_cfg.h"

#include "btcontroller_config.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "classic/hci_lmp.h"

#include "bt/bt_tws.h"
#include "classic/tws_pair.h"
#include "bt/bt_ble.h"
#include "bt/bt.h"


#include "asm/pwm_led.h"
#include "asm/charge.h"
#include "app_charge.h"
#include "ui_manage.h"

#include "app_chargestore.h"
#include "app_online_cfg.h"
#include "app_main.h"
#include "app_power_manage.h"
#include "audio_config.h"

#include "audio_dec.h"
#include "tone_player.h"

#include "bt_common.h"
#include "soundbox.h"

#if TCFG_APP_BT_EN


#ifdef CONFIG_NEW_BREDR_ENABLE
#define CONFIG_NEW_QUICK_CONN
#endif


#if TCFG_USER_TWS_ENABLE

#define LOG_TAG_CONST       BT_TWS
#define LOG_TAG             "[BT-TWS]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#define    BT_TWS_UNPAIRED                      0x0001
#define    BT_TWS_PAIRED                        0x0002
#define    BT_TWS_WAIT_SIBLING_SEARCH           0x0004
#define    BT_TWS_SEARCH_SIBLING                0x0008
#define    BT_TWS_CONNECT_SIBLING               0x0010
#define    BT_TWS_SIBLING_CONNECTED             0x0020
#define    BT_TWS_PHONE_CONNECTED               0x0040
#define    BT_TWS_POWER_ON                      0x0080
#define    BT_TWS_TIMEOUT                       0x0100
#define    BT_TWS_AUDIO_PLAYING                 0x0200


struct tws_user_var {
    u8 addr[6];
    u16 state;
    s16 auto_pair_timeout;   //tws 自动连接时间
    int pair_timer;     //tws 链接时间
    u32 connect_time;  // 回链手机时间计时
    u8  device_role;  //tws 记录那个是active device 活动设备，音源控制端
    u8 bt_task;   ///标志对箱在bt task情况，BIT(0):local  BIT(1):remote
};

struct tws_user_var  gtws;

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
typedef struct {
    u8 miss_flag: 1;
    u8 exchange_bat: 2;
    u8 poweron_flag: 1;
    u8 reserver: 4;
} icon_ctl_t;
static icon_ctl_t ble_icon_contrl;
#endif


static u32 local_us_time = 0;
static u32 local_instant_us_time = 0;
static u8 poweroff_sametime_flag = 0;
static u16 poweroff_sametime_timer_id = 0;
static u8 tws_wair_pair_time = 0;

static u8 tws_going_poweroff = 0;


#if CONFIG_TWS_DISCONN_NO_RECONN
u8 tws_detach_by_remove_key = 0;
#endif


///记录tws是否在蓝牙模式
#define    TWS_LOCAL_IN_BT()      (gtws.bt_task |=BIT(0))
#define    TWS_LOCAL_OUT_BT()   (gtws.bt_task &=~BIT(0))
#define    TWS_REMOTE_IN_BT()      (gtws.bt_task |=BIT(1))
#define    TWS_REMOTE_OUT_BT()    (gtws.bt_task &=~BIT(1))

static void connect_and_connectable_switch(void *_sw);
static void search_and_connectable_switch(void *_sw);
static void bt_tws_connect_sibling(int timeout);

u8 is_tws_going_poweroff()
{
    return tws_going_poweroff;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 判断tws是否都在蓝牙模式
   @param    无
   @return   1:在蓝牙模式    0:不在蓝牙模式
   @note
*/
/*----------------------------------------------------------------------------*/
u8 is_tws_all_in_bt()
{
#if (TCFG_DEC2TWS_ENABLE)
    int state = tws_api_get_tws_state();
    if (state & TWS_STA_SIBLING_CONNECTED) {
        if ((gtws.bt_task & 0x03) == 0x03) {
            return 1;
        }
        return 0;
    }
#endif
    return 1;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 判断tws是否为活动端
   @param    无
   @return   1:活动端    0:非活动端
   @note     音源输入端为活动端
*/
/*----------------------------------------------------------------------------*/
u8 is_tws_active_device(void)
{
    /* log_info("\n    ------  tws device ------   %x \n",gtws.device_role); */
    int state = tws_api_get_tws_state();
    if (state & TWS_STA_SIBLING_CONNECTED) {
        if (gtws.device_role == TWS_ACTIVE_DEIVCE) {
            return 1;
        }
        return 0;
    }
    return 1;
}

u8 tws_network_audio_was_started(void)
{
    if (gtws.state & BT_TWS_AUDIO_PLAYING) {
        return 1;
    }

    return 0;
}

void tws_network_local_audio_start(void)
{
    gtws.state &= ~BT_TWS_AUDIO_PLAYING;
}

bool get_tws_sibling_connect_state(void)
{
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        return TRUE;
    }
    /* if (gtws.state & BT_TWS_SIBLING_CONNECTED) { */
    /*     return TRUE; */
    /* } */
    return FALSE;
}


bool get_tws_phone_connect_state(void)
{
    if (gtws.state & BT_TWS_PHONE_CONNECTED) {
        return TRUE;
    }
    return FALSE;
}

#if CONFIG_TWS_POWEROFF_SAME_TIME
enum {
    FROM_HOST_POWEROFF_CNT_ENOUGH = 1,
    FROM_TWS_POWEROFF_CNT_ENOUGH,
    POWEROFF_CNT_ENOUGH,
};

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 同时关机
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void poweroff_sametime_timer(void *priv)
{
    log_info("poweroff_sametime_timer\n");
    int state = tws_api_get_tws_state();
    if (!(state & TWS_STA_SIBLING_CONNECTED)) {
        sys_enter_soft_poweroff(NULL);
        return;
    }
    if (priv == NULL || poweroff_sametime_flag == POWEROFF_CNT_ENOUGH) {
        poweroff_sametime_timer_id = sys_timeout_add((void *)1, poweroff_sametime_timer, 500);
    } else {
        poweroff_sametime_timer_id = 0;
    }
}

static void poweroff_sametime_timer_init(void)
{
    if (poweroff_sametime_timer_id) {
        return;
    }

    poweroff_sametime_timer_id = sys_timeout_add(NULL, poweroff_sametime_timer, 500);
}
#endif


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 同步执行发送命令函数
   @param    cmd:命令
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void tws_sync_call_fun(int cmd)
{
    struct sys_event event;

    event.type = SYS_BT_EVENT;
    event.arg = (void *)SYS_BT_EVENT_FROM_TWS;

    event.u.bt.event = TWS_EVENT_SYNC_FUN_CMD;
    event.u.bt.args[0] = 0;
    event.u.bt.args[1] = 0;
    event.u.bt.args[2] = cmd;
    sys_event_notify(&event);
}

TWS_SYNC_CALL_REGISTER(tws_tone_sync) = {
    .uuid = 'T',
    .func = tws_sync_call_fun,
};

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 同步执行发送命令函数,可区分发起段
   @param    cmd:命令
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void tws_sync_call_tranid_fun(int cmd)
{
    struct sys_event event;

    event.type = SYS_BT_EVENT;
    event.arg = (void *)SYS_BT_EVENT_FROM_TWS;

    event.u.bt.event = TWS_EVENT_SYNC_FUN_TRANID;
    event.u.bt.args[0] = 0;
    event.u.bt.args[1] = 0;
    event.u.bt.args[2] = cmd;

    sys_event_notify(&event);
}

TWS_SYNC_CALL_REGISTER(tws_tranid_sync) = {
    .uuid = 'D',
    .func = tws_sync_call_tranid_fun,
};

u32 get_local_us_time(u32 *instant_time)
{
    *instant_time = local_instant_us_time;
    return local_us_time;
}

int bt_tws_api_push_cmd(int priv, int delay_ms)
{
    return tws_api_sync_call_by_uuid('T', priv, delay_ms);
}

u16 tws_host_get_battery_voltage()
{
    return get_vbat_level();
}

int tws_host_channel_match(char remote_channel)
{
    /*r_printf("tws_host_channel_match: %c, %c\n", remote_channel,
             bt_tws_get_local_channel());*/

#if (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_MASTER_AS_LEFT)
    return 1;
#endif
    if (remote_channel != bt_tws_get_local_channel() || remote_channel == 'U') {
        return 1;
    }
#if CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_LEFT_START_PAIR || \
    CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_RIGHT_START_PAIR
    if (gtws.state & BT_TWS_SEARCH_SIBLING) {
        return 1;
    }
#endif

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 获取本地左右声道值
   @param    无
   @return   channel
   @note
*/
/*----------------------------------------------------------------------------*/
char tws_host_get_local_channel()
{
    char channel;

#if (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_RIGHT_START_PAIR)
    if (gtws.state & BT_TWS_SEARCH_SIBLING) {
        return 'R';
    }
#elif (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_LEFT_START_PAIR)
    if (gtws.state & BT_TWS_SEARCH_SIBLING) {
        return 'L';
    }
#endif
    channel = bt_tws_get_local_channel();
    if (channel != 'R') {
        channel = 'L';
    }
    /*y_printf("tws_host_get_local_channel: %c\n", channel);*/

    return channel;
}


/*
 *
 * */
u8 auto_pair_code[6] = {0x34, 0x66, 0x33, 0x87, 0x09, 0x42};

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 设置自动回连的识别码 6个byte
   @param    无
   @return   配对码
   @note
*/
/*----------------------------------------------------------------------------*/
u8 *tws_set_auto_pair_code(void)
{
    u16 code = bt_get_tws_device_indicate(NULL);
    auto_pair_code[0] = code >> 8;
    auto_pair_code[1] = code & 0xff;
    return auto_pair_code;
}
#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_FAST_CONN
u8 tws_auto_pair_enable = 1;
#else
u8 tws_auto_pair_enable = 0;
#endif


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 读取vm记忆里面的远端设备
   @param    无
   @return   0:没有找到    -ENOENT:读取成功
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 tws_get_sibling_addr(u8 *addr)
{
    u8 all_ff[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    int len = syscfg_read(CFG_TWS_REMOTE_ADDR, addr, 6);
    if (len != 6 || !memcmp(addr, all_ff, 6)) {
        return -ENOENT;
    }
    return 0;
}

void lmp_hci_write_local_address(const u8 *addr);

/*
 * 获取左右耳信息
 * 'L': 左耳
 * 'R': 右耳
 * 'U': 未知
 */
char bt_tws_get_local_channel()
{
    char channel = 'U';

    syscfg_read(CFG_TWS_CHANNEL, &channel, 1);

    return channel;
}

int get_bt_tws_connect_status()
{
    if (gtws.state & BT_TWS_SIBLING_CONNECTED) {
        return 1;
    }

    return 0;
}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 来电同步播放铃声
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void tws_sync_phone_num_play_wait(void *priv)
{
    log_debug("tws_sync_phone_num_play_wait\n");
    if (bt_user_priv_var.phone_con_sync_num_ring) {
        return;
    }
    if (bt_user_priv_var.phone_num_flag) {
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            tws_api_sync_call_by_uuid('T', SYNC_CMD_PHONE_NUM_TONE, TWS_SYNC_TIME_DO * 2);
        } else { //从机超时还没获取到
            phone_ring_play_start();
        }
    } else {
        /*电话号码还没有获取到，定时查询*/
        if (bt_user_priv_var.get_phone_num_timecnt > 300) {
            bt_user_priv_var.get_phone_num_timecnt = 0;
            tws_api_sync_call_by_uuid('T', SYNC_CMD_PHONE_SYNC_NUM_RING_TONE, TWS_SYNC_TIME_DO);
        } else {
            bt_user_priv_var.phone_timer_id = sys_timeout_add(NULL, tws_sync_phone_num_play_wait, 10);
        }
    }
    bt_user_priv_var.get_phone_num_timecnt++;

}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 来电同步报号
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_tws_sync_phone_num(void *priv)
{
    int state = tws_api_get_tws_state();
    if ((state & TWS_STA_SIBLING_CONNECTED) && (state & TWS_STA_PHONE_CONNECTED)) {
        log_debug("bt_tws_sync_phone_num\n");
#if BT_PHONE_NUMBER
        bt_user_priv_var.get_phone_num_timecnt = 0;
        if (tws_api_get_role() == TWS_ROLE_SLAVE && bt_user_priv_var.phone_con_sync_num_ring) { //从机同步播来电ring,不在发起sync_play
            /* log_debug("phone_con_sync_num_ring_ing return\n"); */
            return 1;
        }
#endif
        if (tws_api_get_role() == TWS_ROLE_SLAVE || bt_user_priv_var.phone_con_sync_num_ring) {

            if (!(state & TWS_STA_MONITOR_START)) {
                /* log_debug(" not monitor ring_tone\n"); */
                /* tws_api_sync_call_by_uuid('T', SYNC_CMD_PHONE_RING_TONE, TWS_SYNC_TIME_DO * 5); */
                bt_user_priv_var.phone_timer_id = sys_timeout_add(NULL, (void (*)(void *priv))bt_tws_sync_phone_num, 10);
                return 1;
            }
#if BT_PHONE_NUMBER
            if (bt_user_priv_var.phone_con_sync_num_ring) {
                /* log_debug("<<<<<<<<<<<send_SYNC_CMD_PHONE_SYNC_NUM_RING_TONE\n"); */
                tws_api_sync_call_by_uuid('T', SYNC_CMD_PHONE_SYNC_NUM_RING_TONE, TWS_SYNC_TIME_DO / 2);

            } else {
                bt_user_priv_var.phone_timer_id = sys_timeout_add(NULL, tws_sync_phone_num_play_wait, 10);
            }
#else
            tws_api_sync_call_by_uuid('T', SYNC_CMD_PHONE_RING_TONE, TWS_SYNC_TIME_DO * 2);
#endif
        } else {
#if BT_PHONE_NUMBER
            if (!bt_user_priv_var.phone_timer_id) { //从机超时还没获取到
                /* bt_user_priv_var.phone_timer_id = sys_timeout_add(NULL, tws_sync_phone_num_play_wait, TWS_SYNC_TIME_DO * 2); */
            }
#endif
        }
        return 1;
    }
    return 0;
}


static void bt_tws_delete_pair_timer()
{
    if (gtws.pair_timer) {
        sys_timeout_del(gtws.pair_timer);
        gtws.pair_timer = 0;
    }
}




/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 开启tws的主机搜索链接
   @param    无
   @return   无
   @note     tws 在  BT_TWS_UNPAIRED 状态开启搜索到tws即可链接

 			根据配对码搜索TWS设备
 			搜索超时会收到事件: TWS_EVENT_SEARCH_TIMEOUT
 			搜索到连接超时会收到事件: TWS_EVENT_CONNECTION_TIMEOUT
 			搜索到并连接成功会收到事件: TWS_EVENT_CONNECTED
*/
/*----------------------------------------------------------------------------*/
static void bt_tws_search_and_pair()
{
    u8 mac_addr[6];

    if (gtws.state & BT_TWS_UNPAIRED) {
        bt_tws_delete_pair_timer();
        tws_api_get_local_addr(gtws.addr);
#if CONFIG_TWS_USE_COMMMON_ADDR
#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_CLICK
        get_random_number(mac_addr, 6);
        lmp_hci_write_local_address(mac_addr);
#endif
#endif


#if CONFIG_TWS_PAIR_BY_BOTH_SIDES
        bt_user_priv_var.search_counter = 5;
        bt_set_pair_code_en(1);
        search_and_connectable_switch(0);
#else
        tws_api_search_sibling_by_code(bt_get_tws_device_indicate(NULL), 15000);
#endif
    }
}


void bt_tws_close_search_and_pair()
{
    tws_close_tws_pair();
}







/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 开启 tws间从机被搜索链接
  @param    无
   @return   无
   @note     tws 在  所有状态开启
*/
/*----------------------------------------------------------------------------*/
void bt_open_tws_wait_pair_all_way()
{
    bt_tws_delete_pair_timer();
    tws_open_tws_wait_pair(bt_get_tws_device_indicate(NULL), bt_get_local_name());
}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 开启 tws间从机被搜索链接
  @param    无
   @return   无
   @note     tws 在  BT_TWS_UNPAIRED 状态开启
*/
/*----------------------------------------------------------------------------*/
void bt_open_tws_wait_pair()
{
    if (gtws.state & BT_TWS_UNPAIRED) {
        bt_tws_delete_pair_timer();
        tws_open_tws_wait_pair(bt_get_tws_device_indicate(NULL), bt_get_local_name());
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 开启 tws间  从机等待链接
   @param    无
   @return   无
   @note     tws必须在已经配对过才可以调用这个函数
*/
/*----------------------------------------------------------------------------*/
void bt_open_tws_wait_conn(u16 timeout)
{
    if (gtws.state & BT_TWS_PAIRED) {
        if (!(gtws.state & BT_TWS_SIBLING_CONNECTED)) { // tws 没有连接上
            bt_tws_delete_pair_timer();
            tws_open_tws_wait_conn(timeout);
        }
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 关闭tws等待链接状态
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_close_tws_wait_pair()
{
    tws_close_tws_pair();
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 开启 tws间  主机链接
   @param    无
   @return   无
   @note     tws必须在已经配对过才可以调用这个函数
*/
/*----------------------------------------------------------------------------*/
int bt_open_tws_conn(u16 timeout)
{
    int err = -EFAULT;
    if (gtws.state & BT_TWS_PAIRED) {
        if (!(gtws.state & BT_TWS_SIBLING_CONNECTED)) { // tws 没有连接上
            if (get_esco_coder_busy_flag()) {
                err = tws_api_connect_in_esco();
                return err;
            }

            err = tws_open_tws_conn(timeout);
        }
    }
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 关闭tws 发起链接状态
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_close_tws_conn()
{
    tws_api_cancle_create_connection();
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 断开tws之间的链接
   @param    无
   @return   无
   @note     tws必须在已经配对过才可以调用这个函数
*/
/*----------------------------------------------------------------------------*/
void bt_disconnect_tws_conn()
{
    if (gtws.state & BT_TWS_SIBLING_CONNECTED) { // tws 没有连接上
        tws_disconnect();
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 清除tws配对信息
   @param    无
   @return   无
   @note     tws有配对过才运行
*/
/*----------------------------------------------------------------------------*/
void bt_remove_tws_pair()
{
    if (gtws.state & BT_TWS_PAIRED) {
        tws_remove_tws_pairs();
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 开启 被手机搜索被链接
   @param    无
   @return   无
   @note     tws 在  没有连接手机 状态开启
*/
/*----------------------------------------------------------------------------*/
void bt_open_phone_wait_pair(void)
{
    /* if (!(gtws.state & BT_TWS_PHONE_CONNECTED)) { //没连接手机 */
    if (!(tws_api_get_tws_state() & TWS_STA_PHONE_CONNECTED)) {
        tws_open_phone_wait_pair(bt_get_tws_device_indicate(NULL), bt_get_local_name());
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 关闭 被手机搜索被链接
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_close_phone_wait_pair()
{
    tws_close_phone_wait_pair();
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 关闭所有tws 链接状态
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void tws_cancle_all_noconn()
{
    bt_tws_delete_pair_timer();

    bt_close_tws_wait_pair();
    bt_close_phone_wait_pair();
    bt_close_tws_conn();
    /* tws_api_cancle_wait_pair();
    tws_api_cancle_create_connection(); */
    user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
}




/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 启动tws 主机搜索链接
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_tws_start_search_and_pair()
{
    int state = get_bt_connect_status();

    if ((get_call_status() == BT_CALL_ACTIVE) ||
        (get_call_status() == BT_CALL_OUTGOING) ||
        (get_call_status() == BT_CALL_ALERT) ||
        (get_call_status() == BT_CALL_INCOMING)) {
        return 0;//通话过程不允许
    }

    if (check_tws_le_aa()) {
        return 0;
    }

#if CONFIG_TWS_USE_COMMMON_ADDR
    if (tws_api_get_tws_state() & TWS_STA_PHONE_CONNECTED) {
        return 0;
    }
#endif



#if CONFIG_TWS_DISCONN_NO_RECONN
    if (tws_detach_by_remove_key) {
        bt_tws_connect_sibling(CONFIG_TWS_CONNECT_SIBLING_TIMEOUT);
        return 0;
    }
#endif



#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_CLICK
    gtws.state |= BT_TWS_SEARCH_SIBLING;
#if CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_LEFT_START_PAIR || \
    CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_RIGHT_START_PAIR || \
    CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_MASTER_AS_LEFT
    tws_api_set_local_channel('U');
#endif
    bt_tws_search_and_pair();
    return 1;
#endif

    return 0;
}




/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 取消tws配对
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
u8  bt_tws_remove_tws_pair()
{
    int state = get_bt_connect_status();

    if ((get_call_status() == BT_CALL_ACTIVE) ||
        (get_call_status() == BT_CALL_OUTGOING) ||
        (get_call_status() == BT_CALL_ALERT) ||
        (get_call_status() == BT_CALL_INCOMING)) {
        return 0;//通话过程不允许
    }

#if CONFIG_TWS_USE_COMMMON_ADDR
    if (tws_api_get_tws_state() & TWS_STA_PHONE_CONNECTED) {
        return 0;
    }
#endif

    if (gtws.state & BT_TWS_SIBLING_CONNECTED) {
        tws_remove_tws_pairs();
        return 1;
    }
    return 0;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 取消tws配对发起tws 链接
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void  bt_tws_search_or_remove_pair()
{
    if (bt_tws_remove_tws_pair()) {
        return;
    }

    bt_tws_start_search_and_pair();
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 链接状态调度
   @param    无
   @return   无
   @note     0:回链手机
   			 1：开启被手机搜索\tws间被搜索链接
		     2:开启链接tws
		     3:开启tws 自动连接
*/
/*----------------------------------------------------------------------------*/
void bt_tws_connect_and_connectable_switch()
{
    bt_tws_delete_pair_timer();

    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        connect_and_connectable_switch(0);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 先开链接tws,开启不了就开启 tws链接状态机
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_tws_connect_sibling(int timeout)
{
    int err;

    bt_tws_delete_pair_timer();
    err = bt_open_tws_conn(timeout * 1000);
    if (err) {
        bt_tws_connect_and_connectable_switch();
    }
}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 打开可发现, 可连接，可被手机和tws搜索到
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_tws_wait_pair_and_phone_connect()
{
    bt_tws_delete_pair_timer();

    bt_open_tws_wait_pair();
    bt_open_phone_wait_pair();
    /* tws_api_wait_pair_by_code(bt_get_tws_device_indicate(NULL), bt_get_local_name(), 0); */
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 打开可以被tws链接状态
   @param    无
   @return   无
   @note     这个是要在tws 已经连接上过才起作用
*/
/*----------------------------------------------------------------------------*/
static void bt_tws_wait_sibling_connect(int timeout)
{
    bt_tws_delete_pair_timer();
    bt_open_tws_wait_conn(timeout);
    /* tws_api_wait_connection(timeout); */
}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 回链手机
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static int bt_tws_connect_phone()
{
    int timeout = 6500;

    bt_user_priv_var.auto_connection_counter -= timeout ;
    bt_close_tws_wait_pair();
    bt_close_phone_wait_pair();
    bt_close_tws_conn();
    /* tws_api_cancle_wait_pair(); */
    /* tws_api_cancle_create_connection(); */
    user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6,
                          bt_user_priv_var.auto_connection_addr);
    return timeout;
}


#ifdef CONFIG_NEW_BREDR_ENABLE
#else

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 回链手机和可被发先被链接状态,tws被链接状态
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static int tws_page_phone()
{
    int timeout = 0;
__again:
    if (bt_user_priv_var.auto_connection_counter > 0) {
        timeout = 4000 + (rand32() % 4 + 1) * 500;
        bt_user_priv_var.auto_connection_counter -= timeout ;

        bt_close_tws_wait_pair();
        bt_close_phone_wait_pair();
        bt_close_tws_conn();

        /* tws_api_cancle_wait_pair();
        tws_api_cancle_create_connection(); */

        user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6,
                              bt_user_priv_var.auto_connection_addr);
    } else {
        bt_user_priv_var.auto_connection_counter = 0;

        BT_STATE_CANCEL_PAGE_SCAN();

        if (gtws.state & BT_TWS_POWER_ON) {
            /*
             * 开机回连,获取下一个设备地址
             */
            if (get_current_poweron_memory_search_index(NULL)) {
                bt_init_ok_search_index();
                goto __again ;
            }
            gtws.state &= ~BT_TWS_POWER_ON;
        }
    }

    return timeout;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 开启tws被发现链接的状态
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static u32 tws_wait_pair_and_wait_conn(u8 sw)
{
    int timeout = 0;
    if (tws_remote_state_check()) {
        timeout = 1000 + (rand32() % 4 + 1) * 500;
    } else {
        timeout = 2000 + (rand32() % 4 + 1) * 500;
    }

    /* user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL); */
    extern void bredr_close_all_scan();
    bredr_close_all_scan();
    if (sw) {
        bt_open_tws_wait_pair_all_way();
    } else {
        bt_open_tws_wait_pair();
        bt_open_tws_wait_conn(0);
    }

    if (sw != 2) {
        bt_open_phone_wait_pair();
    }

    BT_STATE_SET_PAGE_SCAN_ENABLE();

    return timeout;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 开启tws 链接
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static u32 tws_creat_connection()
{
    int timeout = 0;
    if (tws_remote_state_check()) {
        timeout = 4000;
    } else {
        timeout = 1000 + (rand32() % 4 + 1) * 500;
    }
    tws_remote_state_clear();
    bt_open_tws_conn(0);
    return timeout;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 自动连接
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static u32 tws_pari_by_auto()
{
    int timeout = 0;
#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
    timeout = 2000 + (rand32() % 4 + 1) * 500;
    tws_auto_pair_enable = 1;
    tws_le_acc_generation_init();
    tws_api_create_connection(0);
#endif
    return timeout;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 被手机发现链接、tws被搜索链接、tws已配对链接状态切换
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void connect_and_connectable_switch(void *_sw)
{
    int timeout = 0;
    int sw = (int)_sw;

    gtws.pair_timer = 0;



#if CONFIG_TWS_DISCONN_NO_RECONN
    if (tws_detach_by_remove_key) {
        return;
    }
#endif



    log_info("switch: %d, state = %x, %d\n", sw, gtws.state,
             bt_user_priv_var.auto_connection_counter);

#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
    if (tws_auto_pair_enable == 1) {
        tws_auto_pair_enable = 0;
        tws_le_acc_generation_init();
    }
#endif

    /* if (tws_remote_state_check()) {
        if (sw == 0) {
            sw = 1;
        } else if (sw == 3) {
            sw = 2;
        }
    } */


    if (sw == 0) {   //判断是否需要回链手机
        timeout = tws_page_phone();
        if (!timeout) { /// 不需要回链手机
            sw = 1;
        }
    }

    if (sw == 1) {
        timeout = tws_wait_pair_and_wait_conn(1);
    } else if (sw == 2) {
        timeout = tws_creat_connection();
    } else if (sw == 3) {
        timeout = tws_pari_by_auto();
    }

    /// 如果tws 没有配对信息
    int end_sw = 3;
    if (gtws.state & BT_TWS_UNPAIRED) {
        end_sw = 1;
#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
        end_sw = 3;
#endif
    } else if (gtws.state & BT_TWS_PAIRED) {
        /* //// 如果通过按键断开tws后不需要tws马上回链， */
        /* end_sw = 1; */

        // 如果通过tws断开后需要tws马上回链，
        end_sw = 2;
#ifdef CONFIG_TWS_AUTO_PAIR_WITHOUT_UNPAIR
        end_sw = 3;
#endif
    }

    if (++sw > end_sw) {
        sw = 0;
    }

    /* r_printf("++ %d \n",sw); */

    gtws.pair_timer = sys_timeout_add((void *)sw, connect_and_connectable_switch, timeout);

}


#if CONFIG_TWS_DISCONN_NO_RECONN
static void connect_and_connectable_switch_by_noreconn(void *_sw)
{
    int timeout = 0;
    int sw = (int)_sw;

    gtws.pair_timer = 0;


    if (tws_detach_by_remove_key == 0) {
        return;
    }

    bt_close_tws_wait_pair();


    if (sw == 0) {
        bt_tws_wait_sibling_connect(0);
        timeout = 2000 + ((rand32() % 10) * 200);
    } else if (sw == 1) {

        //bt_open_phone_wait_pair();
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);

        timeout = 3000 + ((rand32() % 10) * 200);
    }

    if (++sw > 1) {
        sw = 0;
    }

    //printf("++ %d \n",sw);

    gtws.pair_timer = sys_timeout_add((void *)sw, connect_and_connectable_switch_by_noreconn, timeout);

}
#endif



#if CONFIG_TWS_PAIR_BY_BOTH_SIDES
static int tws_search_both_side()
{
    int timeout = 0;
__again:
    if (bt_user_priv_var.auto_connection_counter > 0) {
        timeout = 4000 + (rand32() % 4 + 1) * 500;
        bt_user_priv_var.auto_connection_counter -= timeout ;

        bt_close_tws_wait_pair();
        bt_close_phone_wait_pair();
        bt_close_tws_conn();

        /* tws_api_cancle_wait_pair();
        tws_api_cancle_create_connection(); */

        user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6,
                              bt_user_priv_var.auto_connection_addr);
    } else {
        bt_user_priv_var.auto_connection_counter = 0;

        BT_STATE_CANCEL_PAGE_SCAN();

        if (gtws.state & BT_TWS_POWER_ON) {
            /*
             * 开机回连,获取下一个设备地址
             */
            if (get_current_poweron_memory_search_index(NULL)) {
                bt_init_ok_search_index();
                goto __again ;
            }
            gtws.state &= ~BT_TWS_POWER_ON;
        }
    }

    return timeout;
}

void tws_remote_search_working(void)
{
    tws_wair_pair_time = 5000;
}

static void search_and_connectable_switch(void *_sw)
{
    int timeout = 0;
    int sw = (int)_sw;

    bt_tws_close_search_and_pair();

    if (sw == 0) {
        timeout = tws_wait_pair_and_wait_conn(2);
    } else if (sw == 1) {

        printf("search cnt %d\n", bt_user_priv_var.search_counter);

        if ((tws_wair_pair_time) && (bt_user_priv_var.search_counter > 0)) {
            printf("here11\n");
            timeout = 5000;
            tws_wait_pair_and_wait_conn(2);
        } else {
            if (bt_user_priv_var.search_counter > 0) {
                printf("here22\n");
                tws_api_search_sibling_by_code(bt_get_tws_device_indicate(NULL), 15000);
                timeout = 1000 + (rand32() % 4 + 1) * 500;

            } else {
                printf("here33\n");
                tws_api_search_sibling_by_code(bt_get_tws_device_indicate(NULL), 2000);
                return;
            }
        }
        bt_user_priv_var.search_counter--;
    }

    tws_wair_pair_time = 0;

    if (++sw > 1) {
        sw = 0;
    }

    r_printf("++ search sw %d \n", sw);

    gtws.pair_timer = sys_timeout_add((void *)sw, search_and_connectable_switch, timeout);

}
#endif
#endif


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws  自动配对状态定时切换函数
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void __bt_tws_auto_pair_switch(void *priv)
{
    u32 timeout;
    int sw = (int)priv;

    gtws.pair_timer = 0;

    log_debug("bt_tws_auto_pair: %d, %d\n", gtws.auto_pair_timeout, bt_user_priv_var.auto_connection_counter);

    if (gtws.auto_pair_timeout < 0 && bt_user_priv_var.auto_connection_counter > 0) {
        gtws.auto_pair_timeout = 4000;
        timeout = bt_tws_connect_phone();
    } else {

        timeout = 1000 + ((rand32() % 10) * 200);

        gtws.auto_pair_timeout -= timeout;

        if (sw == 0) {
            tws_api_wait_pair_by_code(bt_get_tws_device_indicate(NULL), bt_get_local_name(), 0);
        } else if (sw == 1) {
            bt_tws_search_and_pair();
        }

        if (++sw > 1) {
            sw = 0;
        }
    }

    gtws.pair_timer = sys_timeout_add((void *)sw, __bt_tws_auto_pair_switch, timeout);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws  自动配对状态定时切换函数
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_tws_auto_pair_switch(int timeout)
{
    bt_tws_delete_pair_timer();

    gtws.auto_pair_timeout = timeout;
    __bt_tws_auto_pair_switch(NULL);
}

void __set_sbc_cap_bitpool(u8 sbc_cap_bitpoola);


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws链接配对码
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 set_channel_by_code_or_res(void)
{
    u8 count = 0;
    char channel = 0;
    char last_channel = 0;
#if (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_UP_AS_LEFT)
    gpio_set_direction(CONFIG_TWS_CHANNEL_CHECK_IO, 1);
    gpio_set_pull_down(CONFIG_TWS_CHANNEL_CHECK_IO, 1);
    gpio_set_die(CONFIG_TWS_CHANNEL_CHECK_IO, 1);
    for (int i = 0; i < 5; i++) {
        os_time_dly(2);
        if (gpio_read(CONFIG_TWS_CHANNEL_CHECK_IO)) {
            count++;
        }
    }
    channel = (count >= 3) ? 'L' : 'R';
#elif (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_DOWN_AS_LEFT)
    gpio_set_direction(CONFIG_TWS_CHANNEL_CHECK_IO, 1);
    gpio_set_die(CONFIG_TWS_CHANNEL_CHECK_IO, 1);
    gpio_set_pull_up(CONFIG_TWS_CHANNEL_CHECK_IO, 1);
    for (int i = 0; i < 5; i++) {
        os_time_dly(2);
        if (gpio_read(CONFIG_TWS_CHANNEL_CHECK_IO)) {
            count++;
        }
    }
    channel = (count >= 3) ? 'R' : 'L';
#elif (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_AS_LEFT_CHANNEL)
    channel = 'L';
#elif (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_AS_RIGHT_CHANNEL)
    channel = 'R';
#elif (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_SECECT_BY_CHARGESTORE)
    syscfg_read(CFG_CHARGESTORE_TWS_CHANNEL, &channel, 1);
#endif

#if CONFIG_TWS_SECECT_CHARGESTORE_PRIO
    syscfg_read(CFG_CHARGESTORE_TWS_CHANNEL, &channel, 1);
#endif

    if (channel) {
        syscfg_read(CFG_TWS_CHANNEL, &last_channel, 1);
        if (channel != last_channel) {
            syscfg_write(CFG_TWS_CHANNEL, &channel, 1);
        }
        tws_api_set_local_channel(channel);
        return 1;
    }
    return 0;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws初始化
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
__BANK_INIT_ENTRY
int bt_tws_poweron()
{
    int err;
    u8 addr[6];
    char channel;


#if CONFIG_TWS_DISCONN_NO_RECONN
    tws_detach_by_remove_key = 0;
#endif



    gtws.state = BT_TWS_POWER_ON;
    gtws.connect_time = 0;
    gtws.device_role = TWS_ACTIVE_DEIVCE;
    TWS_LOCAL_IN_BT();

#if CONFIG_TWS_PAIR_BY_BOTH_SIDES
    bt_set_pair_code_en(0);
#endif

#if CONFIG_TWS_USE_COMMMON_ADDR
    tws_api_common_addr_en(1);
#else
    tws_api_common_addr_en(0);
    tws_api_auto_role_switch_disable();
#endif

#if CONFIG_TWS_PAIR_ALL_WAY
    tws_api_pair_all_way(1);
#endif

    __set_sbc_cap_bitpool(38);


#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
    memset(&ble_icon_contrl, 0, sizeof(icon_ctl_t));
    ble_icon_contrl.poweron_flag = 1;
#endif

#if (TCFG_DEC2TWS_ENABLE)
    tws_api_auto_role_switch_disable();
#endif


    err = tws_get_sibling_addr(addr);
    if (err == 0) {
        /*
         * 获取到对方地址, 开始连接
         */
        log_info("\n ---------have tws info----------\n");
        gtws.state |= BT_TWS_PAIRED;

        BT_STATE_TWS_INIT(1);

        tws_api_set_sibling_addr(addr);
        if (set_channel_by_code_or_res() == 0) {
            channel = bt_tws_get_local_channel();
            tws_api_set_local_channel(channel);
        }

#if TCFG_TEST_BOX_ENABLE
        if (chargestore_get_testbox_status()) {
        } else
#endif
        {
#ifdef CONFIG_NEW_QUICK_CONN
            if (bt_tws_get_local_channel() == 'L') {
                syscfg_read(CFG_TWS_LOCAL_ADDR, addr, 6);
            }
            tws_api_set_quick_connect_addr(addr);
#endif
            ////<有tws配对信息，开启tws链接
            bt_tws_connect_sibling(CONFIG_TWS_CONNECT_SIBLING_TIMEOUT);
        }

    } else {
        log_info("\n ---------no tws info----------\n");

        BT_STATE_TWS_INIT(0);


        gtws.state |= BT_TWS_UNPAIRED;
        if (set_channel_by_code_or_res() == 0) {
            tws_api_set_local_channel('U');
        }
#if TCFG_TEST_BOX_ENABLE
        if (chargestore_get_testbox_status()) {
        } else
#endif
        {

#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
            /*
             * 未配对, 开始自动配对
             */
#ifdef CONFIG_NEW_BREDR_ENABLE
            tws_api_set_quick_connect_addr(tws_set_auto_pair_code());
#else
            tws_auto_pair_enable = 1;
            tws_le_acc_generation_init();
#endif
            bt_tws_connect_sibling(6);

#else
            /*
             * 未配对, 等待发起配对
             */
            bt_tws_connect_and_connectable_switch();
#endif
        }
    }

#ifndef CONFIG_NEW_BREDR_ENABLE
    tws_remote_state_clear();
#endif

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 手机开始连接
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_tws_hci_event_connect()
{
    log_info("bt_tws_hci_event_connect: %x\n", gtws.state);

    gtws.state &= ~BT_TWS_POWER_ON;

    bt_user_priv_var.auto_connection_counter = 0;

    bt_tws_delete_pair_timer();
    sys_auto_shut_down_disable();

#ifndef CONFIG_NEW_BREDR_ENABLE
    tws_remote_state_clear();
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 手机链接上
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_tws_phone_connected()
{
    int state;

    log_info("bt_tws_phone_connected: %x\n", gtws.state);

#if TCFG_TEST_BOX_ENABLE
    if (chargestore_get_testbox_status()) {
        return 1;
    }
#endif

    gtws.state |= BT_TWS_PHONE_CONNECTED;

    //// tws 没有配对过
    if (gtws.state & BT_TWS_UNPAIRED) {
#if (CONFIG_TWS_PAIR_ALL_WAY == 1)

#if CONFIG_TWS_USE_COMMMON_ADDR

#else
        if (get_call_status() == BT_CALL_HANGUP) {  ///不在通话状态，开启tws 被搜索链接状态
            bt_open_tws_wait_pair();
        }
#endif
#endif
        return 0;
    }

    if (!(gtws.state & BT_TWS_SIBLING_CONNECTED)) { ///tws 已经配对，但是没哟链接，开启被链接
        bt_tws_wait_sibling_connect(0);
        return 0;
    }

    /*
     * 获取tws状态，如果正在播歌或打电话则返回1,不播连接成功提示音
     */

    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        bt_tws_api_push_cmd(SYNC_CMD_LED_PHONE_CONN_STATUS, TWS_SYNC_TIME_DO - 500);   //此时手机已经连接上，同步PHONE_CONN状态

        /* bt_tws_sync_volume(); */
        state = tws_api_get_tws_state();
        if (state & (TWS_STA_SBC_OPEN | TWS_STA_ESCO_OPEN) ||
            (get_call_status() != BT_CALL_HANGUP)) {
            return 1;
        }

        /* log_info("[SYNC] TONE SYNC"); */
        bt_tws_api_push_cmd(SYNC_CMD_PHONE_CONN_TONE, TWS_SYNC_TIME_DO);
    }

    return 1;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 手机断开链接
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_tws_phone_disconnected()
{
    gtws.state &= ~BT_TWS_PHONE_CONNECTED;

    y_printf("**** bt_tws_phone_disconnected: %x\n", gtws.state);

    sys_auto_shut_down_enable();

    bt_user_priv_var.auto_connection_counter = 0;
    if (!(gtws.state & BT_TWS_SIBLING_CONNECTED)) {
        bt_tws_connect_and_connectable_switch();
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 回链手机超时
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_tws_phone_page_timeout()
{
    log_info("bt_tws_phone_page_timeout: %x\n", gtws.state);

    bt_tws_phone_disconnected();
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 链接手机超时
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_tws_phone_connect_timeout()
{
    log_debug("bt_tws_phone_connect_timeout: %x, %d\n", gtws.state, gtws.pair_timer);

    gtws.state &= ~BT_TWS_PHONE_CONNECTED;

    /*
     * 跟手机超时断开,如果对耳未连接则优先连接对耳
     */
    if (gtws.state & (BT_TWS_UNPAIRED | BT_TWS_SIBLING_CONNECTED)) {
        bt_tws_connect_and_connectable_switch();
    } else {
        bt_tws_connect_sibling(CONFIG_TWS_CONNECT_SIBLING_TIMEOUT);
    }
}

void bt_get_vm_mac_addr(u8 *addr);

void bt_page_scan_for_test()
{
    u8 local_addr[6];

    int state = tws_api_get_tws_state();

    /* log_info("\n\n\n\n -------------bt test page scan\n"); */

    bt_tws_delete_pair_timer();

    bt_close_phone_wait_pair();
    bt_close_tws_wait_pair();
    bt_close_tws_conn();

    user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);

    tws_api_detach(TWS_DETACH_BY_POWEROFF);

    user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);

    if (0 == get_total_connect_dev()) {
        /* log_info("<<<<<<<<<<<<<<<<no phone connect,instant page_scan>>>>>>>>>>>>>>>>\n"); */
        bt_get_vm_mac_addr(local_addr);
        //log_info("local_adr\n");
        //put_buf(local_addr,6);
        lmp_hci_write_local_address(local_addr);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    }

    gtws.state = 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 关机
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_tws_poweroff()
{
    log_info("bt_tws_poweroff\n");

    bt_tws_delete_pair_timer();

    bt_close_phone_wait_pair();
    bt_close_tws_wait_pair();
    bt_close_tws_conn();

    user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);

    tws_api_detach(TWS_DETACH_BY_POWEROFF);

    tws_profile_exit();

    if (tws_api_get_tws_state() & TWS_STA_SIBLING_DISCONNECTED) {
        return 1;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 通话过程中tws开启被链接 或者链接
   @param     esco_flag  1:开始通话   0:结束通话
   @return   无
   @note     开始通话的时候要把tws等待链接的状态关闭，通话结束后在开启
*/
/*----------------------------------------------------------------------------*/
extern int tws_api_cancle_wait_pair_internal();
void tws_page_scan_deal_by_esco(u8 esco_flag)
{
    if (gtws.state & BT_TWS_UNPAIRED) {   ///tws 没配对
#if (CONFIG_TWS_PAIR_ALL_WAY == 1)
        if (esco_flag) {
            bt_tws_delete_pair_timer();
            bt_close_tws_wait_pair();
        } else {
            bt_open_tws_wait_pair();

        }
#endif
        return;
    }


    if (esco_flag) {
        bt_tws_delete_pair_timer();
        gtws.state &= ~BT_TWS_CONNECT_SIBLING;
        bt_close_tws_conn();
        tws_api_connect_in_esco();
        /* log_debug("close scan\n"); */
    }

    if (!esco_flag && !(gtws.state & BT_TWS_SIBLING_CONNECTED)) {
        /* log_debug("open scan22\n"); */
        tws_api_cancle_connect_in_esco();
        bt_open_tws_conn(0);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 解除配对，清掉对方地址信息和本地声道信息
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_tws_remove_pairs()
{
    u8 mac_addr[6];
    char channel = 'U';
    char tws_channel = 0;
    int role = 0;

#if (CONFIG_TWS_USE_COMMMON_ADDR == 0)
    syscfg_write(VM_TWS_ROLE, &role, 1);
#endif

    memset(mac_addr, 0xFF, 6);
    syscfg_write(CFG_TWS_COMMON_ADDR, mac_addr, 6);
    syscfg_write(CFG_TWS_REMOTE_ADDR, mac_addr, 6);
#if CONFIG_TWS_USE_COMMMON_ADDR
    bt_reset_and_get_mac_addr(mac_addr);
    delete_last_device_from_vm();
#else
    syscfg_read(CFG_BT_MAC_ADDR, mac_addr, 6);
#endif


    lmp_hci_write_local_address(mac_addr);

#if (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_LEFT_START_PAIR) ||\
    (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_RIGHT_START_PAIR) ||\
    (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_MASTER_AS_LEFT)

#if CONFIG_TWS_SECECT_CHARGESTORE_PRIO
    syscfg_read(CFG_CHARGESTORE_TWS_CHANNEL, &tws_channel, 1);
#endif
    if ((tws_channel != 'L') && (tws_channel != 'R')) {
        syscfg_write(CFG_TWS_CHANNEL, &channel, 1);
        tws_api_set_local_channel(channel);
    }
#endif

    tws_api_clear_connect_aa();
    ////取消配对后开启tws被搜索链接和被手机搜索链接
    bt_tws_wait_pair_and_phone_connect();
}

#if TCFG_ADSP_UART_ENABLE
extern void adsp_deal_sibling_uart_command(u8 type, u8 cmd);
void tws_sync_uart_command(u8 type, u8 cmd)
{
    struct tws_sync_info_t sync_adsp_uart_command;
    sync_adsp_uart_command.type = TWS_SYNC_ADSP_UART_CMD;
    sync_adsp_uart_command.u.adsp_cmd[0] = type;
    sync_adsp_uart_command.u.adsp_cmd[1] = cmd;
    tws_api_send_data_to_sibling((u8 *)&sync_adsp_uart_command, sizeof(sync_adsp_uart_command));
}
#endif


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws音量同步
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_tws_vol_sync(void *_data, u16 len, bool rx)
{
    if (rx) {
        u8 *data = (u8 *)_data;
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, data[0], 1);
        app_audio_set_volume(APP_AUDIO_STATE_CALL, data[1], 1);
        log_debug("----   bt_tws_sync_volume %d %d \n", data[0], data[1]);
        log_debug("vol: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    }

}


REGISTER_TWS_FUNC_STUB(app_vol_sync_stub) = {
    .func_id = TWS_FUNC_ID_VOL_SYNC,
    .func    = bt_tws_vol_sync,
};

void bt_tws_sync_volume()
{
    u8 data[2];

    data[0] = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    data[1] = app_audio_get_volume(APP_AUDIO_STATE_CALL);

    log_debug("---- tx  bt_tws_sync_volume %d %d \n", data[0], data[1]);

    tws_api_send_data_to_sibling(data, 2, TWS_FUNC_ID_VOL_SYNC);
}

void send_bt_event2local(u8 value)
{
    struct sys_event event;
    event.type = SYS_BT_EVENT;
    event.arg = (void *)SYS_BT_EVENT_FORM_SELF;
    event.u.bt.event = 0;
    event.u.bt.value = value;
    sys_event_notify(&event);
}

static void bt_tws_reverb_state_sync(void *_data, u16 len, bool rx)
{
    if (rx) {
        u8 *data = (u8 *)_data;
        app_var.reverb_status = data[0];
        extern void send_bt_event2local(u8 value);
        send_bt_event2local(app_var.reverb_status);
        /* log_debug("----rx  bt_tws_reverb_state_sync %d \n", data[0]); */
    }

}

REGISTER_TWS_FUNC_STUB(reverb_sync_stub) = {
    .func_id = TWS_FUNC_ID_REVERB_SYNC,
    .func    = bt_tws_reverb_state_sync,
};

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws同步同步开启混响
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_tws_sync_reverb_state(u8 status)
{
    u8 data[1];

    app_var.reverb_status = status;
    data[0] = app_var.reverb_status;
    tws_api_send_data_to_sibling(data, 1, TWS_FUNC_ID_REVERB_SYNC);
}

void tws_conn_auto_test(void *p)
{
    cpu_reset();
}

extern u32 audio_bt_time_read(u16 *bt_phase, s32 *bt_bitoff);

static void bt_tws_box_sync(void *_data, u16 len, bool rx)
{
#if (TCFG_DEC2TWS_ENABLE)
    if (rx) {
        u8 *data = (u8 *)_data;
        if (data[0] == TWS_BOX_NOTICE_A2DP_BACK_TO_BT_MODE
            || data[0] == TWS_BOX_A2DP_BACK_TO_BT_MODE_START) {
            tws_local_back_to_bt_mode(data[0], data[1]);
            /* u32 bt_tmr = 0; */
            /* memcpy(&bt_tmr, &data[2], 4); */
            /* a2dp_dec_output_set_start_bt_time(bt_tmr); */
        }
    }
#endif
}

REGISTER_TWS_FUNC_STUB(app_box_sync_stub) = {
    .func_id = TWS_FUNC_ID_BOX_SYNC,
    .func    = bt_tws_box_sync,
};


void tws_user_sync_box(u8 cmd, u8 value)
{
#if (TCFG_DEC2TWS_ENABLE)
    u8 data[6];
    data[0] = cmd;
    data[1] = value;
    u32 bt_tmr = audio_bt_time_read(NULL, NULL) + 1000;
    memcpy(&data[2], &bt_tmr, 4);
    tws_api_send_data_to_sibling(data, sizeof(data), TWS_FUNC_ID_BOX_SYNC);
    /* if (cmd == TWS_BOX_NOTICE_A2DP_BACK_TO_BT_MODE */
    /* || cmd == TWS_BOX_A2DP_BACK_TO_BT_MODE_START) { */
    /* a2dp_dec_output_set_start_bt_time(bt_tmr); */
    /* } */
#else
    u8 data[2];

    data[0] = cmd;
    data[1] = value;
    tws_api_send_data_to_sibling(data, 2, TWS_FUNC_ID_BOX_SYNC);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 手机链接上 local tws设置
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void tws_local_connected(u8 state, u8 role)
{
    //state&TWS_STA_LOCAL_TWS_OPEN 对方是否开启local_tws
    log_info("tws_conn_state=0x%x\n", state);

    u8 cur_task = app_check_curr_task(APP_BT_TASK);

    if (cur_task) {
        TWS_LOCAL_IN_BT();
    } else {
        TWS_LOCAL_OUT_BT();
    }

    if (state & TWS_STA_LOCAL_TWS_OPEN) {
        TWS_REMOTE_OUT_BT();
    } else {
        TWS_REMOTE_IN_BT();
    }


    if (state & TWS_STA_LOCAL_TWS_OPEN) {
#if (TCFG_DEC2TWS_ENABLE)
        user_set_tws_box_mode(1);
#endif
        clear_current_poweron_memory_search_index(0);
        bt_user_priv_var.auto_connection_counter = 0;
        if (!cur_task) {
            gtws.device_role = TWS_ACTIVE_DEIVCE;
            log_debug("\n    ------  tws active device 0 ------   \n");
        } else {
            gtws.device_role = TWS_UNACTIVE_DEIVCE;
            log_debug("\n    ------  tws unactive device0 ------   \n");
        }
    } else if (!cur_task) {
        clear_current_poweron_memory_search_index(0);
        bt_user_priv_var.auto_connection_counter = 0;
        gtws.device_role = TWS_ACTIVE_DEIVCE;
        log_debug("\n    ------  tws active device 1 ------   \n");
    } else {
        if (role == TWS_ROLE_MASTER) {
            gtws.device_role = TWS_ACTIVE_DEIVCE;
            log_debug("\n    ------  tws active device 2 ------   \n");
        } else {
            gtws.device_role = TWS_UNACTIVE_DEIVCE;
            log_debug("\n    ------  tws unactive device2 ------   \n");
        }

    }
}

static void tws_connect_ble_adv(u8 phone_link_connection)
{
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
    bt_ble_icon_slave_en(1);
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        //master enable
        log_debug("master do icon_open\n");
        bt_ble_set_control_en(1);

        if (phone_link_connection) {
            bt_ble_icon_open(ICON_TYPE_RECONNECT);
        } else {
#if (TCFG_CHARGESTORE_ENABLE && !CONFIG_NO_DISPLAY_BUTTON_ICON)
            bt_ble_icon_open(ICON_TYPE_RECONNECT);
#else
            if (ble_icon_contrl.poweron_flag) { //上电标记
                if (bt_user_priv_var.auto_connection_counter > 0) {
                    //有回连手机动作
                    /* g_printf("ICON_TYPE_RECONNECT"); */
                    /* bt_ble_icon_open(ICON_TYPE_RECONNECT); //没按键配对的话，等回连成功的时候才显示电量。如果在这里显示，手机取消配对后耳机开机，会显示出按键的界面*/
                } else {
                    //没有回连，设可连接
                    /* g_printf("ICON_TYPE_INQUIRY"); */
                    bt_ble_icon_open(ICON_TYPE_INQUIRY);
                }

            }
#endif
        }
    } else {
        //slave disable
        bt_ble_set_control_en(0);
    }
    ble_icon_contrl.poweron_flag = 0;
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws链接上
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void tws_event_connected(struct bt_event *evt)
{
    u8 addr[4][6];
    int state;
    int pair_suss = 0;
    int role = evt->args[0];
    int phone_link_connection = evt->args[1];
    int reason = evt->args[2];
    char channel = 0;
    log_info("-----  tws_event_pair_suss: %x\n", gtws.state);

    clr_tws_local_back_role();


#if CONFIG_TWS_DISCONN_NO_RECONN
    tws_detach_by_remove_key = 0;
#endif


#if CONFIG_TWS_PAIR_BY_BOTH_SIDES
    bt_set_pair_code_en(0);
#endif

#if (CONFIG_TWS_USE_COMMMON_ADDR == 0)
    lmp_hci_write_local_address(bt_get_mac_addr());
#endif


    syscfg_read(CFG_TWS_REMOTE_ADDR, addr[0], 6);
    syscfg_read(CFG_TWS_COMMON_ADDR, addr[1], 6);
    tws_api_get_sibling_addr(addr[2]);
    tws_api_get_local_addr(addr[3]);

    /*
     * 记录对方地址
     */
    if (memcmp(addr[0], addr[2], 6)) {
        syscfg_write(CFG_TWS_REMOTE_ADDR, addr[2], 6);
        pair_suss = 1;
        log_info("rec tws addr\n");
    }
    if (memcmp(addr[1], addr[3], 6)) {
        syscfg_write(CFG_TWS_COMMON_ADDR, addr[3], 6);
        pair_suss = 1;
        log_info("rec comm addr\n");
    }

#if (CONFIG_TWS_USE_COMMMON_ADDR == 0)
    syscfg_write(VM_TWS_ROLE, &role, 1);
#endif

    if (pair_suss) {

        gtws.state = BT_TWS_PAIRED;

#if CONFIG_TWS_USE_COMMMON_ADDR
        extern void bt_update_mac_addr(u8 * addr);
        bt_update_mac_addr((void *)addr[3]);
#endif
    }

    /*
     * 记录左右声道
     */
    channel = tws_api_get_local_channel();
    if (channel != bt_tws_get_local_channel()) {
        syscfg_write(CFG_TWS_CHANNEL, &channel, 1);
    }
    log_debug("tws_local_channel: %c\n", channel);

    BT_STATE_TWS_CONNECTED(pair_suss, addr[3]);

#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
    if (tws_auto_pair_enable) {
        tws_auto_pair_enable = 0;
        tws_le_acc_generation_init();
    }
#endif

#ifdef CONFIG_NEW_BREDR_ENABLE
    if (channel == 'L') {
        syscfg_read(CFG_TWS_LOCAL_ADDR, addr[2], 6);
    }
    tws_api_set_quick_connect_addr(addr[2]);
#else
    tws_api_set_connect_aa(channel);
    tws_remote_state_clear();
#endif
    if (reason & (TWS_STA_ESCO_OPEN | TWS_STA_SBC_OPEN)) {
        if (role == TWS_ROLE_SLAVE) {
            gtws.state |= BT_TWS_AUDIO_PLAYING;
        }
    }

    if (!phone_link_connection ||
        (reason & (TWS_STA_ESCO_OPEN | TWS_STA_SBC_OPEN))) {
        pwm_led_clk_set(PWM_LED_CLK_BTOSC_24M);
    }
    /*
     * TWS连接成功, 主机尝试回连手机
     */
    if (gtws.connect_time) {
        if (role == TWS_ROLE_MASTER) {
            bt_user_priv_var.auto_connection_counter = gtws.connect_time;
        }
        gtws.connect_time = 0;
    }

    gtws.state &= ~BT_TWS_TIMEOUT;
    gtws.state |= BT_TWS_SIBLING_CONNECTED;

    state = evt->args[2];
    if (role == TWS_ROLE_MASTER) {
        if (!phone_link_connection) { //!(gtws.state & TWS_STA_PHONE_CONNECTED)) {    //还没连上手机
            log_info("[SYNC] LED SYNC");
            bt_tws_api_push_cmd(SYNC_CMD_LED_TWS_CONN_STATUS, TWS_SYNC_TIME_DO - 500);
        } else {
            bt_tws_api_push_cmd(SYNC_CMD_LED_PHONE_CONN_STATUS, TWS_SYNC_TIME_DO - 500);
        }
        if (bt_get_exit_flag() || ((get_call_status() == BT_CALL_HANGUP) && !(state & TWS_STA_SBC_OPEN))) {  //通话状态不播提示音
            log_info("[SYNC] TONE SYNC");
            bt_tws_api_push_cmd(SYNC_CMD_TWS_CONN_TONE, TWS_SYNC_TIME_DO / 2);
        }
#if BT_SYNC_PHONE_RING
        if (get_call_status() == BT_CALL_INCOMING) {
            if (bt_user_priv_var.phone_ring_flag && !bt_user_priv_var.inband_ringtone) {
                log_debug("<<<<<<<<<phone_con_sync_num_ring \n");
                bt_user_priv_var.phone_con_sync_ring = 1;//主机来电过程，从机连接加入
#if BT_PHONE_NUMBER
                bt_user_priv_var.phone_con_sync_num_ring = 1;//主机来电过程，从机连接加入
                /* tone_file_list_stop(); */
                bt_tws_sync_phone_num(NULL);
#endif
            }
        }
#endif
        bt_tws_sync_volume();
    }
#if TCFG_CHARGESTORE_ENABLE
    chargestore_sync_chg_level();//同步充电舱电量
#endif
    tws_sync_bat_level(); //同步电量到对耳
    bt_tws_delete_pair_timer();

#if (TCFG_DEC2TWS_ENABLE)
    tws_local_connected(state, role);
#endif

    if (phone_link_connection == 0) {   ///没有连接手机

        if (role == TWS_ROLE_MASTER) {
            bt_tws_connect_and_connectable_switch();
        } else {   //从机关闭各种搜索链接
            bt_close_tws_wait_pair();
            bt_close_phone_wait_pair();
        }
    }

    tws_connect_ble_adv(phone_link_connection);

#if SMART_BOX_EN
    extern void sync_setting_by_time_stamp(void);
    sync_setting_by_time_stamp();
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 发起配对超时, 等待手机连接
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void tws_event_search_timeout(struct bt_event *evt)
{
    gtws.state &= ~BT_TWS_SEARCH_SIBLING;


#if CONFIG_TWS_PAIR_BY_BOTH_SIDES
    bt_set_pair_code_en(0);
#endif


#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_CLICK
    tws_api_set_local_channel(bt_tws_get_local_channel());
    lmp_hci_write_local_address(gtws.addr);
#endif
    bt_tws_connect_and_connectable_switch();

}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws TWS连接超时
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void tws_event_connection_timeout(struct bt_event *evt)
{
    int role = evt->args[0];
    int phone_link_connection = evt->args[1];
    int reason = evt->args[2];
#if TCFG_TEST_BOX_ENABLE
    if (chargestore_get_testbox_status()) {
        return;
    }
#endif

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
    bt_ble_icon_slave_en(0);
#endif

    clr_tws_local_back_role();

    if (gtws.state & BT_TWS_UNPAIRED) {
        /*
         * 配对超时
         */
#if CONFIG_TWS_PAIR_BY_BOTH_SIDES
        bt_set_pair_code_en(0);
#endif


        gtws.state &= ~BT_TWS_SEARCH_SIBLING;
#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_CLICK
        tws_api_set_local_channel(bt_tws_get_local_channel());
        lmp_hci_write_local_address(gtws.addr);
#endif
        bt_tws_connect_and_connectable_switch();
    } else {
        if (phone_link_connection) {   ///如果手机连接上，就开启等待链接
            bt_tws_wait_sibling_connect(0);
        } else {
            if (gtws.state & BT_TWS_TIMEOUT) {
                gtws.state &= ~BT_TWS_TIMEOUT;
                gtws.connect_time = bt_user_priv_var.auto_connection_counter;
                bt_user_priv_var.auto_connection_counter = 0;
            }
            bt_tws_connect_and_connectable_switch();
        }
    }

#if (CONFIG_TWS_USE_COMMMON_ADDR == 0)
    u8 mac_addr[6];
    syscfg_read(CFG_BT_MAC_ADDR, mac_addr, 6);
    lmp_hci_write_local_address(mac_addr);
#endif

}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 断开链接
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void tws_event_connection_detach(struct bt_event *evt)
{
    u8 mac_addr[6];
    int role = evt->args[0];
    int phone_link_connection = evt->args[1];
    int reason = evt->args[2];

    clr_tws_local_back_role();

#if (TCFG_DEC2TWS_ENABLE)
    if (app_check_curr_task(APP_BT_TASK)) {
        //tws断开时清除local_tws标志，防止tws重新配对后状态错误
        user_set_tws_box_mode(0);
        // close localtws dec
        localtws_dec_close(0);
    }
#endif

    gtws.device_role = TWS_ACTIVE_DEIVCE;
    app_power_set_tws_sibling_bat_level(0xff, 0xff);

#if TCFG_CHARGESTORE_ENABLE
    chargestore_set_sibling_chg_lev(0xff);
#endif

    if ((!a2dp_get_status()) && (get_call_status() == BT_CALL_HANGUP)) {  //如果当前正在播歌，切回RC会导致下次从机开机回连后灯状态不同步
        pwm_led_clk_set((!TCFG_LOWPOWER_BTOSC_DISABLE) ? PWM_LED_CLK_RC32K : PWM_LED_CLK_BTOSC_24M);
        //pwm_led_clk_set(PWM_LED_CLK_RC32K);
    }

#if TCFG_TEST_BOX_ENABLE
    if (chargestore_get_testbox_status()) {
        return;
    }
#endif

    if (phone_link_connection) {
        ui_update_status(STATUS_BT_CONN);
        extern void power_event_to_user(u8 event);
        power_event_to_user(POWER_EVENT_POWER_CHANGE);
        user_send_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL);           //对耳断开后如果手机还连着，主动推一次电量给手机
    } else {
        ui_update_status(STATUS_BT_TWS_DISCONN);
    }

    if ((gtws.state & BT_TWS_SIBLING_CONNECTED) && (!app_var.goto_poweroff_flag)) {
#if CONFIG_TWS_POWEROFF_SAME_TIME
        extern u8 poweroff_sametime_flag;
        if (!app_var.goto_poweroff_flag && !poweroff_sametime_flag && !phone_link_connection && (reason != TWS_DETACH_BY_REMOVE_PAIRS)) { /*关机不播断开提示音*/
            tone_play_by_path(tone_table[IDEX_TONE_TWS_DISCONN], 1);
        }
#else
        if (!app_var.goto_poweroff_flag && !phone_link_connection && (reason != TWS_DETACH_BY_REMOVE_PAIRS)) {
            tone_play_by_path(tone_table[IDEX_TONE_TWS_DISCONN], 1);
        }
#endif
        gtws.state &= ~BT_TWS_SIBLING_CONNECTED;

#if (CONFIG_TWS_USE_COMMMON_ADDR == 0)
        syscfg_read(CFG_BT_MAC_ADDR, mac_addr, 6);
        lmp_hci_write_local_address(mac_addr);
#endif
    }

    if (reason == TWS_DETACH_BY_REMOVE_PAIRS) {
        gtws.state = BT_TWS_UNPAIRED;
        log_debug("<<<<<<<<<<<<<<<<<<<<<<tws detach by remove pairs>>>>>>>>>>>>>>>>>>>\n");

        if (!phone_link_connection) {
            bt_open_tws_wait_pair();
        } else {
#if (CONFIG_TWS_PAIR_ALL_WAY == 1)
            bt_open_tws_wait_pair();
#endif
        }

        return;
    }

    if (get_esco_coder_busy_flag()) {
        tws_api_connect_in_esco();
        return;
    }

    //非测试盒在仓测试，直连蓝牙
    if (reason == TWS_DETACH_BY_TESTBOX_CON) {
        log_debug(">>>>>>>>>>>>>>>>TWS_DETACH_BY_TESTBOX_CON<<<<<<<<<<<<<<<<<<<\n");
        gtws.state &= ~BT_TWS_PAIRED;
        gtws.state |= BT_TWS_UNPAIRED;
        //syscfg_read(CFG_BT_MAC_ADDR, mac_addr, 6);
        if (!phone_link_connection) {
#if CONFIG_TWS_USE_COMMMON_ADDR
            get_random_number(mac_addr, 6);
            lmp_hci_write_local_address(mac_addr);
#endif

            bt_tws_wait_pair_and_phone_connect();
        }
        return;
    }


#if CONFIG_TWS_DISCONN_NO_RECONN
    if (reason == TWS_DETACH_BY_REMOVE_NO_RECONN) {
        tws_detach_by_remove_key = 1;
        if ((tws_api_get_tws_state() & TWS_STA_PHONE_CONNECTED)) {
            bt_tws_wait_sibling_connect(0);
        } else {
            connect_and_connectable_switch_by_noreconn(0);
        }

        return;
    }
#endif




    if (phone_link_connection) {
        bt_tws_wait_sibling_connect(0);
    } else {
        if (reason == TWS_DETACH_BY_SUPER_TIMEOUT) {
            if (role == TWS_ROLE_SLAVE) {
                gtws.state |= BT_TWS_TIMEOUT;
                gtws.connect_time = bt_user_priv_var.auto_connection_counter;
                bt_user_priv_var.auto_connection_counter = 0;
            }
            bt_tws_connect_sibling(6);
        } else {
            bt_tws_connect_and_connectable_switch();
        }
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 手机断开链接
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void tws_event_phone_link_detach(struct bt_event *evt)
{

    int reason = evt->args[2];
#if TCFG_TEST_BOX_ENABLE
    if (chargestore_get_testbox_status()) {
        break;
    }
#endif

    sys_auto_shut_down_enable();

    if ((reason != 0x08) && (reason != 0x0b))   {
        bt_user_priv_var.auto_connection_counter = 0;
    } else {
        if (reason == 0x0b) {
            bt_user_priv_var.auto_connection_counter = (8 * 1000);
        }
    }


#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
    if (reason == 0x0b) {
        //CONNECTION ALREADY EXISTS
        ble_icon_contrl.miss_flag = 1;
    } else {
        ble_icon_contrl.miss_flag = 0;
    }
#endif

    bt_tws_connect_and_connectable_switch();


}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws 清除配对信息
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void tws_event_remove_pairs(struct bt_event *evt)
{
    tone_play_by_path(tone_table[IDEX_TONE_TWS_DISCONN], 1);
    bt_tws_remove_pairs();
    app_power_set_tws_sibling_bat_level(0xff, 0xff);
#if TCFG_CHARGESTORE_ENABLE
    chargestore_set_sibling_chg_lev(0xff);
#endif
    pwm_led_clk_set((!TCFG_LOWPOWER_BTOSC_DISABLE) ? PWM_LED_CLK_RC32K : PWM_LED_CLK_BTOSC_24M);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 拨号
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void tws_event_cmd_phone_num_tone()
{
    if (bt_user_priv_var.phone_con_sync_num_ring) {
        return;
    }
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        if (bt_user_priv_var.phone_timer_id) {
            sys_timeout_del(bt_user_priv_var.phone_timer_id);
            bt_user_priv_var.phone_timer_id = 0;
        }
    }
    if (bt_user_priv_var.phone_ring_flag) {
        phone_num_play_timer(NULL);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 铃声
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void tws_event_cmd_phone_ring_tone()
{
    if (bt_user_priv_var.phone_ring_flag) {
        if (bt_user_priv_var.phone_con_sync_ring) {
            if (phone_ring_play_start()) {
                bt_user_priv_var.phone_con_sync_ring = 0;
                if (tws_api_get_role() == TWS_ROLE_MASTER) {
                    //播完一声来电铃声之后,关闭aec、msbc_dec之后再次同步播来电铃声
                    /* bt_tws_api_push_cmd(SYNC_CMD_PHONE_RING_TONE, TWS_SYNC_TIME_DO * 2 + 400); */
                }
            }
        } else {
            phone_ring_play_start();
        }
    }
}


static void tws_event_cmd_sync_num_ring_tone()
{
    if (bt_user_priv_var.phone_ring_flag) {
        if (phone_ring_play_start()) {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                //播完一声来电铃声之后,关闭aec、msbc_dec之后再次同步播来电铃声
                bt_user_priv_var.phone_con_sync_ring = 0;
                /* bt_tws_api_push_cmd(SYNC_CMD_PHONE_RING_TONE, TWS_SYNC_TIME_DO * 2 + 400); */
            }
            bt_user_priv_var.phone_con_sync_num_ring = 1;

        }
    }
}

static void tws_event_cmd_led_tws_conn_status()
{
    pwm_led_mode_set(PWM_LED_ALL_OFF);
    ui_update_status(STATUS_BT_TWS_CONN);
}

static void tws_event_cmd_led_phone_conn_status()
{
    pwm_led_mode_set(PWM_LED_ALL_OFF);
    ui_update_status(STATUS_BT_CONN);
}

static void tws_event_cmd_led_phone_disconn_status()
{
    ui_update_status(STATUS_BT_TWS_CONN);
    if (!app_var.goto_poweroff_flag) { /*关机不播断开提示音*/
        /* tone_play(TONE_DISCONN); */
        if (is_tws_all_in_bt()) {
            tone_play_by_path(tone_table[IDEX_TONE_BT_DISCONN], 1);
        }
    }
}

static void tws_event_cmd_max_vol()
{

#if TCFG_MAX_VOL_PROMPT
    tone_play_by_path(tone_table[IDEX_TONE_MAX_VOL], 0);
#endif
}

static void tws_event_cmd_earphone_charge_start()
{
    if (a2dp_get_status() != BT_MUSIC_STATUS_STARTING) {
        /* g_printf("TWS MUSIC PLAY"); */
        user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
    }
}

static void tws_event_cmd_irsensor_event_near()
{
    if (a2dp_get_status() != BT_MUSIC_STATUS_STARTING) {
        /* g_printf("TWS MUSIC PLAY"); */
        user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
    }
}


static void tws_event_cmd_irsensor_event_far()
{
    if (a2dp_get_status() == BT_MUSIC_STATUS_STARTING) {
        /* r_printf("STOP...\n"); */
        user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL);
    }
}


void tws_event_cmd_mode_change(void)
{
    if (get_bt_init_status() && (bt_get_task_state() == 0)) {
#if TWFG_APP_POWERON_IGNORE_DEV
        if ((timer_get_ms() - app_var.start_time) > TWFG_APP_POWERON_IGNORE_DEV)
#endif//TWFG_APP_POWERON_IGNORE_DEV

        {
            printf("KEY_CHANGE_MODE\n");
            app_task_switch_next();
        }
    }
}

extern void sys_enter_soft_poweroff(void *priv);
static void tws_poweroff_same_time_tone_end(void *priv, int flag)
{
    //提示音结束， 才进入关机流程
    u32 index = (u32)priv;
    switch (index) {
    case IDEX_TONE_POWER_OFF:
        ///提示音播放结束， 启动播放器播放
        sys_enter_soft_poweroff(NULL);
        break;
    default:
        printf("%s callback err!!!!!!\n", __FUNCTION__);
        break;
    }
}

static void tws_poweroff_same_time_deal(void)
{
    ///先播放提示音， 保证同时播放

    tws_going_poweroff  = 1;

    int ret = tone_play_with_callback_by_name(tone_table[IDEX_TONE_POWER_OFF], 1,
              tws_poweroff_same_time_tone_end, (void *)IDEX_TONE_POWER_OFF);
    if (ret) {
        sys_enter_soft_poweroff(NULL);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws 主从同步调用函数处理
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void tws_event_sync_fun_cmd(struct bt_event *evt)
{
    int reason = evt->args[2];
    switch (reason) {
    case  SYNC_CMD_TWS_CONN_TONE:
        tone_play_by_path(tone_table[IDEX_TONE_TWS_CONN], 1);
        break;
    case  SYNC_CMD_PHONE_CONN_TONE:
        tone_play_by_path(tone_table[IDEX_TONE_BT_CONN], 1);
        break;
    case   SYNC_CMD_PHONE_NUM_TONE:
        tws_event_cmd_phone_num_tone();
        break;
    case  SYNC_CMD_PHONE_RING_TONE:
        tws_event_cmd_phone_ring_tone();
        break;
    case  SYNC_CMD_PHONE_SYNC_NUM_RING_TONE:
        tws_event_cmd_sync_num_ring_tone();
        break;

    case   SYNC_CMD_LED_TWS_CONN_STATUS:
        tws_event_cmd_led_tws_conn_status();
        break;
    case SYNC_CMD_LED_PHONE_CONN_STATUS:
        tws_event_cmd_led_phone_conn_status();
        break;
    case SYNC_CMD_LED_PHONE_DISCONN_STATUS:
        tws_event_cmd_led_phone_disconn_status();
        break;

    case  SYNC_CMD_POWER_OFF_TOGETHER:
        //sys_enter_soft_poweroff(NULL);
        tws_poweroff_same_time_deal();
        break;
    case   SYNC_CMD_MAX_VOL:
        tws_event_cmd_max_vol();
        break;

    case SYNC_CMD_MODE_BT :
    case SYNC_CMD_MODE_MUSIC :
    case SYNC_CMD_MODE_LINEIN:
    case SYNC_CMD_MODE_FM:
    case SYNC_CMD_MODE_PC:
    case SYNC_CMD_MODE_ENC:
    case SYNC_CMD_MODE_RTC:
    case SYNC_CMD_MODE_SPDIF:
#if (TCFG_TONE2TWS_ENABLE)
        /* app_mode_tone_play_by_tws(reason); */
#endif
        break;

    case  SYNC_CMD_LOW_LATENCY_ENABLE:
        earphone_a2dp_codec_set_low_latency_mode(1);
        break;
    case   SYNC_CMD_LOW_LATENCY_DISABLE:
        earphone_a2dp_codec_set_low_latency_mode(0);
        break;
    case    SYNC_CMD_EARPHONE_CHAREG_START:
        tws_event_cmd_earphone_charge_start() ;
        break;
    case    SYNC_CMD_IRSENSOR_EVENT_NEAR:
        tws_event_cmd_irsensor_event_near() ;
        break;
    case    SYNC_CMD_IRSENSOR_EVENT_FAR:
        tws_event_cmd_irsensor_event_far() ;
        break;

    case SYNC_CMD_MODE_CHANGE:
        tws_event_cmd_mode_change();
        break;
    default :
        break;
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws本地解码状态设置
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void tws_event_sync_fun_tranid(struct bt_event *evt)
{
    int reason = evt->args[2];
    if (reason == SYNC_CMD_BOX_EXIT_BT) {
#if (TCFG_DEC2TWS_ENABLE)
        user_set_tws_box_mode(1);
#endif
        log_debug("\n    ------   TWS_UNACTIVE_DEIVCE \n");
        TWS_LOCAL_IN_BT();
        TWS_REMOTE_OUT_BT();
        gtws.device_role =  TWS_UNACTIVE_DEIVCE;
        tws_local_back_to_bt_mode(TWS_BOX_EXIT_BT, gtws.device_role);
    } else if (reason == SYNC_CMD_BOX_INIT_EXIT_BT) {
        TWS_LOCAL_OUT_BT();
        TWS_REMOTE_IN_BT();
#if (TCFG_DEC2TWS_ENABLE)
        user_set_tws_box_mode(2);
#endif
        log_debug("\n    ------   TWS_ACTIVE_DEIVCE \n");
        gtws.device_role =  TWS_ACTIVE_DEIVCE;
        tws_local_back_to_bt_mode(TWS_BOX_EXIT_BT, gtws.device_role);
    } else if (reason == SYNC_CMD_BOX_ENTER_BT) {
        log_debug("----  SYNC_CMD_BOX_ENTER_BT");
        TWS_LOCAL_IN_BT();
        TWS_REMOTE_IN_BT();
#if (TCFG_DEC2TWS_ENABLE)
        user_set_tws_box_mode(0);
#endif
        tws_local_back_to_bt_mode(TWS_BOX_ENTER_BT, gtws.device_role);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws本地解码启动
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void tws_event_local_media_start(struct bt_event *evt)
{
    if (false == app_check_curr_task(APP_BT_TASK)) {
        /* log_debug("is not bt mode \n"); */
        return;
    }
    if (a2dp_dec_close()) {
        /* tws_api_local_media_trans_clear(); */
    }
    localtws_dec_open(evt->value);
}


static void tws_event_local_media_stop(struct bt_event *evt)
{
    if (false == app_check_curr_task(APP_BT_TASK)) {
        /* log_debug("is not bt mode \n"); */
        return;
    }
    if (localtws_dec_close(0)) {
    }
}


static void tws_event_role_switch(struct bt_event *evt)
{
    int role = evt->args[0];
    if (role == TWS_ROLE_MASTER) {

    } else {

    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws通话音量同步
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void tws_event_esco_add_connect(struct bt_event *evt)
{
    int role = evt->args[0];
    if (role == TWS_ROLE_MASTER) {
        bt_tws_sync_volume();
    }
}

#if (CONFIG_TWS_USE_COMMMON_ADDR == 0)
int tws_host_get_local_role()
{
    int role = 0;

    if (syscfg_read(VM_TWS_ROLE, &role, 1) == 1) {
        if (role == TWS_ROLE_SLAVE) {


            //printf("\n\n\nrole is slave\n\n\n");

            return TWS_ROLE_SLAVE;
        }
    }
    return TWS_ROLE_MASTER;
}
#endif


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙tws tws事件状态处理函数
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_tws_connction_status_event_handler(struct bt_event *evt)
{
    u8 ret = 0;
    u8 addr[4][6];
    int state;
    int pair_suss = 0;
    int role = evt->args[0];
    int phone_link_connection = evt->args[1];
    int reason = evt->args[2];
    u16 random_num = 0;
    char channel = 0;
    u8 mac_addr[6];

    log_info("tws-user: role= %d, phone_link_connection %d, reason=%d,event= %d\n",
             role, phone_link_connection, reason, evt->event);

    switch (evt->event) {
    case TWS_EVENT_CONNECTED:
        log_info(" TWS_EVENT_CONNECTED \n");
        tws_event_connected(evt);
        ret = 1;
        break;
    case TWS_EVENT_SEARCH_TIMEOUT:
        log_info(" TWS_EVENT_SEARCH_TIMEOUT \n");
        tws_event_search_timeout(evt);
        ret = 1;
        break;
    case TWS_EVENT_CONNECTION_TIMEOUT:
        log_info(" TWS_EVENT_CONNECTION_TIMEOUT \n");
        tws_event_connection_timeout(evt);
        ret = 1;
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        log_info(" TWS_EVENT_CONNECTION_DETACH \n");
        tws_event_connection_detach(evt);
        ret = 1;
        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        log_info(" TWS_EVENT_PHONE_LINK_DETACH \n");
        tws_event_phone_link_detach(evt);
        ret = 1;
        break;
    case TWS_EVENT_REMOVE_PAIRS:
        log_info(" TWS_EVENT_REMOVE_PAIRS \n");
        tws_event_remove_pairs(evt);
        ret = 1;
        break;
    case TWS_EVENT_SYNC_FUN_CMD:
        log_info(" TWS_EVENT_SYNC_FUN_CMD \n");
        tws_event_sync_fun_cmd(evt);
        ret = 1;
        break;
    case TWS_EVENT_SYNC_FUN_TRANID:
        log_info(" TWS_EVENT_SYNC_FUN_TRANID \n");
        tws_event_sync_fun_tranid(evt);
        ret = 1;
        break;
#if (TCFG_DEC2TWS_ENABLE)
    case TWS_EVENT_LOCAL_MEDIA_START:
        log_info(" TWS_EVENT_LOCAL_MEDIA_START \n");
        tws_event_local_media_start(evt);
        ret = 1;
        break;
    case TWS_EVENT_LOCAL_MEDIA_STOP:
        log_info(" TWS_EVENT_LOCAL_MEDIA_STOP \n");
        tws_event_local_media_stop(evt);
        ret = 1;
        break;
#endif
    case TWS_EVENT_ROLE_SWITCH:
        log_info(" TWS_EVENT_ROLE_SWITCH \n");
        tws_event_role_switch(evt);
        ret = 1;
        break;
    case TWS_EVENT_ESCO_ADD_CONNECT:
        log_info(" TWS_EVENT_ESCO_ADD_CONNECT \n");
        tws_event_esco_add_connect(evt);
        ret = 1;
        break;
    }
    return ret;
}


#endif
#endif
