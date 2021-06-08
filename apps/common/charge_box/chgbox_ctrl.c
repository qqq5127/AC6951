#include "system/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "chargeIc_manage.h"
#include "chgbox_ctrl.h"
#include "chgbox_box.h"
#include "chgbox_det.h"
#include "chgbox_wireless.h"
#include "key_event_deal.h"
#include "device/chargebox.h"
#include "chgbox_ui.h"
#include "chgbox_handshake.h"
#include "asm/pwm_led.h"
#include "le_smartbox_module.h"

#if(TCFG_CHARGE_BOX_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHG_CTRL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

//强制给耳机充电的时间，为0则不需要强制充电
#define CHGBOX_FORCE_CHARGE_TIMES 240 //耳机电池不带保护建议设置2min

//蓝牙开启后多久自动关闭
#define CHGBOX_BT_AUTO_OFF_TIMES  360 //3min未检测到升级就关闭蓝牙

//耳机满电检测次数，满电电压
#define CHG_EAR_FULL_DET_CNT       6 //注意调用位置，再计算总时间
#define CHG_EAR_FULL_DET_LEVEL     100 //电压值
#define TEMP_PROTECT_TIMEOUT       300000 //温度保护超时未恢复关机时间

void chargebox_set_newstatus(u8 newstatus);

