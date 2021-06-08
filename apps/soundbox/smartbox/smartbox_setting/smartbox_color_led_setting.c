#include "app_config.h"
#include "syscfg_id.h"
#include "user_cfg.h"
#include "le_smartbox_module.h"

#include "smartbox_setting_sync.h"
#include "smartbox_setting_opt.h"

//#if (SMART_BOX_EN && RCSP_SMARTBOX_ADV_EN)
#if (SMART_BOX_EN)

#if RCSP_ADV_COLOR_LED_SET_ENABLE
#include "ui_manage.h"
#include "asm/pwm_led.h"
#include "color_led_app.h"
#define IS_COLOR_LED_LEGAL(color_led) \
	((color_led.open_status <= 2) && \
	 (color_led.mode <= 2) && \
	 (color_led.fre_mode <= 3))

enum {
    OPEN_STATUS_CLOSE 		= 0,
    OPEN_STATUS_OPEN 		= 1,
    OPEN_STATUS_SETTING		= 2,
};
enum {
    MODE_LIGHT				= 0,
    MODE_TWINKLE			= 1,
    MODE_USER				= 2,
};
const u32 twinkle_color_table[] = {
    COLOR_RED,
    COLOR_ORANGE,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_CYAN,
    COLOR_BLUE,
    COLOR_PURPLE,
};

typedef struct {
    u8 open_status  : 2;//0:关闭  1：打开  2：设置模式
    u8 mode			: 3;//0:彩色  2：闪烁  3：情景模式
    u8 reseved		: 3;
    u8 red;
    u8 green;
    u8 blue;
    u8 twinkle_mode;
    u8 fre_mode;
    u8 user_mode;
    u16 hue;
    u8 saturation;
    u8 lightness;
} __attribute__((packed)) color_led_t;
static color_led_t color_led;

#define COLOR_LED_PROTOCOL_DATA_LEN		 	(sizeof(color_led_t))

u32 get_color_led_setting_data_len(void)
{
    return COLOR_LED_PROTOCOL_DATA_LEN;
}

static void set_color_led_setting(u8 *color_led_setting_info)
{
    put_buf(color_led_setting_info, COLOR_LED_PROTOCOL_DATA_LEN);
    color_led_t color_led_cur;
    memcpy((u8 *)&color_led_cur, color_led_setting_info, sizeof(color_led_t));
    if ((color_led_cur.open_status == 0) || (color_led_cur.open_status == 1)) {
        //switch active,not set other_status
        color_led.open_status = color_led_cur.open_status;
        return;
    }

    memcpy((u8 *)&color_led, color_led_setting_info, sizeof(color_led_t));
}

static int get_color_led_setting(u8 *color_led_setting_info)
{
    color_led_t color_led_cur;
    int len = syscfg_read(VM_COLOR_LED_SETTING, color_led_setting_info, COLOR_LED_PROTOCOL_DATA_LEN);

    if (len != COLOR_LED_PROTOCOL_DATA_LEN) {
        printf("!!!!!!!!color led data read error !!!!!!!!!!!!!!! \n");
    }
    return 0;
}

// 1、写入VM
static void update_color_led_setting_vm_value(u8 *color_led_setting_info)
{
    color_led_t color_led_cur;
    memcpy((u8 *)&color_led_cur, color_led_setting_info, sizeof(color_led_t));
    if ((color_led_cur.open_status == 0) || (color_led_cur.open_status == 1)) {
        //switch active,not set other_status
        u8 open_status = color_led_cur.open_status;
        int len = syscfg_read(VM_COLOR_LED_SETTING, (u8 *)&color_led_cur, COLOR_LED_PROTOCOL_DATA_LEN);
        color_led_cur.open_status = open_status;
        syscfg_write(VM_COLOR_LED_SETTING, (u8 *)&color_led_cur, COLOR_LED_PROTOCOL_DATA_LEN);
        return;
    }

    syscfg_write(VM_COLOR_LED_SETTING, color_led_setting_info, COLOR_LED_PROTOCOL_DATA_LEN);
}
// 2、同步对端
static void color_led_setting_sync(u8 *color_led_setting_info)
{
#if TCFG_USER_TWS_ENABLE
    extern int get_bt_tws_connect_status();
    if (get_bt_tws_connect_status()) {
        update_smartbox_setting(ATTR_TYPE_COLOR_LED_SETTING);
    }
#endif
}

