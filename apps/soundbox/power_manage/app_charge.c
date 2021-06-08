#include "app_config.h"
#include "asm/charge.h"
#include "asm/pwm_led.h"
#include "asm/power_interface.h"
#include "ui_manage.h"
#include "system/event.h"
#include "system/app_core.h"
#include "system/includes.h"
#include "app_action.h"
#include "asm/wdt.h"
#include "app_power_manage.h"
#include "app_chargestore.h"
#include "btstack/avctp_user.h"
#include "app_action.h"
#include "app_main.h"
#include "bt_tws.h"
#include "usb/otg.h"
#include "bt_common.h"
#include "app_task.h"

#define LOG_TAG_CONST       APP_CHARGE
#define LOG_TAG             "[APP_CHARGE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

extern void sys_enter_soft_poweroff(void *priv);
extern void gSensor_wkupup_enable(void);
extern void gSensor_wkupup_disable(void);
extern u32 dual_bank_update_exist_flag_get(void);
extern u32 classic_update_task_exist_flag_get(void);

#if TCFG_CHARGE_ENABLE

#if (!TCFG_IOKEY_ENABLE && !TCFG_ADKEY_ENABLE)
#ifndef TCFG_CHARGE_CHECK_SET_LDOIN_PINR
#define TCFG_CHARGE_CHECK_SET_LDOIN_PINR
#endif
#endif

static u8 charge_full_flag = 0;
#if TCFG_TEST_BOX_ENABLE
static int ldo5v_keep_timer = 0;
#endif

extern void power_event_to_user(u8 event);

u8 get_charge_full_flag(void)
{
    return charge_full_flag;
}

void charge_start_deal(void)
{
    log_info("%s\n", __FUNCTION__);

    power_set_mode(PWR_LDO15);  //充电切到LDO模式，DCDC模式会直接消耗VBat的电

    u8 app = app_get_curr_task();
    if (app == APP_IDLE_TASK) {
        ui_update_status(STATUS_CHARGE_START);
    }
}


static void ldo5v_keep_delay_deal(void *priv)
{
    log_info("%s\n", __func__);

#if TCFG_TEST_BOX_ENABLE
    ldo5v_keep_timer = 0;
    if (chargestore_get_testbox_status()) {
        log_info("testbox online!\n");
        return;
    }
#endif

    u8 app = app_get_curr_task();
    if (app != APP_IDLE_TASK) {
        sys_enter_soft_poweroff((void *)2);
    }

    pwm_led_mode_set(PWM_LED_ALL_OFF);
    os_time_dly(30);
    pwm_led_mode_set(PWM_LED1_ON);
    os_time_dly(40);

    //兼容一些充电仓5v输出慢的时候会导致无法充电的问题
    if (get_lvcmp_det()) {
        log_info("...charge ing...\n");
        cpu_reset();
    }

    log_info("get_charge_online_flag:%d %d\n", get_charge_online_flag(), get_ldo5v_online_hw());
    if (get_ldo5v_online_hw() && get_charge_online_flag()) {
        power_set_soft_poweroff();
    } else {
#if TCFG_CHARGE_OFF_POWERON_NE
        cpu_reset();
#else
#ifdef TCFG_CHARGE_CHECK_SET_LDOIN_PINR
        charge_check_and_set_pinr(1);
#endif
        power_set_soft_poweroff();
#endif
    }
}


/*ldoin电压大于拔出电压(0.6V左右)且小于VBat电压时调用该函数进行一些错误提示或其他处理*/
void ldo5v_keep_deal(void)
{

    log_info("%s\n", __func__);

    //插入交换
    power_event_to_user(POWER_EVENT_POWER_CHANGE);

#if TCFG_GSENSOR_ENABLE
    //在舱关闭gSensor
    gSensor_wkupup_disable();
#endif

#if TCFG_LP_TOUCH_KEY_ENABLE
    extern void lp_touch_key_charge_mode_enter();
    lp_touch_key_charge_mode_enter();
#endif /* #if TCFG_LP_TOUCH_KEY_ENABLE */

#if TCFG_GX8002_NPU_ENABLE
    extern void gx8002_module_suspend(u8 keep_vddio);
    gx8002_module_suspend(0);
#endif /* #if TCFG_GX8002_NPU_ENABLE */


#ifdef TCFG_CHARGE_CHECK_SET_LDOIN_PINR
    charge_check_and_set_pinr(0);
#endif

    if (get_charge_poweron_en() == 0) {
#if TCFG_CHARGESTORE_ENABLE
        //智能充电舱不处理充电err
        u8 app = app_get_curr_task();
        if (app == APP_IDLE_TASK) {
            ui_update_status(STATUS_CHARGE_LDO5V_OFF);
        }

#else //TCFG_CHARGESTORE_ENABLE

#if defined(TCFG_CHARGE_KEEP_UPDATA) && TCFG_CHARGE_KEEP_UPDATA
        if (dual_bank_update_exist_flag_get() || classic_update_task_exist_flag_get()) {
            return;
        }
#endif

#if TCFG_TEST_BOX_ENABLE
        if (ldo5v_keep_timer == 0) {
            //延时执行避免测试盒通信不上
            ldo5v_keep_timer = sys_timeout_add(NULL, ldo5v_keep_delay_deal, 250);
        }
#else
        ldo5v_keep_delay_deal(NULL);
#endif//TCFG_TEST_BOX_ENABLE
#endif//TCFG_CHARGESTORE_ENABLE
    } else {
        ui_update_status(STATUS_CHARGE_ERR);
    }
}