SYS_INFO sys_info;
EAR_INFO ear_info;
static int temp_protect_timer;
static u32 bt_auto_off_cnt;
static int auto_shutdown_timer = 0;//timer 句柄
extern void chgbox_enter_soft_power_off(void);
extern void usb_key_check_entry(u32 timeout);
extern u16 get_curr_channel_state();
extern OS_MUTEX power_mutex;
/*------------------------------------------------------------------------------------*/
/**@brief    自动关机计数
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
static void sys_auto_shutdown_deal(void *priv)
{
#if TCFG_APP_BT_EN
    if (bt_auto_off_cnt) {
        return;
    }
#endif
    sys_info.life_cnt++;
    if (sys_info.life_cnt > TCFG_CHARGGBOX_AUTO_SHUT_DOWN_TIME) {
        log_info("charegebox enter soft poweroff\n");
        chgbox_enter_soft_power_off();
    }
}

void sys_auto_shutdown_reset(void)
{
    sys_info.life_cnt = 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    自动关机使能
   @param    无
   @return   无
   @note     将自动关机处理函数注册到timer里
*/
/*------------------------------------------------------------------------------------*/
void sys_auto_shutdown_enable(void)
{
    if (!auto_shutdown_timer) {
        sys_info.life_cnt = 0;
        auto_shutdown_timer = sys_timer_add(NULL, sys_auto_shutdown_deal, 1000);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    自动关机关闭
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void sys_auto_shutdown_disable(void)
{
    if (auto_shutdown_timer) {
        sys_timer_del(auto_shutdown_timer);
        auto_shutdown_timer = 0;
        sys_info.life_cnt = 0;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    超时时关闭自动升压
   @param    无
   @return   无
   @note     若USB在线或者无线充在线则延长超时时间
*/
/*------------------------------------------------------------------------------------*/
static void chargebox_temp_protect_timeout(void *priv)
{
    temp_protect_timer = 0;
    if (sys_info.status[USB_DET] == STATUS_ONLINE || sys_info.status[WIRELESS_DET] == STATUS_ONLINE) {
        temp_protect_timer = sys_timeout_add(NULL, chargebox_temp_protect_timeout, TEMP_PROTECT_TIMEOUT);
    } else {
        chargeIc_boost_ctrl(0);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    通过IO状态判断输出是否短路
   @param    无
   @return   0:没有发送消息, 1:初次触发短路/过流保护发送消息
   @note     在开启升压输出前调用
*/
/*------------------------------------------------------------------------------------*/
u8 chargebox_check_output_short(void)
{
    if (sys_info.current_limit) {
        return 0;
    }
    gpio_set_pull_up(TCFG_CHARGEBOX_L_PORT, 1);
    gpio_set_pull_up(TCFG_CHARGEBOX_R_PORT, 1);
    gpio_direction_input(TCFG_CHARGEBOX_L_PORT);
    gpio_direction_input(TCFG_CHARGEBOX_R_PORT);
    if ((gpio_read(TCFG_CHARGEBOX_L_PORT) == 0) || (gpio_read(TCFG_CHARGEBOX_R_PORT) == 0)) {
        log_error("gpio status err, mabe short, L:%d, R:%d!\n", gpio_read(TCFG_CHARGEBOX_L_PORT), gpio_read(TCFG_CHARGEBOX_R_PORT));
        sys_info.current_limit = 1;
        app_chargebox_event_to_user(CHGBOX_EVENT_ENTER_CURRENT_PROTECT);
        return 1;
    }
    return 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电过程中短路触发的保护唤醒时调用
   @param    无
   @return   无
   @note     在唤醒中断里面调用
*/
/*------------------------------------------------------------------------------------*/
void chargebox_set_output_short(void)
{
    if (sys_info.current_limit) {
        return;
    }
    chargeIc_boost_ctrl(0);//关闭升压
    chargeIc_pwr_ctrl(0);//关闭充电开关
    sys_info.current_limit = 1;
    app_chargebox_event_to_user(CHGBOX_EVENT_ENTER_CURRENT_PROTECT);
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电舱公共消息处理接口
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
static int chargebox_common_event_handler(struct chargebox_event *e)
{
    switch (e->event) {
#if (TCFG_WIRELESS_ENABLE)
    case CHGBOX_EVENT_WL_DATA_OVER:
        wireless_data_over_run();
        break;
    case CHGBOX_EVENT_WIRELESS_ONLINE:
        log_info("WL_ONLINE_1\n");
        wireless_api_open();
        sys_auto_shutdown_reset();
        break;
    case CHGBOX_EVENT_WIRELESS_OFFLINE:
        log_info("WL_OFF_1\n");
        wireless_api_close();
        sys_auto_shutdown_reset();
        break;
#endif
#if TCFG_TEMPERATURE_ENABLE
    case CHGBOX_EVENT_ENTER_TEMP_PROTECT:
        //关闭给耳机充电
        if (sys_info.charge) {
            chargeIc_pwr_ctrl(0);
            chargebox_api_open_port(EAR_L);
            chargebox_api_open_port(EAR_R);
            if (temp_protect_timer) {
                sys_timer_del(temp_protect_timer);
            }
            temp_protect_timer = sys_timeout_add(NULL, chargebox_temp_protect_timeout, TEMP_PROTECT_TIMEOUT);
        }
        //关闭给电池充电
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            chargebox_charge_close();
        }
        break;
    case CHGBOX_EVENT_EXIT_TEMP_PROTECT:
        //恢复给耳机充电
        if (sys_info.charge) {
            os_mutex_pend(&power_mutex, 0);
            u8 msg_flag = chargebox_check_output_short();
            if (sys_info.current_limit) {
                chargeIc_boost_ctrl(1);
                chargebox_api_shutdown_port(EAR_L);
                chargebox_api_shutdown_port(EAR_R);
                chargeIc_pwr_ctrl(1);
            } else if (msg_flag == 0) {
                sys_info.force_charge = 0;
                sys_info.earfull  = 1;
                app_chargebox_event_to_user(CHGBOX_EVENT_EAR_FULL);//让进入休眠
            }
            os_mutex_post(&power_mutex);
            if (temp_protect_timer) {
                sys_timer_del(temp_protect_timer);
                temp_protect_timer = 0;
            }
        }
        //恢复给电池充电
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            chargebox_charge_start();
        }
        break;
#endif
    case CHGBOX_EVENT_USB_IN:
        if (sys_info.current_limit) {
            sys_info.current_limit = 0;
            app_chargebox_event_to_user(CHGBOX_EVENT_EXIT_CURRENT_PROTECT);
        }
        //判断当前无线充是否在线，在线不响应
        if ((sys_info.status[WIRELESS_DET] == STATUS_OFFLINE) && (app_get_curr_task() != APP_PC_TASK)) {
#if TCFG_USB_KEY_UPDATE_ENABLE
            //先检测是否USB升级工具插入
            usb_key_check_entry(3);
#endif
#if TCFG_HANDSHAKE_ENABLE
            chgbox_handshake_run_app();
            chgbox_handshake_set_repeat(2);//多握手几次
#else
            app_chargebox_event_to_user(CHGBOX_EVENT_HANDSHAKE_OK);
#endif
        } else {
            app_chargebox_event_to_user(CHGBOX_EVENT_HANDSHAKE_OK);
        }
        break;
    case CHGBOX_EVENT_USB_OUT:
        chargebox_charge_close();
#if TCFG_HANDSHAKE_ENABLE
        chgbox_handshake_set_repeat(0);
#endif
#if TCFG_APP_PC_EN
        if (app_get_curr_task() != APP_IDLE_TASK) {
            app_task_switch_to(APP_IDLE_TASK);
        }
#endif
        break;
    case CHGBOX_EVENT_HANDSHAKE_OK:
        if ((sys_info.status[USB_DET] == STATUS_ONLINE) && (sys_info.temperature_limit == 0)) {
            chargebox_charge_start();
        }
#if TCFG_APP_PC_EN
        if (app_get_curr_task() != APP_PC_TASK) {
            app_task_switch_to(APP_PC_TASK);
        }
#endif
        break;
    }
    return true;
}

/******************************************************************************/
/*************************合盖充电模式相关处理**********************************/
/******************************************************************************/
static u8 ear_power_check_time;//耳机电量检测次数

/*------------------------------------------------------------------------------------*/
/**@brief    合盖充电模式  合盖处理
   @param    无
   @return   无
   @note     先发合盖命令给耳机(里面会有检测耳机在线的检测)，再打开升压进行充电
*/
/*------------------------------------------------------------------------------------*/
static void charge_app_lid_close_deal(void)
{
    if (sys_info.lid_cnt & BIT(7)) {
        if (sys_info.lid_cnt & (~BIT(7))) {
            sys_info.lid_cnt--;
            app_chargebox_send_mag(CHGBOX_MSG_SEND_CLOSE_LID);
        } else {
            sys_info.lid_cnt = 0;
            ear_power_check_time = 0;
            u8 msg_flag = chargebox_check_output_short();
            if (sys_info.current_limit == 0) {
                sys_info.charge = 1;//发完盒盖命令再开升压
                //先关闭IO,在打开开关输出5V
                if (sys_info.temperature_limit == 0) {
                    os_mutex_pend(&power_mutex, 0);
                    chargeIc_boost_ctrl(1);
                    chargebox_api_shutdown_port(EAR_L);
                    chargebox_api_shutdown_port(EAR_R);
                    chargeIc_pwr_ctrl(1);
                    os_mutex_post(&power_mutex);
                } else {
                    //过温保护,盒盖再推一次消息
                    app_chargebox_event_to_user(CHGBOX_EVENT_ENTER_TEMP_PROTECT);
                }
                //如果两只耳机都能检测在仓，说明耳机都有电，不需要强制充电
                if (ear_info.online[EAR_L] && ear_info.online[EAR_R]) {
                    sys_info.force_charge = 0;
                }
            } else if (msg_flag == 0) {
                //电流异常进入耳机关机流程
                sys_info.force_charge = 0;
                sys_info.earfull  = 1;
                app_chargebox_event_to_user(CHGBOX_EVENT_EAR_FULL);//让进入休眠
            }
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    合盖充电模式 让耳机进入关机 处理
   @param    无
   @return   无
   @note     先发命令让耳机关机，再关闭相关IO
*/
/*------------------------------------------------------------------------------------*/
static void charge_app_shut_down_deal(void)
{
    if (sys_info.shut_cnt & BIT(7)) {
        if (sys_info.shut_cnt & (~BIT(7))) {
            sys_info.shut_cnt--;
            app_chargebox_send_mag(CHGBOX_MSG_SEND_SHUTDOWN);
        } else {
            //关机命令后,电源线断电
            sys_info.shut_cnt = 0;
            chargebox_api_shutdown_port(EAR_L);
            chargebox_api_shutdown_port(EAR_R);
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    耳机电量满检测
   @param    无
   @return   无
   @note     检测耳机电量，若满则发满电事件
*/
/*------------------------------------------------------------------------------------*/
void app_chargebox_ear_full_det(void *priv)
{
    if (sys_info.chgbox_status != CHG_STATUS_CHARGE) {
        return;
    }

    /* log_info("L:%d,F:%d,C:%d\n",ear_info.online[EAR_L],sys_info.ear_l_full,ear_info.full_cnt[EAR_L]); */
    if (ear_info.online[EAR_L]) { //在线
        if ((ear_info.power[EAR_L] & 0x7f) == CHG_EAR_FULL_DET_LEVEL && sys_info.ear_l_full == 0) { //power的最高bit为标志位
            ear_info.full_cnt[EAR_L]++;
            if (ear_info.full_cnt[EAR_L] >= CHG_EAR_FULL_DET_CNT) {
                sys_info.ear_l_full = 1;       //充满标志置位
            }
        } else {
            ear_info.full_cnt[EAR_L] = 0;
        }
    } else {
        ear_info.full_cnt[EAR_L] = 0;  //左计数清0
        sys_info.ear_l_full = 0;       //左充满标志清0
    }

    if (ear_info.online[EAR_R]) { //在线
        if ((ear_info.power[EAR_R] & 0x7f) == CHG_EAR_FULL_DET_LEVEL && sys_info.ear_r_full == 0) { //power的最高bit为标志位
            ear_info.full_cnt[EAR_R]++;
            if (ear_info.full_cnt[EAR_R] >= CHG_EAR_FULL_DET_CNT) {
                sys_info.ear_r_full = 1;       //充满标志置位
            }
        } else {
            ear_info.full_cnt[EAR_R] = 0;
        }
    } else {
        ear_info.full_cnt[EAR_R] = 0;  //右计数清0
        sys_info.ear_r_full = 0;       //右充满标志清0
    }

    if (sys_info.earfull == 0) {
        //同时在线两个在线都满了、单个在线电满了
        if ((sys_info.ear_r_full && sys_info.ear_l_full)
            || (sys_info.ear_l_full && ear_info.online[EAR_R] == 0)
            || (sys_info.ear_r_full && ear_info.online[EAR_L] == 0)) {
            if (sys_info.force_charge == 0) { //强制充电已过
                sys_info.earfull  = 1;
                log_info("ear online full\n");
                app_chargebox_event_to_user(CHGBOX_EVENT_EAR_FULL);
            }
        }
    } else { //没满但在线
        if ((!sys_info.ear_l_full && ear_info.online[EAR_L])
            || (!sys_info.ear_r_full && ear_info.online[EAR_R])) {
            sys_info.earfull  = 0;//总标志清0
        }
    }

    ///已过强制充电时间，但两都不在线,走full，里面会判断仓是否充电
    if (ear_info.online[EAR_L] == 0 && ear_info.online[EAR_R] == 0 && sys_info.force_charge == 0) {
        log_info("no ear and force charge end\n");
        sys_info.earfull  = 1;
        app_chargebox_event_to_user(CHGBOX_EVENT_EAR_FULL);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    获取tws电量
   @param    无
   @return   无
   @note     检测电量，发送命令获取tws电量
*/
/*------------------------------------------------------------------------------------*/
static void charge_app_send_power(void)
{
    if (sys_info.shut_cnt || sys_info.lid_cnt || sys_info.earfull || sys_info.force_charge) {
        return;
    }
    ear_power_check_time++;
    if (ear_power_check_time > 25) { //5s
        ear_power_check_time = 0;
        app_chargebox_send_mag(CHGBOX_MSG_SEND_POWER_CLOSE);
        //发完命令后还要等对方回复，线程会切换，定时去查询
        sys_timeout_add(NULL, app_chargebox_ear_full_det, 200);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    自动关机检测
   @param    无
   @return   无
   @note     根据条件判断是否使能自动关机
*/
/*------------------------------------------------------------------------------------*/
static void charge_app_check_enable_auto_shutdown(void)
{
    //1、充电不在线时,判断耳机是否充满,充满则关机
    //2、充满关机使能后,USB在线时耳机和充电舱都充满则关机
#if TCFG_WIRELESS_ENABLE
    if ((sys_info.status[USB_DET] == STATUS_OFFLINE) && (sys_info.status[WIRELESS_DET] == STATUS_OFFLINE)) {
#else
    if (sys_info.status[USB_DET] == STATUS_OFFLINE) {
#endif
        if (sys_info.earfull) {
            sys_auto_shutdown_enable();
            return;
        }
    } else {
#if TCFG_CHARGE_FULL_ENTER_SOFTOFF
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            if (sys_info.localfull && sys_info.earfull) {
                sys_auto_shutdown_enable();
            }
        }
#endif
    }
}

#define LDO_NOT_SUCC_TIMES  20   //次数，注意时间尺度
static u8 ldo_not_succ_cnt = 0;  //LDO无法升压 计数
/*------------------------------------------------------------------------------------*/
/**@brief    合盖充电半秒处理
   @param    无
   @return   无
   @note     充电升压不成功处理、强制充电超时计数
*/
/*------------------------------------------------------------------------------------*/
static void charge_deal_half_second(void)
{
    //没有接充电线，充电IC不升压，超时进入休眠
#if TCFG_WIRELESS_ENABLE
    if ((sys_info.status[USB_DET] == STATUS_OFFLINE) && (sys_info.status[WIRELESS_DET] == STATUS_OFFLINE)) {
#else
    if (sys_info.status[USB_DET] == STATUS_OFFLINE) {
#endif
        if (sys_info.status[LDO_DET] == STATUS_OFFLINE) { //无法升压
            if (ldo_not_succ_cnt < LDO_NOT_SUCC_TIMES) {
                ldo_not_succ_cnt++;
                if (ldo_not_succ_cnt == LDO_NOT_SUCC_TIMES) {
                    log_info("auto shutdown by ldo not succ\n");
                    if (sys_info.force_charge || sys_info.temperature_limit) {
                        sys_info.force_charge = 0;
                        sys_info.earfull  = 1;
                        //走这个分支，如果仓还在充电就等仓满再关
                        app_chargebox_event_to_user(CHGBOX_EVENT_EAR_FULL);//进入休眠
                    } else {
                        sys_auto_shutdown_enable();
                    }
                }
            }
        } else {
            ldo_not_succ_cnt = 0;
        }
    }

    //强制充电超时计数（有可能是耳机不在线，有可能是耳机完全没电,先保持充电一段时间）
    if (sys_info.force_charge) {
        sys_info.force_charge--;
    }

    if (ear_info.online[EAR_L] && ear_info.online[EAR_R]) {
        sys_info.force_charge = 0;
    }

#if TCFG_APP_BT_EN
    if (get_curr_channel_state()) {
        bt_auto_off_cnt = CHGBOX_BT_AUTO_OFF_TIMES;
    } else if (bt_auto_off_cnt) {
        bt_auto_off_cnt--;
        if (bt_auto_off_cnt == 0) {
            app_task_switch_to(APP_IDLE_TASK);
        }
    }
#endif
}

static int charge_chargebox_event_handler(struct chargebox_event *e)
{
    switch (e->event) {
    case CHGBOX_EVENT_200MS:
        charge_app_lid_close_deal();
        charge_app_send_power();
        charge_app_shut_down_deal();
        break;
    case CHGBOX_EVENT_500MS:
        charge_deal_half_second();
        break;
#if TCFG_WIRELESS_ENABLE
    case CHGBOX_EVENT_WIRELESS_ONLINE:
        sys_auto_shutdown_disable();
        break;
    case CHGBOX_EVENT_WIRELESS_OFFLINE:
        charge_app_check_enable_auto_shutdown();
        break;
#endif
    case CHGBOX_EVENT_USB_IN:
        log_info("USB_IN_1\n");
        chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_USB_IN);
        sys_auto_shutdown_disable();
        if (sys_info.status[LID_DET] == STATUS_ONLINE) {
            //开盖插入USB
            chargebox_set_newstatus(CHG_STATUS_COMM);     //设置新模式
        }
        break;
    case CHGBOX_EVENT_USB_OUT:
        log_info("USB_OUT_1\n");
        chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_USB_OUT);
        charge_app_check_enable_auto_shutdown();
        if (sys_info.status[LID_DET] == STATUS_ONLINE) {
            //开盖拔出USB
            chargebox_set_newstatus(CHG_STATUS_COMM);     //设置新模式
        }
        break;
    case CHGBOX_EVENT_OPEN_LID:
        log_info("OPEN_LID_1\n");
#if SMART_BOX_EN
        bt_ble_rcsp_adv_enable();
#endif
        chargebox_set_newstatus(CHG_STATUS_COMM);     //设置新模式
        break;
    case CHGBOX_EVENT_CLOSE_LID:
        log_info("CLOSE_LID_1\n");
        //开盖超时进来的,可以不做任何操作
#if SMART_BOX_EN
        bt_ble_rcsp_adv_disable();
#endif
        break;
    case CHGBOX_EVENT_EAR_L_ONLINE:
        log_info("EAR_L_IN_1\n");
        chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_EAR_L_IN);
        break;
    case CHGBOX_EVENT_EAR_L_OFFLINE:
        log_info("EAR_L_OUT_1\n");
        chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_EAR_L_OUT);
        break;
    case CHGBOX_EVENT_EAR_R_ONLINE:
        log_info("EAR_R_IN_1\n");
        chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_EAR_R_IN);
        break;
    case CHGBOX_EVENT_EAR_R_OFFLINE:
        log_info("EAR_R_OUT_1\n");
        chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_EAR_R_OUT);
        break;
    case CHGBOX_EVENT_ENTER_LOWPOWER:
    case CHGBOX_EVENT_NEED_SHUTDOWN:
        //关闭升压,进入低电模式
        chargebox_set_newstatus(CHG_STATUS_LOWPOWER);     //设置新模式
        break;
    case CHGBOX_EVENT_EAR_FULL:
        //耳机充满后发送shutdown指令
        log_info("EAR_FULL_1\n");
        if (!sys_info.current_limit) {
            chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_EAR_FULL);
        }
        //充满电时,先关闭输出再打开IO
        chargeIc_pwr_ctrl(0);
        chargebox_api_open_port(EAR_L);
        chargebox_api_open_port(EAR_R);
        chargeIc_boost_ctrl(0);
        sys_info.shut_cnt = BIT(7) | TCFG_SEND_SHUT_DOWN_MAX;
        sys_info.lid_cnt = 0;
        sys_info.charge = 0;//此时充电结束
        charge_app_check_enable_auto_shutdown();
        break;
    case CHGBOX_EVENT_LOCAL_FULL:
        log_info("LOCAL_FULL_1\n");
        chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_LOCAL_FULL);
        charge_app_check_enable_auto_shutdown();
        break;
    case CHGBOX_EVENT_ENTER_CURRENT_PROTECT:
        log_info("CHGBOX_EVENT_OVER_CURRENT");
        chargeIc_pwr_ctrl(0);//关闭充电开关
        chargeIc_boost_ctrl(0);//关闭升压
        sys_info.force_charge = 0;
        sys_info.earfull  = 1;
        app_chargebox_event_to_user(CHGBOX_EVENT_EAR_FULL);//让进入休眠
        chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_OVER_CURRENT);
        break;
    case CHGBOX_EVENT_EXIT_CURRENT_PROTECT:
        ear_power_check_time = 0;
        sys_info.shut_cnt = 0;
        sys_info.lid_cnt = BIT(7) | TCFG_SEND_CLOSE_LID_MAX;
        sys_info.charge = 0;//先关闭,等合盖命令发完才打开升压
        sys_info.ear_l_full = 0;
        sys_info.ear_r_full = 0;
        sys_info.earfull = 0;
        sys_info.force_charge = CHGBOX_FORCE_CHARGE_TIMES;
        sys_auto_shutdown_disable();
        break;
    }
    return 0;
}







/******************************************************************************/
/*************************开盖通信模式相关处理**********************************/
/******************************************************************************/
#define KEY_PAIR_CNT    10
#define COMM_LIFE_MAX   (60*2)//一分钟超时
static u8 goto_pair_cnt;//配对功能按键时间计数
static u8 auto_exit_cnt;//退出开盖通信模式计数

/*------------------------------------------------------------------------------------*/
/**@brief    配对状态检测处理
   @param    无
   @return   无
   @note     当 pair_status==2时，判断是否配对成功
*/
/*------------------------------------------------------------------------------------*/
static void comm_pair_connecting(void)
{
    if (sys_info.pair_status == 2) {
        if (sys_info.pair_succ) {
            sys_info.pair_status = 0;
            chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_PAIR_SUCC);
        } else {
            app_chargebox_send_mag(CHGBOX_MSG_SEND_PAIR);
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    开盖通信模式超时
   @param    无
   @return   无
   @note     开盖时间过长，切换模式
*/
/*------------------------------------------------------------------------------------*/
static void comm_app_auto_exit(void)
{
    if (auto_exit_cnt++ > COMM_LIFE_MAX) {
#if SMART_BOX_EN
        bt_ble_rcsp_adv_disable();
#endif
        if (sys_info.lowpower_flag) {
            chargebox_set_newstatus(CHG_STATUS_LOWPOWER);     //设置新模式
        } else {
            chargebox_set_newstatus(CHG_STATUS_CHARGE);     //设置新模式
        }
    }
#if TCFG_APP_BT_EN
    if (get_curr_channel_state()) {
        bt_auto_off_cnt = CHGBOX_BT_AUTO_OFF_TIMES;
    } else if (bt_auto_off_cnt) {
        bt_auto_off_cnt--;
        if (bt_auto_off_cnt == 0) {
            app_task_switch_to(APP_IDLE_TASK);
        }
    }
#endif
}

static int comm_chargebox_event_handler(struct chargebox_event *e)
{
    switch (e->event) {
    case CHGBOX_EVENT_200MS:
        if (chgbox_adv_addr_scan()) {
            app_chargebox_send_mag(CHGBOX_MSG_SEND_POWER_OPEN);
        }
        break;
    case CHGBOX_EVENT_500MS:
        comm_pair_connecting();
        comm_app_auto_exit();
        break;
    case CHGBOX_EVENT_USB_IN:
        log_info("USB_IN_2\n");
        chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_USB_IN);
        app_chargebox_send_mag(CHGBOX_MSG_SEND_POWER_OPEN);
        auto_exit_cnt = 0;
        break;
    case CHGBOX_EVENT_USB_OUT:
        log_info("USB_OUT_2\n");
        chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_USB_OUT);
        app_chargebox_send_mag(CHGBOX_MSG_SEND_POWER_OPEN);
        auto_exit_cnt = 0;
        break;
    case CHGBOX_EVENT_LOCAL_FULL:
        log_info("LOCAL_FULL_2\n");
        chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_LOCAL_FULL);
        break;
    case CHGBOX_EVENT_CLOSE_LID:
        log_info("CLOSE_LID_2\n");
#if SMART_BOX_EN
        bt_ble_rcsp_adv_disable();
#endif
        if (sys_info.lowpower_flag) {
            chargebox_set_newstatus(CHG_STATUS_LOWPOWER);     //设置新模式
        } else {
            chargebox_set_newstatus(CHG_STATUS_CHARGE);     //设置新模式
        }
        break;
    case CHGBOX_EVENT_NEED_SHUTDOWN:
        chargebox_set_newstatus(CHG_STATUS_LOWPOWER);     //设置新模式
        break;
    case CHGBOX_EVENT_EAR_L_ONLINE:
        log_info("EAR_L_IN_2\n");
        auto_exit_cnt = 0;
        if (chgbox_get_ui_power_on() == 0) {
            chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_EAR_L_IN);
        }
        break;
    case CHGBOX_EVENT_EAR_L_OFFLINE:
        log_info("EAR_L_OUT_2\n");
        auto_exit_cnt = 0;
#if SMART_BOX_EN
        bt_ble_rcsp_adv_disable();
#endif
        chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_EAR_L_OUT);
        break;
    case CHGBOX_EVENT_EAR_R_ONLINE:
        log_info("EAR_R_IN_2\n");
        auto_exit_cnt = 0;
        if (chgbox_get_ui_power_on() == 0) {
            chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_EAR_R_IN);
        }
        break;
    case CHGBOX_EVENT_EAR_R_OFFLINE:
        log_info("EAR_R_OUT_2\n");
        auto_exit_cnt = 0;
#if SMART_BOX_EN
        bt_ble_rcsp_adv_disable();
#endif
        chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_EAR_R_OUT);
        break;
    }
    return 0;
}


/******************************************************************************/
/*************************低电模式处理*****************************************/
/******************************************************************************/
/*------------------------------------------------------------------------------------*/
/**@brief   低电休眠计数
   @param    无
   @return   无
   @note     先发关机指令，再休眠
*/
/*------------------------------------------------------------------------------------*/
static void lowpower_shut_down_deal(void)
{
    if (sys_info.shut_cnt & BIT(7)) {
        if (sys_info.shut_cnt & (~BIT(7))) {
            sys_info.shut_cnt--;
            app_chargebox_send_mag(CHGBOX_MSG_SEND_SHUTDOWN);
        } else {
            //关机命令后,电源线断电
            sys_info.charge = 0;
            sys_info.shut_cnt = 0;
            chargebox_api_shutdown_port(EAR_L);
            chargebox_api_shutdown_port(EAR_R);
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    低电合盖命令计数
   @param    无
   @return   无
   @note     lid_cnt标志置位后先发合盖命令，再置位shut_cnt标志
*/
/*------------------------------------------------------------------------------------*/
static void lowpower_lid_close_deal(void)
{
    if (sys_info.lid_cnt & BIT(7)) {
        if (sys_info.lid_cnt & (~BIT(7))) {
            sys_info.lid_cnt--;
            app_chargebox_send_mag(CHGBOX_MSG_SEND_CLOSE_LID);
        } else {
            sys_info.lid_cnt = 0;
            sys_info.shut_cnt = BIT(7) | TCFG_SEND_SHUT_DOWN_MAX;
        }
    }
}

static int lowpower_chargebox_event_handler(struct chargebox_event *e)
{
    switch (e->event) {
    case CHGBOX_EVENT_200MS:
        lowpower_lid_close_deal();
        lowpower_shut_down_deal();
        break;
    case CHGBOX_EVENT_500MS:
        break;
#if TCFG_WIRELESS_ENABLE
    case CHGBOX_EVENT_WIRELESS_ONLINE:
        sys_auto_shutdown_disable();
        break;
    case CHGBOX_EVENT_WIRELESS_OFFLINE:
        sys_auto_shutdown_enable();
        break;
#endif
    case CHGBOX_EVENT_USB_IN:
        log_info("USB_IN_3\n");
        sys_auto_shutdown_disable();
        chgbox_ui_update_status(UI_MODE_LOWPOWER, CHGBOX_UI_USB_IN);
        break;
    case CHGBOX_EVENT_USB_OUT:
        log_info("USB_OUT_3\n");
#if TCFG_WIRELESS_ENABLE
        if (sys_info.status[WIRELESS_DET] == STATUS_OFFLINE)
#endif
        {
            sys_auto_shutdown_enable();
        }
        chgbox_ui_update_status(UI_MODE_LOWPOWER, CHGBOX_UI_USB_OUT);
        break;
    case CHGBOX_EVENT_OPEN_LID:
        log_info("OPEN_LID_3\n");
        chgbox_ui_update_status(UI_MODE_LOWPOWER, CHGBOX_UI_OPEN_LID);
#if SMART_BOX_EN
        bt_ble_rcsp_adv_enable();
#endif
        chargebox_set_newstatus(CHG_STATUS_COMM);     //设置新模式
        break;
    case CHGBOX_EVENT_CLOSE_LID:
        log_info("CLOSE_LID_3\n");
        chgbox_ui_update_status(UI_MODE_LOWPOWER, CHGBOX_UI_CLOSE_LID);
#if SMART_BOX_EN
        bt_ble_rcsp_adv_disable();
#endif
        break;
    case CHGBOX_EVENT_NEED_SHUTDOWN:
        if ((!sys_info.lid_cnt) && (sys_info.shut_cnt)) {
            power_set_soft_poweroff();
        }
        break;
    case CHGBOX_EVENT_EXIT_LOWPOWER:
        log_info("exit lower\n");
        if (sys_info.status[LID_DET] == STATUS_ONLINE) {
            chargebox_set_newstatus(CHG_STATUS_COMM);     //设置新模式
        } else {
            chargebox_set_newstatus(CHG_STATUS_CHARGE);     //设置新模式
        }
        break;
    }
    return 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓事件处理
   @param    event:传入的事件结构体，携带事件信息
   @return   无
   @note     分模式处理相关各种事件（相同事件在不同模式可能会有不同处理）
*/
/*------------------------------------------------------------------------------------*/
int charge_box_ctrl_event_handler(struct chargebox_event *chg_event)
{
    chargebox_common_event_handler(chg_event);
    if (sys_info.chgbox_status == CHG_STATUS_CHARGE) {
        charge_chargebox_event_handler(chg_event);
    } else if (sys_info.chgbox_status == CHG_STATUS_COMM) {
        comm_chargebox_event_handler(chg_event);
    } else if (sys_info.chgbox_status == CHG_STATUS_LOWPOWER) {
        lowpower_chargebox_event_handler(chg_event);
    }
    return 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓按键处理
   @param    event:按键事件
   @return   无
   @note     分模式处理相关按键
*/
/*------------------------------------------------------------------------------------*/
int charge_box_key_event_handler(u16 event)
{
    if (sys_info.chgbox_status == CHG_STATUS_CHARGE) {
        sys_info.life_cnt = 0;
        switch (event) {
        case KEY_BOX_POWER_CLICK:
            log_info("KEY_POWER_CLICK_chg\n");
            if (sys_info.status[LID_DET] == STATUS_ONLINE) {
                //开盖拔出USB
                chargebox_set_newstatus(CHG_STATUS_COMM);     //设置新模式
            } else {
                chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_KEY_CLICK);
            }
            break;
        case KEY_BOX_POWER_FIVE:
            log_info("KEY_POWER_FIVE_chg\n");
#if TCFG_APP_BT_EN
            if (app_get_curr_task() == APP_IDLE_TASK) {
                bt_auto_off_cnt = CHGBOX_BT_AUTO_OFF_TIMES;
                app_task_switch_to(APP_BT_TASK);
            }
#endif
            break;
        default:
            break;
        }
    } else if (sys_info.chgbox_status == CHG_STATUS_COMM) {
        auto_exit_cnt = 0;
        switch (event) {
        case KEY_BOX_POWER_CLICK:
            log_info("KEY_POWER_CLICK_comm\n");
            chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_KEY_CLICK);
            break;
        case KEY_BOX_POWER_LONG:
            log_info("KEY_POWER_LONG\n");
            if (sys_info.pair_status == 0) {
                sys_info.pair_status = 1;
                goto_pair_cnt = 0;
                chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_KEY_LONG);
            }
            break;
        case KEY_BOX_POWER_HOLD:
            /* log_info("KEY_POWER_HOLD\n"); */
            if (sys_info.pair_status == 1) {
                goto_pair_cnt++;
                if (goto_pair_cnt >= KEY_PAIR_CNT) {
                    sys_info.pair_status = 2;
                    sys_info.pair_succ = 0;
                    chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_PAIR_START);
                }
            }
            break;
        case KEY_BOX_POWER_UP:
            log_info("KEY_POWER_UP\n");
            if (sys_info.pair_status != 2) {
                sys_info.pair_status = 0;
                chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_PAIR_STOP);
            }
            break;
        case KEY_BOX_POWER_FIVE:
            log_info("KEY_POWER_FIVE_comm\n");
#if TCFG_APP_BT_EN
            if (app_get_curr_task() == APP_IDLE_TASK) {
                bt_auto_off_cnt = CHGBOX_BT_AUTO_OFF_TIMES;
                app_task_switch_to(APP_BT_TASK);
            }
#endif
            break;
        default:
            break;
        }
    } else if (sys_info.chgbox_status == CHG_STATUS_LOWPOWER) {
        switch (event) {
        case KEY_BOX_POWER_CLICK:
            log_info("KEY_POWER_CLICK_low\n");
            if (sys_info.status[USB_DET] == STATUS_OFFLINE) {
                chgbox_ui_update_status(UI_MODE_LOWPOWER, CHGBOX_UI_KEY_CLICK);
            }
            break;
        default:
            break;
        }
    }
    return 0;
}


/*------------------------------------------------------------------------------------*/
/**@brief    充电仓模式设置函数
   @param    newstatus:新的模式
   @return   无
   @note     退出当前模式，设置新的模式的参数
*/
/*------------------------------------------------------------------------------------*/
void chargebox_set_newstatus(u8 newstatus)
{
    ///先退出当前状态
    log_info("chargebbox exit:%d\n", sys_info.chgbox_status);
    if (newstatus == sys_info.chgbox_status) {
        log_info("chargebbox status same\n");
        return;
    }

    if (sys_info.chgbox_status  == CHG_STATUS_COMM) {
        sys_auto_shutdown_enable();
        if (sys_info.pair_status) {
            sys_info.pair_status = 0;
            chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_ALL_OFF);
        }
    } else if (sys_info.chgbox_status == CHG_STATUS_CHARGE) {
        sys_info.lid_cnt = 0;
        sys_info.shut_cnt = 0;
        sys_info.charge = 0;//不在充电状态
        //关闭充电输出时,需要把IO打开
        chargeIc_boost_ctrl(0);
        chargeIc_pwr_ctrl(0);
        chargebox_api_open_port(EAR_L);
        chargebox_api_open_port(EAR_R);
        sys_auto_shutdown_enable();
    } else if (sys_info.chgbox_status == CHG_STATUS_LOWPOWER) {
        sys_info.shut_cnt = 0;
        sys_info.lid_cnt = 0;
    }

    ///设置新状态
    sys_info.chgbox_status = newstatus;
    if (newstatus  == CHG_STATUS_COMM) {
        sys_auto_shutdown_disable();
        sys_info.shut_cnt = 0;
        sys_info.pair_status = 0;
        sys_info.pair_succ = 0;
        sys_info.ear_l_full = 0;
        sys_info.ear_r_full = 0;
        sys_info.earfull = 0;
        goto_pair_cnt = 0;
        auto_exit_cnt = 0;
        chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_OPEN_LID);
    } else if (newstatus == CHG_STATUS_CHARGE) {
        ear_power_check_time = 0;
        sys_info.shut_cnt = 0;
        sys_info.lid_cnt = BIT(7) | TCFG_SEND_CLOSE_LID_MAX;
        sys_info.charge = 0;//先关闭,等合盖命令发完才打开升压
        sys_info.ear_l_full = 0;
        sys_info.ear_r_full = 0;
        sys_info.earfull = 0;
        sys_info.force_charge = CHGBOX_FORCE_CHARGE_TIMES;
        sys_auto_shutdown_disable();
        if (sys_info.current_limit == 0) {
            chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_CLOSE_LID);
        }
    } else if (newstatus == CHG_STATUS_LOWPOWER) {
        sys_info.shut_cnt = 0;
        sys_info.lid_cnt = 0;
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            sys_auto_shutdown_disable();
            chgbox_ui_update_status(UI_MODE_LOWPOWER, CHGBOX_UI_USB_IN);
        } else {
            sys_auto_shutdown_enable();
            chgbox_ui_update_status(UI_MODE_LOWPOWER, CHGBOX_UI_LOWPOWER);
        }

        if (!sys_info.earfull) {
            sys_info.lid_cnt = BIT(7) | TCFG_SEND_CLOSE_LID_MAX;
        }
#if TCFG_APP_BT_EN
        app_task_switch_to(APP_IDLE_TASK);
        bt_auto_off_cnt = 0;
#endif
    }
    log_info("chargebbox newstatus:%d\n", newstatus);
}

