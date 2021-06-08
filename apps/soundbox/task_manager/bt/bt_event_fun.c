

#include "system/includes.h"
#include "media/includes.h"

#include "app_config.h"
#include "app_task.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "btstack/btstack_error.h"
#include "btctrler/btctrler_task.h"
#include "classic/hci_lmp.h"

#include "bt/bt_tws.h"
#include "bt/bt_ble.h"
#include "bt/bt.h"
#include "bt/vol_sync.h"
#include "bt/bt_emitter.h"
#include "bt_common.h"
#include "aec_user.h"
#include "soundbox.h"

#include "math.h"
#include "spp_user.h"


#include "app_chargestore.h"
#include "app_charge.h"
#include "app_main.h"
#include "app_power_manage.h"
#include "user_cfg.h"

#include "asm/pwm_led.h"
#include "asm/timer.h"
#include "asm/hwi.h"
#include "cpu.h"

#include "ui/ui_api.h"
#include "ui_manage.h"
#include "ui/ui_style.h"

#include "key_event_deal.h"
#include "clock_cfg.h"
#include "gSensor/gSensor_manage.h"
#include "soundcard/soundcard.h"

#include "audio_dec.h"
#include "tone_player.h"
#include "dac.h"
#include "audio_recorder_mix.h"


#define LOG_TAG_CONST        BT
#define LOG_TAG             "[BT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_APP_BT_EN

#define __this 	(&app_bt_hdl)

static int bt_switch_back_timer = 0;

/*************************************************************
    此文件函数主要是蓝牙模式各种状态处理

    蓝牙播歌通话各种状态处理

 	 蓝牙协议栈事件处理
**************************************************************/

u8 get_bt_init_status()
{
    return __this->init_ok;
}

u8 get_bt_back_flag()
{
    return __this->bt_back_flag;
}

void set_bt_back_flag(u8 flag)
{
    __this->bt_back_flag = flag;
}


void clr_tws_local_back_role(void)
{
    __this->tws_local_back_role = 0;
}


void bt_reverb_status_change(struct bt_event *bt)
{
}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式切换等待解码停止
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int bt_switch_back(void *p)
{
    if (!__this->call_flag) {
        return 0;
    }

    if (bt_switch_back_timer) {
        sys_timeout_del(bt_switch_back_timer);
        bt_switch_back_timer = 0;
    }

    if (bt_must_work()) {
        bt_switch_back_timer = sys_timeout_add(NULL, bt_switch_back, 500);
        return 0;
    }

    if (bt_phone_dec_is_running()) {
        bt_switch_back_timer = sys_timeout_add(NULL, bt_switch_back, 500);
        return 0;
    }
    /* __this->call_flag = 0; */
    __this->bt_back_flag = 2;
    app_task_switch_back();
    return 0;
}

u8 get_a2dp_start_flag()
{
    return __this->a2dp_start_flag;
}


