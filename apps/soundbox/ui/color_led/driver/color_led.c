#include "system/includes.h"
#include "app_config.h"
#include "user_cfg.h"
#include "color_led_app.h"

#if TCFG_COLORLED_ENABLE
#define LOG_TAG_CONST       COLOR_LED
#define LOG_TAG             "[COLOR_LED]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define CPU_ENTER_CRITICAL() local_irq_disable()
#define CPU_EXIT_CRITICAL() local_irq_enable()
typedef struct {
    const color_cell_t *begin;
    const color_cell_t *cur;
    const color_cell_t *loop;
    void (*led_select)(u32 led_io);
    u32 next_step_timer;
    u32 loop_cnt;
    u32 total_step;
    u32 pwm_loop_timer;
    u32 pwm_cur_color;
    u32 breath_step_cnt;
    u8 pwm_step_dir	: 3;
} color_led_control_t;

static color_led_control_t color_led_hdl;
#define __this (&color_led_hdl)

static void color_led_timer(void *priv);
static void pwm_loop(void *priv);
static int color_led_active(const color_cell_t *color, color_led_control_t *hdl)
{
    if ((hdl == NULL) || (color == NULL)) {
        return (-1);
    }
    const color_cell_t *active_color = NULL;

    CPU_ENTER_CRITICAL();
    hdl->cur = color;
    CPU_EXIT_CRITICAL();
    switch (hdl->cur->type) {
    case COLOR_TYPE_SET:
        if (hdl->led_select) {
            hdl->led_select(hdl->cur->led_io);
        }

        COLOR_SET(hdl->cur->type_action.color.color);
        if (hdl->cur->show_time) {
            hdl->next_step_timer = sys_hi_timeout_add(hdl, color_led_timer, hdl->cur->show_time);
        }
        break;
    case COLOR_TYPE_LOOP:
        if (hdl->loop != color) {
            /*load new loop*/
            hdl->loop = color;
            hdl->loop_cnt = color->type_action.loop.loop_num;
        }

        if (hdl->loop_cnt == 0) {
            /*loop end*/
            active_color = ++(hdl->cur);
            color_led_active(active_color, hdl);
            break;
        }

        if (hdl->total_step < hdl->loop->type_action.loop.back_step) {
            log_info("!!!back_step bigger than the number of elements of table \n");
            break;
        }

        active_color = hdl->begin + hdl->loop->type_action.loop.back_step;
        hdl->loop_cnt--;
        CPU_ENTER_CRITICAL();
        if (active_color->type == COLOR_TYPE_PWM) {
            hdl->breath_step_cnt = 0;
            hdl->pwm_step_dir = active_color->type_action.pwm_color.start_direction;
            hdl->pwm_cur_color = active_color->type_action.pwm_color.start_color;
        }
        CPU_EXIT_CRITICAL();
        color_led_active(active_color, hdl);
        break;
    case COLOR_TYPE_PWM:
        if (hdl->led_select) {
            hdl->led_select(hdl->cur->led_io);
        }

        COLOR_SET(hdl->pwm_cur_color);
        if (hdl->cur->show_time && (!hdl->cur->type_action.pwm_color.display_time_by_step)) {
            hdl->next_step_timer = sys_hi_timeout_add(hdl, color_led_timer, hdl->cur->show_time);
        }

        //register pwm loop
        if (hdl->cur->type_action.pwm_color.color_sel && hdl->cur->type_action.pwm_color.step_time) {
            hdl->pwm_loop_timer = sys_hi_timeout_add(hdl, pwm_loop, hdl->cur->type_action.pwm_color.step_time);
        }

        break;
    default:
        break;
    }

    return 0;
}

