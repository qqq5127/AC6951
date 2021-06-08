#include "typedef.h"
#include "asm/pwm_led.h"
#include "system/includes.h"
#include "chgbox_ctrl.h"
#include "chargeIc_manage.h"
#include "chgbox_ui.h"
#include "app_config.h"

//使用PEMLED推灯,支持低功耗

#if (TCFG_CHARGE_BOX_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[CHGBOXUI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if (TCFG_CHARGE_BOX_UI_ENABLE && TCFG_CHGBOX_UI_PWMLED)

#define CFG_LED_LIGHT						200  	//10 ~ 500, 值越大, 亮度越高

#define CFG_SINGLE_FAST_FLASH_FREQ			500		//LED单独快闪速度, ms闪烁一次(100 ~ 1000)
#define CFG_SINGLE_FAST_LIGHT_TIME 			150  	//单灯快闪灯亮持续时间, 单位ms

#define CFG_SINGLE_SLOW_FLASH_FREQ			1000	//LED单独慢闪速度, ms闪烁一次(1000 ~ 20000)
#define CFG_SINGLE_SLOW_LIGHT_TIME 			300  	//单灯慢闪灯亮持续时间, 单位ms
/***************** LED0/LED1单独每隔5S单闪时, 可供调节参数 ********************/
#define CFG_LED_5S_FLASH_LIGHT_TIME			100		//LED 5S 闪烁时灯亮持续时间, 单位ms
/***************** 呼吸模式配置参数, 可供调节参数 ********************/
#define CFG_LED_SLOW_BREATH_TIME			1000	//呼吸时间灭->亮->灭, 单位ms
#define CFG_LED_FAST_BREATH_TIME			500		//呼吸时间灭->亮->灭, 单位ms
#define CFG_LED_BREATH_BRIGHT 				300		//呼吸亮度, 范围: 0 ~ 500
#define CFG_LED_BREATH_BLINK_TIME 			100		//灭灯延时, 单位ms

const pwm_led_on_para pwm_led_on_para_table[] = {
    /*
    u16 led0_bright;//led0_bright, LED0亮度: 0 ~ 500
    u16 led1_bright;//led1_bright, LED1亮度: 0 ~ 500
    */
    {CFG_LED_LIGHT, CFG_LED_LIGHT},//PWM_LED_ON	亮
};

const pwm_led_one_flash_para pwm_led_one_flash_para_table[] = {
    /*
    u16 led0_bright;//led0_bright: led0亮度(0 ~ 500),
    u16 led1_bright;//led1_bright: led1亮度(0 ~ 500),
    u32 period;//period: 闪灯周期(ms), 多少ms闪一下(100 ~ 20000), 100ms - 20S,
    u32 start_light_time;//start_light_time: 在周期中开始亮灯的时间, -1: 周期最后亮灯, 默认填-1即可,
    u32 light_time;//light_time: 灯亮持续时间,
    */
    {CFG_LED_LIGHT, CFG_LED_LIGHT, CFG_SINGLE_SLOW_FLASH_FREQ, -1, CFG_SINGLE_SLOW_LIGHT_TIME},//PWM_LED_SLOW_FLASH   慢闪
    {CFG_LED_LIGHT, CFG_LED_LIGHT, CFG_SINGLE_FAST_FLASH_FREQ, -1, CFG_SINGLE_FAST_LIGHT_TIME},//PWM_LED_FAST_FLASH   快闪
    {CFG_LED_LIGHT, CFG_LED_LIGHT, 5000, 10, CFG_LED_5S_FLASH_LIGHT_TIME},//PWM_LED_ONE_FLASH_5S  5秒连闪1下
};

const pwm_led_double_flash_para pwm_led_double_flash_para_table[] = {
    /*
    u16 led0_bright;//led0_bright: led0亮度,
    u16 led1_bright;//led1_bright: led1亮度,
    u32 period;//period: 闪灯周期(ms), 多少ms闪一下
    u32 first_light_time;//first_light_time: 第一次亮灯持续时间,
    u32 gap_time;//gap_time: 两次亮灯时间间隔,
    u32 second_light_time;//second_light_time: 第二次亮灯持续时间,
    */
    {CFG_LED_LIGHT, CFG_LED_LIGHT, 5000, 100, 200, 100}, //PWM_LED_DOUBLE_FLASH_5S	5秒连闪两下
};

