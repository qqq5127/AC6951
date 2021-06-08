
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

extern void bt_start_a2dp_slience_detect(int ingore_packet_num) ;
extern void bt_drop_a2dp_frame_start(void);

/*************************************************************

             此文件函数主要是蓝牙模式创建，模式退出和进去

			 后台、非后台的处理


**************************************************************/

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式关机退出道idle模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void wait_exit_btstack_flag(void *priv)
{
    sys_timer_del(app_var.wait_exit_timer);
    app_var.wait_exit_timer = 0;
    if (priv == NULL) {
        app_task_switch_to(APP_POWEROFF_TASK);
    } else if (priv == (void *)1) {
        log_info("cpu_reset!!!\n");
        cpu_reset();
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式后台回来蓝牙模式恢复
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_resume_deal(void)
{
    sys_key_event_enable();
    sys_auto_sniff_controle(1, NULL);
    if (get_call_status() != BT_CALL_HANGUP) {
        log_debug("background return by call");
        return;
    }
    bt_set_led_status(0);
    if (get_total_connect_dev() == 0) {
        sys_auto_shut_down_enable();
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   判断蓝牙通话是否在运行
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_must_work(void)
{
    if ((app_var.siri_stu) && (app_var.siri_stu != 3)) {
        // siri不退出
        return true;
    }

    if ((get_call_status() == BT_CALL_OUTGOING)
        || (get_call_status() == BT_CALL_ALERT)
        || (get_call_status() == BT_CALL_INCOMING)
        || (get_call_status() == BT_CALL_ACTIVE)
       ) {
        // 通话不退出
        return true;
    }
    return false;
}


/*----------------------------------------------------------------------------*/
/**@brief     后台的时候判断有音频需要播放歌曲
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void a2dp_media_packet_play_start(void *p)
{
    __this->back_mode_systime = 0;
    if ((__this->exit_flag == 0) || (__this->sbc_packet_step == 2)) {
        log_debug(" a2dp back \n");
#if (TCFG_DEC2TWS_ENABLE)
        /* tws_user_sync_box(TWS_BOX_A2DP_BACK_TO_BT_MODE_START, __this->a2dp_decoder_type); */
        __this->tws_local_to_bt_mode = 1;
        localtws_set_wait_a2dp_start(1);
#endif
        app_task_switch_to(APP_BT_TASK);
        struct sys_event event;
        event.type = SYS_BT_EVENT;
        event.arg = (void *)SYS_BT_EVENT_TYPE_CON_STATUS;
        event.u.bt.event = BT_STATUS_A2DP_MEDIA_START;
        event.u.bt.value = __this->a2dp_decoder_type;
        sys_event_notify(&event);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙模式发起播放命令
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int a2dp_media_packet_cmd_pp(void *p)
{
    if ((__this->exit_flag == 1) && (__this->sbc_packet_step == 1)) {
        /* log_info("send pp "); */
        user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
    }
    return 0;
}


u8 bt_get_exit_flag()
{
    return __this->exit_flag;
}

extern int a2dp_media_clear_packet_before_seqn(u16 seqn_number);
extern void *a2dp_media_fetch_packet(int *len, void *prev_packet);
static u16 slience_timer;
u16 clear_to_seqn;
void a2dp_slience_detect(void *p)
{
    int ingore_packet_num = (int)p;
    int role = tws_api_get_role();
    int len;

    u8 *packet = a2dp_media_fetch_packet(&len, NULL);
    if (!packet) {
        return;
    }
    if (role == TWS_ROLE_MASTER) {
        if (packet) {
            u16 seqn = (packet[2] << 8) | packet[3];
            if (clear_to_seqn == 0) {
                clear_to_seqn = seqn + ingore_packet_num;
                g_printf("clear_to_seqn=%d\n", clear_to_seqn);
                if (clear_to_seqn == 0) {
                    clear_to_seqn = 1;
                }
                a2dp_media_clear_packet_before_seqn(clear_to_seqn);
            }
        }
    }
    if (clear_to_seqn) {
        bt_drop_a2dp_frame_start();
        __a2dp_drop_frame(NULL);
    }

}
void bt_start_a2dp_slience_detect(int ingore_packet_num)
{
    clear_to_seqn = 0;
    if (slience_timer) {
        sys_timer_del(slience_timer);
    }
    r_printf("bt_start_a2dp_slience_detect=%d", ingore_packet_num);
    a2dp_slience_detect((void *)ingore_packet_num);
    if (clear_to_seqn == 0) {
        slience_timer = sys_timer_add((void *)ingore_packet_num, a2dp_slience_detect, 40);
    }
}
void bt_stop_a2dp_slience_detect()
{
    if (slience_timer) {
        sys_timer_del(slience_timer);
        slience_timer = 0;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  tws后台回蓝牙模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void tws_local_back_to_bt_mode(u8 mode, u8 value)
{
    /* log_info("rx_tws_local_back_to_bt_mode=%d\n", mode); */
#if (TCFG_DEC2TWS_ENABLE)
    if (mode == TWS_BOX_NOTICE_A2DP_BACK_TO_BT_MODE) {
        if (__this->tws_local_back_role == 1) {
            localtws_set_wait_a2dp_start(1);
            __this->tws_local_back_role = 0;
            if (__this->back_mode_systime == 0) {
                __this->cmd_flag = 1;
                __this->back_mode_systime = sys_timeout_add(NULL, a2dp_media_packet_play_start, 1);
            }
            /* r_printf("tws_local_back_role ==role_new back mode_switch=%d\n", __this->back_mode_systime); */
        }
    } else if (mode == TWS_BOX_A2DP_BACK_TO_BT_MODE_START) {
        __this->tws_local_back_role = 0;
        struct sys_event event;
        event.type = SYS_BT_EVENT;
        event.arg = (void *)SYS_BT_EVENT_TYPE_CON_STATUS;
        event.u.bt.event = BT_STATUS_A2DP_MEDIA_START;
        event.u.bt.value = value;
        sys_event_notify(&event);
    } else if (mode == TWS_BOX_EXIT_BT) {
        __this->a2dp_start_flag = 0;
        if (value == TWS_UNACTIVE_DEIVCE) {
            __this->tws_local_back_role = 2;
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                log_debug("\n ----- master pause  -------  \n");
                user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL);
            }
            if (a2dp_dec_close()) {
                bt_start_a2dp_slience_detect(300);
                bt_drop_a2dp_frame_start();
                __a2dp_drop_frame(NULL);
            }
            if (bt_get_exit_flag()) {
                __this->cmd_flag = 2;
            }
            /* __this->tws_local_back_role = 2; */
            /* log_debug("__this->cmd_flag=0x%x,%d\n", __this->tws_local_back_role, __this->cmd_flag); */
            app_task_switch_to(APP_BT_TASK);
        } else {
            __this->exiting = 0;
        }
    } else if (mode == TWS_BOX_ENTER_BT) {
        __this->tws_local_back_role = 0;
        if (!localtws_dec_close(1)) {
            tws_api_local_media_trans_clear();
        }
    }
#endif

}


/*----------------------------------------------------------------------------*/
/**@brief   后台模式下sbc丢包加能量检测, 返回-INVALUE表示要丢掉此包数据
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int a2dp_media_packet_user_handler(u8 *data, u16 size)
{
    if ((get_call_status() != BT_CALL_HANGUP) || bt_phone_dec_is_running() || (__this->call_flag)) {
        if (get_call_status() != BT_CALL_HANGUP) {
            /* puts("call!=hangup"); */
        }
        return -EINVAL;
    }

    if (__this->tws_local_back_role == 2) {
        /* putchar('$'); */
        return -EINVAL;
    }

    if (app_var.goto_poweroff_flag) {
        return -EINVAL;
    }


#if TCFG_USER_TWS_ENABLE
    if (is_tws_going_poweroff()) {
        return -EINVAL;
    }
#endif

    local_irq_disable();
    if (__this->exit_flag == 0) {
        if ((!bt_media_is_running()) && (__this->sbc_packet_lose_cnt)) {
            __this->sbc_packet_lose_cnt++;
            if (__this->sbc_packet_lose_cnt > 10) {
                __this->sbc_packet_lose_cnt = 0;
                __this->cmd_flag = 0;
                __this->sbc_packet_step = 0;
                local_irq_enable();
                __this->a2dp_decoder_type = a2dp_media_packet_codec_type(data);
                /* log_info("sbc lose over %d", __this->a2dp_decoder_type); */
                if (__this->back_mode_systime == 0) {
                    __this->back_mode_systime = sys_timeout_add(NULL, a2dp_media_packet_play_start, 1);
                }
                return 0;
            }
        } else {
            __this->sbc_packet_lose_cnt = 0;
        }
        local_irq_enable();
        return 0;
    }
    u32 media_type = a2dp_media_packet_codec_type(data);
    u32 cur_time = timer_get_ms();
    int energy = 0;
    if (media_type == 0/* A2DP_CODEC_SBC */) {//后台返回蓝牙
        energy = sbc_energy_check(data, size);
    } else {
        if (a2dp_get_status() == BT_MUSIC_STATUS_STARTING) {
            energy = 2000;
            //其它格式不方便做能量检测。简易判断，该标志成立就直接认为是有效声音
        }
    }
    /* log_info("sbc_filter: %d ", energy); */
    if (__this->sbc_packet_step == 1) {
        // 退出
        if ((cur_time > __this->no_sbc_packet_to) || ((cur_time > __this->sbc_packet_filter_to))) {
            // 转换为后台模式
            log_debug("goto back mode \n");
            __this->sbc_packet_step = 2;
            __this->no_sbc_packet_to = cur_time;
            __this->sbc_packet_filter_to = cur_time + SBC_FILTER_TIME_MS;
            __this->a2dp_decoder_type = a2dp_media_packet_codec_type(data);
        } else {
            // 还在退出
            /* log_info("exit,, "); */
            __this->no_sbc_packet_to = cur_time + NO_SBC_TIME_MS;
            if (energy > 1000) {
                __this->sbc_packet_filter_to = cur_time + SBC_ZERO_TIME_MS;
                __this->sbc_packet_valid_cnt ++;
                if (__this->sbc_packet_valid_cnt > __this->sbc_packet_valid_cnt_max) {
                    __this->sbc_packet_valid_cnt = 0;
                    if (__this->sbc_packet_valid_cnt_max < 80) {
                        __this->sbc_packet_valid_cnt_max += 20;
                    } else {
                        log_debug("goto back mode0 \n");
                        __this->sbc_packet_step = 2;
                        __this->no_sbc_packet_to = cur_time;
                        __this->sbc_packet_filter_to = cur_time + SBC_FILTER_TIME_MS;
                    }
                    ///在退出的时候已经发送暂停
                    /* sys_timeout_add(NULL, a2dp_media_packet_cmd_pp, 1); */
                }
            } else {
                __this->sbc_packet_valid_cnt = 0;
            }
        }
    } else if (__this->sbc_packet_step == 2) {
        // 后台
        if (cur_time >= __this->no_sbc_packet_to) {
            // 新的开始
            if (energy > 1000) {
                log_debug("new back mode \n");
#if 0 // 超时太短，容易误触发返回蓝牙模式
                if (__this->tws_local_back_role) {
                    /* puts("tws_new_back_mode\n"); */
                    __this->no_sbc_packet_to = cur_time + NO_SBC_TIME_MS * 2;
                    __this->sbc_packet_filter_to = cur_time + SBC_FILTER_TIME_MS / 4;
                } else
#endif
                {
                    __this->no_sbc_packet_to = cur_time + NO_SBC_TIME_MS;
                    __this->sbc_packet_filter_to = cur_time + SBC_FILTER_TIME_MS;
                }
            } else {
                /* log_info("energy limit \n"); */
            }
        } else {
            /* log_info("bkm:%d, ", __this->sbc_packet_filter_to - cur_time); */
            if (__this->tws_local_back_role) {
                __this->no_sbc_packet_to = cur_time + NO_SBC_TIME_MS * 2;
            } else {
                __this->no_sbc_packet_to = cur_time + NO_SBC_TIME_MS;
            }
            if (cur_time > __this->sbc_packet_filter_to) {
                //过滤时间耗完
                __this->no_sbc_packet_to = cur_time;
                if (energy > 1000) {
                    log_debug("start back mode \n");
                    __this->cmd_flag = 1;
                    __this->tws_local_back_role = 0;
                    local_irq_enable();
                    __this->a2dp_decoder_type = a2dp_media_packet_codec_type(data);
                    if (__this->back_mode_systime == 0) {
                        __this->back_mode_systime = sys_timeout_add(NULL, a2dp_media_packet_play_start, 1);
                    }
                    return -EINVAL;
                }
            }
        }
    } else {
        local_irq_enable();
        return 0;
    }
    local_irq_enable();
    return -EINVAL;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙非后台模式退出蓝牙等待蓝牙状态可以退出
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_no_background_exit_check(void *priv)
{
    if (bt_user_priv_var.auto_connection_timer) {
        sys_timeout_del(bt_user_priv_var.auto_connection_timer);
        bt_user_priv_var.auto_connection_timer = 0;
    }

    if (__this->init_ok == 0) {
        /* putchar('#'); */
        return;
    }

    if (bt_audio_is_running()) {
        /* putchar('$'); */
        return;
    }

#if TCFG_USER_BLE_ENABLE
    bt_ble_exit();
#endif
    btstack_exit();
    log_info("bt_exit_check ok\n");
    __this->exit_flag = 1;
    sys_timer_del(__this->timer);
    __this->init_ok = 0;
    __this->init_start = 0;
    __this->timer = 0;
    __this->exiting = 0;
    set_stack_exiting(0);
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙后台模式退出蓝牙
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 bt_background_exit()
{
    u8 suepend_rx_bulk = 1;
    __this->cmd_flag = 0;
    __this->call_flag = 0;

    u32 cur_time = timer_get_ms();
    local_irq_disable();
    __this->exit_flag = 1;
    __this->sbc_packet_step = 1;
    __this->sbc_packet_valid_cnt = 0;
    __this->sbc_packet_valid_cnt_max = 2;
    __this->no_sbc_packet_to = cur_time + NO_SBC_TIME_MS;
    __this->sbc_packet_filter_to = cur_time + SBC_ZERO_TIME_MS;
    local_irq_enable();

#if (TCFG_DEC2TWS_ENABLE)
    int state = tws_api_get_tws_state();
    suepend_rx_bulk = 0;
    __this->tws_local_back_role = 0;
    if (state & TWS_STA_SIBLING_CONNECTED) {
        __this->tws_local_back_role = 1;
        tws_api_sync_call_by_uuid('D', SYNC_CMD_BOX_INIT_EXIT_BT, 0);// delay不宜太长，避免两边结束时间差异大
        localtws_dec_close(1);
    } else {
        user_set_tws_box_mode(1);
    }
    /* r_printf("__this->tws_local_back_role=0x%x\n", __this->tws_local_back_role); */
#endif
    if (a2dp_dec_close()) {
        bt_start_a2dp_slience_detect(300);
    }
    extern int btctrler_suspend(u8 suepend_rx_bulk);
    btctrler_suspend(suepend_rx_bulk);
    extern int bredr_suspend();
    bredr_suspend();

#if (TCFG_DEC2TWS_ENABLE)
    if (state & TWS_STA_SIBLING_CONNECTED) {
        __this->exiting = 1;
        return -EINVAL;
    }
#endif
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙后台模式关机，蓝牙关闭所有状态
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 bt_background_poweroff_exit()
{
    __this->exiting = 1;
    set_stack_exiting(1);


#if TCFG_USER_TWS_ENABLE
    bt_tws_poweroff();
#endif

    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);

    if (__this->exit_flag) {
        __this->sbc_packet_step = 0;
        __this->no_sbc_packet_to = 0;
        __this->sbc_packet_filter_to = 0;
        __this->sbc_packet_lose_cnt = 1;
    }

    if (__this->timer == 0) {
        __this->tmr_cnt = 0;
        __this->timer = sys_timer_add(NULL, bt_no_background_exit_check, 10);
        /* printf("set exit timer\n"); */
    }
    return -EINVAL;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙非后台模式退出模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 bt_nobackground_exit()
{
    if (!__this->init_start) {
        return -EINVAL;
    }

    __this->exiting = 1;
    set_stack_exiting(1);
    __a2dp_drop_frame(NULL);//临时解决非后台退出杂音问题
#if TCFG_USER_TWS_ENABLE
    bt_tws_poweroff();
#endif
    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);

    if (__this->timer == 0) {
        __this->tmr_cnt = 0;
        __this->timer = sys_timer_add(NULL, bt_no_background_exit_check, 10);
        printf("set exit timer\n");
    }

    return -EINVAL;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙通话检测
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void esco_check_state(void *priv)
{
    /* if (bt_sco_state()) { */
    if (true == bt_phone_dec_is_running()) {
        sys_timeout_add(NULL, esco_check_state, 20);
    } else {
#if (TCFG_DEC2TWS_ENABLE)
        extern bool get_tws_sibling_connect_state(void);
        if (get_tws_sibling_connect_state()) {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                bt_tws_api_push_cmd(SYNC_CMD_POWER_OFF_TOGETHER, TWS_SYNC_TIME_DO);
            }
        } else
#endif
        {
            sys_enter_soft_poweroff(NULL);
        }
    }
}


u8 bt_app_switch_exit_check()
{
    if ((get_call_status() == BT_CALL_OUTGOING)
        || (get_call_status() == BT_CALL_ALERT)
        || (get_call_status() == BT_CALL_INCOMING)
        || (get_call_status() == BT_CALL_ACTIVE)
       ) {
        // 通话不退出
        return 0;
    }

    if (__this->exit_flag) {
        // 正在退出就不用重新设置
        // 避免快速调用两次导致发了两次pp，结束不了解码
        log_debug("exit flag ");
#if TCFG_BLUETOOTH_BACK_MODE
        if ((app_var.goto_poweroff_flag == 0) && (__this->exiting == 1)) {
            return 0;
        } else {
            return 1;
        }
#else
        return 1;
#endif
    }

#if PHONE_CALL_FORCE_POWEROFF
    if (true == bt_phone_dec_is_running()) {
        log_debug("bt_phone_dec_is_running");
        if (app_var.goto_poweroff_flag) {
            user_send_cmd_prepare(USER_CTRL_DISCONN_SCO, 0, NULL);
            sys_timeout_add(NULL, esco_check_state, 20);
        }
        app_var.goto_poweroff_flag = 0;
        return 0;
    }
#else

    if (__this->force_poweroff) {
        if (true == bt_must_work()) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
            sys_timeout_add(NULL, esco_check_state, 20);
            app_var.goto_poweroff_flag = 0;
            return 0;
        }
        if (true == bt_phone_dec_is_running()) {
            user_send_cmd_prepare(USER_CTRL_DISCONN_SCO, 0, NULL);
            sys_timeout_add(NULL, esco_check_state, 20);
            app_var.goto_poweroff_flag = 0;
            return 0;
        }
        __this->force_poweroff = 0;
    }

    if (true == bt_must_work()) {
        log_debug("bt_must_work");
        app_var.goto_poweroff_flag = 0;
        return 0;
    }

    if (true == bt_phone_dec_is_running()) {
        log_debug("bt_phone_dec_is_running");
        app_var.goto_poweroff_flag = 0;
        return 0;
    }

#endif
    return 1;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙退出模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_app_exit(void)
{
    struct sys_event clear_key_event = {.type =  SYS_KEY_EVENT, .arg = "key"};

    log_info("bt_app_exit");

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
    bt_emitter_mic_close();
#endif

    __this->ignore_discon_tone = 1;
    bt_drop_a2dp_frame_stop();
    /* __a2dp_drop_frame(NULL); */
    sys_key_event_disable();
    sys_event_clear(&clear_key_event);
    sys_auto_shut_down_disable();
    tone_play_stop();
    UI_HIDE_CURR_WINDOW();
#if TCFG_BLUETOOTH_BACK_MODE
    if (app_var.goto_poweroff_flag == 0) {
        return	bt_background_exit();
    } else {
        return bt_background_poweroff_exit();
    }
#else
    return bt_nobackground_exit();
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙后台进入模式初始化
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_background_init()
{
    if (__this->back_mode_systime) {
        sys_timeout_del(__this->back_mode_systime);
        __this->back_mode_systime = 0;
        /* log_debug("__this->init_ok clear back_mode_systime\n"); */
    }
#if (TCFG_DEC2TWS_ENABLE)
    int state = tws_api_get_tws_state();
    if (state & TWS_STA_SIBLING_CONNECTED) {
        if (__this->cmd_flag != 2) {
            if (__this->tws_local_to_bt_mode) {
                tws_user_sync_box(TWS_BOX_A2DP_BACK_TO_BT_MODE_START, __this->a2dp_decoder_type);
            }
            log_debug("tx_SYNC_CMD_BOX_ENTER_BT\n");
            tws_api_sync_call_by_uuid('D', SYNC_CMD_BOX_ENTER_BT, 10);
        }
    } else {
        /* user_set_tws_box_mode(2); */
        user_set_tws_box_mode(0);
    }
    __this->tws_local_to_bt_mode = 0;
#endif
    local_irq_disable();
    __this->sbc_packet_step = 0;
    __this->no_sbc_packet_to = 0;
    __this->sbc_packet_filter_to = 0;
    __this->sbc_packet_lose_cnt = 1;
    __this->tws_local_back_role = 0;
    local_irq_enable();
    extern void bredr_resume();//background resume
    bredr_resume();
    void btctrler_resume();
    btctrler_resume();

    bt_resume_deal();
    __this->cmd_flag = 0;
    UI_SHOW_MENU(MENU_BT, 1000, 0, NULL);

}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙进入模式初始化(时钟和显示部分和变量)
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_task_init()
{
    clock_idle(BT_IDLE_CLOCK);
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

    __this->ignore_discon_tone = 0;
    __this->exiting = 0;
    __this->wait_exit = 0;
    __this->force_poweroff  = 0 ;
    __this->init_start = 0;//蓝牙协议栈已经开始初始化标志位

    UI_SHOW_WINDOW(ID_WINDOW_BT);
    UI_SHOW_MENU(MENU_BT, 1000, 0, NULL);

#if TCFG_BLUETOOTH_BACK_MODE
    if (__this->init_ok) {
        __this->init_start = 1;//蓝牙协议栈已经开始初始化标志位
        /* 按键消息使能 */
        sys_key_event_enable();//第一次需要蓝牙初始化成功才开按键 不然会没有蓝牙后台
    }
#else
    sys_key_event_enable();
#endif

}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙进入模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_task_start()
{
    if (__this->exit_flag == 0) {
        return;
    }

    __this->exit_flag = 0;

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
    if (bt_emitter_stu_get() == true) {
        bt_emitter_mic_open();
    }
#endif

#if TCFG_BLUETOOTH_BACK_MODE
    if (__this->init_ok) {
        bt_background_init();
        return;
    }
#else
    app_var.goto_poweroff_flag = 0;
#endif
    bt_function_select_init();
    bredr_handle_register();
    btstack_init();
#if TCFG_USER_TWS_ENABLE
    tws_profile_init();
#endif

    BT_STATE_INIT();

    __this->init_start = 1;//蓝牙协议栈已经开始初始化

    sys_auto_shut_down_enable();
    sys_auto_sniff_controle(1, NULL);

#if TCFG_BLUETOOTH_BACK_MODE
    sys_key_event_enable();//蓝牙带后台情况第一次开按键需要在协议栈初始化之后
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙模式退出
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_task_close()
{
    tone_play_stop();
    if (__this->timer) {
        sys_timer_del(__this->timer);
        __this->timer = 0;
    }
    sys_auto_shut_down_disable();
    __this->auto_exit_limit_time = (u32) - 1;
    bt_app_exit();
}


/*----------------------------------------------------------------------------*/
/**@brief  蓝牙直接开关
   @param
   @return
   @note   如果想后台开机不需要进蓝牙，可以在poweron模式
   			直接调用这个函数初始化蓝牙
*/
/*----------------------------------------------------------------------------*/

void bt_direct_init()
{
#if TCFG_BLUETOOTH_BACK_MODE
    if (__this->bt_direct_init) {
        return;
    }
    log_info(" bt_direct_init  \n");

    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

    __this->ignore_discon_tone = 0;
    __this->bt_direct_init = 1;

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
    if (bt_emitter_stu_get() == true) {
        bt_emitter_mic_open();
    }
#endif
    bt_function_select_init();
    bredr_handle_register();
    btstack_init();
#if TCFG_USER_TWS_ENABLE
    tws_profile_init();
#endif

    BT_STATE_INIT();

    /* 按键消息使能 */
    sys_key_event_enable();
    sys_auto_shut_down_enable();
    sys_auto_sniff_controle(1, NULL);
#endif
}


/*----------------------------------------------------------------------------*/
/**@brief  蓝牙 退出蓝牙等待蓝牙状态可以退出
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_direct_close_check(void *priv)
{
#if TCFG_BLUETOOTH_BACK_MODE
    if (bt_user_priv_var.auto_connection_timer) {
        sys_timeout_del(bt_user_priv_var.auto_connection_timer);
        bt_user_priv_var.auto_connection_timer = 0;
    }

    if (__this->init_ok == 0) {
        /* putchar('#'); */
        return;
    }

    if (bt_audio_is_running()) {
        /* putchar('$'); */
        return;
    }

#if TCFG_USER_BLE_ENABLE
    bt_ble_exit();
#endif
#if TCFG_USER_TWS_ENABLE
    bt_tws_poweroff();
#endif
    btstack_exit();
    log_info(" bt_direct_close_check ok\n");
    sys_timer_del(__this->timer);
    __this->init_ok = 0;
    __this->init_start = 0;
    __this->timer = 0;
    __this->bt_direct_init = 0;
    set_stack_exiting(0);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙后台直接关闭
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_direct_close(void)
{
#if TCFG_BLUETOOTH_BACK_MODE
    if (__this->init_ok == 0) {
        /* putchar('#'); */
        return;
    }

    log_info(" bt_direct_close");

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
    bt_emitter_mic_close();
#endif

    __this->ignore_discon_tone = 1;
    sys_auto_shut_down_disable();

    set_stack_exiting(1);
    __a2dp_drop_frame(NULL);//临时解决非后台退出杂音问题
    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);

    if (__this->timer == 0) {
        __this->tmr_cnt = 0;
        __this->timer = sys_timer_add(NULL, bt_direct_close_check, 10);
        printf("set exit timer\n");
    }

#endif
}


/*----------------------------------------------------------------------------*/
/**@brief  蓝牙  关闭bredr
   @param
   @return
   @note   主要把bredr 运行状态都关闭即可
*/
/*----------------------------------------------------------------------------*/
void bt_close_bredr()
{
    __this->bt_close_bredr = 1;
    if (bt_user_priv_var.auto_connection_timer) {
        sys_timeout_del(bt_user_priv_var.auto_connection_timer);
        bt_user_priv_var.auto_connection_timer = 0;
    }
    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_INQUIRY_CANCEL, 0, NULL);

#if TCFG_USER_TWS_ENABLE
    tws_cancle_all_noconn();
#endif
    sys_auto_sniff_controle(0, NULL);
    btctrler_task_close_bredr();
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙  开启bredr
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_init_bredr()
{
    __this->bt_close_bredr = 0;
    btctrler_task_init_bredr();
    bt_wait_phone_connect_control(1);
    sys_auto_sniff_controle(1, NULL);
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙  开关a2dp
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bredr_a2dp_open_and_close()
{
    if (get_curr_channel_state() & A2DP_CH) {
        puts("start to disconnect a2dp ");
        user_send_cmd_prepare(USER_CTRL_DISCONN_A2DP, 0, NULL);
    } else {
        puts("start to connect a2dp ");
        user_send_cmd_prepare(USER_CTRL_CONN_A2DP, 0, NULL);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙  开关hfp
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bredr_hfp_open_and_close()
{
    if (get_curr_channel_state() & HFP_CH) {
        user_send_cmd_prepare(USER_CTRL_HFP_DISCONNECT, 0, NULL);
    } else {
        user_send_cmd_prepare(USER_CTRL_HFP_CMD_BEGIN, 0, NULL);
    }
}

u8 bt_get_task_state()
{
    return __this->exiting;
}

#endif
