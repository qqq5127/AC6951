#ifndef _COLOR_LED_APP_H
#define _COLOR_LED_APP_H
#include "color_led_driver.h"
#include "color_led.h"

typedef enum {
    COLOR_LED_MODE_RAINBOW						= 0,
    COLOR_LED_MODE_HEARTBEAT					= 1,
    COLOR_LED_MODE_CANDLELIGHT					= 2,
    COLOR_LED_MODE_NIGHTLIGNT					= 3,
    COLOR_LED_MODE_STAGE						= 4,
    COLOR_LED_MODE_WANDERFUL_BREATH				= 5,
    COLOR_LED_MODE_RED_BREATH					= 6,
    COLOR_LED_MODE_GREEN_BREATH					= 7,
    COLOR_LED_MODE_BLUE_BREATH					= 8,
    COLOR_LED_MODE_NICE_EMOTION					= 9,
    COLOR_LED_MODE_SUNSET						= 10,
    COLOR_LED_MODE_FOLLOW_MUSIC					= 11,
    COLOR_LED_MODE_LIGHT						= 12,
    COLOR_LED_MODE_COLORFUL_TWINKLE				= 13,
    COLOR_LED_MODE_TWINKLE						= 14,
} COLOR_LED_MODE;

#define TWINKLE_FAST			0
#define TWINKLE_SLOW			1
#define TWINKLE_MID				2
#define TWINKLE_FOLLOW_MUSIC 	3
#define TWINKLE_PAUSE			4

#define TWINKLE_SLOW_FRE		900
#define TWINKLE_MID_FRE			600
#define TWINKLE_FAST_FRE		300

#define TWINKLE_SLOW_THRESHOLD	100
#define TWINKLE_MID_THRESHOLD	1000

#if TCFG_COLORLED_ENABLE
#define COLOR_LED_SET(table,led_select) \
	color_led_set(table,table##_size,led_select)
#else
#define COLOR_LED_SET(table,led_select)
#endif

#define COLORLED_PLATFORM_DATA_BEGIN(data) \
		const colorled_platform_data data = {

#define COLORLED_PLATFORM_DATA_END() \
};

void color_led_init(void *data);
int color_led_set_api(u8 mode, u8 fre_mode, u32 color);
#endif
