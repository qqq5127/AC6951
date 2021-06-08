#ifndef _CHGBOX_UI_H_
#define _CHGBOX_UI_H_

#include "typedef.h"
//关于仓ui的说明，分为三个部分
//1.ui状态层
//2.ui中间层
//3.ui驱动层
//状态层主要就是外部把仓的状态传进来，中间层是一个过渡，如不想用本驱动，可以自己更换中间层
//或者只使用本驱动层作其他使用
/////////////////////////////////////////////////////////////////////////////////////////////
//ui状态层
typedef enum {
    CHGBOX_UI_NULL = 0,

    CHGBOX_UI_ALL_OFF,
    CHGBOX_UI_ALL_ON,

    CHGBOX_UI_POWER,
    CHGBOX_UI_EAR_FULL,
    CHGBOX_UI_LOCAL_FULL,
    CHGBOX_UI_LOWPOWER,

    CHGBOX_UI_EAR_L_IN,
    CHGBOX_UI_EAR_R_IN,
    CHGBOX_UI_EAR_L_OUT,
    CHGBOX_UI_EAR_R_OUT,

    CHGBOX_UI_KEY_CLICK,
    CHGBOX_UI_KEY_LONG,
    CHGBOX_UI_PAIR_START,
    CHGBOX_UI_PAIR_SUCC,
    CHGBOX_UI_PAIR_STOP,

    CHGBOX_UI_OPEN_LID,
    CHGBOX_UI_CLOSE_LID,

    CHGBOX_UI_USB_IN,
    CHGBOX_UI_USB_OUT,

    CHGBOX_UI_OVER_CURRENT,
} UI_STATUS;

enum {
    UI_MODE_CHARGE,
    UI_MODE_COMM,
    UI_MODE_LOWPOWER,
};

void  chgbox_ui_manage_init(void);
void chgbox_ui_update_status(u8 mode, u8 status);
void chgbox_ui_set_power_on(u8 flag);
u8 chgbox_get_ui_power_on(void);



/////////////////////////////////////////////////////////////////////////////////////////////
//ui中间层

//点灯模式
enum {
    CHGBOX_LED_RED_OFF,//呼吸灭
    CHGBOX_LED_RED_FAST_OFF,//直接灭
    CHGBOX_LED_RED_ON,//呼吸亮
    CHGBOX_LED_RED_FAST_ON,//直接亮
    CHGBOX_LED_RED_SLOW_FLASH,//慢闪
    CHGBOX_LED_RED_FLAST_FLASH,//快闪
    CHGBOX_LED_RED_SLOW_BRE,//呼吸慢闪
    CHGBOX_LED_RED_FAST_BRE,//呼吸快闪

    CHGBOX_LED_GREEN_OFF,
    CHGBOX_LED_GREEN_FAST_OFF,
    CHGBOX_LED_GREEN_ON,
    CHGBOX_LED_GREEN_FAST_ON,
    CHGBOX_LED_GREEN_SLOW_FLASH,
    CHGBOX_LED_GREEN_FAST_FLASH,
    CHGBOX_LED_GREEN_SLOW_BRE,
    CHGBOX_LED_GREEN_FAST_BRE,

    CHGBOX_LED_BLUE_ON,
    CHGBOX_LED_BLUE_FAST_ON,
    CHGBOX_LED_BLUE_OFF,
    CHGBOX_LED_BLUE_FAST_OFF,
    CHGBOX_LED_BLUE_SLOW_FLASH,
    CHGBOX_LED_BLUE_FAST_FLASH,
    CHGBOX_LED_BLUE_SLOW_BRE,
    CHGBOX_LED_BLUE_FAST_BRE,

    CHGBOX_LED_ALL_OFF,
    CHGBOX_LED_ALL_FAST_OFF,
    CHGBOX_LED_ALL_ON,
    CHGBOX_LED_ALL_FAST_ON,
};
void chgbox_led_set_mode(u8 mode);


/////////////////////////////////////////////////////////////////////////////////////////////
//ui驱动层
//定义n个灯
enum {
    CHG_LED_RED,
    CHG_LED_GREEN,
    CHG_LED_BLUE,
    CHG_LED_MAX,
};

//闪烁快慢
enum {
    LED_FLASH_FAST,
    LED_FLASH_SLOW,
};

//led驱动初始化
void chgbox_led_init(void);
void chgbox_set_led_stu(u8 led_type, u8 on_off, u8 sp_flicker, u8 fade);
void chgbox_set_led_bre(u8 led_type, u8 speed_mode, u8 is_bre, u16 time);
void chgbox_set_led_all_off(u8 fade);
void chgbox_set_led_all_on(u8 fade);

#endif    //_APP_CHARGEBOX_H_

