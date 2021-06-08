#ifndef _COLOR_LED_DRIVER_H
#define _COLOR_LED_DRIVER_H


#define COLOR_LED_MODE_NORNAL   0
#define COLOR_LED_MODE_PWM	    1
#define PWM_NO_OUTPUT_CH 		(-1)

typedef struct _colorled_platform_data {
    struct {
        u8 pin;
        JL_TIMER_TypeDef *timer_hdl;
        int output_ch;/*"-1": do not used output channel*/
    } pin_red, pin_gree, pin_blue;
    u8 work_mode;
} colorled_platform_data;

void __color_set(u32 color);
void __color_led_io_set(const colorled_platform_data *cfg_data);
#define COLOR_SET(color) __color_set(color)
#define COLOR_LED_IO_SET(cfg_data) __color_led_io_set(cfg_data)

/*
 * BYTE3(reserved)  BYTE2(BLUE)  BYTE1(GREE)  BYTE0(RED)
 * */
#define COLOR_RED		0X000000FF
#define COLOR_GREEN		0X0000FF00
#define COLOR_BLUE		0X00FF0000
#define COLOR_YELLOW    0X0000FFFF
#define COLOR_PURPLE	0X00FF00FF
#define COLOR_CYAN		0X00FFFF00
#define COLOR_WHITE		0X00FFFFFF
#define COLOR_BLACK		0X00000000
#define COLOR_ORANGE	0X000080FF



#endif
