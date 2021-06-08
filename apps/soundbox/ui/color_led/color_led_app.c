#include "system/includes.h"
#include "app_config.h"
#include "user_cfg.h"
#include "color_led_app.h"
#include "color_led_table.h"
#include "music_player.h"

#if TCFG_COLORLED_ENABLE
#define LOG_TAG_CONST       COLOR_LED
#define LOG_TAG             "[COLOR_LED]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if AUDIO_OUTPUT_AUTOMUTE
static void audio_dac_energy_scan(void *priv);
static u8 color_led_dac_energy_to_fre(int energy);
int audio_dac_energy_get(void);

#define TWINKLE_MUSIC_TIME		500
#endif

typedef struct {
    u8 cur_mode;
    u32 cur_color;
    int cur_fremode;
    u32 music_twinkle_timer_hdl;
    u32 colorful_twinkle_pause_color;
} color_led_app_t;
static color_led_app_t color_led_app;
#define __this (&color_led_app)

#define CPU_ENTER_CRITICAL() local_irq_disable()
#define CPU_EXIT_CRITICAL() local_irq_enable()
static color_cell_t color_table_space[4];
COLOR_TABLE_REGISTER(color_table_space);

static void led_select(u32 led_io)
{
//	log_info("led_io:%x",led_io);
}

static void colorful_twinkle_color_cb(u32 color)
{
    if (color == COLOR_BLACK) {
        return;
    }

    __this->colorful_twinkle_pause_color = color;
//	log_info("led_io:%x",led_io);
}
static void color_led_colorful_twinkle_deal(u8 fre_mode)
{
    switch (fre_mode) {
    case TWINKLE_SLOW:
        COLOR_LED_SET(color_table_colorful_twinkle_slow, colorful_twinkle_color_cb);
        log_info("colorful twinkle scale slow \n");
        break;
    case TWINKLE_MID:
        COLOR_LED_SET(color_table_colorful_twinkle_mid, colorful_twinkle_color_cb);
        log_info("colorful twinkle scale middle \n");
        break;
    case TWINKLE_FAST:
        COLOR_LED_SET(color_table_colorful_twinkle_fast, colorful_twinkle_color_cb);
        log_info("colorful twinkle scale fast \n");
        break;
    case TWINKLE_FOLLOW_MUSIC:
#if AUDIO_OUTPUT_AUTOMUTE
        __this->cur_mode = COLOR_LED_MODE_COLORFUL_TWINKLE;
        __this->cur_fremode = (-1);
        __this->music_twinkle_timer_hdl = sys_hi_timer_add(__this, audio_dac_energy_scan, TWINKLE_MUSIC_TIME);
        return;
#endif
        break;
    case TWINKLE_PAUSE:
        if (__this->colorful_twinkle_pause_color == 0) {
            __this->colorful_twinkle_pause_color = COLOR_GREEN;
        }
        CPU_ENTER_CRITICAL();
        memcpy((u8 *)color_table_space, (u8 *)color_table_light_base, color_table_light_base_size * sizeof(color_cell_t));
        color_table_space[0].show_time = 0;
        color_table_space[0].type_action.color.color = __this->colorful_twinkle_pause_color;
        COLOR_LED_SET(color_table_space, led_select);
        CPU_EXIT_CRITICAL();
        log_info("colorful twinkle scale pause \n");
        return;
        break;
    default:
        break;
    }
}

static void color_led_color_twinkle_deal(u8 fre_mode, u32 color)
{
    if (sizeof(color_table_space) < color_table_twinkle_base_size * sizeof(color_cell_t)) {
        log_info("space no enough \n");
        return;
    }

    u32 fre = 0;
    switch (fre_mode) {
    case TWINKLE_SLOW:
        fre = TWINKLE_SLOW_FRE;
        log_info("twinkle scale slow \n");
        break;
    case TWINKLE_MID:
        fre = TWINKLE_MID_FRE;
        log_info("twinkle scale middle \n");
        break;
    case TWINKLE_FAST:
        fre = TWINKLE_FAST_FRE;
        log_info("twinkle scale fast \n");
        break;

    case TWINKLE_FOLLOW_MUSIC:
#if AUDIO_OUTPUT_AUTOMUTE
        __this->cur_mode = COLOR_LED_MODE_TWINKLE;
        __this->cur_color = color;
        __this->cur_fremode = (-1);
        __this->music_twinkle_timer_hdl = sys_hi_timer_add(__this, audio_dac_energy_scan, TWINKLE_MUSIC_TIME);
        return;
#endif
        break;
    case TWINKLE_PAUSE:
        fre = 0;
        log_info("twinkle scale pause \n");
        break;
    default:
        fre = TWINKLE_SLOW_FRE;
        break;
    }

    CPU_ENTER_CRITICAL();
    memcpy((u8 *)color_table_space, (u8 *)color_table_twinkle_base, color_table_twinkle_base_size * sizeof(color_cell_t));
    color_table_space[0].show_time = fre;
    color_table_space[0].type_action.color.color = color;
    color_table_space[1].show_time = fre;
    COLOR_LED_SET(color_table_space, led_select);
    CPU_EXIT_CRITICAL();

}

static void color_led_color_light_deal(u32 color)
{
    if (sizeof(color_table_space) < color_table_light_base_size * sizeof(color_cell_t)) {
        log_info("space no enough \n");
        return;
    }

    CPU_ENTER_CRITICAL();
    memcpy((u8 *)color_table_space, (u8 *)color_table_light_base, color_table_light_base_size * sizeof(color_cell_t));
    color_table_space[0].show_time = 0;
    color_table_space[0].type_action.color.color = color;
    COLOR_LED_SET(color_table_space, led_select);
    CPU_EXIT_CRITICAL();

}