const pwm_led_breathe_para pwm_led_breathe_para_table[] = {
    /*
    u16 breathe_time;//breathe_time: 呼吸周期(灭->最亮->灭), 设置范围: 500ms以上;
    u16 led0_bright;//led0_bright: led0呼吸到最亮的亮度(0 ~ 500);
    u16 led1_bright;//led1_bright: led1呼吸到最亮的亮度(0 ~ 500);
    u32 led0_light_delay_time;//led0_light_delay_time: led0最高亮度延时(0 ~ 100ms);
    u32 led1_light_delay_time;//led1_light_delay_time: led1最高亮度延时(0 ~ 100ms);
    u32 led_blink_delay_time;//led_blink_delay_time: led0和led1灭灯延时(0 ~ 20000ms), 0 ~ 20S;
    */
    {CFG_LED_SLOW_BREATH_TIME, CFG_LED_BREATH_BRIGHT, CFG_LED_BREATH_BRIGHT, 0, 0, CFG_LED_BREATH_BLINK_TIME},//PWM_LED_BREATHE  慢呼吸灯模式
    {CFG_LED_FAST_BREATH_TIME, CFG_LED_BREATH_BRIGHT, CFG_LED_BREATH_BRIGHT, 0, 0, CFG_LED_BREATH_BLINK_TIME},//PWM_LED_BREATHE  快呼吸灯模式
};

static void chgbox_led_set_enable(u8 gpio)
{
    gpio_set_pull_up(gpio, 1);
    gpio_set_pull_down(gpio, 1);
    gpio_set_die(gpio, 1);
    gpio_set_output_value(gpio, 1);
    gpio_set_direction(gpio, 0);
}

static void chgbox_led_set_disable(u8 gpio)
{
    gpio_set_pull_down(gpio, 0);
    gpio_set_pull_up(gpio, 0);
    gpio_direction_input(gpio);
}

static void chgbox_ui_mode_set(u8 display, u8 sub)
{
    pwm_led_para para = {0};
    if (display == PWM_LED_NULL) {
        return;
    }
    switch (display) {
//灯常灭
    case PWM_LED0_OFF:
        break;
//灯常亮
    case PWM_LED0_ON:
        para.on = pwm_led_on_para_table[0];
        break;

//单灯单闪
    case PWM_LED0_SLOW_FLASH:
        para.one_flash = pwm_led_one_flash_para_table[0];
        break;
    case PWM_LED0_FAST_FLASH:
        para.one_flash = pwm_led_one_flash_para_table[1];
        break;
    case PWM_LED0_ONE_FLASH_5S:
        para.one_flash = pwm_led_one_flash_para_table[2];
        break;

//单灯双闪
    case PWM_LED0_DOUBLE_FLASH_5S:
        para.double_flash = pwm_led_double_flash_para_table[0];
        break;

//呼吸模式
    case PWM_LED0_BREATHE:
        para.breathe = pwm_led_breathe_para_table[sub];
        break;
    }
    pwm_led_mode_set_with_para(display, para);
}

