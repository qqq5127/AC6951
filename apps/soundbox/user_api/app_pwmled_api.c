#include "generic/gpio.h"
#include "asm/includes.h"
#include "asm/pwm_led.h"
#include "system/timer.h"
#include "board_config.h"

#ifdef CONFIG_BOARD_AC696X_DEMO
//注意事项：
//1.使用该模块需确认 TCFG_PWMLED_ENABLE 和 UI中的pwm_led 都处于DISABLE状态
//2.进入蓝牙模式时需要在蓝牙初始化完成后(BT_STATUS_INIT_OK)再设置灯效,蓝牙初始化过程会影响pwm_led模块
//3.一个pwm周期内最多只能进行4次IO口反转，能够实现双闪动作，如果需要进行更多的闪动则需要配合PWM的中断来实现
//
//
//=============周期灯========================================================================================================================================
//
//                            __第一次反转后状态保持时间__                            __第三次反转后状态保持时间__
//           holdtime1_ms     |     holdtime2_ms         |      holdtime3_ms          |      holdtime4_ms        |     cycle_ms减去所有hondtime
//高亮灯：__初始状态保持时间__|                          |__第二次反转后状态保持时间__|                          |___最后电平持续时间，持续到周期结束_______
//低亮灯：_____________________                          ______________________________                          ___________________________________________
//                            |                          |                            |                          |
//                            |__________________________|                            |__________________________|
//        |<---------------------------------------------整个PWM亮灭周期(中断周期)：cycle_ms--------------------------------------------------------------->|
//
//1.PMW周期从灭灯开始，高亮灯初始电平为低，低亮灯初始电平为高
//
//===========================================================================================================================================================
//struct pwm_cycle_pattern{
//	u32 cycle_ms;         //PWM产生的一个亮灭周期，单位ms
//	u32 holdtime1_ms;  	  //初始状态保持时间,等于0时接近于PWM周期从亮灯开始
//	u32 holdtime2_ms;     //第一次反转后状态保持时间,等于0时关闭
//	u32 holdtime3_ms;     //第二次反转后状态保持时间,等于0时关闭
//	u32 holdtime4_ms;     //第三次反转后状态保持时间,等于0时关闭
//	u16 led_L_bright;     //低电平灯亮度设置
//	u16 led_H_bright;     //高电平灯亮度设置
//	u8  led_type;         //LED类型 0：低亮 1：高亮
//	u8  shift_duty;       //每隔shift_duty个周期后输出电平取反，亮度设置不变，也就是说当前灯的图案会切换到两外一个灯上，用于周期变色处理,等于0时功能关闭
//};
//=============呼吸灯========================================================================================================================================
//
//                                           _最高亮度保持时间__                                          __
//                                        __|  led_H_holdtime   |__                                    __|
//                breathe_interval     __|                         |__         breathe_interval     __|
//高亮灯：____________呼吸间隔________|         breathe_time          |____________呼吸间隔________|
//                                    |<----------呼吸时间----------->|
//       |<---PWM周期(中断周期) = breathe_interval + breathe_time---->|
//
//低亮灯：
//
//===========================================================================================================================================================
//struct pwm_breathe_pattern{
//	u32 breathe_interval; //呼吸间隔，LED处于灭灯状态,PWM初始化后经过一个呼吸间隔后才开始第一次呼吸动作。
//	u16 breathe_time;     //呼吸时间(灭->最亮->灭), 设置范围: 500ms以上;
//  u32 led_L_holdtime;   //低电平灯最高亮度保持时间,呼吸时间-保持时间=渐变时间	(0 ~ 100ms);
//	u32 led_H_holdtime;   //高电平灯最高亮度保持时间,呼吸时间-保持时间=渐变时间	(0 ~ 100ms);
//	u16 led_L_bright;     //低电平灯呼吸到最亮的亮度设置(0 ~ 500);
//	u16 led_H_bright;     //高电平灯呼吸到最亮的亮度设置(0 ~ 500);
//	u8  led_type;         //LED类型 1：高亮   0：低亮
//	u8  shift_duty;       //每隔shift_duty个周期后输出电平取反，亮度设置不变，也就是说当前灯的呼吸效果会切换到两外一个灯上，用于周期变色处理,等于0时功能关闭
//};


