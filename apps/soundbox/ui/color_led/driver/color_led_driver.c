#include "system/includes.h"
#include "app_config.h"
#include "user_cfg.h"
#include "color_led_driver.h"

#define LOG_TAG_CONST       COLOR_LED
#define LOG_TAG             "[COLOR_LED]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_COLORLED_ENABLE
int timer_pwm_init(JL_TIMER_TypeDef *JL_TIMERx, u32 fre, u32 duty, u32 port, int output_ch);
void set_timer_pwm_duty(JL_TIMER_TypeDef *JL_TIMERx, u32 fre, u32 duty);
#define PWM_FRE (0XFF*0XFF)

#define BYTE(data,n) ((data>>(n*8)) & 0x000000ff)
static const colorled_platform_data *color_led_cfg_data = NULL;
#define __this (color_led_cfg_data)
#define PWM_DETY_SET(timer_hdl,duty)	set_timer_pwm_duty(timer_hdl,PWM_FRE,duty*0xff)

void __color_led_normal_mode_io_set(const colorled_platform_data *cfg_data)
{
    u8 pin[3] = {cfg_data->pin_red.pin, cfg_data->pin_gree.pin, cfg_data->pin_blue.pin};

    for (int i = 0; i < sizeof(pin); i++) {
        gpio_set_die(pin[i], 1);
        gpio_set_pull_up(pin[i], 0);
        gpio_set_pull_down(pin[i], 0);
        gpio_set_direction(pin[i], 0);
    }
}

/*
 * BYTE3(reserved)  BYTE2(BLUE)  BYTE1(GREE)  BYTE0(RED)
 * */
void __color_normal_mode_set(u32 color)
{
    //log_info("color:%8x %d %d %d  \n", color,(BYTE(color,0)!=0),(BYTE(color,1)!=0),(BYTE(color,2)!=0));

    {
        //RED
        gpio_direction_output(__this->pin_red.pin, (BYTE(color, 0) != 0));
    }

    {
        //GREE
        gpio_direction_output(__this->pin_gree.pin, (BYTE(color, 1) != 0));
    }

    {
        ///BLUE
        gpio_direction_output(__this->pin_blue.pin, (BYTE(color, 2) != 0));
    }
}

void __color_led_pwm_mode_io_set(const colorled_platform_data *cfg_data)
{

    if (timer_pwm_init(__this->pin_red.timer_hdl, PWM_FRE, 0, __this->pin_red.pin, __this->pin_red.output_ch) == (-1)) {
        ASSERT(0, "!!!warning : pwm not support io,please select others or used output channel mode \n");
    }
    if (timer_pwm_init(__this->pin_gree.timer_hdl, PWM_FRE, 0, __this->pin_gree.pin, __this->pin_gree.output_ch) == (-1)) {
        ASSERT(0, "!!!warning : pwm not support io,please select others or used output channel mode \n");
    }
    if (timer_pwm_init(__this->pin_blue.timer_hdl, PWM_FRE, 0, __this->pin_blue.pin, __this->pin_blue.output_ch) == (-1)) {
        ASSERT(0, "!!!warning : pwm not support io,please select others or used output channel mode \n");
    }
}

void __color_pwm_mode_set(u32 color)
{
//    log_info("color:%8x \n", color);
    PWM_DETY_SET(__this->pin_red.timer_hdl, (BYTE(color, 0)));
    PWM_DETY_SET(__this->pin_gree.timer_hdl, (BYTE(color, 1)));
    PWM_DETY_SET(__this->pin_blue.timer_hdl, (BYTE(color, 2)));
}
#endif

void __color_led_io_set(const colorled_platform_data *cfg_data)
{
#if TCFG_COLORLED_ENABLE
    color_led_cfg_data = cfg_data;

    switch (__this->work_mode) {
    case COLOR_LED_MODE_NORNAL:
        __color_led_normal_mode_io_set(__this);
        break;
    case COLOR_LED_MODE_PWM:
        __color_led_pwm_mode_io_set(__this);
        break;
    default:
        break;
    }
#endif
}
void __color_set(u32 color)
{
#if TCFG_COLORLED_ENABLE
    switch (__this->work_mode) {
    case COLOR_LED_MODE_NORNAL:
        __color_normal_mode_set(color);
        break;
    case COLOR_LED_MODE_PWM:
        __color_pwm_mode_set(color);
        break;
    default:
        break;
    }
#endif
}
