#include "chargeIc_manage.h"
#include "device/device.h"
#include "app_config.h"
#include "app_main.h"
#include "user_cfg.h"
#include "chgbox_det.h"
#include "chgbox_ctrl.h"
#include "chgbox_wireless.h"

#if (TCFG_CHARGE_BOX_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHG_IC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

//充电涓流转恒流的电压点--使用内置充电时有效
#define CHARGE_CURRENT_CHANGE   (3000)
#define CHARGE_FULL_MA          (CHARGE_FULL_mA_30)
#define CHARGE_SLOW_MA          (CHARGE_mA_40)
#define CHARGE_WL_FAST_MA       (CHARGE_mA_200)//无线充电时,使用200mA充电电流
#define CHARGE_FAST_MA          (CHARGE_mA_320)//线充是使用320mA充电电流

#if (TCFG_CHARGE_MOUDLE_OUTSIDE == DISABLE)
static volatile u8 inside_ma;
static int charge_timer;
#endif
#if TCFG_SHORT_PROTECT_ENABLE
extern void short_det_wakeup_disable();
extern void short_det_wakeup_enable();
#endif
static volatile u8 charge_en;

/*------------------------------------------------------------------------------------*/
/**@brief    根据电池电压设置电流档位
   @param    无
   @return   无
   @note     开始充电时调用该接口切到对应档位
*/
/*------------------------------------------------------------------------------------*/
#if (TCFG_CHARGE_MOUDLE_OUTSIDE == DISABLE)
void chargebox_charge_change_current(void *priv)
{
    u8 inside_target = CHARGE_FAST_MA;
    u16 vbat_volt = get_vbat_voltage();
    charge_timer = 0;
    if (vbat_volt > CHARGE_CURRENT_CHANGE) {
#if TCFG_WIRELESS_ENABLE
        if (sys_info.status[WIRELESS_DET] == STATUS_ONLINE) {
            inside_target = CHARGE_WL_FAST_MA;
        }
#endif
        CHARGE_mA_SEL(inside_target);
        inside_ma = inside_target;
    } else {
        //涓流充电时,10s查看一次电量
        CHARGE_mA_SEL(CHARGE_SLOW_MA);
        inside_ma = CHARGE_SLOW_MA;
        charge_timer = sys_timeout_add(NULL, chargebox_charge_change_current, 10000);
    }
}
#endif