/*------------------------------------------------------------------------------------*/
/**@brief    获取对耳的在线状态,是否同时在线
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
u8 get_tws_ear_status(void)
{
    return (ear_info.online[EAR_L] && ear_info.online[EAR_R]);
}

/*------------------------------------------------------------------------------------*/
/**@brief    获取仓的合盖状态
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
u8 get_chgbox_lid_status(void)
{
    return (sys_info.status[LID_DET] == STATUS_ONLINE);
}


/*------------------------------------------------------------------------------------*/
/**@brief    仓控制初始化函数
   @param    无
   @return   无
   @note     根据上电状态选择进入开盖通信、低电量或合盖充电模式，初始化一些控制量
*/
/*------------------------------------------------------------------------------------*/
void charge_box_ctrl_init(void)
{
    //充电IC初始不成功,进行休眠(电压低等原因）
    if (!sys_info.init_ok) {
        log_error("chargeIc not ok, need softoff!\n");
        chgbox_enter_soft_power_off();
        return;
    }

    sys_info.power_on = 0;
    sys_info.shut_cnt = 0;
    sys_info.pair_status = 0;
    sys_info.pair_succ = 0;
    sys_info.charge = 0;//先关闭,等合盖命令发完才打开升压
    sys_info.ear_l_full = 0;
    sys_info.ear_r_full = 0;
    sys_info.earfull = 0;

    goto_pair_cnt = 0;
    auto_exit_cnt = 0;

    ///进入模式判断
    if (sys_info.status[LID_DET] == STATUS_ONLINE) {
        sys_info.chgbox_status = CHG_STATUS_COMM;                   //开盖
        sys_auto_shutdown_disable();
        if (sys_info.wireless_wakeup == 0) {
            chgbox_ui_update_status(UI_MODE_COMM, CHGBOX_UI_OPEN_LID);
        }
    } else {
        if (sys_info.lowpower_flag) {
            sys_info.chgbox_status = CHG_STATUS_LOWPOWER;           //低电量
            sys_info.lid_cnt = 0;
            if (sys_info.status[USB_DET] == STATUS_ONLINE) {
                sys_auto_shutdown_disable();
                chgbox_ui_update_status(UI_MODE_LOWPOWER, CHGBOX_UI_USB_IN);
            } else {
                sys_auto_shutdown_enable();
                if (sys_info.wireless_wakeup == 0) {
                    chgbox_ui_update_status(UI_MODE_LOWPOWER, CHGBOX_UI_LOWPOWER);
                }
            }
        } else {
            sys_info.chgbox_status = CHG_STATUS_CHARGE;            //合盖充电
            ear_power_check_time = 0;
            sys_info.lid_cnt = BIT(7) | TCFG_SEND_CLOSE_LID_MAX;
            sys_info.force_charge = CHGBOX_FORCE_CHARGE_TIMES;
            sys_auto_shutdown_disable();
            if (sys_info.wireless_wakeup == 0) {
                chgbox_ui_update_status(UI_MODE_CHARGE, CHGBOX_UI_POWER);
            }
        }
    }

    chgbox_ui_set_power_on(1);//ui上电标志

    if (sys_info.status[USB_DET] == STATUS_ONLINE) {
        app_chargebox_event_to_user(CHGBOX_EVENT_USB_IN);
    }
    log_info("chgbox_poweron_status:%d\n", sys_info.chgbox_status);
    sys_info.wireless_wakeup = 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    智能充电仓初始化函数
   @param    无
   @return   无
   @note     充电ic、霍尔传感器、无线充、lighting握手、ui、流程控制的初始化
*/
/*------------------------------------------------------------------------------------*/
void chgbox_init_app(void)
{
    //注意：提前初始化的内容放在了 __initcall(chargebox_advanced_init);
    chargeIc_init();
    chargebox_det_init();
#if (TCFG_WIRELESS_ENABLE)
    wireless_init_api();
#endif
#if (TCFG_HANDSHAKE_ENABLE)
    chgbox_handshake_init();
#endif
#if TCFG_CHARGE_BOX_UI_ENABLE
    chgbox_ui_manage_init();
#endif
    charge_box_ctrl_init();
}

#endif
