#include "app_config.h"
#include "syscfg_id.h"
#include "user_cfg.h"
#include "le_smartbox_module.h"

#include "smartbox_setting_sync.h"
#include "smartbox_setting_opt.h"

//#if (SMART_BOX_EN && RCSP_SMARTBOX_ADV_EN)
#if (SMART_BOX_EN)
#if RCSP_ADV_LED_SET_ENABLE
#include "asm/pwm_led.h"
#include "ui_manage.h"

enum ADV_LED_TYPE {
    ADV_LED_TYPE_ALL_OFF = 0x0,
    ADV_LED_TYPE_RED_ON,
    ADV_LED_TYPE_BLUE_ON,
    ADV_LED_TYPE_RAD_BREATHE,
    ADV_LED_TYPE_BLUE_BREATHE,
    ADV_LED_TYPE_FAST_FLASH,
    ADV_LED_TYPE_SLOW_FLASH,
};

static u8 g_led_status[14] = {
    0x01, 0x06, \
    0x02, 0x05, \
    0x03, 0x04, \
    0x04, 0x00, \
    0x05, 0x00, \
    0x06, 0x00, \
    0x07, 0x00
};

extern STATUS *get_led_config(void);
extern int get_bt_tws_connect_status();


__attribute__((weak))
u8 adv_get_led_status(void)
{
    return 1;
}

static u8 adv_led_value_conversion(u8 adv_led_opt)
{
    u8 conversion_value = adv_led_opt;
    switch (adv_led_opt) {
    case ADV_LED_TYPE_ALL_OFF:
        conversion_value = PWM_LED_ALL_OFF;
        break;
    case ADV_LED_TYPE_RED_ON:
        conversion_value = PWM_LED1_ON;
        break;
    case ADV_LED_TYPE_BLUE_ON:
        conversion_value = PWM_LED0_ON;
        break;
    case ADV_LED_TYPE_RAD_BREATHE:
        conversion_value = PWM_LED1_BREATHE;
        break;
    case ADV_LED_TYPE_BLUE_BREATHE:
        conversion_value = PWM_LED0_BREATHE;
        break;
    case ADV_LED_TYPE_FAST_FLASH:
        conversion_value = PWM_LED0_LED1_FAST_FLASH;
        break;
    case ADV_LED_TYPE_SLOW_FLASH:
        conversion_value = PWM_LED0_LED1_SLOW_FLASH;
        break;
    }
    return conversion_value;
}

static void set_led_setting(u8 *led_setting_info)
{
    g_led_status[2 * 0 + 1] = led_setting_info[0];
    g_led_status[2 * 1 + 1] = led_setting_info[1];
    g_led_status[2 * 2 + 1] = led_setting_info[2];
    g_led_status[2 * 3 + 1] = led_setting_info[3];
    g_led_status[2 * 4 + 1] = led_setting_info[4];
    g_led_status[2 * 5 + 1] = led_setting_info[5];
    g_led_status[2 * 6 + 1] = led_setting_info[6];
}

static int get_led_setting(u8 *led_setting_info)
{
    led_setting_info[0] = g_led_status[2 * 0 + 1];
    led_setting_info[1] = g_led_status[2 * 1 + 1];
    led_setting_info[2] = g_led_status[2 * 2 + 1];
    led_setting_info[3] = g_led_status[2 * 3 + 1];
    led_setting_info[4] = g_led_status[2 * 4 + 1];
    led_setting_info[5] = g_led_status[2 * 5 + 1];
    led_setting_info[6] = g_led_status[2 * 6 + 1];
    return 0;
}

// 1、写入VM
static void update_led_setting_vm_value(u8 *led_setting_info)
{
    syscfg_write(CFG_RCSP_ADV_LED_SETTING, led_setting_info, 7);
}
// 2、同步对端
static void led_setting_sync(u8 *led_setting_info)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_smartbox_setting(ATTR_TYPE_LED_SETTING);
    }