static void color_led_follow_music_deal(u8 fre_mode)
{
    switch (fre_mode) {
    case TWINKLE_SLOW:
        COLOR_LED_SET(color_table_green_breath, led_select);
        log_info("colorful twinkle scale slow \n");
        break;
    case TWINKLE_MID:
        COLOR_LED_SET(color_table_blue_breath, led_select);
        log_info("colorful twinkle scale middle \n");
        break;
    case TWINKLE_FAST:
        COLOR_LED_SET(color_table_red_breath, led_select);
        log_info("colorful twinkle scale fast \n");
        break;
    case TWINKLE_FOLLOW_MUSIC:
#if AUDIO_OUTPUT_AUTOMUTE
        __this->cur_mode = COLOR_LED_MODE_FOLLOW_MUSIC;
        __this->cur_fremode = (-1);
        __this->music_twinkle_timer_hdl = sys_hi_timer_add(__this, audio_dac_energy_scan, TWINKLE_MUSIC_TIME);
        return;
#endif
        break;
    case TWINKLE_PAUSE:
        COLOR_LED_SET(color_table_green_breath, led_select);
        log_info("colorful twinkle scale pause \n");
        break;
    default:
        break;
    }
}
int color_led_set_api(u8 mode, u8 fre_mode, u32 color)
{
    if (__this->music_twinkle_timer_hdl) {
        sys_hi_timer_del(__this->music_twinkle_timer_hdl);
        __this->music_twinkle_timer_hdl = 0;
    }

    switch (mode) {
    case COLOR_LED_MODE_SUNSET:
        COLOR_LED_SET(color_table_sunset, led_select);
        break;
    case COLOR_LED_MODE_NICE_EMOTION:
        COLOR_LED_SET(color_table_nice_emotion, led_select);
        break;
    case COLOR_LED_MODE_BLUE_BREATH:
        COLOR_LED_SET(color_table_blue_breath, led_select);
        break;
    case COLOR_LED_MODE_GREEN_BREATH:
        COLOR_LED_SET(color_table_green_breath, led_select);
        break;
    case COLOR_LED_MODE_RED_BREATH:
        COLOR_LED_SET(color_table_red_breath, led_select);
        break;
    case COLOR_LED_MODE_WANDERFUL_BREATH:
        COLOR_LED_SET(color_table_wonderful_breath, led_select);
        break;
    case COLOR_LED_MODE_NIGHTLIGNT:
        COLOR_LED_SET(color_table_nightlight, led_select);
        break;
    case COLOR_LED_MODE_CANDLELIGHT:
        COLOR_LED_SET(color_table_candlelight, led_select);
        break;
    case COLOR_LED_MODE_HEARTBEAT:
        COLOR_LED_SET(color_table_heartbeat, led_select);
        break;
    case COLOR_LED_MODE_RAINBOW:
        COLOR_LED_SET(color_table_rainbow, led_select);
        break;
    case COLOR_LED_MODE_COLORFUL_TWINKLE:
        color_led_colorful_twinkle_deal(fre_mode);
        break;
    case COLOR_LED_MODE_TWINKLE:
        color_led_color_twinkle_deal(fre_mode, color);
        break;
    case COLOR_LED_MODE_LIGHT:
        color_led_color_light_deal(color);
        break;
    case COLOR_LED_MODE_FOLLOW_MUSIC:
        color_led_follow_music_deal(TWINKLE_FOLLOW_MUSIC);
        break;
    case COLOR_LED_MODE_STAGE:
        COLOR_LED_SET(color_table_stage, led_select);
        break;
    default:
        return (-1);
        break;

    }

    return 0;
}

#if AUDIO_OUTPUT_AUTOMUTE
static u8 color_led_dac_energy_to_fremode(int energy)
{
    if (energy < 0) {
        energy = 0;
    }

    switch (energy) {
    case 0:
        return TWINKLE_PAUSE;
        break;
    case 1 ... TWINKLE_SLOW_THRESHOLD:
        return TWINKLE_SLOW;
        break;
    case (TWINKLE_SLOW_THRESHOLD+1) ... TWINKLE_MID_THRESHOLD:
        return TWINKLE_MID;
        break;
    default:
        return TWINKLE_FAST;
        break;
    }
    return 1;
}

static void audio_dac_energy_scan(void *priv)
{
    color_led_app_t *hdl = priv;
    if (hdl == NULL) {
        return;
    }
    int energy = audio_dac_energy_get();
    if (energy < 0) {
        putchar('e');
        energy = 0;
    }
//	printf("energy:%d \n",energy);
    u8 fre_mode = color_led_dac_energy_to_fremode(energy);
    if (fre_mode == hdl->cur_fremode) {
        return;
    }
    hdl->cur_fremode = fre_mode;

    switch (hdl->cur_mode) {
    case COLOR_LED_MODE_COLORFUL_TWINKLE:
        color_led_colorful_twinkle_deal(hdl->cur_fremode);
        break;
    case COLOR_LED_MODE_TWINKLE:
        color_led_color_twinkle_deal(hdl->cur_fremode, hdl->cur_color);
        break;
    case COLOR_LED_MODE_FOLLOW_MUSIC:
        color_led_follow_music_deal(hdl->cur_fremode);
        break;
    default:
        return;
        break;

    }
}
#endif

void color_led_init(void *data)
{
    log_info("color_led_init \n");
    const colorled_platform_data *cfg_data = (const colorled_platform_data *)data;
    COLOR_LED_IO_SET(cfg_data);

//    color_led_set_api(COLOR_LED_MODE_LIGHT, TWINKLE_SLOW, 0x00ff0000);
}
#else
void color_led_init(void *data)
{

}

int color_led_set_api(u8 mode, u8 fre_mode, u32 color)
{
    return 0;
}
#endif