static void color_led_timer(void *priv)
{
    color_led_control_t *hdl = (color_led_control_t *)priv;
    if (hdl == NULL) {
        return;
    }

    //del pwm loop
    if (hdl->pwm_loop_timer) {
        sys_hi_timer_del(hdl->pwm_loop_timer);
        hdl->pwm_loop_timer = 0;
    }

    hdl->next_step_timer = 0;

    /*to next step*/
    const color_cell_t *active_color = ++(hdl->cur);
    CPU_ENTER_CRITICAL();
    if (hdl->cur->type == COLOR_TYPE_PWM) {
        hdl->breath_step_cnt = 0;
        hdl->pwm_step_dir = hdl->cur->type_action.pwm_color.start_direction;
        hdl->pwm_cur_color = hdl->cur->type_action.pwm_color.start_color;
    }
    CPU_EXIT_CRITICAL();
    color_led_active(active_color, hdl);
}

static void pwm_loop(void *priv)
{
#define RED_COLOR_SET(red_color,color)		((red_color  & 0x000000ff) + (color & 0xffffff00))
#define GREE_COLOR_SET(gree_color,color)	((gree_color & 0x0000ff00) + (color & 0xffff00ff))
#define BLUE_COLOR_SET(blue_color,color)	((blue_color & 0x00ff0000) + (color & 0xff00ffff))
#define COLOR_LED_BREATH_DATA_IS_LEGAL(color,floor,ceil)	((color>=floor) && (color<=ceil))
    color_led_control_t *hdl = (color_led_control_t *)priv;
    if (hdl == NULL) {
        return;
    }

    hdl->breath_step_cnt ++;

    if (hdl->cur->type_action.pwm_color.color_sel & BIT(0)) {
        //set red value
        u8 dir = hdl->pwm_step_dir & BIT(0);/*0:down 1:up*/
        u32 color = hdl->pwm_cur_color;
        color &= 0x000000ff;
        if (!COLOR_LED_BREATH_DATA_IS_LEGAL(color, \
                                            hdl->cur->type_action.pwm_color.flood, hdl->cur->type_action.pwm_color.ceil)) {
            log_info("!!!data illegal color:%x floor:%x ceil:%x \n", color, \
                     hdl->cur->type_action.pwm_color.flood, hdl->cur->type_action.pwm_color.ceil);
            return;
        }

        if (dir == 0) {
            if ((color - hdl->cur->type_action.pwm_color.flood) > hdl->cur->type_action.pwm_color.step) {
                color -= hdl->cur->type_action.pwm_color.step;
            } else {
                color = hdl->cur->type_action.pwm_color.flood;
                //down to up
                hdl->pwm_step_dir ^= BIT(0);
            }
        } else {
            if ((color + hdl->cur->type_action.pwm_color.step) >= hdl->cur->type_action.pwm_color.ceil) {
                color = hdl->cur->type_action.pwm_color.ceil;
                //up to down
                hdl->pwm_step_dir ^= BIT(0);
            } else {
                color += hdl->cur->type_action.pwm_color.step;
            }
        }

        hdl->pwm_cur_color = RED_COLOR_SET(color, hdl->pwm_cur_color);
    }

    if (hdl->cur->type_action.pwm_color.color_sel & BIT(1)) {
        //set gree value
        u8 dir = hdl->pwm_step_dir & BIT(1);/*0:down 1:up*/
        u32 color = hdl->pwm_cur_color;
        color &= 0x0000ff00;
        color >>= 8;
        if (!COLOR_LED_BREATH_DATA_IS_LEGAL(color, \
                                            hdl->cur->type_action.pwm_color.flood, hdl->cur->type_action.pwm_color.ceil)) {
            log_info("!!!data illegal color:%x floor:%x ceil:%x \n", color, \
                     hdl->cur->type_action.pwm_color.flood, hdl->cur->type_action.pwm_color.ceil);
            return;
        }

        if (dir == 0) {
            if ((color - hdl->cur->type_action.pwm_color.flood) > hdl->cur->type_action.pwm_color.step) {
                color -= hdl->cur->type_action.pwm_color.step;
            } else {
                color = hdl->cur->type_action.pwm_color.flood;
                //down to up
                hdl->pwm_step_dir ^= BIT(1);
            }
        } else {
            if ((color + hdl->cur->type_action.pwm_color.step) >= hdl->cur->type_action.pwm_color.ceil) {
                color = hdl->cur->type_action.pwm_color.ceil;
                //up to down
                hdl->pwm_step_dir ^= BIT(1);
            } else {
                color += hdl->cur->type_action.pwm_color.step;
            }
        }
        color <<= 8;

        hdl->pwm_cur_color = GREE_COLOR_SET(color, hdl->pwm_cur_color);
    }

    if (hdl->cur->type_action.pwm_color.color_sel & BIT(2)) {
        //set gree value
        u8 dir = hdl->pwm_step_dir & BIT(2);/*0:down 1:up*/
        u32 color = hdl->pwm_cur_color;
        color &= 0x00ff0000;
        color >>= 16;
        if (!COLOR_LED_BREATH_DATA_IS_LEGAL(color, \
                                            hdl->cur->type_action.pwm_color.flood, hdl->cur->type_action.pwm_color.ceil)) {
            log_info("!!!data illegal color:%x floor:%x ceil:%x \n", color, \
                     hdl->cur->type_action.pwm_color.flood, hdl->cur->type_action.pwm_color.ceil);
            return;
        }

        if (dir == 0) {
            if ((color - hdl->cur->type_action.pwm_color.flood) > hdl->cur->type_action.pwm_color.step) {
                color -= hdl->cur->type_action.pwm_color.step;
            } else {
                color = hdl->cur->type_action.pwm_color.flood;
                //down to up
                hdl->pwm_step_dir ^= BIT(2);
            }
        } else {
            if ((color + hdl->cur->type_action.pwm_color.step) >= hdl->cur->type_action.pwm_color.ceil) {
                color = hdl->cur->type_action.pwm_color.ceil;
                //up to down
                hdl->pwm_step_dir ^= BIT(2);
            } else {
                color += hdl->cur->type_action.pwm_color.step;
            }
        }
        color <<= 16;

        hdl->pwm_cur_color = BLUE_COLOR_SET(color, hdl->pwm_cur_color);
    }

    //log_info("color:%x \n",hdl->pwm_cur_color);
    COLOR_SET(hdl->pwm_cur_color);
    if (hdl->cur->type_action.pwm_color.color_sel && hdl->cur->type_action.pwm_color.step_time) {
        hdl->pwm_loop_timer = sys_hi_timeout_add(hdl, pwm_loop, hdl->cur->type_action.pwm_color.step_time);
    }

    if (hdl->cur->type_action.pwm_color.display_time_by_step && \
        (hdl->breath_step_cnt == hdl->cur->type_action.pwm_color.display_time_by_step)) {
        color_led_timer(hdl);
    }
}

