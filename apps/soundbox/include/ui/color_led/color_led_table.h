#ifndef _COLOR_LED_TABLE_H
#define _COLOR_LED_TABLE_H
#include "color_led_app.h"

#define RED_EN		BIT(0)
#define GREEN_EN	BIT(1)
#define BLUE_EN		BIT(2)
/*
description:
	本步骤可以设置RGB灯处于一个特定的颜色状态，并且可以持续的时间

parameter:
	led_io:
		对应int color_led_set(const color_cell_t table[], u32 numberofmembers, void (*led_select)(u32 led_io))接口中的
	led_select回调接口中的led_io参数
	show_time:本步骤持续的时间，单位ms,0为本步骤持续无限久
	color:显示的颜色，和三基色对应，BIT(0)对应红色，BIT(1)对应绿色，BIT(2)对应蓝色，BIT(3)保留未使用
 * */
#define COLOR_TYPE_COLOR_INIT(led_io,show_time,color) {COLOR_TYPE_SET,led_io,show_time,color,0x11223344,0x11223344}
/*
description:
	本步骤可以让现在的控制跳到特定的步骤，从而实现循环控制。

parameter:
	back_step:返回到相对于开始位置的步骤值
	loop_num:循环的次数
 * */
#define COLOR_TYPE_LOOP_INIT(back_step,loop_num) {COLOR_TYPE_LOOP,0xffffffff,0,0x11223344,loop_num,back_step}
/*
description:
	本步骤可以实现RGB灯的呼吸灯效果控制。可以选择RGB其中的某几个颜色做呼吸灯控制，另外的灯保持start_color状态。也可以选择
	RGB灯每个颜色灯开始是PWM递增还是PWM递减。

parameter:
	show_time:本步骤持续的时间，单位ms,0为本步骤持续无限久
	start_color:开始color设置
	color_sel:选择RGB灯中的特定灯做呼吸灯效果变化，BIT(0)对应红色，BIT(1)对应绿色，BIT(2)对应蓝色，‘1’为做呼吸灯效果，‘0’为
		保持start_color的状态。
	start_direction:RGB灯每个颜色灯开始是PWM递增还是PWM递减。BIT(0)对应红色，BIT(1)对应绿色，BIT(2)对应蓝色，‘1’为递增，‘0’
		为递减。
	step_time:每递增或者递减一个step长度的时间。
	step:每一个步骤递增或者递减的RGB值。例如R当前为0xA8，方向为递减，step为2，下一个step_time时间R会变成0xA6。
	floor:递减的最小值，当递减值小于floor时候，方向就会自动变为递增。
	ceil:递增的最大值，当递增至大于ceil时候，方向就会自动变为递减。
	display_time_by_step:显示的时间，时间单位是“step_time”
 * */
#define BREATH_RGB_SEL(R,G,B) (R|G|B)
#define BREATH_RGB_DIR_UP(R,G,B) (R|G|B)
#define COLOR_TYPE_BREATH_INIT(led_io,show_time,start_color,color_sel,start_direction,step_time,step,floor,ceil) \
			{COLOR_TYPE_PWM,led_io,show_time,0x11223344,0x11223344,0x11223344,\
				start_color,color_sel,start_direction,0,step_time,step,floor,ceil,0}
#define COLOR_TYPE_BREATH_MEASURE_BY_STEP_INIT(led_io,display_time_by_step,start_color,color_sel,start_direction,step_time,step,floor,ceil) \
			{COLOR_TYPE_PWM,led_io,0,0x11223344,0x11223344,0x11223344,\
				start_color,color_sel,start_direction,0,step_time,step,floor,ceil,display_time_by_step}

/*
description:
	加个结束标志，效果是黑色（灯全灭），一直持续。防止table不规范导致控制越界。

parameter:
 * */
#define COLOR_END() COLOR_TYPE_COLOR_INIT(0xffffffff,0,COLOR_BLACK)

#define COLOR_TABLE_REGISTER(name) const u32 name##_size = sizeof(name) / sizeof(name[0]);
#define COLOR_TABLE_EXTERN(name) \
	extern const color_cell_t name[];\
	extern const u32 name##_size;

COLOR_TABLE_EXTERN(color_table_demo1);
COLOR_TABLE_EXTERN(color_table_sunset);
COLOR_TABLE_EXTERN(color_table_nice_emotion);
COLOR_TABLE_EXTERN(color_table_blue_breath);
COLOR_TABLE_EXTERN(color_table_green_breath);
COLOR_TABLE_EXTERN(color_table_red_breath);
COLOR_TABLE_EXTERN(color_table_wonderful_breath);
COLOR_TABLE_EXTERN(color_table_nightlight);
COLOR_TABLE_EXTERN(color_table_candlelight);
COLOR_TABLE_EXTERN(color_table_heartbeat);
COLOR_TABLE_EXTERN(color_table_rainbow);
COLOR_TABLE_EXTERN(color_table_colorful_twinkle_slow);
COLOR_TABLE_EXTERN(color_table_colorful_twinkle_mid);
COLOR_TABLE_EXTERN(color_table_colorful_twinkle_fast);
COLOR_TABLE_EXTERN(color_table_twinkle_base);
COLOR_TABLE_EXTERN(color_table_light_base);
COLOR_TABLE_EXTERN(color_table_stage);
COLOR_TABLE_EXTERN(color_table_colorful_twinkle_pause);

#endif