/*------------------------------------------------------------------------------------*/
/**@brief    设置灯为常亮、常暗模式
   @param    led_type:  灯序号
             on_off:    1-->常亮 0-->常暗
             sp_flicker:是否闪烁标志 1-->在亮--亮 暗--暗过程中取反一下
             fade:      1-->快速响应 0-->渐变响应
   @return   无
   @note     把灯设置为 常亮/常暗 模式(默认同时设置其他灯为常暗)
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_stu(u8 led_type, u8 on_off, u8 sp_flicker, u8 fade)
{
    chgbox_led_set_disable(CHG_RED_LED_IO);
    chgbox_led_set_disable(CHG_GREEN_LED_IO);
    chgbox_led_set_disable(CHG_BLUE_LED_IO);
    if (on_off) {
        chgbox_ui_mode_set(PWM_LED0_ON, 0);
    } else {
        chgbox_ui_mode_set(PWM_LED0_OFF, 0);
    }
    switch (led_type) {
    case CHG_LED_RED:
        chgbox_led_set_enable(CHG_RED_LED_IO);
        break;
    case CHG_LED_GREEN:
        chgbox_led_set_enable(CHG_GREEN_LED_IO);
        break;
    case CHG_LED_BLUE:
        chgbox_led_set_enable(CHG_BLUE_LED_IO);
        break;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    设置灯为呼吸模式
   @param    led_type:  灯序号
             speed_mode:闪烁参数选择，这里默认为快、慢两种
             is_bre:是否为呼吸模式
             time:呼吸/闪烁多少次
   @return   无
   @note     把灯设置为呼吸模式(默认同时其他灯为常暗)
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_bre(u8 led_type, u8 speed_mode, u8 is_bre, u16 time)
{
    chgbox_led_set_disable(CHG_RED_LED_IO);
    chgbox_led_set_disable(CHG_GREEN_LED_IO);
    chgbox_led_set_disable(CHG_BLUE_LED_IO);
    if (is_bre) {
        if (speed_mode == LED_FLASH_FAST) {
            chgbox_ui_mode_set(PWM_LED0_BREATHE, 1);
        } else {
            chgbox_ui_mode_set(PWM_LED0_BREATHE, 0);
        }
    } else {
        if (speed_mode == LED_FLASH_FAST) {
            chgbox_ui_mode_set(PWM_LED0_FAST_FLASH, 0);
        } else {
            chgbox_ui_mode_set(PWM_LED0_SLOW_FLASH, 0);
        }
    }
    switch (led_type) {
    case CHG_LED_RED:
        chgbox_led_set_enable(CHG_RED_LED_IO);
        break;
    case CHG_LED_GREEN:
        chgbox_led_set_enable(CHG_GREEN_LED_IO);
        break;
    case CHG_LED_BLUE:
        chgbox_led_set_enable(CHG_BLUE_LED_IO);
        break;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    呼吸灯全暗
   @param    fade:是否淡入
   @return   无
   @note     把所有的灯设置为常暗模式
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_all_off(u8 fade)
{
    chgbox_led_set_disable(CHG_RED_LED_IO);
    chgbox_led_set_disable(CHG_GREEN_LED_IO);
    chgbox_led_set_disable(CHG_BLUE_LED_IO);
    chgbox_ui_mode_set(PWM_LED0_OFF, 0);
}

/*------------------------------------------------------------------------------------*/
/**@brief    呼吸灯全亮
   @param    fade:是否淡入
   @return   无
   @note     把所有的灯设置为常亮模式
*/
/*------------------------------------------------------------------------------------*/
void chgbox_set_led_all_on(u8 fade)
{
    chgbox_led_set_disable(CHG_RED_LED_IO);
    chgbox_led_set_disable(CHG_GREEN_LED_IO);
    chgbox_led_set_disable(CHG_BLUE_LED_IO);
    chgbox_ui_mode_set(PWM_LED0_ON, 0);
    chgbox_led_set_enable(CHG_RED_LED_IO);
    chgbox_led_set_enable(CHG_GREEN_LED_IO);
    chgbox_led_set_enable(CHG_BLUE_LED_IO);
}

/*------------------------------------------------------------------------------------*/
/**@brief    led呼吸灯初始化
   @param    无
   @return   无
   @note     初始化每个led:渐亮、亮、渐暗、暗，最大亮度，对应IO的初始化.mc_clk的初始化，用于
             控制pwm
*/
/*-----------------------------------------------------------------------------------*/
extern const struct led_platform_data chgbox_pwm_led_data;
void chgbox_led_init(void)
{
    chgbox_led_set_disable(CHG_RED_LED_IO);
    chgbox_led_set_disable(CHG_GREEN_LED_IO);
    chgbox_led_set_disable(CHG_BLUE_LED_IO);
    pwm_led_init(&chgbox_pwm_led_data);
}

#endif

#endif