extern void pwm_cycle_pattern_init(struct pwm_cycle_pattern *pattern);
extern void pwm_breathe_pattern_init(struct pwm_breathe_pattern *pattern);
extern void pwmled_pwm_init();
extern void led_pwm_clear_pending();
extern void led_pwm_isr_en(u8 en);
extern void led_pin_set_enable(u8 gpio);
extern void led_pin_set_disable(u8 disable_pin);

extern void pwm_ledL_off_Nms(int off_ms);
extern void pwm_ledL_four_flash_test();

volatile u8 pwm_isr_state;
volatile u32 pwm_isr_cnt;

enum {
    four_flash_cnt = 0,
    four_flash_interval,
    change_PWM_IO,
};
//===========================================================================================================================================================
//pwm周期中断函数
//
//===========================================================================================================================================================
___interrupt
void pwm_cycle_isr_func()
{
    led_pwm_clear_pending();
    switch (pwm_isr_state) {
    case four_flash_cnt:
        pwm_isr_cnt++;
        if (pwm_isr_cnt == 4) {
            pwm_ledL_off_Nms(2000);//闪动4次之后切换到灭灯周期
        }
        break;
    case four_flash_interval:
        pwm_ledL_four_flash_test();//灭灯周期结束后切换到4闪周期
        break;
    case change_PWM_IO:
        pwm_isr_cnt++;
        if (pwm_isr_cnt == 2) { //进行PWM IO口切换动作
            led_pin_set_disable(IO_PORTA_00);
            led_pin_set_enable(IO_PORTA_01);
        } else if (pwm_isr_cnt >= 4) {
            pwm_isr_cnt = 0;
            led_pin_set_disable(IO_PORTA_01);
            led_pin_set_enable(IO_PORTA_00);
        }
        break;
    default:
        break;
    }
}
//===========================================================================================================================================================
//高亮灯，每隔1秒进行一次双闪示例
//
//                            __第一次反转后状态保持时间__                            __第三次反转后状态保持时间__
//           holdtime1_ms     |     holdtime2_ms         |      holdtime3_ms          |      holdtime4_ms        |     cycle_ms减去所有hondtime
//高亮灯：__初始状态保持时间__|                          |__第二次反转后状态保持时间__|                          |___最后电平持续时间，持续到周期结束_______
//        |<---------------------------------------------整个PWM亮灭周期(中断周期)：cycle_ms--------------------------------------------------------------->|
//
//
//===========================================================================================================================================================
void pwm_ledH_double_flash_test()
{
    struct pwm_cycle_pattern pattern = {0};
    pattern.cycle_ms = 1000;      		//每隔1秒进行一次动作，需要设置PWM周期为1000ms
    pattern.holdtime1_ms = 10;     		//设置初始灭灯状态的保持时间,主要作用是产生PWM中断之后能在灭灯状态下进行相关设置，时间到则IO反转进入亮灯状态。
    pattern.holdtime2_ms = 100;    		//设置第一次亮灯保持时间,时间到则IO反转进入灭灯状态。
    pattern.holdtime3_ms = 100;    		//设置第一次灭灯保持时间，即两次亮灯的时间间隔，时间到则IO反转进入亮灯状态。
    pattern.holdtime4_ms = 100;    		//设置第二次亮灯保持时间，时间到则IO反转进入灭灯状态并持续要周期结束。
    pattern.led_L_bright = 0;      		//只操作高亮灯时，低亮灯亮度设置为0。
    pattern.led_H_bright = 500;    		//设置高亮灯的亮度。
    pattern.led_type = 1;          		//设置亮灯类型：0：低亮  1：高亮  2：两灯跳变效果
    pattern.shift_duty = 0;        		//关闭周期取反功能

    pwm_cycle_pattern_init(&pattern);	//根据配置好的pwm周期图案进行初始话并启动PWM_LED模块
    led_pwm_isr_en(0);                  //关闭pwm中断
}
//===========================================================================================================================================================
//PMW周期图案加周期中断实现低亮灯周期4闪例子
//
//低亮灯：___holdtime1_ms______                          ____
//                            |                          |
//                            |____holdtime1_ms:200ms____|
//        |<--整个PWM亮灭周期(中断周期)：cycle_ms:400ms->|
//
//实现方法：
//     1.画出单闪的PWM图案和一个灭灯的PWM图案
//     2.使能PWM中断计算4次pwm单闪后切换到灭灯PWM周期
//     3.灭灯PWM周期结束后再切换到4个单闪周期
//===========================================================================================================================================================
void pwm_ledL_off_Nms(int off_ms)
{
    struct pwm_cycle_pattern pattern = {0};
    pattern.cycle_ms = off_ms;      	//设置灭灯时间，时间到产生pwm中断
    pattern.holdtime1_ms = 0;
    pattern.holdtime2_ms = 0;
    pattern.holdtime3_ms = 0;
    pattern.holdtime4_ms = 0;
    pattern.led_L_bright = 0;
    pattern.led_H_bright = 0;
    pattern.led_type = 0;
    pattern.shift_duty = 0;

    pwm_cycle_pattern_init(&pattern);
    pwm_isr_state = four_flash_interval;//设置灭灯周期结束后进行闪动切换处理
    pwm_isr_cnt = 0;
    led_pwm_isr_en(1);
}

