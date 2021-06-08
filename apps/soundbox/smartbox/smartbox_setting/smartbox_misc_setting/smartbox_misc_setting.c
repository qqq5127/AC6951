#include "smartbox/config.h"
#include "syscfg_id.h"
#include "smartbox_misc_setting.h"

#include "smartbox_setting_sync.h"
#include "smartbox_setting_opt.h"
#include "le_smartbox_module.h"

#include "smartbox/function.h"
#if (SMART_BOX_EN)

extern int get_bt_tws_connect_status();

static int misc_setting_init(void);
static int get_misc_setting_info(u8 *misc_setting);
static int set_misc_setting_extra_handle(void *setting_data, void *param);
static void set_misc_setting_info(u8 *misc_setting);
static void deal_misc_setting(u8 *misc_setting, u8 write_vm, u8 tws_sync);
static int smartbox_misc_key_event_deal(int event, void *param);
static int smartbox_misc_setting_release(void);

static SMARTBOX_SETTING_OPT misc_setting_opt = {
    .data_len = 0,
    .setting_type = ATTR_TYPE_MISC_SETTING,
    .deal_opt_setting = deal_misc_setting,
    .set_setting = set_misc_setting_info,
    .get_setting = get_misc_setting_info,
    .custom_setting_init = misc_setting_init,
    .set_setting_extra_handle = set_misc_setting_extra_handle,
    .custom_setting_key_event_update = smartbox_misc_key_event_deal,
    .custom_setting_release = smartbox_misc_setting_release,
};
REGISTER_APP_SETTING_OPT(misc_setting_opt);

static SMARTBOX_MISC_SETTING_OPT *g_misc_opt_link_head = NULL;
static u32 g_misc_setting_update_type = -1;

int register_smartbox_setting_misc_setting(void *misc_setting)
{
    // 需要排序
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    SMARTBOX_MISC_SETTING_OPT *item = (SMARTBOX_SETTING_OPT *)misc_setting;
    SMARTBOX_MISC_SETTING_OPT *existed_item = NULL;
    while (misc_opt && item) {
        if (misc_opt->misc_setting_type == item->misc_setting_type) {
            return 0;
        } else if (item->misc_setting_type > misc_opt->misc_setting_type) {
            existed_item = misc_opt;
        }
        misc_opt = misc_opt->next;
    }

    if (!g_misc_opt_link_head) {
        item->next = g_misc_opt_link_head;
        g_misc_opt_link_head = item;
        return 0;
    } else if (item && existed_item) {
        item->next = existed_item->next;
        existed_item->next = item;
        return 0;
    }

    return -1;
}

static int misc_setting_init(void)
{
    u32 total_len = sizeof(u32); // 1个mask的长度
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        u8 *data = zalloc(misc_opt->misc_data_len);
        if (misc_opt->misc_custom_setting_init) {
            misc_opt->misc_custom_setting_init();
        } else if (smartbox_read_data_from_vm(misc_opt->misc_syscfg_id, data, misc_opt->misc_data_len)) {
            if (misc_opt->misc_set_setting) {
                misc_opt->misc_set_setting(data, 0);
            }
        }
        if (data) {
            free(data);
        }
        total_len += misc_opt->misc_data_len;
        misc_opt = misc_opt->next;
    }
    if (g_misc_opt_link_head && g_misc_opt_link_head != misc_opt) {
        misc_setting_opt.data_len = total_len;
        deal_misc_setting(NULL, 0, 0);
    }
    return 0;
}

static int get_misc_setting_info(u8 *misc_setting)
{
    u32 mask = 0;
    u32 offset = sizeof(mask);
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    u32 update_mics_setting_type = g_misc_setting_update_type;
    g_misc_setting_update_type = -1;
    while (misc_opt) {
        if (update_mics_setting_type & BIT(misc_opt->misc_setting_type)) {
            if (misc_opt->misc_get_setting) {
                misc_opt->misc_get_setting(misc_setting + offset, 1);
            }
            offset += misc_opt->misc_data_len;
            mask |= BIT(misc_opt->misc_setting_type);
        }
        misc_opt = misc_opt->next;
    }

    misc_setting[0] = (mask >> 24) & 0xFF;
    misc_setting[1] = (mask >> 16) & 0xFF;
    misc_setting[2] = (mask >> 8) & 0xFF;
    misc_setting[3] = (mask) & 0xFF;
    return offset;
}