#endif
}
// 3、更新状态
void update_led_setting_state(void)
{
    u8 led_setting_info[7];
    get_led_setting(led_setting_info);

    // 直接修改led表
    LED_REMAP_STATUS *p_led = led_get_remap_t();
    // 未配对
    p_led->bt_init_ok = adv_led_value_conversion(led_setting_info[0]);
    // 未连接
    p_led->tws_connect_ok = adv_led_value_conversion(led_setting_info[1]);
    // 连接
    p_led->bt_connect_ok = adv_led_value_conversion(led_setting_info[2]);
    // 断开连接
    p_led->bt_disconnect = adv_led_value_conversion(led_setting_info[1]);
    // 音乐模式播放音乐
    p_led->music_play = adv_led_value_conversion(led_setting_info[3]);
    // 音乐模式停止播放
    p_led->music_pause = adv_led_value_conversion(led_setting_info[4]);
    // Linein模式播放音乐
    p_led->linein_play = adv_led_value_conversion(led_setting_info[5]);
    // Linein模式静音
    p_led->linein_mute = adv_led_value_conversion(led_setting_info[6]);

    switch (adv_get_led_status()) {
    case STATUS_BT_INIT_OK:
    case STATUS_BT_DISCONN:
        ui_update_status(STATUS_BT_INIT_OK);
        break;
    case STATUS_BT_TWS_CONN:
        ui_update_status(STATUS_BT_TWS_CONN);
        break;
    case STATUS_PHONE_ACTIV:
    case STATUS_PHONE_OUT:
    case STATUS_PHONE_INCOME:
    case STATUS_BT_CONN:
        ui_update_status(STATUS_BT_CONN);
        break;
    case STATUS_MUSIC_MODE:
    case STATUS_MUSIC_PLAY:
        ui_update_status(STATUS_MUSIC_PLAY);
        break;
    case STATUS_MUSIC_PAUSE:
        ui_update_status(STATUS_MUSIC_PAUSE);
        break;
    case STATUS_LINEIN_MODE:
    case STATUS_LINEIN_PLAY:
        ui_update_status(STATUS_LINEIN_PLAY);
        break;
    case STATUS_LINEIN_PAUSE:
        ui_update_status(STATUS_LINEIN_PAUSE);
        break;
    }
}

static void deal_led_setting(u8 *led_setting_info, u8 write_vm, u8 tws_sync)
{
    u8 led_setting[7];
    if (!led_setting_info) {
        get_led_setting(led_setting);
    } else {
        memcpy(led_setting, led_setting_info, 7);
        set_led_setting(led_setting_info);
    }
    if (write_vm) {
        update_led_setting_vm_value(led_setting);
    }
    if (tws_sync) {
        led_setting_sync(led_setting);
    }
    update_led_setting_state();
}

static int led_set_setting_extra_handle(void *setting_data, void *param)
{
    u8 dlen = *((u8 *)param);
    u8 *led_setting_data = (u8 *) setting_data;
    while (dlen >= 2) {
        if (led_setting_data[0] == 0 || led_setting_data[0] > 7) {
            return -1;
        } else {
            g_led_status[2 * (led_setting_data[0] - 1) + 1] = led_setting_data[1];
        }
        dlen -= 2;
        led_setting_data += 2;
    }
    return 0;
}

static int led_get_setting_extra_handle(void *setting_data, void *param)
{
    int **setting_data_ptr = (int **)setting_data;
    *setting_data_ptr = g_led_status;
    return sizeof(g_led_status);
}

static SMARTBOX_SETTING_OPT adv_led_opt = {
    .data_len = 7,
    .setting_type = ATTR_TYPE_LED_SETTING,
    .syscfg_id = CFG_RCSP_ADV_LED_SETTING,
    .deal_opt_setting = deal_led_setting,
    .set_setting = set_led_setting,
    .get_setting = get_led_setting,
    .custom_setting_init = NULL,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = NULL,
    .custom_setting_release = NULL,
    .set_setting_extra_handle = led_set_setting_extra_handle,
    .get_setting_extra_handle = led_get_setting_extra_handle,
};
REGISTER_APP_SETTING_OPT(adv_led_opt);

#endif
#endif