void pwm_ledL_four_flash_test()
{
    struct pwm_cycle_pattern pattern = {0};
    pattern.cycle_ms = 400;      		//设置4闪过程中的亮灭周期，每闪动一次周期是400ms
    pattern.holdtime1_ms = 10;     		//设置周期开始时灭灯保持时间
    pattern.holdtime2_ms = 200;    		//设置每个闪动周期的亮灯时间200ms
    pattern.holdtime3_ms = 0;    		//
    pattern.holdtime4_ms = 0;    		//
    pattern.led_L_bright = 500;    		//设置低亮灯亮度
    pattern.led_H_bright = 0;    		//
    pattern.led_type = 0;          		//
    pattern.shift_duty = 0;        		//

    pwm_cycle_pattern_init(&pattern);	//根据配置好的pwm周期图案进行初始话并启动PWM_LED模块
    pwm_isr_state = four_flash_cnt;     //设置pwm中断时进行4闪的计数处理
    pwm_isr_cnt = 0;
    led_pwm_isr_en(1);                  //使能pwm中断
}
//===========================================================================================================================================================
//PWM呼吸图案加周期中断处理，实现两个IO口推4灯轮流呼吸例子
//
//                                           _最高亮度保持时间__                                          __
//                                        __|  led_H_holdtime   |__                                    __|
//                breathe_interval     __|                         |__         breathe_interval     __|
//高亮灯：____________呼吸间隔________|         breathe_time          |____________呼吸间隔________|
//                                    |<----------呼吸时间----------->|
//       |<---PWM周期(中断周期) = breathe_interval + breathe_time---->|
//
//实现方法：
//	1.画出单个灯的呼吸图案
//	2.打开双灯模式和设置状态周期互换实现第二个灯的切换
//	3.产生两次PWM中断后切换IO口，进行第三和第四个灯的呼吸效果
//===========================================================================================================================================================
void pwm_four_led_breathe_test()
{
    struct pwm_breathe_pattern pattern = {0};
    pattern.breathe_interval = 1000;    //设置呼吸间隔为1000ms，实现每次灭灯间隔1s后进行一次呼吸动作。
    pattern.breathe_time = 2000;        //设置呼吸时间为2000ms，2秒内实现 灭->最亮->灭
    pattern.led_H_bright = 500;         //设置呼吸时间内高亮灯最高亮度
    pattern.led_H_holdtime = 100;		//设置呼吸时间内高亮灯最高亮度维持时
    pattern.led_L_bright = 500;			//设置呼吸时间内低亮灯最高亮度
    pattern.led_L_holdtime = 100;       //设置呼吸时间内低亮灯最高亮度维持时
    pattern.led_type = 2;               //推双灯模式
    pattern.shift_duty = 1;             //每个周期进行双灯状态互换

    pwm_breathe_pattern_init(&pattern);
    pwm_isr_state = change_PWM_IO;
    pwm_isr_cnt = 0;
    led_pwm_isr_en(1);
}
//===========================================================================================================================================================
//pwm初始化函数,用户使用该API时需要在 app_main() 调用该函数进行模块初始化
//
//===========================================================================================================================================================
void app_pwmled_init()
{
    request_irq(IRQ_PWM_LED_IDX, 1, pwm_cycle_isr_func, 0);  //初始化PWM中断函数
    led_pin_set_enable(IO_PORTA_00);                         //初始化PWM IO口
    pwmled_pwm_init();                                       //初始化PWM模块
}

#endif