static int set_misc_setting_extra_handle(void *setting_data, void *param)
{
    u8 *misc_setting = (u8 *)setting_data;
    u32 mask = (misc_setting[0] << 24 | misc_setting[1] << 16 | misc_setting[2] << 8 | misc_setting[3]);
    if (0 == mask) {
        return -1;
    }
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    u32 update_mics_setting_type = 0;
    while (misc_opt) {
        if (mask & BIT(misc_opt->misc_setting_type) && misc_opt->be_notify_app) {
            update_mics_setting_type |= BIT(misc_opt->misc_setting_type);
        }
        misc_opt = misc_opt->next;
    }
    if (!param && update_mics_setting_type) {
        g_misc_setting_update_type = update_mics_setting_type;
    }
    return 0;
}

static u32 g_set_mask = -1;
static void set_misc_setting_info(u8 *misc_setting)
{
    u32 offset = 0;
    g_set_mask = (misc_setting[offset++] << 24 | misc_setting[offset++] << 16 | misc_setting[offset++] << 8 | misc_setting[offset++]);
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        if (g_set_mask & BIT(misc_opt->misc_setting_type)) {
            if (misc_opt->misc_set_setting) {
                misc_opt->misc_set_setting(misc_setting + offset, 1);
            }
            offset += misc_opt->misc_data_len;
        }
        misc_opt = misc_opt->next;
    }
}

static int write_misc_setting_vm_value(int syscfg_id, u8 *data, u32 data_len)
{
    if (!data_len) {
        goto end;
    }
    u8 *vm_data = zalloc(data_len);
    syscfg_read(syscfg_id, vm_data, data_len);
    if (memcmp(vm_data, data, data_len)) {
        syscfg_write(syscfg_id, data, data_len);
    }
    if (vm_data) {
        free(vm_data);
    }
end:
    return 0;
}

static void update_misc_setting_vm_value(u8 *misc_setting)
{
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        if (g_set_mask & BIT(misc_opt->misc_setting_type)) {
            if (misc_opt->misc_custom_write_vm) {
                misc_opt->misc_custom_write_vm(misc_setting);
            } else if (misc_opt->misc_syscfg_id && misc_opt->misc_get_setting) {
                u8 *data = zalloc(misc_opt->misc_data_len);
                misc_opt->misc_get_setting(data, 0);
                write_misc_setting_vm_value(misc_opt->misc_syscfg_id, data, misc_opt->misc_data_len);
                if (data) {
                    free(data);
                }
            }
        }
        misc_opt = misc_opt->next;
    }
}

static void misc_setting_sync(u8 *misc_setting)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_smartbox_setting(ATTR_TYPE_MISC_SETTING);
    }
#endif
}

static void misc_setting_state_update(u8 *misc_setting)
{
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        if (g_set_mask & BIT(misc_opt->misc_setting_type)) {
            if (misc_opt->misc_state_update) {
                misc_opt->misc_state_update(misc_setting);
            }
        }
        misc_opt = misc_opt->next;
    }
}

static void deal_misc_setting(u8 *misc_setting, u8 write_vm, u8 tws_sync)
{
    if (misc_setting) {
        set_misc_setting_info(misc_setting);
    }
    if (write_vm) {
        update_misc_setting_vm_value(misc_setting);
    }
    if (tws_sync) {
        misc_setting_sync(misc_setting);
    }
    misc_setting_state_update(misc_setting);
}

static int smartbox_misc_key_event_deal(int event, void *param)
{
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        if (misc_opt->misc_custom_key_event_callback_deal && misc_opt->misc_custom_key_event_callback_deal(event, param)) {
            if (misc_opt->be_notify_app) {
                SMARTBOX_UPDATE(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_MISC_SETTING_INFO));
            }
            return true;
        }
        misc_opt = misc_opt->next;
    }
    g_misc_setting_update_type = -1;
    return false;
}

static int smartbox_misc_setting_release(void)
{
    SMARTBOX_MISC_SETTING_OPT *misc_opt = g_misc_opt_link_head;
    while (misc_opt) {
        if (misc_opt->misc_custom_setting_release) {
            misc_opt->misc_custom_setting_release();
        }
        misc_opt = misc_opt->next;
    }
    return 0;
}

u32 get_misc_setting_data_len(void)
{
    return misc_setting_opt.data_len;
}

#else

u32 get_misc_setting_data_len(void)
{
    return 0;
}

#endif