/*------------------------------------------------------------------------------------*/
/**@brief    开始对电池仓充电
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chargebox_charge_start(void)
{
    log_info("charge start!\n");
#if TCFG_CHARGE_MOUDLE_OUTSIDE
    gpio_direction_input(TCFG_STOP_CHARGE_IO);
    usb_charge_full_wakeup_deal();//外挂充电,开充电时主动去查询一次是否充满
#else
    //先切根据电压切换电流档位,再开充电
    chargebox_charge_change_current(NULL);
    charge_start();
#endif
    charge_en = 1;
}

/*------------------------------------------------------------------------------------*/
/**@brief    停止对电池仓充电
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chargebox_charge_close(void)
{
    log_info("charge close!\n");
#if TCFG_CHARGE_MOUDLE_OUTSIDE
    gpio_direction_output(TCFG_STOP_CHARGE_IO, 1);
#else
    if (charge_timer) {
        sys_timer_del(charge_timer);
        charge_timer = 0;
    }
    charge_close();
    CHARGE_mA_SEL(CHARGE_SLOW_MA);
    inside_ma = CHARGE_SLOW_MA;
#endif
    charge_en = 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    获取系统是否在充电
   @param    无
   @return   1:正在充电, 0:充电关闭
   @note
*/
/*------------------------------------------------------------------------------------*/
u8 chargebox_get_charge_en(void)
{
    return charge_en;
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电升压开关
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chargeIc_boost_ctrl(u8 en)
{
#if TCFG_SHORT_PROTECT_ENABLE
    if (en == 0) {
        short_det_wakeup_disable();
    }
#endif
    gpio_direction_output(TCFG_BOOST_CTRL_IO, en);
    log_debug("boost_ctrl:%s\n", en ? "open" : "close");
#if TCFG_LDO_DET_ENABLE
    ldo_wakeup_deal(NULL);
#else
    sys_info.status[LDO_DET] = en ? STATUS_ONLINE : STATUS_OFFLINE;
#endif
#if TCFG_CURRENT_LIMIT_ENABLE
    //开升压时才去检测耳机充电电流
    ear_current_detect_enable(en);
#endif
#if TCFG_SHORT_PROTECT_ENABLE
    if (en) {
        short_det_wakeup_enable();
    }
#endif
}

/*------------------------------------------------------------------------------------*/
/**@brief    耳机充电电源开关
   @param    无
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void chargeIc_pwr_ctrl(u8 en)
{
#if (TCFG_PWR_CTRL_TYPE == PWR_CTRL_TYPE_PU_PD)
    if (en == 0) {
        gpio_set_pull_up(TCFG_PWR_CTRL_IO, 0);
        gpio_set_pull_down(TCFG_PWR_CTRL_IO, 1);
    } else {
        gpio_set_pull_down(TCFG_PWR_CTRL_IO, 0);
        gpio_set_pull_up(TCFG_PWR_CTRL_IO, 1);
    }
#elif (TCFG_PWR_CTRL_TYPE == PWR_CTRL_TYPE_OUTPUT_0)
    if (en == 0) {
        gpio_direction_input(TCFG_PWR_CTRL_IO);
    } else {
        gpio_direction_output(TCFG_PWR_CTRL_IO, 0);
    }
#elif (TCFG_PWR_CTRL_TYPE == PWR_CTRL_TYPE_OUTPUT_1)
    if (en == 0) {
        gpio_direction_input(TCFG_PWR_CTRL_IO);
    } else {
        gpio_direction_output(TCFG_PWR_CTRL_IO, 1);
    }
#endif
    log_debug("vol_ctrl:%s\n", en ? "open" : "close");
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电IC初始化
   @param    null
   @return   null
   @note     初始化充电IC相关的引脚
*/
/*------------------------------------------------------------------------------------*/
void chargeIc_init(void)
{
    //使能IO输出低电平
    gpio_set_die(TCFG_BOOST_CTRL_IO, 0);
    gpio_set_pull_down(TCFG_BOOST_CTRL_IO, 0);
    gpio_set_pull_up(TCFG_BOOST_CTRL_IO, 0);
    gpio_direction_output(TCFG_BOOST_CTRL_IO, 0);

    //PWR_CTRL初始化
    gpio_set_die(TCFG_PWR_CTRL_IO, 0);
    gpio_set_pull_up(TCFG_PWR_CTRL_IO, 0);
    gpio_direction_input(TCFG_PWR_CTRL_IO);
#if (TCFG_PWR_CTRL_TYPE == PWR_CTRL_TYPE_PU_PD)
    gpio_set_pull_down(TCFG_PWR_CTRL_IO, 1);
#else
    gpio_set_pull_down(TCFG_PWR_CTRL_IO, 0);
#endif

#if TCFG_CHARGE_MOUDLE_OUTSIDE
    //充电控制脚,高阻时正常充电,输出1时充电截止
    charge_en = 1;
    gpio_set_die(TCFG_STOP_CHARGE_IO, 1);
    gpio_set_pull_down(TCFG_STOP_CHARGE_IO, 0);
    gpio_set_pull_up(TCFG_STOP_CHARGE_IO, 0);
    if (charge_en == 1) {
        gpio_direction_input(TCFG_STOP_CHARGE_IO);
    } else {
        gpio_direction_output(TCFG_STOP_CHARGE_IO, 1);
    }
#else
    //内置充电初始化
    CHGBG_EN(0);
    CHARGE_EN(0);
    CHARGE_WKUP_PND_CLR();

    LVCMP_EN(1);
    LVCMP_EDGE_SEL(0);
    LVCMP_PND_CLR();
    LVCMP_EDGE_WKUP_EN(1);
    LVCMP_CMP_SEL(1);
    u16 charge_4202_trim_val = get_vbat_trim();
    if (charge_4202_trim_val == 0xf) {
        log_info("vbat not trim, use default config!!!!!!");
    }
    CHARGE_FULL_V_SEL(charge_4202_trim_val);
    CHARGE_FULL_mA_SEL(CHARGE_FULL_MA);
    CHARGE_mA_SEL(CHARGE_SLOW_MA);
    inside_ma = CHARGE_SLOW_MA;
    charge_en = 0;
#endif
}

#endif