void charge_full_deal(void)
{
    log_info("%s\n", __func__);

    charge_full_flag = 1;

    /* charge_close(); */
    if (get_charge_poweron_en() == 0) {
        /* power_set_soft_poweroff(); */
#if TCFG_USER_TWS_ENABLE

#else
        ui_update_status(STATUS_CHARGE_FULL);
#endif
#if (!TCFG_CHARGESTORE_ENABLE)
        vbat_timer_delete();
#endif
    } else {
        ui_update_status(STATUS_CHARGE_FULL);
    }
}

void charge_close_deal(void)
{

    log_info("%s\n", __FUNCTION__);

    power_set_mode(TCFG_LOWPOWER_POWER_SEL);

#if TCFG_USER_TWS_ENABLE
    //在idle的时候才执行充电关闭的UI
    u8 app = app_get_curr_task();
    if (app == APP_IDLE_TASK) {
        ui_update_status(STATUS_CHARGE_LDO5V_OFF);
    }
#endif
}


void charge_ldo5v_in_deal(void)
{
    log_info("%s\n", __FUNCTION__);


#if TCFG_IRSENSOR_ENABLE
    if (get_bt_tws_connect_status()) {
        tws_api_sync_call_by_uuid('T', SYNC_CMD_EARPHONE_CHAREG_START, 300);
    }
#endif

    //插入交换
    power_event_to_user(POWER_EVENT_POWER_CHANGE);

    charge_full_flag = 0;

#if TCFG_GSENSOR_ENABLE
    //入舱关闭gSensor
    gSensor_wkupup_disable();
#endif

#if TCFG_LP_TOUCH_KEY_ENABLE
    extern void lp_touch_key_charge_mode_enter();
    lp_touch_key_charge_mode_enter();
#endif /* #if TCFG_LP_TOUCH_KEY_ENABLE */

#if TCFG_GX8002_NPU_ENABLE
    extern void gx8002_module_suspend(u8 keep_vddio);
    gx8002_module_suspend(0);
#endif /* #if TCFG_GX8002_NPU_ENABLE */

#ifdef TCFG_CHARGE_CHECK_SET_LDOIN_PINR
    charge_check_and_set_pinr(0);
#endif

#if TCFG_TEST_BOX_ENABLE
    chargestore_clear_testbox_status();
    if (ldo5v_keep_timer) {
        sys_timeout_del(ldo5v_keep_timer);
        ldo5v_keep_timer = 0;
    }
#endif

#if TCFG_CHARGESTORE_ENABLE
    chargestore_shutdown_reset();
#endif

    if (get_charge_poweron_en() == 0) {
#if defined(TCFG_CHARGE_KEEP_UPDATA) && TCFG_CHARGE_KEEP_UPDATA
        if (dual_bank_update_exist_flag_get() || classic_update_task_exist_flag_get()) {
            return;
        }
#endif
        u8 app = app_get_curr_task();
        if (app != APP_IDLE_TASK) {

#if (TCFG_CHARGESTORE_ENABLE && TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
            if (!chargestore_check_going_to_poweroff()) {
#endif
                sys_enter_soft_poweroff((void *)1);
#if (TCFG_CHARGESTORE_ENABLE && TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
            } else {
                log_info("chargestore do poweroff!\n");
            }
#endif
        } else {
            //添加cpu_reset防止刚开机app还是NULL的时候,进这里导致无法进idle充电
            extern int cpu_reset_by_soft();
            if (cpu_reset_by_soft()) {
                charge_start();
            } else {
                cpu_reset();
            }
            wdt_init(WDT_32S);
            log_info("set wdt to 32s!\n");
            goto _check_reset;
        }
    } else {
        charge_start();
        goto _check_reset;
    }
    return;

_check_reset:
    //防止耳机低电时,插拔充电有几率出现关机不充电问题
    if (app_var.goto_poweroff_flag) {
        cpu_reset();
    }
}

