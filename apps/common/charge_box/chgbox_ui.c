#include "typedef.h"
#include "asm/pwm_led.h"
#include "system/includes.h"
#include "chgbox_ctrl.h"
#include "chargeIc_manage.h"
#include "chgbox_ui.h"
#include "app_config.h"

#if (TCFG_CHARGE_BOX_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHGBOXUI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if (TCFG_CHARGE_BOX_UI_ENABLE)

//关于仓ui的说明，分为三个部分
//1.ui状态层
//2.ui中间层
//3.ui驱动层
//状态层主要就是外部把仓的状态传进来，中间层是一个过渡，如不想用本驱动，可以自己更换中间层
//或者只使用本驱动层作其他使用

/////////////////////////////////////////////////////////////////////////////////////////////
//ui状态层
typedef struct _chgbox_ui_var_ {
    int ui_timer;
    u8  ui_power_on; //上电标志
} _chgbox_ui_var;

static _chgbox_ui_var chgbox_ui_var;
#define __this  (&chgbox_ui_var)

/*------------------------------------------------------------------------------------*/
/**@brief    UI超时函数
   @param    priv:ui状态
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_update_timeout(void *priv)
{
    u8 ledmode = (u8)priv;
    __this->ui_timer = 0;
    chgbox_led_set_mode(ledmode);
}

/*------------------------------------------------------------------------------------*/
/**@brief    UI超时设置
   @param    priv:传到func的参数
             func:超时后的回调函数
             msec:N毫秒后调用func
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
u16 chgbox_ui_timeout_add(int priv, void (*func)(void *priv), u32 msec)
{
    if (__this->ui_timer) {
        sys_timer_del(__this->ui_timer);
        __this->ui_timer = 0;
    }
    if (func != NULL) {
        __this->ui_timer = sys_timeout_add((void *)priv, func, msec);
    }
    return __this->ui_timer;
}

/*------------------------------------------------------------------------------------*/
/**@brief    设置ui上电标志位
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_set_power_on(u8 flag)
{
    __this->ui_power_on = flag;
}

/*------------------------------------------------------------------------------------*/
/**@brief    获取ui上电标志位
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
u8 chgbox_get_ui_power_on(void)
{
    return __this->ui_power_on;
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓电量态ui更新
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_update_local_power(void)
{
    //配对状态,不显示舱电量UI
    if (sys_info.pair_status) {
        return;
    }
    if (sys_info.status[USB_DET] == STATUS_ONLINE) {
        chgbox_ui_timeout_add(0, NULL, 0);
        if (sys_info.localfull) {
            chgbox_led_set_mode(CHGBOX_LED_GREEN_ON);//充满后绿灯常亮
        } else {
            chgbox_led_set_mode(CHGBOX_LED_RED_SLOW_BRE);//充电中灯慢闪
        }
    } else {
        if (sys_info.lowpower_flag) {
            chgbox_led_set_mode(CHGBOX_LED_RED_FAST_BRE); //快闪4秒
            chgbox_ui_timeout_add(CHGBOX_LED_RED_OFF, chgbox_ui_update_timeout, 4000);
        } else {
            chgbox_led_set_mode(CHGBOX_LED_GREEN_ON);
            chgbox_ui_timeout_add(CHGBOX_LED_GREEN_OFF, chgbox_ui_update_timeout, 8000);
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓公共态ui更新
   @param    status:UI状态
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_updata_default_status(u8 status)
{
    switch (status) {
    case CHGBOX_UI_ALL_OFF:
        chgbox_ui_timeout_add(0, NULL, 0);
        chgbox_led_set_mode(CHGBOX_LED_ALL_OFF);
        break;
    case CHGBOX_UI_ALL_ON:
        chgbox_ui_timeout_add(0, NULL, 0);
        chgbox_led_set_mode(CHGBOX_LED_ALL_ON);
        break;
    case CHGBOX_UI_POWER:
        chgbox_ui_update_local_power();
        break;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓合盖充电ui更新
   @param    status:UI状态
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_updata_charge_status(u8 status)
{
    switch (status) {
    case CHGBOX_UI_USB_IN:
    case CHGBOX_UI_KEY_CLICK:
    case CHGBOX_UI_LOCAL_FULL:
        chgbox_ui_update_local_power();
        break;
    case CHGBOX_UI_USB_OUT:
        chgbox_ui_timeout_add(0, NULL, 0);
        chgbox_led_set_mode(CHGBOX_LED_ALL_OFF);
        break;
    case CHGBOX_UI_CLOSE_LID:
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            chgbox_ui_update_local_power();
        } else {
            chgbox_ui_timeout_add(0, NULL, 0);
            chgbox_led_set_mode(CHGBOX_LED_GREEN_OFF);
        }
        break;
    case CHGBOX_UI_EAR_FULL:
        break;
    case CHGBOX_UI_OVER_CURRENT:
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            chgbox_ui_update_local_power();
        } else {
            chgbox_led_set_mode(CHGBOX_LED_RED_FAST_BRE); //快闪4秒
            chgbox_ui_timeout_add(CHGBOX_LED_RED_OFF, chgbox_ui_update_timeout, 4000);
        }
        break;
    default:
        chgbox_ui_updata_default_status(status);
        break;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓开盖通信ui更新
   @param    status:UI状态
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_updata_comm_status(u8 status)
{
    switch (status) {
    case CHGBOX_UI_USB_IN:
    case CHGBOX_UI_LOCAL_FULL:
    case CHGBOX_UI_KEY_CLICK:
    case CHGBOX_UI_OPEN_LID:
        chgbox_ui_update_local_power();
        break;
    case CHGBOX_UI_USB_OUT:
        if (!sys_info.pair_status) {
            chgbox_ui_timeout_add(0, NULL, 0);
            chgbox_led_set_mode(CHGBOX_LED_RED_OFF);
        }
        break;
    case CHGBOX_UI_EAR_L_IN:
    case CHGBOX_UI_EAR_R_IN:
    case CHGBOX_UI_EAR_L_OUT:
    case CHGBOX_UI_EAR_R_OUT:
        if (!sys_info.pair_status) {
            if (sys_info.status[USB_DET] == STATUS_ONLINE) {
                if (sys_info.localfull) {
                    chgbox_led_set_mode(CHGBOX_LED_RED_OFF);
                    chgbox_ui_timeout_add(CHGBOX_LED_RED_ON, chgbox_ui_update_timeout, 500);
                }
            } else {
                chgbox_led_set_mode(CHGBOX_LED_GREEN_ON);
                chgbox_ui_timeout_add(CHGBOX_LED_GREEN_OFF, chgbox_ui_update_timeout, 500);
            }
        }
        break;
    case CHGBOX_UI_KEY_LONG:
        chgbox_ui_timeout_add(0, NULL, 0);
        chgbox_led_set_mode(CHGBOX_LED_BLUE_ON);
        break;
    case CHGBOX_UI_PAIR_START:
        chgbox_ui_timeout_add(0, NULL, 0);
        chgbox_led_set_mode(CHGBOX_LED_BLUE_FAST_FLASH);
        break;
    case CHGBOX_UI_PAIR_SUCC:
        if (sys_info.status[USB_DET] == STATUS_OFFLINE) {
            chgbox_ui_timeout_add(CHGBOX_LED_BLUE_OFF, chgbox_ui_update_timeout, 500);
        } else {
            if (!sys_info.localfull) {
                chgbox_ui_timeout_add(CHGBOX_LED_RED_SLOW_FLASH, chgbox_ui_update_timeout, 500);
            } else {
                chgbox_ui_timeout_add(0, NULL, 0);
                chgbox_led_set_mode(CHGBOX_LED_BLUE_ON);
            }
        }
        break;
    case CHGBOX_UI_PAIR_STOP:
        if (sys_info.status[USB_DET] == STATUS_ONLINE) {
            chgbox_ui_update_local_power();
        } else {
            chgbox_ui_timeout_add(0, NULL, 0);
            chgbox_led_set_mode(CHGBOX_LED_BLUE_OFF);
        }
        break;
    default:
        chgbox_ui_updata_default_status(status);
        break;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓低电量ui更新
   @param    status:UI状态
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_updata_lowpower_status(u8 status)
{
    switch (status) {
    case CHGBOX_UI_LOCAL_FULL:
    case CHGBOX_UI_LOWPOWER:
    case CHGBOX_UI_KEY_CLICK:
    case CHGBOX_UI_OPEN_LID:
    case CHGBOX_UI_CLOSE_LID:
    case CHGBOX_UI_USB_OUT:
    case CHGBOX_UI_USB_IN:
        chgbox_ui_update_local_power();
        break;
    default:
        chgbox_ui_updata_default_status(status);
        break;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓UI更新仓状态
   @param    mode:  充电仓当前的UI模式（与充电仓的三个模式对应）
             status:充电仓当前状态
   @return
   @note     各个模式根据状态控制ui变化
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_update_status(u8 mode, u8 status)
{
    switch (mode) {
    case UI_MODE_CHARGE:
        chgbox_ui_updata_charge_status(status);
        break;
    case UI_MODE_COMM:
        chgbox_ui_updata_comm_status(status);
        break;
    case UI_MODE_LOWPOWER:
        chgbox_ui_updata_lowpower_status(status);
        break;
    }
    chgbox_ui_set_power_on(0);
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓UI初始化
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chgbox_ui_manage_init(void)
{
    chgbox_led_init();
}


/////////////////////////////////////////////////////////////////////////////////////////////
//ui中间层

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓设置呼吸灯模式
   @param    mode: 灯模式
   @return   无
   @note     设置充电仓灯的状态,不同的亮暗或闪烁可以调配
*/
/*------------------------------------------------------------------------------------*/
void chgbox_led_set_mode(u8 mode)
{
    u8 i;
    log_info("CHG_LED_mode:%d\n", mode);
    switch (mode) {
    case CHGBOX_LED_RED_OFF://红灯
        chgbox_set_led_stu(CHG_LED_RED, 0, 0, 1);
        break;
    case CHGBOX_LED_RED_FAST_OFF:
        chgbox_set_led_stu(CHG_LED_RED, 0, 0, 0);
        break;
    case CHGBOX_LED_RED_ON://红灯
        chgbox_set_led_stu(CHG_LED_RED, 1, 0, 1);
        break;
    case CHGBOX_LED_RED_FAST_ON:
        chgbox_set_led_stu(CHG_LED_RED, 1, 0, 0);
        break;
    case CHGBOX_LED_RED_SLOW_FLASH:
        chgbox_set_led_bre(CHG_LED_RED, LED_FLASH_SLOW, 0, 0xffff);
        break;
    case CHGBOX_LED_RED_FLAST_FLASH:
        chgbox_set_led_bre(CHG_LED_RED, LED_FLASH_FAST, 0, 0xffff);
        break;
    case CHGBOX_LED_RED_SLOW_BRE:
        chgbox_set_led_bre(CHG_LED_RED, LED_FLASH_SLOW, 1, 0xffff);
        break;
    case CHGBOX_LED_RED_FAST_BRE:
        chgbox_set_led_bre(CHG_LED_RED, LED_FLASH_FAST, 1, 0xffff);
        break;
    case CHGBOX_LED_GREEN_OFF:
        chgbox_set_led_stu(CHG_LED_GREEN, 0, 1, 1);
        break;
    case CHGBOX_LED_GREEN_FAST_OFF:
        chgbox_set_led_stu(CHG_LED_GREEN, 0, 0, 0);
        break;
    case CHGBOX_LED_GREEN_ON:
        chgbox_set_led_stu(CHG_LED_GREEN, 1, 1, 1);
        break;
    case CHGBOX_LED_GREEN_FAST_ON:
        chgbox_set_led_stu(CHG_LED_GREEN, 1, 0, 0);
        break;
    case CHGBOX_LED_GREEN_SLOW_FLASH:
        chgbox_set_led_bre(CHG_LED_GREEN, LED_FLASH_SLOW, 0, 0xffff);
        break;
    case CHGBOX_LED_GREEN_FAST_FLASH:
        chgbox_set_led_bre(CHG_LED_GREEN, LED_FLASH_FAST, 0, 0xffff);
        break;
    case CHGBOX_LED_GREEN_SLOW_BRE:
        chgbox_set_led_bre(CHG_LED_GREEN, LED_FLASH_SLOW, 1, 0xffff);
        break;
    case CHGBOX_LED_GREEN_FAST_BRE:
        chgbox_set_led_bre(CHG_LED_GREEN, LED_FLASH_FAST, 1, 0xffff);
        break;
    case CHGBOX_LED_BLUE_OFF:
        chgbox_set_led_stu(CHG_LED_BLUE, 0, 0, 1);
        break;
    case CHGBOX_LED_BLUE_FAST_OFF:
        chgbox_set_led_stu(CHG_LED_BLUE, 0, 0, 0);
        break;
    case CHGBOX_LED_BLUE_ON:
        chgbox_set_led_stu(CHG_LED_BLUE, 1, 0, 1);
        break;
    case CHGBOX_LED_BLUE_FAST_ON:
        chgbox_set_led_stu(CHG_LED_BLUE, 1, 0, 0);
        break;
    case CHGBOX_LED_BLUE_SLOW_FLASH:
        chgbox_set_led_bre(CHG_LED_BLUE, LED_FLASH_SLOW, 0, 0xffff);
        break;
    case CHGBOX_LED_BLUE_FAST_FLASH:
        chgbox_set_led_bre(CHG_LED_BLUE, LED_FLASH_FAST, 0, 0xffff);
        break;
    case CHGBOX_LED_BLUE_SLOW_BRE:
        chgbox_set_led_bre(CHG_LED_BLUE, LED_FLASH_SLOW, 1, 0xffff);
        break;
    case CHGBOX_LED_BLUE_FAST_BRE:
        chgbox_set_led_bre(CHG_LED_BLUE, LED_FLASH_FAST, 1, 0xffff);
        break;
    case CHGBOX_LED_ALL_OFF:
        chgbox_set_led_all_off(1);
        break;
    case CHGBOX_LED_ALL_FAST_OFF:
        chgbox_set_led_all_off(0);
        break;
    case CHGBOX_LED_ALL_ON:
        chgbox_set_led_all_on(1);
        break;
    case CHGBOX_LED_ALL_FAST_ON:
        chgbox_set_led_all_on(0);
        break;
    }
}

#else

void chgbox_ui_set_power_on(u8 flag)
{
}

u8 chgbox_get_ui_power_on(void)
{
    return 0;
}

void chgbox_ui_update_status(u8 mode, u8 status)
{
}

void chgbox_ui_manage_init(void)
{
}

#endif

#endif
