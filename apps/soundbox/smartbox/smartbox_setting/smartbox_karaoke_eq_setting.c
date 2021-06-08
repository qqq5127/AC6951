#include "app_config.h"
#include "syscfg_id.h"
#include "le_smartbox_module.h"

#include "smartbox_setting_sync.h"
#include "smartbox_setting_opt.h"

#if (TCFG_EQ_ENABLE && MIC_EFFECT_EQ_EN && SMART_BOX_EN && RCSP_ADV_KARAOKE_EQ_SET_ENABLE && RCSP_ADV_KARAOKE_SET_ENABLE)

#include "effect_cfg.h"
#include "effect_tool.h"
#include "mic_effect.h"

static u8 g_karaoke_setting_info[9] = {EFFECT_EQ_SECTION_MAX, 0};

static void update_eq_setting_info(u8 *eq_info_data)
{
    // 更新卡拉ok的EQ值
    struct eq_seg_info *eq_info = NULL;
    for (u8 i = 0; i < EFFECT_EQ_SECTION_MAX; i++) {
        eq_info = mic_eq_get_info(i);
        s8 gain = g_karaoke_setting_info[i + 1];
        if (gain != (eq_info->gain >> 20)) {
            eq_info->gain = gain << 20;
            mic_eq_set_info(eq_info);
        }
    }
}

static void set_karaoke_eq_setting(u8 *eq_setting)
{
    memcpy(g_karaoke_setting_info, eq_setting, sizeof(g_karaoke_setting_info));
}

static int get_karaoke_eq_setting(u8 *eq_setting)
{
    memcpy(eq_setting, g_karaoke_setting_info, sizeof(g_karaoke_setting_info));
    return sizeof(g_karaoke_setting_info);
}

// 1、写入VM
static void update_karaoke_eq_vm_value(u8 *eq_setting)
{
    syscfg_write(VM_KARAOKE_EQ_SETTING, g_karaoke_setting_info, 9);
}

// 2、同步对端
static void karaoke_eq_sync(u8 *eq_setting)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_smartbox_setting(ATTR_TYPE_KARAOKE_EQ_SETTING);
    }
#endif
}

static void deal_karaoke_eq_setting(u8 *eq_setting, u8 write_vm, u8 tws_sync)
{
    if (eq_setting) {
        set_karaoke_eq_setting(eq_setting);
    }
    if (write_vm) {
        update_karaoke_eq_vm_value(eq_setting);
    }
    if (tws_sync) {
        karaoke_eq_sync(eq_setting);
    }
    update_eq_setting_info(eq_setting);
}

// 0 - gain
// 1 - freq
static u8 get_karaoke_eq_info(u8 *get_eq_info, u8 type)
{
    u8 i;
    u8 data_len = 1;
    struct eq_seg_info *eq_info = NULL;
    get_eq_info[0] = EFFECT_EQ_SECTION_MAX;
    for (i = 0; i < EFFECT_EQ_SECTION_MAX; i++) {
        eq_info = mic_eq_get_info(i);
        if (type) {
            u16 eq_freq = eq_info->freq;
            eq_freq = ((((u8 *)&eq_freq)[0] << 8)  + ((u8 *)&eq_freq)[1]);
            memcpy(get_eq_info + data_len, &eq_freq, sizeof(eq_freq));
            data_len += sizeof(eq_freq);
        } else {
            u8 eq_gain = (eq_info->gain >> 20) & 0xFF;
            memcpy(get_eq_info + data_len, &eq_gain, sizeof(eq_gain));
            data_len += sizeof(eq_gain);
        }
    }

    return data_len;
}

static u8 get_karaoke_eq_all_freq_info(u8 *get_eq_info)
{
    return get_karaoke_eq_info(get_eq_info, 1);
}

static u8 get_karoke_eq_all_gain_info(u8 *get_eq_info)
{
    return get_karaoke_eq_info(get_eq_info, 0);
}

static int karaoke_eq_get_setting_extra_handle(void *setting_data, void *param)
{
    u16 data_len = *((u16 *) param);
    if (data_len > sizeof(g_karaoke_setting_info)) {
        data_len = get_karaoke_eq_all_freq_info(setting_data);
    } else {
        data_len = get_karaoke_eq_setting(setting_data);
    }
    return data_len;
}

static SMARTBOX_SETTING_OPT karaoke_eq_opt = {
    .data_len = 9,
    .setting_type = ATTR_TYPE_KARAOKE_EQ_SETTING,
    .syscfg_id = VM_KARAOKE_EQ_SETTING,
    .deal_opt_setting = deal_karaoke_eq_setting,
    .set_setting = set_karaoke_eq_setting,
    .get_setting = get_karaoke_eq_setting,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = NULL,
    .custom_setting_release = NULL,
    .set_setting_extra_handle = NULL,
    .get_setting_extra_handle = karaoke_eq_get_setting_extra_handle,
};
REGISTER_APP_SETTING_OPT(karaoke_eq_opt);

#endif