void charge_ldo5v_off_deal(void)
{
    log_info("%s\n", __FUNCTION__);


    //拨出交换
    power_event_to_user(POWER_EVENT_POWER_CHANGE);
#if (TCFG_CHARGESTORE_ENABLE && TCFG_USER_TWS_ENABLE)
    if (TWS_ROLE_SLAVE == tws_api_get_role()) {
        chargestore_set_power_level(0xFF);
    }
#endif
    charge_full_flag = 0;

    charge_close();

    u8 app = app_get_curr_task();
    if (app == APP_IDLE_TASK) {
        ui_update_status(STATUS_CHARGE_LDO5V_OFF);
    }

#ifdef TCFG_CHARGE_CHECK_SET_LDOIN_PINR
    charge_check_and_set_pinr(1);
#endif

#if TCFG_TEST_BOX_ENABLE
    if (chargestore_get_testbox_status() && !get_total_connect_dev()) {
        log_info("<<<<<<<<<<<<<<testbox out and bt noconn reset>>>>>>>>>>>>>>>\n");
        cpu_reset();
    }
    chargestore_clear_testbox_status();
    if (ldo5v_keep_timer) {
        sys_timeout_del(ldo5v_keep_timer);
        ldo5v_keep_timer = 0;
    }
#endif

#if TCFG_CHARGESTORE_ENABLE
    chargestore_shutdown_reset();
#endif

    if ((get_charge_poweron_en() == 0)) {

        wdt_init(WDT_4S);
        log_info("set wdt to 4s!\n");
#if TCFG_CHARGESTORE_ENABLE
        if (chargestore_get_power_status()) {
#endif
#if TCFG_CHARGE_OFF_POWERON_NE
            log_info("ldo5v off,task switch to BT\n");
            app_var.play_poweron_tone = 0;
#if TCFG_SYS_LVD_EN
            vbat_check_init();
#endif
            if ((app != APP_BT_TASK) && (app_var.goto_poweroff_flag == 0)) {
#if TCFG_SYS_LVD_EN
                if (get_vbat_need_shutdown() == FALSE) {
                    task_switch_to_bt();
                } else {
                    log_info("ldo5v off,lowpower,need enter softpoweroff\n");
                    power_set_soft_poweroff();
                }
#else //TCFG_SYS_LVD_EN
                task_switch_to_bt();
#endif //TCFG_SYS_LVD_EN
            }
#else //TCFG_CHARGE_OFF_POWERON_NE
            log_info("ldo5v off,enter softpoweroff\n");
            power_set_soft_poweroff();
#endif //TCFG_CHARGE_OFF_POWERON_NE
#if TCFG_CHARGESTORE_ENABLE
        } else {
            log_info("ldo5v off,enter softpoweroff\n");
            if (app != APP_IDLE_TASK) {
                sys_enter_soft_poweroff(NULL);//软关机
            } else {
                power_set_soft_poweroff();
            }
        }
#endif
    } else {
        if (app == APP_IDLE_TASK) {
            power_set_soft_poweroff();
        }
    }
#if TCFG_GSENSOR_ENABLE
    //出舱使能gSensor
    gSensor_wkupup_enable();
#endif

#if TCFG_LP_TOUCH_KEY_ENABLE
    extern void lp_touch_key_charge_mode_exit();
    lp_touch_key_charge_mode_exit();
#endif /* #if TCFG_LP_TOUCH_KEY_ENABLE */

#if TCFG_GX8002_NPU_ENABLE
    extern void gx8002_module_resumed();
    gx8002_module_resumed();
#endif /* #if TCFG_GX8002_NPU_ENABLE */
}

int app_charge_event_handler(struct device_event *dev)
{
    int ret = false;
    u8 otg_status = 0;

    switch (dev->event) {
    case CHARGE_EVENT_CHARGE_START:
        charge_start_deal();
        break;
    case CHARGE_EVENT_CHARGE_CLOSE:
        charge_close_deal();
        break;
    case CHARGE_EVENT_CHARGE_FULL:
        charge_full_deal();
        break;
    case CHARGE_EVENT_LDO5V_KEEP:
        ldo5v_keep_deal();
        break;
    case CHARGE_EVENT_LDO5V_IN:
#if ((TCFG_OTG_MODE & OTG_SLAVE_MODE) && (TCFG_OTG_MODE & OTG_CHARGE_MODE))
        while (1) {
            otg_status = usb_otg_online(0);
            if (otg_status != IDLE_MODE) {
                break;
            }
            os_time_dly(2);
        }
#endif
        if (otg_status != SLAVE_MODE) {
            charge_ldo5v_in_deal();
        }
        break;
    case CHARGE_EVENT_LDO5V_OFF:
#if ((TCFG_OTG_MODE & OTG_SLAVE_MODE) && (TCFG_OTG_MODE & OTG_CHARGE_MODE))
        while (1) {
            otg_status = usb_otg_online(0);
            if (otg_status != IDLE_MODE) {
                break;
            }
            os_time_dly(2);
        }
#endif
        if (otg_status != SLAVE_MODE) {
            charge_ldo5v_off_deal();
        }
        break;
    default:
        break;
    }

    return ret;
}

#else

u8 get_charge_full_flag(void)
{
    return 0;
}

#endif