u8 bt_sco_state(void)
{
    return bt_user_priv_var.phone_call_dec_begin;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙eir信息追加数据
   @param
   @return
   @note    注释掉add_data_2_eir函数就不追加数据
   			追加数据不可以超150byte
*/
/*----------------------------------------------------------------------------*/
u8 const eir_more_data_uuid[] = {
    0x0F, 0x03, 0x00, 0x12, 0x1F, 0x11, 0x2F,
    0x11, 0x0A, 0x11, 0x0C, 0x11, 0x32, 0x11, 0x01, 0x18
};

/* u8 add_data_2_eir(u8 *eir)
{
	u8 len = sizeof(eir_more_data_uuid);
	memcpy(eir,eir_more_data_uuid,len);

	return len;
}  */

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙电话本获取回调函数
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void phonebook_packet_handler(u8 type, const u8 *name, const u8 *number, const u8 *date)
{
    static u16 number_cnt = 0;
    log_debug("NO.%d:", number_cnt);
    number_cnt++;
    log_debug("type:%d ", type);
    if (type == 0xff) {
        number_cnt = 0;
    }
    if (name) {
        log_debug(" NAME:%s  ", name);
    }
    if (number) {
        log_debug("number:%s  ", number);
    }
    if (date) {
        log_debug("date:%s ", date);
    }
    /* putchar('\n'); */
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式led状态更新
   @param
   @return   status : 蓝牙状态
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_set_led_status(u8 status)
{
    static u8 bt_status = STATUS_BT_INIT_OK;

    u8 app = app_get_curr_task();
    if (status) {
        bt_status = status;
    }
    if (app == APP_BT_TASK) { //蓝牙模式或者state_machine
        pwm_led_mode_set(PWM_LED_ALL_OFF);
        ui_update_status(bt_status);
    }
}


/*************************************************************
                开关可发现可连接的函数接口
**************************************************************/
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙可发现可连接使能
   @param    enable 1:开启   0：关闭
   @return
   @note     函数根据状态开启，开启tws后该函数不起作用
   			 1拖2的时候会限制开启
*/
/*----------------------------------------------------------------------------*/
void bt_wait_phone_connect_control(u8 enable)
{
#if TCFG_USER_TWS_ENABLE
    return;
#endif

    if (enable && app_var.goto_poweroff_flag && (__this->exiting)) {
        return;
    }

    if (enable) {
        log_debug("is_1t2_connection:%d \t total_conn_dev:%d\n", is_1t2_connection(), get_total_connect_dev());
        if (is_1t2_connection()) {
            /*达到最大连接数，可发现(0)可连接(0)*/
            user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        } else {
            if (get_total_connect_dev() == 1) {
                /*支持连接2台，只连接一台的情况下，可发现(0)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            } else {
                /*可发现(1)可连接(1)*/
                BT_STATE_SET_PAGE_SCAN_ENABLE();
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            }
        }
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙可发现可连接\回链轮询使能
   @param    p 1:开启轮询   0：不开启轮询,只开启可发现可连接
   @return
   @note     auto_connection_counter:回链设备的总时间
*/
/*----------------------------------------------------------------------------*/
int bt_wait_connect_and_phone_connect_switch(void *p)
{
    int ret = 0;
    int timeout = 0;
    s32 wait_timeout;

    if (__this->exiting) {
        return 0;
    }

    log_debug("connect_switch: %d, %d\n", (int)p, bt_user_priv_var.auto_connection_counter);

    if (bt_user_priv_var.auto_connection_timer) {
        sys_timeout_del(bt_user_priv_var.auto_connection_timer);
        bt_user_priv_var.auto_connection_timer = 0;
    }

    if (bt_user_priv_var.auto_connection_counter <= 0) {
        bt_user_priv_var.auto_connection_timer = 0;
        bt_user_priv_var.auto_connection_counter = 0;


        BT_STATE_CANCEL_PAGE_SCAN();

        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        if (get_current_poweron_memory_search_index(NULL)) {
            bt_init_ok_search_index();
            return bt_wait_connect_and_phone_connect_switch(0);
        } else {
            bt_wait_phone_connect_control(1);
            return 0;
        }
    }
    /* log_info(">>>phone_connect_switch=%d\n",bt_user_priv_var.auto_connection_counter ); */
    if ((int)p == 0) {  //// 根据地址回链
        if (bt_user_priv_var.auto_connection_counter) {
            timeout = 6000;     ////设置回链6s
            bt_wait_phone_connect_control(0);
            user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, bt_user_priv_var.auto_connection_addr);
            ret = 1;
            bt_user_priv_var.auto_connection_counter -= timeout ;
        }
    } else {
        timeout = 2000;   ////设置开始被发现被连接2s
        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        bt_wait_phone_connect_control(1);
    }
    /* if (bt_user_priv_var.auto_connection_counter) { */
    /*     bt_user_priv_var.auto_connection_counter -= timeout ; */
    /*     #<{(| log_info("do=%d\n", bt_user_priv_var.auto_connection_counter); |)}># */
    /* } */
    bt_user_priv_var.auto_connection_timer = sys_timeout_add((void *)(!(int)p),
            bt_wait_connect_and_phone_connect_switch, timeout);

    return ret;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙关闭可发现可连接
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_close_page_scan(void *p)
{
    /* log_info(">>>%s\n", __func__); */
    bt_wait_phone_connect_control(0);
    sys_timer_del(app_var.auto_stop_page_scan_timer);
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙通话时候设置不让系统进入idle状态,不让系统进powerdown
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 check_phone_income_idle(void)
{
    if (bt_user_priv_var.phone_ring_flag) {
        return 0;
    }
    return 1;
}
REGISTER_LP_TARGET(phone_incom_lp_target) = {
    .name       = "phone_check",
    .is_idle    = check_phone_income_idle,
};


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙连接配置，提供app层用户可以输入配对鉴定key
   @param    key :配对需要输入的数字
   @return
   @note     配对需要输入6位数字的时候，按照顺序从左到右一个个输入
*/
/*----------------------------------------------------------------------------*/
void bt_send_keypress(u8 key)
{
    user_send_cmd_prepare(USER_CTRL_KEYPRESS, 1, &key);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙连接配置，提供app层用户可以输入确定或者否定
   @param    en 0:否定   1:确定
   @return
   @note     在连接过程中类似手机弹出 确定和否定 按键，可以供用户界面设置
*/
/*----------------------------------------------------------------------------*/
void bt_send_pair(u8 en)
{
    user_send_cmd_prepare(USER_CTRL_PAIR, 1, &en);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙获取vm连接记录信息
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_init_ok_search_index(void)
{
    if (!bt_user_priv_var.auto_connection_counter && get_current_poweron_memory_search_index(bt_user_priv_var.auto_connection_addr)) {
        /* log_info("bt_wait_connect_and_phone_connect_switch\n"); */
        clear_current_poweron_memory_search_index(1);
        bt_user_priv_var.auto_connection_counter = POWERON_AUTO_CONN_TIME * 1000; //8000ms
        BT_STATE_GET_CONNECT_MAC_ADDR();
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙获取样机当前电量
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_get_battery_value()
{
    //取消默认蓝牙定时发送电量给手机，需要更新电量给手机使用USER_CTRL_HFP_CMD_UPDATE_BATTARY命令
    /*电量协议的是0-9个等级，请比例换算*/
    return get_cur_battery_level();
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙spp 协议数据 回调
   @param    packet_type:数据类型
  			 ch         :
			 packet     :数据缓存
			size        ：数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void spp_data_handler(u8 packet_type, u16 ch, u8 *packet, u16 size)
{
    switch (packet_type) {
    case 1:
        log_debug("spp connect\n");
        break;
    case 2:
        log_debug("spp disconnect\n");
        break;
    case 7:
        //log_info("spp_rx:");
        //put_buf(packet,size);
#if AEC_DEBUG_ONLINE
        aec_debug_online(packet, size);
#endif
        break;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙音乐同步,设置音量
   @param    volume : 设置音量
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_set_music_device_volume(int volume)
{
#if (SMART_BOX_EN)
    extern bool smartbox_set_device_volume(int volume);
    if (smartbox_set_device_volume(volume)) {
        return;
    }
#endif

    if (true == app_check_curr_task(APP_BT_TASK)) {
        set_music_device_volume(volume);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙通话音量同步,设置音量
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void phone_sync_vol(void *priv)
{
    log_debug("phone_sync_vol\n");
    bt_user_priv_var.phone_vol = 14;
    user_send_cmd_prepare(USER_CTRL_HFP_CALL_SET_VOLUME, 1, &bt_user_priv_var.phone_vol);
    user_send_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_UP, 0, NULL);
    bt_user_priv_var.phone_vol = 15;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙获取在连接设备名字回调
   @param    status : 1：获取失败   0：获取成功
			 addr   : 配对设备地址
			 name   :配对设备名字
   @return
   @note     需要连接上设备后发起USER_CTRL_READ_REMOTE_NAME
   			 命令来
*/
/*----------------------------------------------------------------------------*/
void bt_read_remote_name(u8 status, u8 *addr, u8 *name)
{
    if (status) {
        log_debug("remote_name fail \n");
    } else {
        log_debug("remote_name : %s \n", name);
    }

    log_debug_hexdump(addr, 6);

#if TCFG_USER_EMITTER_ENABLE
    emitter_search_noname(status, addr, name);
#endif
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙歌词信息获取回调
   @param
   @return
   @note
   const u8 more_avctp_cmd_support = 1;置上1
   需要在void bredr_handle_register()注册回调函数
   要动态获取播放时间的，可以发送USER_CTRL_AVCTP_OPID_GET_PLAY_TIME命令就可以了
   要半秒或者1秒获取就做个定时发这个命令
*/
/*----------------------------------------------------------------------------*/
void user_get_bt_music_info(u8 type, u32 time, u8 *info, u16 len)
{
    //profile define type: 1-title 2-artist name 3-album names 4-track number 5-total number of tracks 6-genre  7-playing time
    //JL define 0x10-total time , 0x11 current play position
    u8  min, sec;
    //printf("type %d\n", type );
    if ((info != NULL) && (len != 0)) {
        log_debug(" %s \n", info);
    }
    if (time != 0) {
        min = time / 1000 / 60;
        sec = time / 1000 - (min * 60);
        log_debug(" time %d %d\n ", min, sec);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙通话开始丢包数
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
#if ESCO_DUMP_PACKET_ADJUST
u8 get_esco_packet_dump(void)
{
    //log_info("esco_dump_packet:%d\n", esco_dump_packet);
    return __this->esco_dump_packet;
}
#endif






/*************************************************************
               蓝牙丢高级音频数据
**************************************************************/
static u32 bt_timer = 0;
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙丢音频数据包
   @param
   @return
   @note     如果在播放提示音等打断高级音频解码的话，导致蓝牙底层数据满了，
  			 太久没有接收手机数据会导致手机断开链接
*/
/*----------------------------------------------------------------------------*/
static void bt_a2dp_drop_frame(void *p)
{
    int len;
    u8 *frame;
    /* putchar('@'); */

#if TCFG_DEC2TWS_ENABLE
    if (tone_get_status() || localtws_dec_is_open()) {
#else
    if (tone_get_status()) {
#endif
        bt_timer = sys_timeout_add(NULL, bt_a2dp_drop_frame, 100);
        __a2dp_drop_frame(NULL);
    } else {
        local_irq_disable();
        bt_timer = 0;
        local_irq_enable();
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙启动音频数据包
   @param
   @return
   @note     如果在播放提示音等打断高级音频解码的话，导致蓝牙底层数据满了，
  			 太久没有接收手机数据会导致手机断开链接
*/
/*----------------------------------------------------------------------------*/
void bt_drop_a2dp_frame_start(void)
{
    log_debug("bt_drop_frame_start %d\n", bt_timer);
    local_irq_disable();
    if (bt_timer == 0) {
        bt_timer = sys_timeout_add(NULL, bt_a2dp_drop_frame, 100);
    }
    local_irq_enable();

}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙停止音频数据包
   @param
   @return
   @note     如果在播放提示音等打断高级音频解码的话，导致蓝牙底层数据满了，
  			 太久没有接收手机数据会导致手机断开链接
*/
/*----------------------------------------------------------------------------*/
void bt_drop_a2dp_frame_stop()
{
    log_debug("bt_drop_frame_stop %d\n", bt_timer);
    local_irq_disable();
    if (bt_timer) {
        sys_timeout_del(bt_timer);
        bt_timer = 0;
    }
    extern void bt_stop_a2dp_slience_detect();
    bt_stop_a2dp_slience_detect();
    local_irq_enable();
}





/*************************************************************
  	     蓝牙模式app层检测空闲是否可以进入sniff模式
**************************************************************/
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙退出sniff模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_check_exit_sniff()
{
    sys_timeout_del(__this->exit_sniff_timer);
    __this->exit_sniff_timer = 0;

#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
#endif
    user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙检测是否空闲，空闲就发起sniff
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_check_enter_sniff()
{
    struct sniff_ctrl_config_t sniff_ctrl_config;
    u8 addr[12];
    u8 conn_cnt = 0;
    u8 i = 0;
    /*putchar('H');*/
    conn_cnt = bt_api_enter_sniff_status_check(SNIFF_CNT_TIME, addr);

    ASSERT(conn_cnt <= 2);

    for (i = 0; i < conn_cnt; i++) {
        log_debug("-----USER SEND SNIFF IN %d %d\n", i, conn_cnt);
        sniff_ctrl_config.sniff_max_interval = SNIFF_MAX_INTERVALSLOT;
        sniff_ctrl_config.sniff_mix_interval = SNIFF_MIN_INTERVALSLOT;
        sniff_ctrl_config.sniff_attemp = SNIFF_ATTEMPT_SLOT;
        sniff_ctrl_config.sniff_timeout  = SNIFF_TIMEOUT_SLOT;
        memcpy(sniff_ctrl_config.sniff_addr, addr + i * 6, 6);
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            return;
        }
#endif
        user_send_cmd_prepare(USER_CTRL_SNIFF_IN, sizeof(struct sniff_ctrl_config_t), (u8 *)&sniff_ctrl_config);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙启动 检测空闲 发起sniff
   @param   enable : 1 启动  0:关闭
   			addr:要和对应设备进入sniff 状态的地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void sys_auto_sniff_controle(u8 enable, u8 *addr)
{
#if (TCFG_BT_SNIFF_ENABLE == 0)
    return;
#endif

    if (addr) {
        if (bt_api_conn_mode_check(enable, addr) == 0) {
            log_debug("sniff ctr not change\n");
            return;
        }
    }

    if (enable) {
        if (get_total_connect_dev() == 0) {
            return;
        }

        if (addr) {
            log_debug("sniff cmd timer init\n");
            user_cmd_timer_init();
        }

        if (bt_user_priv_var.sniff_timer == 0) {
            log_debug("check_sniff_enable\n");
            bt_user_priv_var.sniff_timer = sys_timer_add(NULL, bt_check_enter_sniff, 1000);
        }
    } else {
        if (get_total_connect_dev() > 0) {
            return;
        }
        if (addr) {
            log_debug("sniff cmd timer remove\n");
            remove_user_cmd_timer();
        }

        if (bt_user_priv_var.sniff_timer) {
            log_info("check_sniff_disable\n");
            sys_timeout_del(bt_user_priv_var.sniff_timer);
            bt_user_priv_var.sniff_timer = 0;

            if (__this->exit_sniff_timer == 0) {
                /* exit_sniff_timer = sys_timer_add(NULL, bt_check_exit_sniff, 5000); */
            }
        }
    }
}

void soft_poweroff_mode(u8 mode)
{
    __this->force_poweroff = mode;
}

/*************************************************************
  蓝牙模式关机处理
 **************************************************************/
/*----------------------------------------------------------------------------*/
/**@brief    软关机蓝牙模式处理
   @param
   @return
   @note     断开蓝牙链接\关闭所有状态
*/
/*----------------------------------------------------------------------------*/
void sys_enter_soft_poweroff(void *priv)
{
    int detach_phone = 1;
    struct sys_event clear_key_event = {.type =  SYS_KEY_EVENT, .arg = "key"};

    log_info("%s", __func__);

    bt_user_priv_var.emitter_or_receiver = 0;
    if (app_var.goto_poweroff_flag) {
        return;
    }

#if TCFG_BLUETOOTH_BACK_MODE
    if (__this->exit_flag == 1 && __this->init_ok == 1) {
        __this->cmd_flag = 1;
        app_task_switch_to(APP_BT_TASK);
    }
#endif

    ui_update_status(STATUS_POWEROFF);

    app_var.goto_poweroff_flag = 1;
    app_var.goto_poweroff_cnt = 0;


#if 0
    sys_key_event_disable();
    sys_event_clear(&clear_key_event);
    sys_auto_shut_down_disable();
    sys_auto_sniff_controle(0, NULL);

    BT_STATE_ENTER_SOFT_POWEROFF();


#if TCFG_CHARGESTORE_ENABLE
    if (chargestore_get_earphone_online() != 2)
#else
    if (1)
#endif
    {
#if TCFG_USER_TWS_ENABLE
        detach_phone = bt_tws_poweroff();
#endif
    }

    /* log_debug("detach_phone = %d\n", detach_phone); */

    if (detach_phone) {
        user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    }
#endif

    if (app_var.wait_exit_timer == 0) {
        app_var.wait_exit_timer = sys_timer_add(priv, wait_exit_btstack_flag, 50);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   自动关机开关
   @param
   @return
   @note    开启后，如果一段时间内没有连接等操作就会进入关机
*/
/*----------------------------------------------------------------------------*/
void sys_auto_shut_down_enable(void)
{

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_TRANS_DATA)
    extern u16 bt_ble_is_connected(void);
    if (bt_ble_is_connected()) {
        return;
    }
#endif


#if TCFG_AUTO_SHUT_DOWN_TIME
    log_debug("sys_auto_shut_down_enable\n");

    if (app_var.auto_shut_down_timer == 0) {

        u8 app = app_get_curr_task();
        if (app == APP_BT_TASK) {
            app_var.auto_shut_down_timer = sys_timeout_add(NULL, sys_enter_soft_poweroff, (app_var.auto_off_time * 1000));
        } else {
            /* log_debug("cur_app:%s, return", app->name); */
        }
    } else {//在切换到蓝牙任务APP_STA_START中，current_app为空
        sys_auto_shut_down_disable();
        app_var.auto_shut_down_timer = sys_timeout_add(NULL, sys_enter_soft_poweroff, (app_var.auto_off_time * 1000));
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief   关闭自动关机
   @param
   @return
   @note    链接上设备后会调用关闭
*/
/*----------------------------------------------------------------------------*/
void sys_auto_shut_down_disable(void)
{
#if TCFG_AUTO_SHUT_DOWN_TIME
    log_debug("sys_auto_shut_down_disable\n");
    if (app_var.auto_shut_down_timer) {
        sys_timeout_del(app_var.auto_shut_down_timer);
        app_var.auto_shut_down_timer = 0;
    }
#endif
}


/*************************************************************
                蓝牙模式提示音播放
**************************************************************/
static const u32 num0_9[] = {
    (u32)TONE_NUM_0,
    (u32)TONE_NUM_1,
    (u32)TONE_NUM_2,
    (u32)TONE_NUM_3,
    (u32)TONE_NUM_4,
    (u32)TONE_NUM_5,
    (u32)TONE_NUM_6,
    (u32)TONE_NUM_7,
    (u32)TONE_NUM_8,
    (u32)TONE_NUM_9,
} ;



static void number_to_play_list(char *num, u32 *lst)
{
    u8 i = 0;

    if (num) {
        for (; i < strlen(num); i++) {
            lst[i] = num0_9[num[i] - '0'] ;
        }
    }
    lst[i++] = (u32)TONE_REPEAT_BEGIN(-1);
    lst[i++] = (u32)TONE_RING;
    lst[i++] = (u32)TONE_REPEAT_END();
    lst[i++] = (u32)NULL;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙来电报号
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void phone_num_play_timer(void *priv)
{
    char *len_lst[34];

    if (get_call_status() == BT_CALL_HANGUP) {
        /* log_debug("hangup,--phone num play return\n"); */
        return;
    }
    log_info("%s\n", __FUNCTION__);

    if (bt_user_priv_var.phone_num_flag) {
        tone_play_stop();
        number_to_play_list((char *)(bt_user_priv_var.income_phone_num), len_lst);
        tone_file_list_play(len_lst, 1);
    } else {
        /*电话号码还没有获取到，定时查询*/
        bt_user_priv_var.phone_timer_id = sys_timeout_add(NULL, phone_num_play_timer, 200);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙来电报号启动
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void phone_num_play_start(void)
{
    /* check if support inband ringtone */
    if (!bt_user_priv_var.inband_ringtone) {
        bt_user_priv_var.phone_num_flag = 0;
        bt_user_priv_var.phone_timer_id = sys_timeout_add(NULL, phone_num_play_timer, 500);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙来电铃声播放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 phone_ring_play_start(void)
{
    char *len_lst[34];

    if (get_call_status() == BT_CALL_HANGUP) {
        /* log_debug("hangup,--phone ring play return\n"); */
        return 0;
    }
    log_info("%s\n", __FUNCTION__);
    /* check if support inband ringtone */
    if (!bt_user_priv_var.inband_ringtone) {
        tone_play_stop();
        number_to_play_list(NULL, len_lst);
        tone_file_list_play(len_lst, 1);
        return 1;
    }
    return 0;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙提示音播放统一接口
   @param    index:提示音序号
   			 preemption：是否打断/1:打断、0：不打断
		     priv :
   @return   播放是否成功
   @note
  			蓝牙模式播放提示音接口，防止提示音播放时候打开 a2dp 解码不及时，
  			导致底层 a2dp buff 阻塞，手机超时断开蓝牙.
*/
/*----------------------------------------------------------------------------*/
int bt_tone_play_index(u8 index, u8 preemption, void *priv)
{
    bt_drop_a2dp_frame_start();
    return tone_play_with_callback_by_name(tone_table[index], preemption, NULL, priv);
}

__attribute__((weak))
int earphone_a2dp_codec_get_low_latency_mode()
{
    return 0;
}

__attribute__((weak))
void earphone_a2dp_codec_set_low_latency_mode(int enable)
{
    return;
}

int bt_get_low_latency_mode()
{
    return earphone_a2dp_codec_get_low_latency_mode();
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙tws 低电处理
   @param
   @return
   @note    电量高的做主机，电量低的做从机
*/
/*----------------------------------------------------------------------------*/
void bt_set_low_latency_mode(int enable)
{
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            if (enable) {
                bt_tws_api_push_cmd(SYNC_CMD_LOW_LATENCY_ENABLE, 300);
            } else {
                bt_tws_api_push_cmd(SYNC_CMD_LOW_LATENCY_DISABLE, 300);
            }
        } else {
            earphone_a2dp_codec_set_low_latency_mode(enable);
        }
    }
#else
    earphone_a2dp_codec_set_low_latency_mode(enable);
#endif
}



/*************************************************************
             蓝牙模式协议栈状态事件处理
**************************************************************/
/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 事件过滤
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_status_event_filter(struct bt_event *bt)
{
    if (__this->init_ok &&  app_var.goto_poweroff_flag) {
        if (bt->event != BT_STATUS_A2DP_MEDIA_STOP) {
            return false;
        }
    }

#if (RECORDER_MIX_EN)
    recorder_mix_bt_status_event(bt);
#endif//RECORDER_MIX_EN

#if SOUNDCARD_ENABLE
    soundcard_bt_connect_status_event(bt);
#endif//SOUNDCARD_ENABLE

    UI_MSG_POST("bt_status:event=%4", bt->event);
    return true;
}

void  bt_status_init_ok(struct bt_event *bt)
{
    __this->init_ok = 1;
    if (__this->init_ok_time == 0) {
        __this->init_ok_time = timer_get_ms();
        __this->auto_exit_limit_time = POWERON_AUTO_CONN_TIME * 1000;
        /* tone_dec_wait_stop(3000); */
    }
#if TCFG_USER_BLE_ENABLE
    if (BT_MODE_IS(BT_BQB)) {
        ble_bqb_test_thread_init();
    } else {
        bt_ble_init();
    }
#endif

#if TCFG_USER_EMITTER_ENABLE
    ////发射器设置回调等
    void bt_emitter_init() ;
    bt_emitter_init();
    extern u8 emitter_search_result(char *name, u8 name_len, u8 * addr, u32 dev_class, char rssi);
    inquiry_result_handle_register(emitter_search_result);
    extern void emitter_or_receiver_switch(u8 flag);
    emitter_or_receiver_switch(BT_EMITTER_EN);
    return;
#endif



    bt_init_ok_search_index();
    bt_set_led_status(STATUS_BT_INIT_OK);
#if TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE
    chargestore_set_bt_init_ok(1);
#endif


#if ((CONFIG_BT_MODE == BT_BQB)||(CONFIG_BT_MODE == BT_PER))
    bt_wait_phone_connect_control(1);
#else
#if TCFG_USER_TWS_ENABLE
    bt_tws_poweron();
#else
    bt_wait_connect_and_phone_connect_switch(0);
#endif
#endif
}

void bt_status_init_ok_background(struct bt_event *bt)
{
    __this->init_ok = 1;

#if TCFG_USER_BLE_ENABLE
    if (BT_MODE_IS(BT_BQB)) {
        ble_bqb_test_thread_init();
    } else {
        bt_ble_init();
    }
#endif

#if TCFG_USER_EMITTER_ENABLE
    ////发射器设置回调等
    void bt_emitter_init() ;
    bt_emitter_init();
    extern u8 emitter_search_result(char *name, u8 name_len, u8 * addr, u32 dev_class, char rssi);
    inquiry_result_handle_register(emitter_search_result);

    extern void emitter_or_receiver_switch(u8 flag);
    emitter_or_receiver_switch(BT_EMITTER_EN);

    /* bt_user_priv_var.emitter_or_receiver = BT_EMITTER_EN; */
    return;
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 设备连接上状态
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_connect(struct bt_event *bt)
{

    sys_auto_sniff_controle(1, NULL);
    sys_auto_shut_down_disable();
#if SMART_BOX_EN
    memcpy(__this->last_connecting_addr, bt->args, 6);
    extern void smartbox_update_bt_emitter_connect_state(u8 state, u8 * addr);
    smartbox_update_bt_emitter_connect_state(1, __this->last_connecting_addr);
#endif



    /* UI_MSG_POST("bt_status:n=%4", 1); */

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
    {
        u8 connet_type;
        if (get_auto_connect_state(bt->args)) {
            connet_type = ICON_TYPE_RECONNECT;
        } else {
            connet_type = ICON_TYPE_CONNECTED;
        }

#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            bt_ble_icon_open(connet_type);
        } else {
            //maybe slave already open
            bt_ble_icon_close(0);
        }

#else
        bt_ble_icon_open(connet_type);
#endif
    }
#endif


#if (TCFG_BD_NUM == 2)
    if (get_current_poweron_memory_search_index(NULL) == 0) {
        bt_wait_phone_connect_control(1);
    }
#endif

#if TCFG_USER_TWS_ENABLE
    if (!get_bt_tws_connect_status()) {   //如果对耳还未连接就连上手机，要切换闪灯状态
        bt_set_led_status(STATUS_BT_CONN);
    }

    if (bt_tws_phone_connected()) {
        return ;
    }
#else
    bt_set_led_status(STATUS_BT_CONN);    //单台在此处设置连接状态,对耳的连接状态需要同步，在bt_tws.c中去设置
#if (TCFG_AUTO_STOP_PAGE_SCAN_TIME && TCFG_BD_NUM == 2)
    if (get_total_connect_dev() == 1) {   //当前有一台连接上了
        if (app_var.auto_stop_page_scan_timer == 0) {
            app_var.auto_stop_page_scan_timer = sys_timeout_add(NULL, bt_close_page_scan, (TCFG_AUTO_STOP_PAGE_SCAN_TIME * 1000)); //2
        }
    } else {
        if (app_var.auto_stop_page_scan_timer) {
            sys_timeout_del(app_var.auto_stop_page_scan_timer);
            app_var.auto_stop_page_scan_timer = 0;
        }
    }
#endif   //endif AUTO_STOP_PAGE_SCAN_TIME
#endif
    /* log_debug("tone status:%d\n", tone_get_status()); */
    if (get_call_status() == BT_CALL_HANGUP) {
        bt_tone_play_index(IDEX_TONE_BT_CONN, 1, NULL);
    }
}

void bt_status_connect_background(struct bt_event *bt)
{
    sys_auto_sniff_controle(1, NULL);
    sys_auto_shut_down_disable();

#if SMART_BOX_EN
    memcpy(__this->last_connecting_addr, bt->args, 6);
    extern void smartbox_update_bt_emitter_connect_state(u8 state, u8 * addr);
    smartbox_update_bt_emitter_connect_state(1, __this->last_connecting_addr);
#endif

    static u8 mac_addr[6];
    memcpy(mac_addr, bt->args, 6);
    UI_MSG_POST("bt_connect_info:mac=%4", mac_addr);

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
    {
        u8 connet_type;
        if (get_auto_connect_state(bt->args)) {
            connet_type = ICON_TYPE_RECONNECT;
        } else {
            connet_type = ICON_TYPE_CONNECTED;
        }

#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            bt_ble_icon_open(connet_type);
        } else {
            //maybe slave already open
            bt_ble_icon_close(0);
        }

#else
        bt_ble_icon_open(connet_type);
#endif
    }
#endif


#if (TCFG_BD_NUM == 2)
    if ((get_total_connect_dev() == 1) && bt->value) {   //当前有一台连接上了,并且连接的是sink设备
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    }
    /* if (get_current_poweron_memory_search_index(NULL) == 0) { */
    /*     bt_wait_phone_connect_control(1); */
    /* } */
#endif

#if TCFG_USER_TWS_ENABLE
    if (!get_bt_tws_connect_status()) {   //如果对耳还未连接就连上手机，要切换闪灯状态
        bt_set_led_status(STATUS_BT_CONN);
    }

    if (bt_tws_phone_connected()) {
        return ;
    }
#else
    bt_set_led_status(STATUS_BT_CONN);    //单台在此处设置连接状态,对耳的连接状态需要同步，在bt_tws.c中去设置
#if (TCFG_AUTO_STOP_PAGE_SCAN_TIME && TCFG_BD_NUM == 2)
    if ((get_total_connect_dev() == 1) && bt->value) {   //当前有一台连接上了,并且连接的是sink设备
        if (app_var.auto_stop_page_scan_timer == 0) {
            app_var.auto_stop_page_scan_timer = sys_timeout_add(NULL, bt_close_page_scan, (TCFG_AUTO_STOP_PAGE_SCAN_TIME * 1000)); //2
        }
    } else {
        if (app_var.auto_stop_page_scan_timer) {
            sys_timeout_del(app_var.auto_stop_page_scan_timer);
            app_var.auto_stop_page_scan_timer = 0;
        }
    }
#endif   //endif AUTO_STOP_PAGE_SCAN_TIME
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 设备断开连接
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_disconnect(struct bt_event *bt)
{
    sys_auto_sniff_controle(0, NULL);
    __this->call_flag = 0;
    /* log_info("BT_STATUS_DISCONNECT\n"); */
#if (SMART_BOX_EN)
    smartbox_update_bt_emitter_connect_state(0, __this->last_connecting_addr);
#endif
    /* UI_MSG_POST("bt_status:n=%4", 0); */
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
    bt_emitter_stu_set(0);
#endif

    bt_set_led_status(STATUS_BT_DISCONN);
#if(TCFG_BD_NUM == 2)               //对耳在bt_tws同步播放提示音
    if (!app_var.goto_poweroff_flag) { /*关机不播断开提示音*/
        if (!__this->ignore_discon_tone) {
            tone_play_by_path(tone_table[IDEX_TONE_BT_DISCONN], 1);
        }
    }
#else
#if TCFG_USER_TWS_ENABLE
    if (!get_bt_tws_connect_status())
#endif
    {
        if (!app_var.goto_poweroff_flag) { /*关机不播断开提示音*/
            if (!__this->ignore_discon_tone) {
                bt_tone_play_index(IDEX_TONE_BT_DISCONN, 1, NULL);
            }
        }
    }
#endif

#if TCFG_USER_TWS_ENABLE
    STATUS *p_led = get_led_config();
    if (get_bt_tws_connect_status()) {
#if TCFG_CHARGESTORE_ENABLE
        chargestore_set_phone_disconnect();
#endif
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            bt_tws_api_push_cmd(SYNC_CMD_LED_PHONE_DISCONN_STATUS, 400);
        }
    } else {
        //断开手机时，如果对耳未连接，要把LED时钟切到RC（因为单台会进SNIFF）
        if (!app_var.goto_poweroff_flag) { /*关机时不改UI*/
            pwm_led_clk_set(PWM_LED_CLK_RC32K);
            bt_set_led_status(STATUS_BT_DISCONN);
        }
    }
    __this->tws_local_back_role = 0;
#else
    if (get_total_connect_dev() == 0) {    //已经没有设备连接
        if (!app_var.goto_poweroff_flag) { /*关机时不改UI*/
            bt_set_led_status(STATUS_BT_DISCONN);
        }
        sys_auto_shut_down_enable();
    }
#endif
}

void bt_status_disconnect_background(struct bt_event *bt)
{
    sys_auto_sniff_controle(0, NULL);
    __this->call_flag = 0;
    /* log_info("BT_STATUS_DISCONNECT\n"); */
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
    if (bt->value) {
        emitter_close(1);
    }
#endif

#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        bt_tws_api_push_cmd(SYNC_CMD_LED_PHONE_DISCONN_STATUS, 400);
    }
    __this->tws_local_back_role = 0;
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机来电
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_phone_income(struct bt_event *bt)
{
    __this->esco_dump_packet = ESCO_DUMP_PACKET_CALL;
    ui_update_status(STATUS_PHONE_INCOME);
    u8 tmp_bd_addr[6];
    memcpy(tmp_bd_addr, bt->args, 6);
    if (bt_switch_back_timer) {
        sys_timeout_del(bt_switch_back_timer);
        bt_switch_back_timer = 0;
    }
    /*
     *(1)1t2有一台通话的时候，另一台如果来电不要提示
     *(2)1t2两台同时来电，现来的题示，后来的不播
     */
    if ((check_esco_state_via_addr(tmp_bd_addr) != BD_ESCO_BUSY_OTHER) && (bt_user_priv_var.phone_ring_flag == 0)) {
#if BT_INBAND_RINGTONE
        extern u8 get_device_inband_ringtone_flag(void);
        bt_user_priv_var.inband_ringtone = get_device_inband_ringtone_flag();
#else
        bt_user_priv_var.inband_ringtone = 0 ;
        lmp_private_esco_suspend_resume(3);
#endif

        bt_user_priv_var.phone_ring_flag = 1;
        bt_user_priv_var.phone_income_flag = 1;

#if TCFG_USER_TWS_ENABLE
#if BT_SYNC_PHONE_RING
        if (!bt_tws_sync_phone_num(NULL))
#else
        if (tws_api_get_role() == TWS_ROLE_MASTER)
#endif
#endif
        {
#if BT_PHONE_NUMBER
            phone_num_play_start();
#else
            phone_ring_play_start();
#endif

        }
        user_send_cmd_prepare(USER_CTRL_HFP_CALL_CURRENT, 0, NULL); //发命令获取电话号码
    } else {
        /* log_debug("SCO busy now:%d,%d\n", check_esco_state_via_addr(tmp_bd_addr), bt_user_priv_var.phone_ring_flag); */
    }
}


/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机打出电话
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_phone_out(struct bt_event *bt)
{
    if (bt_switch_back_timer) {
        sys_timeout_del(bt_switch_back_timer);
        bt_switch_back_timer = 0;
    }
    lmp_private_esco_suspend_resume(4);
    __this->esco_dump_packet = ESCO_DUMP_PACKET_CALL;
    ui_update_status(STATUS_PHONE_OUT);
    bt_user_priv_var.phone_income_flag = 0;
    user_send_cmd_prepare(USER_CTRL_HFP_CALL_CURRENT, 0, NULL); //发命令获取电话号码
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机接通电话
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_phone_active(struct bt_event *bt)
{
    ui_update_status(STATUS_PHONE_ACTIV);
    if (bt_user_priv_var.phone_call_dec_begin) {
        /* log_debug("call_active,dump_packet clear\n"); */
        __this->esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
    }
    if (bt_user_priv_var.phone_ring_flag) {
        bt_user_priv_var.phone_ring_flag = 0;
        tone_play_stop();
        if (bt_user_priv_var.phone_timer_id) {
            sys_timeout_del(bt_user_priv_var.phone_timer_id);
            bt_user_priv_var.phone_timer_id = 0;
        }
    }
    lmp_private_esco_suspend_resume(4);
    bt_user_priv_var.phone_income_flag = 0;
    bt_user_priv_var.phone_num_flag = 0;
    bt_user_priv_var.phone_con_sync_num_ring = 0;
    bt_user_priv_var.phone_con_sync_ring = 0;
    bt_user_priv_var.phone_vol = 15;
    /* log_debug("phone_active:%d\n", app_var.call_volume); */
#ifdef PHONE_CALL_DEFAULT_MAX_VOL
    bt_user_priv_var.set_call_vol_flag |= BIT(1);
    if ((BIT(1) | BIT(2)) == bt_user_priv_var.set_call_vol_flag) {
        app_audio_set_volume(APP_AUDIO_STATE_CALL, 15, 1);
        sys_timeout_add(NULL, phone_sync_vol, 1200);
    }
#else
    app_audio_set_volume(APP_AUDIO_STATE_CALL, app_var.call_volume, 1);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机挂断电话
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_phone_hangup(struct bt_event *bt)
{
    __this->esco_dump_packet = ESCO_DUMP_PACKET_CALL;
    /* log_info("phone_handup\n"); */
    if (bt_user_priv_var.phone_ring_flag) {
        bt_user_priv_var.phone_ring_flag = 0;
        tone_play_stop();
        if (bt_user_priv_var.phone_timer_id) {
            sys_timeout_del(bt_user_priv_var.phone_timer_id);
            bt_user_priv_var.phone_timer_id = 0;
        }
    }
    lmp_private_esco_suspend_resume(4);
    bt_user_priv_var.phone_num_flag = 0;
    bt_user_priv_var.phone_con_sync_num_ring = 0;
    bt_user_priv_var.phone_con_sync_ring = 0;
    __this->call_back_flag &= ~BIT(0);
    if ((__this->call_back_flag == 0) && __this->call_flag) {
        if (bt_switch_back_timer == 0) {
            r_printf("timerout_add:bt_switch_back_timer");
            bt_switch_back_timer = sys_timeout_add(NULL, bt_switch_back, 500);
        }
    }
    if (get_call_status() == BT_CALL_HANGUP) {
        //call handup
        bt_user_priv_var.set_call_vol_flag  = 0;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机来电号码
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_phone_number(struct bt_event *bt)
{
    u8 *phone_number = NULL;
    phone_number = (u8 *)bt->value;
    //put_buf(phone_number, strlen((const char *)phone_number));
    if (bt_user_priv_var.phone_num_flag == 1) {
        return ;
    }
    bt_user_priv_var.income_phone_len = 0;
    memset(bt_user_priv_var.income_phone_num, '\0', sizeof(bt_user_priv_var.income_phone_num));
    for (int i = 0; i < strlen((const char *)phone_number); i++) {
        if (phone_number[i] >= '0' && phone_number[i] <= '9') {
            //过滤，只有数字才能报号
            bt_user_priv_var.income_phone_num[bt_user_priv_var.income_phone_len++] = phone_number[i];
            if (bt_user_priv_var.income_phone_len >= sizeof(bt_user_priv_var.income_phone_num)) {
                return;    /*buffer 空间不够，后面不要了*/
            }
        }
    }
    if (bt_user_priv_var.income_phone_len > 0) {
        bt_user_priv_var.phone_num_flag = 1;
    } else {
        log_debug("PHONE_NUMBER len err\n");
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机来电铃声
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_inband_ringtone(struct bt_event *bt)
{

#if BT_INBAND_RINGTONE
    bt_user_priv_var.inband_ringtone = bt->value;
#else
    bt_user_priv_var.inband_ringtone = 0;
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙停止正在播放的模式提示音
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
#if (TCFG_DEC2TWS_ENABLE)
void bt_mode_stop_other_mode_tone(void)
{
    log_i("bt_mode_stop_other_mode_tone \n");
#if (TCFG_APP_MUSIC_EN)
    tone_play_stop_by_path(tone_table[IDEX_TONE_MUSIC]);
    tone_play_stop_by_path(tone_table[IDEX_TONE_SD]);
    tone_play_stop_by_path(tone_table[IDEX_TONE_UDISK]);
#endif
#if (TCFG_APP_LINEIN_EN)
    tone_play_stop_by_path(tone_table[IDEX_TONE_LINEIN]);
#endif
#if (TCFG_APP_FM_EN)
    tone_play_stop_by_path(tone_table[IDEX_TONE_FM]);
#endif
#if (TCFG_APP_PC_EN)
    tone_play_stop_by_path(tone_table[IDEX_TONE_PC]);
#endif
#if (TCFG_APP_RTC_EN)
    tone_play_stop_by_path(tone_table[IDEX_TONE_RTC]);
#endif
#if (TCFG_APP_RECORD_EN)
    tone_play_stop_by_path(tone_table[IDEX_TONE_RECORD]);
#endif
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机高级音频开始播放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_a2dp_media_start(struct bt_event *bt)
{

    __this->call_flag = 0;
    __this->a2dp_start_flag = 1;
#if (TCFG_DEC2TWS_ENABLE)
    bt_drop_a2dp_frame_stop();
    a2dp_media_clear_packet_before_seqn(0);

    if (bt_media_is_running()) {
        log_debug("is_a2dp_dec_open break\n");
        return ;
    }
    if (localtws_dec_is_open()) {
        // 先发命令再关闭解码，减少tws两边结束的时间差异
        bt_a2dp_drop_frame(NULL);
        tws_user_sync_box(TWS_BOX_NOTICE_A2DP_BACK_TO_BT_MODE, 0);
        os_time_dly(2);
        localtws_dec_close(1);
    }
    // 停止其他模式提示音
    bt_mode_stop_other_mode_tone();
#endif
    a2dp_dec_open(bt->value);
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机高级音频停止播放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_a2dp_media_stop(struct bt_event *bt)
{
    __this->a2dp_start_flag = 0;
    bt_drop_a2dp_frame_stop();
    a2dp_dec_close();

#if (TCFG_DEC2TWS_ENABLE)
    if (is_tws_active_device()) {
        return ;
    }
#endif
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机通话链路切换
   @param
   @return
   @note   手机音频切换、微信等都会有这个状态
*/
/*----------------------------------------------------------------------------*/
void bt_status_sco_change(struct bt_event *bt)
{
    void mem_stats(void);
    mem_stats();
    /* log_info(" BT_STATUS_SCO_STATUS_CHANGE len:%d ,type:%d", (bt->value >> 16), (bt->value & 0x0000ffff)); */
    if (bt->value != 0xff) {
#ifdef PHONE_CALL_DEFAULT_MAX_VOL
        bt_user_priv_var.set_call_vol_flag |= BIT(2);
        if ((BIT(1) | BIT(2)) == bt_user_priv_var.set_call_vol_flag) {
            /* app_audio_set_volume(APP_AUDIO_STATE_CALL, app_var.aec_dac_gain, 1); */
            app_audio_set_volume(APP_AUDIO_STATE_CALL, 15, 1);
            sys_timeout_add(NULL, phone_sync_vol, 1200);
        }
        /* log_debug("--test printfjjj --%x\n", bt_user_priv_var.set_call_vol_flag); */
#endif
        log_info("<<<<<<<<<<<esco_dec_stat\n");
        __this->call_back_flag |= BIT(1);

        app_reset_vddiom_lev(VDDIOM_VOL_32V);

#if 0   //debug
        void mic_capless_feedback_toggle(u8 toggle);
        mic_capless_feedback_toggle(0);
#endif

#if PHONE_CALL_USE_LDO15
        if ((get_chip_version() & 0xff) <= 0x04) { //F版芯片以下的，通话需要使用LDO模式
            power_set_mode(PWR_LDO15);
        }
#endif

#if (TCFG_DEC2TWS_ENABLE)
        if (localtws_dec_close(1)) {
            bt_a2dp_drop_frame(NULL);
        }
        // 停止其他模式提示音
        bt_mode_stop_other_mode_tone();
#endif

#if TCFG_USER_TWS_ENABLE
#if	TCFG_USER_ESCO_SLAVE_MUTE
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            esco_dec_open(&bt->value, 1);
        } else
#endif
#endif
        {
            esco_dec_open(&bt->value, 0);
        }
        bt_user_priv_var.phone_call_dec_begin = 1;
        if (get_call_status() == BT_CALL_ACTIVE) {
            log_debug("dec_begin,dump_packet clear\n");
            __this->esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
        }
#if TCFG_USER_TWS_ENABLE
        tws_page_scan_deal_by_esco(1);
#endif
#if (RECORDER_MIX_EN)
        recorder_mix_call_status_change(1);
#endif/*RECORDER_MIX_EN*/
    } else {
        log_info("<<<<<<<<<<<esco_dec_stop\n");
        __this->call_back_flag &= ~BIT(1);

        app_reset_vddiom_lev(VDDIOM_VOL_34V);

        bt_user_priv_var.set_call_vol_flag &= ~BIT(2);
        bt_user_priv_var.phone_call_dec_begin = 0;
        __this->esco_dump_packet = ESCO_DUMP_PACKET_CALL;
        esco_dec_close();
#if (RECORDER_MIX_EN)
        recorder_mix_call_status_change(0);
#endif/*RECORDER_MIX_EN*/
#if TCFG_USER_TWS_ENABLE
        tws_page_scan_deal_by_esco(0);
#endif
#if PHONE_CALL_USE_LDO15
        if ((get_chip_version() & 0xff) <= 0x04) { //F版芯片以下的，通话需要使用LDO模式
            power_set_mode(TCFG_LOWPOWER_POWER_SEL);
        }
#endif

#if (TCFG_DEC2TWS_ENABLE)
        if (is_tws_active_device()) {
            return ;
        }
#endif
        if ((__this->call_back_flag == 0) && __this->call_flag) {
            if (bt_switch_back_timer == 0) {
                g_printf("timerout_add:bt_switch_back_timer");
                bt_switch_back_timer = sys_timeout_add(NULL, bt_switch_back, 500);
            }
        }
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机通话过程中设置音量产生状态
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_call_vol_change(struct bt_event *bt)
{

    u8 volume = bt->value;  //app_audio_get_max_volume() * bt->value / 15;
    u8 call_status = get_call_status();
    bt_user_priv_var.phone_vol = bt->value;
    if ((call_status == BT_CALL_ACTIVE) || (call_status == BT_CALL_OUTGOING) || app_var.siri_stu) {
        app_audio_set_volume(APP_AUDIO_STATE_CALL, volume, 1);
    } else if (call_status != BT_CALL_HANGUP) {
        /*只保存，不设置到dac*/
        app_var.call_volume = volume;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status  样机和链接设备进入sniff模式后返回来的状态
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_sniff_state_update(struct bt_event *bt)
{

    if (bt->value == 0) {
#if (TCFG_USER_TWS_ENABLE == 1)
        pwm_led_clk_set(PWM_LED_CLK_BTOSC_24M);
#endif
        sys_auto_sniff_controle(1, bt->args);
    } else {
#if (TCFG_USER_TWS_ENABLE == 1)
        pwm_led_clk_set(PWM_LED_CLK_RC32K);
#endif
        sys_auto_sniff_controle(0, bt->args);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 最后拨打电话类型
   @param
   @return
   @note    只区分打入和打出
*/
/*----------------------------------------------------------------------------*/
void bt_status_last_call_type_change(struct bt_event *bt)
{
    __this->call_back_flag |= BIT(0);
    bt_user_priv_var.last_call_type = bt->value;
}


/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 高级音频链接上
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_conn_a2dp_ch(struct bt_event *bt)
{
#if TCFG_USER_EMITTER_ENABLE
    if (bt_user_priv_var.emitter_or_receiver == BT_EMITTER_EN) {
        /* extern int music_player_get_play_status(void); */
        /* if (bt->value && (music_player_get_play_status() == FILE_DEC_STATUS_PLAY)) { */
        /*     emitter_open(1); */
        /* } */
        app_task_put_key_msg(KEY_BT_EMITTER_SW, 0);
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status 手机音频链接上
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_conn_hfp_ch(struct bt_event *bt)
{
#if !TCFG_USER_EMITTER_ENABLE
    if ((!is_1t2_connection()) && (get_current_poweron_memory_search_index(NULL))) { //回连下一个device
        if (get_esco_coder_busy_flag()) {
            clear_current_poweron_memory_search_index(0);
        } else {
            user_send_cmd_prepare(USER_CTRL_START_CONNECTION, 0, NULL);
        }
    }
#endif
}


/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status   获取手机厂商
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_phone_menufactuer(struct bt_event *bt)
{
    extern const u8 hid_conn_depend_on_dev_company;
    app_var.remote_dev_company = bt->value;
    if (hid_conn_depend_on_dev_company) {
        if (bt->value) {
            //user_send_cmd_prepare(USER_CTRL_HID_CONN, 0, NULL);
        } else {
            user_send_cmd_prepare(USER_CTRL_HID_DISCONNECT, 0, NULL);
        }
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status   语音识别
   @param
   @return
   @note    bt->value 0-siri关闭状态 ，
   				      1-siri开启状态 ，
					  2-手动打开siri成功状态.
                      3-安卓手机上选择语音识别软件状态
*/
/*----------------------------------------------------------------------------*/
void bt_status_voice_recognition(struct bt_event *bt)
{
    __this->esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
    /* put_buf(bt, sizeof(struct bt_event)); */
    app_var.siri_stu = bt->value;
    if (__this->call_flag && ((app_var.siri_stu == 0) || (app_var.siri_stu == 3))) {
        if (bt_switch_back_timer == 0) {
            bt_switch_back_timer = sys_timeout_add(NULL, bt_switch_back, 500);
        }
    }
//#if (RECORDER_MIX_EN)
//    if (app_var.siri_stu) {
//        recorder_mix_stop();
//    }
//#endif/*RECORDER_MIX_EN*/
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙status   接收远端设备发过来的avrcp命令
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_status_avrcp_income_opid(struct bt_event *bt)
{

#define AVC_VOLUME_UP			0x41
#define AVC_VOLUME_DOWN			0x42
    log_debug("BT_STATUS_AVRCP_INCOME_OPID:%d\n", bt->value);
    if (bt->value == AVC_VOLUME_UP) {

    }
    if (bt->value == AVC_VOLUME_DOWN) {

    }
}



/*************************************************************
  			蓝牙模式协议栈事件事件处理
**************************************************************/
/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event     蓝牙协议栈事件过滤
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_hci_event_filter(struct bt_event *bt)
{
    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        if (0 == bt->value) {
            set_remote_test_flag(0);
            /* log_debug("clear_test_box_flag"); */
            return 0;
        } else {

#if TCFG_USER_BLE_ENABLE
            //1:edr con;2:ble con;
            if (1 == bt->value) {
                extern void bt_ble_adv_enable(u8 enable);
                bt_ble_adv_enable(0);
            }
#endif

#if TCFG_USER_TWS_ENABLE
            bt_tws_poweroff();
#endif
        }
    }

    if ((bt->event != HCI_EVENT_CONNECTION_COMPLETE) ||
        ((bt->event == HCI_EVENT_CONNECTION_COMPLETE) && (bt->value != ERROR_CODE_SUCCESS))) {
#if TCFG_TEST_BOX_ENABLE
        if (chargestore_get_testbox_status()) {
            if (get_remote_test_flag()) {
                chargestore_clear_connect_status();
            }
            //return 0;
        }
#endif
        if (get_remote_test_flag() \
            && !(HCI_EVENT_DISCONNECTION_COMPLETE == bt->event) \
            && !(HCI_EVENT_VENDOR_REMOTE_TEST == bt->event) \
            && !get_total_connect_dev()) {
            log_info("cpu reset\n");
            cpu_reset();
        }
    }
    if (__this->bt_close_bredr) {
        return 0;
    }

#if CONFIG_TWS_DISCONN_NO_RECONN
    extern u8 tws_detach_by_remove_key;
    if (tws_detach_by_remove_key) {
        return  0;
    }
#endif


    return 1;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   搜索结束
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_hci_event_inquiry(struct bt_event *bt)
{
#if TCFG_USER_EMITTER_ENABLE
    if (bt_user_priv_var.emitter_or_receiver == BT_EMITTER_EN) {
        extern void emitter_search_stop(u8 result);
        emitter_search_stop(bt->value);
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接成功
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_hci_event_connection(struct bt_event *bt)
{
    /* log_debug("tws_conn_state=%d\n", bt_user_priv_var.tws_conn_state); */

#if TCFG_USER_TWS_ENABLE
    bt_tws_hci_event_connect();
#ifndef CONFIG_NEW_BREDR_ENABLE
    tws_try_connect_disable();
#endif
#else
    if (bt_user_priv_var.auto_connection_timer) {
        sys_timeout_del(bt_user_priv_var.auto_connection_timer);
        bt_user_priv_var.auto_connection_timer = 0;
    }
    bt_user_priv_var.auto_connection_counter = 0;
    bt_wait_phone_connect_control(0);
    user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接断开
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_hci_event_disconnect(struct bt_event *bt)
{
    u8 local_addr[6];
    if (app_var.goto_poweroff_flag || __this->exiting) {
        return;
    }

    if (get_total_connect_dev() == 0) {    //已经没有设备连接
        sys_auto_shut_down_enable();
    }


#if TCFG_TEST_BOX_ENABLE
    if (chargestore_get_testbox_status()) {
        /* log_info("<<<<<<<<<<<<<<<<<wait bt discon,then page_scan>>>>>>>>>>>>>\n"); */
        bt_get_vm_mac_addr(local_addr);
        //log_info("local_adr\n");
        //put_buf(local_addr,6);
        lmp_hci_write_local_address(local_addr);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
        return;
    }
#endif

#if (TCFG_AUTO_STOP_PAGE_SCAN_TIME && TCFG_BD_NUM == 2)
    if (get_total_connect_dev() == 1) {   //当前有一台连接上了
        if (app_var.auto_stop_page_scan_timer == 0) {
            app_var.auto_stop_page_scan_timer = sys_timeout_add(NULL, bt_close_page_scan, (TCFG_AUTO_STOP_PAGE_SCAN_TIME * 1000)); //2
        }
    } else {
        if (app_var.auto_stop_page_scan_timer) {
            sys_timeout_del(app_var.auto_stop_page_scan_timer);
            app_var.auto_stop_page_scan_timer = 0;
        }
    }
#endif   //endif AUTO_STOP_PAGE_SCAN_TIME

#if (TCFG_BD_NUM == 2)
    if ((bt->value == ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR) ||
        (bt->value == ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED)) {
        /*
         *连接接受超时
         *如果支持1t2，可以选择继续回连下一台，除非已经回连完毕
         */
        if (get_current_poweron_memory_search_index(NULL)) {
            user_send_cmd_prepare(USER_CTRL_START_CONNECTION, 0, NULL);
            return;
        }
    }
#endif

#if TCFG_USER_TWS_ENABLE
    /* bt_tws_phone_disconnected(); */
    if (bt->value == ERROR_CODE_CONNECTION_TIMEOUT) {
        bt_tws_phone_connect_timeout();
    } else {
        bt_tws_phone_disconnected();
    }
#else
    if (bt_user_priv_var.emitter_or_receiver != BT_EMITTER_EN) {
        bt_wait_phone_connect_control(1);
    } else {
#if (TCFG_SPI_LCD_ENABLE)
        //彩屏方案又ui 控制
        /* if (bt_search_status()) { */
        /* return; */
        /* } */

        u8 search_index = 0;
        search_index = get_current_poweron_memory_search_index(NULL);

        if (search_index) {
            /* log_debug(">>>>>>>>> search_index =%d \n", search_index); */
            user_send_cmd_prepare(USER_CTRL_START_CONNECTION, 0, NULL);
            return;
        }

        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
#else
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
#endif
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接link key 错误
   @param
   @return
   @note   可能是取消配对
*/
/*----------------------------------------------------------------------------*/
void bt_hci_event_linkkey_missing(struct bt_event *bt)
{
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
#if (CONFIG_NO_DISPLAY_BUTTON_ICON && TCFG_CHARGESTORE_ENABLE)
    //已取消配对了
    if (bt_ble_icon_get_adv_state() == ADV_ST_RECONN) {
        //切换广播
        /* log_info("switch_INQ\n"); */
        bt_ble_icon_open(ICON_TYPE_INQUIRY);
    }
#endif
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   回链超时
   @param
   @return
   @note    回链超时内没有连接上设备
*/
/*----------------------------------------------------------------------------*/
void bt_hci_event_page_timeout(struct bt_event *bt)
{
    log_info("-----------HCI_EVENT_PAGE_TIMEOUT conuter %d", bt_user_priv_var.auto_connection_counter);
#if TCFG_USER_TWS_ENABLE
    bt_tws_phone_page_timeout();
#else
    /* if (!bt_user_priv_var.auto_connection_counter) { */
    if (bt_user_priv_var.auto_connection_timer) {
        sys_timer_del(bt_user_priv_var.auto_connection_timer);
        bt_user_priv_var.auto_connection_timer = 0;
    }

#if (TCFG_BD_NUM == 2)
    if (get_current_poweron_memory_search_index(NULL)) {
        user_send_cmd_prepare(USER_CTRL_START_CONNECTION, 0, NULL);
        return;
    }
#endif
    /* if (bt_user_priv_var.emitter_or_receiver != BT_EMITTER_EN) { */
    bt_wait_phone_connect_control(1);
    /* } else { */
    /*     user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL); */
    /* } */
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接超时
   @param
   @return
   @note    链接超时，设备拿远导致的超时等
*/
/*----------------------------------------------------------------------------*/
void bt_hci_event_connection_timeout(struct bt_event *bt)
{
    if (!get_remote_test_flag() && !get_esco_busy_flag()) {
        bt_user_priv_var.auto_connection_counter = (TIMEOUT_CONN_TIME * 1000);
        memcpy(bt_user_priv_var.auto_connection_addr, bt->args, 6);
#if TCFG_USER_TWS_ENABLE
        bt_tws_phone_connect_timeout();
#else

#if ((CONFIG_BT_MODE == BT_BQB)||(CONFIG_BT_MODE == BT_PER))
        bt_wait_phone_connect_control(1);
#else
        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        bt_wait_connect_and_phone_connect_switch(0);
#endif
        //user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, bt->args);
#endif
    } else {
#if TCFG_USER_TWS_ENABLE
        bt_tws_phone_disconnected();
#else
        bt_wait_phone_connect_control(1);
#endif
    }

}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接已经存在
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_hci_event_connection_exist(struct bt_event *bt)
{
    if (!get_remote_test_flag() && !get_esco_busy_flag()) {
        bt_user_priv_var.auto_connection_counter = (8 * 1000);
        memcpy(bt_user_priv_var.auto_connection_addr, bt->args, 6);
#if TCFG_USER_TWS_ENABLE
        bt_tws_phone_connect_timeout();
#else
        if (bt_user_priv_var.auto_connection_timer) {
            sys_timer_del(bt_user_priv_var.auto_connection_timer);
            bt_user_priv_var.auto_connection_timer = 0;
        }
        bt_wait_connect_and_phone_connect_switch(0);
#endif
    } else {
#if TCFG_USER_TWS_ENABLE
        bt_tws_phone_disconnected();
#else
        bt_wait_phone_connect_control(1);
#endif
    }
}

u8 is_call_now()
{
    if (get_call_status() != BT_CALL_HANGUP) {
        return 1;
    }

    return 0;
}
#else

void sys_enter_soft_poweroff(void *priv)
{
    if (priv == NULL) {
        app_task_switch_to(APP_POWEROFF_TASK);
    } else if (priv == (void *)1) {
        log_info("cpu_reset!!!\n");
        cpu_reset();
    }
}


#endif
