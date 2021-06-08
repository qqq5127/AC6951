#include "app_config.h"
#include "syscfg_id.h"
#include "key_event_deal.h"
#include "le_smartbox_module.h"

#include "smartbox_setting_sync.h"
#include "smartbox_setting_opt.h"

//#if (SMART_BOX_EN && RCSP_SMARTBOX_ADV_EN)
#if (SMART_BOX_EN)
#if RCSP_ADV_KEY_SET_ENABLE

extern int get_bt_tws_connect_status();
static u8 rcsp_adv_key_event_flag = 0x0;


static u8 g_key_setting[12] = {
    0x01, 0x01, 0x05, \
    0x02, 0x01, 0x05, \
    0x01, 0x02, 0x08, \
    0x02, 0x02, 0x08
};

static void enable_adv_key_event(void)
{
    rcsp_adv_key_event_flag = 1;
}

static u8 get_adv_key_event_status(void)
{
    return rcsp_adv_key_event_flag;
}

static void disable_adv_key_event(void)
{
    rcsp_adv_key_event_flag = 0;
}

static void set_key_setting(u8 *key_setting_info)
{
    g_key_setting[3 * 0 + 2] = key_setting_info[0];
    g_key_setting[3 * 2 + 2] = key_setting_info[1];
    g_key_setting[3 * 1 + 2] = key_setting_info[2];
    g_key_setting[3 * 3 + 2] = key_setting_info[3];
}

static int get_key_setting(u8 *key_setting_info)
{
    key_setting_info[0] = g_key_setting[3 * 0 + 2];
    key_setting_info[1] = g_key_setting[3 * 2 + 2];
    key_setting_info[2] = g_key_setting[3 * 1 + 2];
    key_setting_info[3] = g_key_setting[3 * 3 + 2];
    return 0;
}

static void update_key_setting_vm_value(u8 *key_setting_info)
{
    syscfg_write(CFG_RCSP_ADV_KEY_SETTING, key_setting_info, 4);
}

static void key_setting_sync(u8 *key_setting_info)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_smartbox_setting(ATTR_TYPE_KEY_SETTING);
    }
#endif
}

static void deal_key_setting(u8 *key_setting_info, u8 write_vm, u8 tws_sync)
{
    u8 key_setting[4];
    if (!key_setting_info) {
        get_key_setting(key_setting);
    } else {
        memcpy(key_setting, key_setting_info, 4);
    }
    // 1、写入VM
    if (write_vm) {
        update_key_setting_vm_value(key_setting);
    }
    // 2、同步对端
    if (tws_sync) {
        key_setting_sync(key_setting);
    }
    // 3、更新状态
    enable_adv_key_event();
}

#define ADV_POWER_ON_OFF	0
enum ADV_KEY_TYPE {
    ADV_KEY_TYPE_NULL = 0x0,
#if ADV_POWER_ON_OFF
    ADV_KEY_TYPE_POWER_ON,
    ADV_KEY_TYPE_POWER_OFF,
#endif
    ADV_KEY_TYPE_PREV = 0x3,
    ADV_KEY_TYPE_NEXT,
    ADV_KEY_TYPE_PP,
    ADV_KEY_TYPE_ANSWER_CALL,
    ADV_KEY_TYPE_HANG_UP,
    ADV_KEY_TYPE_CALL_BACK,
    ADV_KEY_TYPE_INC_VOICE,
    ADV_KEY_TYPE_DESC_VOICE,
    ADV_KEY_TYPE_TAKE_PHOTO,
};