static void color_led_close(color_led_control_t *hdl)
{
    if (hdl == NULL) {
        return;
    }
    if (hdl->next_step_timer) {
        sys_hi_timer_del(hdl->next_step_timer);
        hdl->next_step_timer = 0;
    }
    if (hdl->pwm_loop_timer) {
        sys_hi_timer_del(hdl->pwm_loop_timer);
        hdl->pwm_loop_timer = 0;
    }

    COLOR_SET(COLOR_BLACK);
}

int color_led_set(const color_cell_t table[], u32 numberofmembers, void (*led_select)(u32 led_io))
{
    if (table == NULL) {
        return (-1);
    }

    color_led_control_t *hdl = __this;
    color_led_close(hdl);
    CPU_ENTER_CRITICAL();
    hdl->begin = table;
    hdl->cur = table;
    hdl->loop = NULL;
    hdl->total_step = numberofmembers;
    hdl->loop_cnt = 0;
    hdl->led_select = led_select;
    if (hdl->cur->type == COLOR_TYPE_PWM) {
        hdl->breath_step_cnt = 0;
        hdl->pwm_step_dir = hdl->cur->type_action.pwm_color.start_direction;
        hdl->pwm_cur_color = hdl->cur->type_action.pwm_color.start_color;
    }
    CPU_EXIT_CRITICAL();

    color_led_active(hdl->cur, hdl);
    return 0;
}

#endif

