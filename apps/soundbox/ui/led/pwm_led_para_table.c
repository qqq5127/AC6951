#include "ui_manage.h"
#include "asm/pwm_led.h"
#include "system/includes.h"
#include "app_config.h"
#include "user_cfg.h"

#if TCFG_PWMLED_ENABLE

#define CFG_LED0_LIGHT						100  	//10 ~ 500, 值越大, (红灯)亮度越高
#define CFG_LED1_LIGHT						100  	//10 ~ 500, 值越大, (蓝灯)亮度越高

#define CFG_SINGLE_FAST_FLASH_FREQ			500		//LED单独快闪速度, ms闪烁一次(100 ~ 1000)
#define CFG_SINGLE_FAST_LIGHT_TIME 			100  	//单灯快闪灯亮持续时间, 单位ms

#define CFG_SINGLE_SLOW_FLASH_FREQ			2000	//LED单独慢闪速度, ms闪烁一次(1000 ~ 20000)
#define CFG_SINGLE_SLOW_LIGHT_TIME 			100  	//单灯慢闪灯亮持续时间, 单位ms

#define CFG_DOUBLE_FAST_FLASH_FREQ			500		//LED交替快闪速度, ms闪烁一次(100 ~ 1000)
#define CFG_DOUBLE_SLOW_FLASH_FREQ			2000	//LED交替慢闪速度, ms闪烁一次(1000 ~ 20000)

/***************** LED0/LED1单独每隔5S单闪时, 可供调节参数 ********************/
#define CFG_LED_5S_FLASH_LIGHT_TIME			100		//LED 5S 闪烁时灯亮持续时间, 单位ms

/***************** 呼吸模式配置参数, 可供调节参数 ********************/
#define CFG_LED_BREATH_TIME 				1000		//呼吸时间灭->亮->灭, 单位ms
#define CFG_LED0_BREATH_BRIGHT 				300			//呼吸亮度, 范围: 0 ~ 500
#define CFG_LED1_BREATH_BRIGHT 				300			//呼吸亮度, 范围: 0 ~ 500
#define CFG_LED_BREATH_BLINK_TIME 			1000		//灭灯延时, 单位ms

const pwm_led_on_para pwm_led_on_para_table[] = {
    /*
    u16 led0_bright;//led0_bright, LED0亮度: 0 ~ 500
    u16 led1_bright;//led1_bright, LED1亮度: 0 ~ 500
    */
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT},//PWM_LED0_ON	蓝亮
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT},//PWM_LED1_ON   红亮
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT},//PWM_LED_ALL_ON	全亮
    {300, 100},//demo para
};

const pwm_led_one_flash_para pwm_led_one_flash_para_table[] = {
    /*
    u16 led0_bright;//led0_bright: led0亮度(0 ~ 500),
    u16 led1_bright;//led1_bright: led1亮度(0 ~ 500),
    u32 period;//period: 闪灯周期(ms), 多少ms闪一下(100 ~ 20000), 100ms - 20S,
    u32 start_light_time;//start_light_time: 在周期中开始亮灯的时间, -1: 周期最后亮灯, 默认填-1即可,
    u32 light_time;//light_time: 灯亮持续时间,
    */
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_SINGLE_SLOW_FLASH_FREQ, -1, CFG_SINGLE_SLOW_LIGHT_TIME},//PWM_LED0_SLOW_FLASH	蓝慢闪
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_SINGLE_SLOW_FLASH_FREQ, -1, CFG_SINGLE_SLOW_LIGHT_TIME},//PWM_LED1_SLOW_FLASH   红慢闪
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_SINGLE_FAST_FLASH_FREQ, -1, CFG_SINGLE_FAST_LIGHT_TIME},//PWM_LED0_FAST_FLASH   蓝快闪
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_SINGLE_FAST_FLASH_FREQ, -1, CFG_SINGLE_FAST_LIGHT_TIME},//PWM_LED1_FAST_FLASH   红快闪
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT, 5000, 10, CFG_LED_5S_FLASH_LIGHT_TIME},//PWM_LED0_ONE_FLASH_5S  蓝灯5秒连闪1下
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT, 5000, 10, CFG_LED_5S_FLASH_LIGHT_TIME},//PWM_LED1_ONE_FLASH_5S  红灯5秒闪1下
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_DOUBLE_FAST_FLASH_FREQ, -1, -1},//PWM_LED0_LED1_FAST_FLASH  红蓝交替闪（快闪）
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_DOUBLE_SLOW_FLASH_FREQ, -1, -1},//PWM_LED0_LED1_SLOW_FLASH  红蓝交替闪（慢闪）
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
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT, 5000, 100, 200, 100}, //PWM_LED0_DOUBLE_FLASH_5S	蓝灯5秒连闪两下
    {CFG_LED0_LIGHT, CFG_LED1_LIGHT, 5000, 100, 200, 100}, //PWM_LED1_DOUBLE_FLASH_5S   红灯5秒连闪两下
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
    {CFG_LED_BREATH_TIME, CFG_LED0_BREATH_BRIGHT, CFG_LED1_BREATH_BRIGHT, 0, 0, CFG_LED_BREATH_BLINK_TIME},//PWM_LED0_BREATHE  蓝灯呼吸灯模式
    {CFG_LED_BREATH_TIME, CFG_LED0_BREATH_BRIGHT, CFG_LED1_BREATH_BRIGHT, 0, 0, CFG_LED_BREATH_BLINK_TIME},//PWM_LED1_BREATHE  红灯呼吸灯模式
    {CFG_LED_BREATH_TIME, CFG_LED0_BREATH_BRIGHT, CFG_LED1_BREATH_BRIGHT, 0, 0, CFG_LED_BREATH_BLINK_TIME},//PWM_LED0_LED1_BREATHE	红蓝交替呼吸灯模式
};

#endif