static void color_led_flash(void)
{

    color_led_t color_led_cur;
    get_color_led_setting((u8 *)&color_led_cur);
    if (!IS_COLOR_LED_LEGAL(color_led_cur)) {
        printf("color led illegal data \n");
        return;
    }

    u8 mode = 0;
    u8 fre_mode = 0;
    u32 color = 0;

    if (color_led_cur.open_status == OPEN_STATUS_CLOSE) {
        mode = COLOR_LED_MODE_LIGHT;
        color = COLOR_BLACK;
        color_led_set_api(mode, fre_mode, color);
        return;
    }

    fre_mode = color_led_cur.fre_mode;
    color = color_led_cur.red + ((int)color_led_cur.green << 8) + ((int)color_led_cur.blue << 16);
    switch (color_led_cur.mode) {
    case MODE_LIGHT:
        mode = COLOR_LED_MODE_LIGHT;
        break;
    case MODE_TWINKLE:
        if (color_led_cur.twinkle_mode == 0) {
            mode = COLOR_LED_MODE_COLORFUL_TWINKLE;
        } else {
            mode = COLOR_LED_MODE_TWINKLE;
            color = twinkle_color_table[color_led_cur.twinkle_mode - 1];
        }
        break;
    case MODE_USER:
        mode = COLOR_LED_MODE_RAINBOW + color_led_cur.user_mode;
        break;
    default:
        break;
    }

    printf("mode:%d fre_mode:%d color:0x%x \n", mode, fre_mode, color);
    color_led_set_api(mode, fre_mode, color);
}

static void deal_color_led_setting(u8 *color_led_setting_info, u8 write_vm, u8 tws_sync)
{
    u8 led_setting[COLOR_LED_PROTOCOL_DATA_LEN];
    if (!color_led_setting_info) {
        get_color_led_setting(led_setting);
    } else {
        memcpy(led_setting, color_led_setting_info, COLOR_LED_PROTOCOL_DATA_LEN);
        set_color_led_setting(color_led_setting_info);
    }
    if (write_vm) {
        update_color_led_setting_vm_value(led_setting);
    }
    if (tws_sync) {
        color_led_setting_sync(led_setting);
    }

    color_led_flash();
}

static int smartbox_color_led_init(void)
{
    color_led_t color_led_cur;
    int len = syscfg_read(VM_COLOR_LED_SETTING, (u8 *)&color_led_cur, COLOR_LED_PROTOCOL_DATA_LEN);

    if ((!IS_COLOR_LED_LEGAL(color_led)) || (len != COLOR_LED_PROTOCOL_DATA_LEN)) {
        printf("first time start device,reset color led data \n");
        memset((u8 *)&color_led_cur, 0x00, sizeof(color_led_cur));
        color_led_cur.open_status = OPEN_STATUS_SETTING;
        color_led_cur.mode = MODE_USER;
        color_led_cur.red = 64;
        color_led_cur.green = 191;
        color_led_cur.blue = 191;
        color_led_cur.hue = 180;
        color_led_cur.saturation = 50;
        color_led_cur.lightness = 50;
        color_led_set_api(COLOR_LED_MODE_RAINBOW + color_led_cur.user_mode, 0, 0);
        syscfg_write(VM_COLOR_LED_SETTING, (u8 *)&color_led_cur, COLOR_LED_PROTOCOL_DATA_LEN);
        color_led = color_led_cur;
    }

    set_color_led_setting((u8 *)&color_led_cur);
    deal_color_led_setting(NULL, 0, 0);

    return 0;
}

static SMARTBOX_SETTING_OPT adv_color_led_opt = {
    .data_len = COLOR_LED_PROTOCOL_DATA_LEN,
    .setting_type = ATTR_TYPE_COLOR_LED_SETTING,
    .syscfg_id = VM_COLOR_LED_SETTING,
    .deal_opt_setting = deal_color_led_setting,
    .set_setting = set_color_led_setting,
    .get_setting = get_color_led_setting,
    .custom_setting_init = smartbox_color_led_init,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = NULL,
    .custom_setting_release = NULL,
};
REGISTER_APP_SETTING_OPT(adv_color_led_opt);

#else

u32 get_color_led_setting_data_len(void)
{
    return 0;
}
#endif
#endif