static u8 get_adv_key_opt(u8 key_action, u8 channel)
{
    u8 opt;
    for (opt = 0; opt < sizeof(g_key_setting); opt += 3) {
        if (g_key_setting[opt] == channel &&
            g_key_setting[opt + 1] == key_action) {
            break;
        }
    }
    if (sizeof(g_key_setting) == opt) {
        return -1;
    }

    switch (g_key_setting[opt + 2]) {
    case ADV_KEY_TYPE_NULL:
        //opt = KEY_NULL;
        opt = ADV_KEY_TYPE_NULL;
        break;
#if ADV_POWER_ON_OFF
    case ADV_KEY_TYPE_POWER_ON:
        opt = KEY_POWER_ON;
        break;
    case ADV_KEY_TYPE_POWER_OFF:
        opt = KEY_RCSP_POWEROFF;
        break;
#endif
    case ADV_KEY_TYPE_PREV:
        opt = KEY_MUSIC_PREV;
        break;
    case ADV_KEY_TYPE_NEXT:
        opt = KEY_MUSIC_NEXT;
        break;
    case ADV_KEY_TYPE_PP:
        opt = KEY_MUSIC_PP;
        break;
    case ADV_KEY_TYPE_ANSWER_CALL:
        opt = KEY_CALL_ANSWER;
        break;
    case ADV_KEY_TYPE_HANG_UP:
        opt = KEY_CALL_HANG_UP;
        break;
    case ADV_KEY_TYPE_CALL_BACK:
        opt = KEY_CALL_LAST_NO;
        break;
    case ADV_KEY_TYPE_INC_VOICE:
        opt = KEY_VOL_UP;
        break;
    case ADV_KEY_TYPE_DESC_VOICE:
        opt = KEY_VOL_DOWN;
        break;
    case ADV_KEY_TYPE_TAKE_PHOTO:
        opt = KEY_HID_CONTROL;
        break;
    }
    return opt;
}

void set_key_event_by_rcsp_info(struct sys_event *event, u8 *key_event)
{
    if (0 == get_adv_key_event_status()) {
        return;
    }

    u8 key_action = 0;
    struct key_event *key = &event->u.key;
    switch (key->event) {
    case 0:
        // 单击
        key_action = 0x1;
        break;
    case 4:
        // 双击
        key_action = 0x2;
        break;
    default:
        return;
    }

#if (TCFG_CHARGESTORE_ENABLE && TCFG_USER_TWS_ENABLE)
    u8 channel = tws_api_get_local_channel();

    if (get_bt_tws_connect_status()) {
        channel = tws_api_get_local_channel();
    }
#else
    u8 channel = 'U';
#endif

    switch (channel) {
    case 'U':
    case 'L':
        channel = (u32) event->arg == KEY_EVENT_FROM_TWS ? 2 : 1;
        break;
    case 'R':
        channel = (u32) event->arg == KEY_EVENT_FROM_TWS ? 1 : 2;
        break;
    default:
        return;
    }

    key_action = get_adv_key_opt(key_action, channel);
    if ((u8) - 1 != key_action) {
        *key_event = key_action ? key_action : KEY_NULL;
    }
}

static int key_set_setting_extra_handle(void *setting_data, void *param)
{
    u8 dlen = *((u8 *)param);
    u8 *key_setting_data = (u8 *) setting_data;
    while (dlen >= 3) {
        if (key_setting_data[0] == 0x01) {
            if (key_setting_data[1] == 0x01) {
                g_key_setting[2] = key_setting_data[2];
            } else if (key_setting_data[1] == 0x02) {
                g_key_setting[8] = key_setting_data[2];
            }
        } else if (key_setting_data[0] == 0x02) {
            if (key_setting_data[1] == 0x01) {
                g_key_setting[5] = key_setting_data[2];
            } else if (key_setting_data[1] == 0x02) {
                g_key_setting[11] = key_setting_data[2];
            }
        }
        dlen -= 3;
        key_setting_data += 3;
    }
    return 0;
}

static int key_get_setting_extra_handle(void *setting_data, void *param)
{
    int **setting_data_ptr = (int **)setting_data;
    *setting_data_ptr = g_key_setting;
    return sizeof(g_key_setting);
}

static SMARTBOX_SETTING_OPT adv_key_opt = {
    .data_len = 4,
    .setting_type = ATTR_TYPE_KEY_SETTING,
    .syscfg_id = CFG_RCSP_ADV_KEY_SETTING,
    .deal_opt_setting = deal_key_setting,
    .set_setting = set_key_setting,
    .get_setting = get_key_setting,
    .custom_setting_init = NULL,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = NULL,
    .custom_setting_release = NULL,
    .set_setting_extra_handle = key_set_setting_extra_handle,
    .get_setting_extra_handle = key_get_setting_extra_handle,
};
REGISTER_APP_SETTING_OPT(adv_key_opt);

#endif
#endif
