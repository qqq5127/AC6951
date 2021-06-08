#ifndef _COLOR_LED_H
#define _COLOR_LED_H

typedef struct _color_cell {
    u8 type;
    u32 led_io;
    u32 show_time;/*ms,'0' means never stop,used display_time_by_step if display_time_by_step not zero */
    struct {
        struct {
            u32 color;
        } color;

        struct {
            u32 loop_num;
            u32 back_step;
        } loop;

        struct {
            u32 start_color;
            u8 color_sel 		: 3/*BIT0:RED BIT1:GREE BIT2:BLUE*/;
            u8 start_direction	: 3;
            u8 reserved			: 2;
            u32 step_time;
            u8 step;
            u8 flood;
            u8 ceil;
            u32 display_time_by_step;/*used display_time_by_step if display_time_by_step not zero */
        } pwm_color;
    } type_action;
} __attribute__((packed)) color_cell_t;

typedef enum {
    COLOR_TYPE_SET = 1,
    COLOR_TYPE_LOOP,
    COLOR_TYPE_PWM,
} COLOR_TYPE;

int color_led_set(const color_cell_t table[], u32 numberofmembers, void (*led_select)(u32 led_io));
#endif
