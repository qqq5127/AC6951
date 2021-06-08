#include "app_config.h"
#include "syscfg_id.h"

#include "smartbox_misc_setting.h"
#include "le_smartbox_module.h"

#if (SMART_BOX_EN && SMARTBOX_DRC_VAL_SETTING && TCFG_AUDIO_OUT_EQ_ENABLE && TCFG_DRC_ENABLE && TCFG_AUDIO_OUT_DRC_ENABLE && RCSP_ADV_EQ_SET_ENABLE)
#include "audio_dec.h"

extern int high_bass_drc_set_filter_info(int th);

static s16 drc_val = 0;

static int drc_setting_set(u8 *misc_setting, u8 is_conversion)
{
    u32 offset = 0;
    if (is_conversion) {
        drc_val = misc_setting[offset++] << 8 | misc_setting[offset++];
        drc_val = drc_val < 0 ? (drc_val + 61) : drc_val;
    } else {
        memcpy((u8 *)&drc_val, misc_setting, sizeof(drc_val));
        offset += 2;
    }
    return offset;
}

static int drc_setting_get(u8 *misc_setting, u8 is_conversion)
{
    u32 offset = 0;
    if (is_conversion) {
        drc_val = drc_val > 0 ? (drc_val - 61) : drc_val;
        misc_setting[offset++] = (drc_val >> 8) & 0xFF;
        misc_setting[offset++] = (drc_val) & 0xFF;
    } else {
        memcpy(misc_setting, (u8 *)&drc_val, sizeof(drc_val));
        offset += 2;
    }
    return offset;
}

static int drc_state_update(u8 *misc_setting)
{
    // 值不相同才设置
    static s16 prev_drc_val = -1;
    if (-1 == prev_drc_val || prev_drc_val != drc_val) {
        high_bass_drc_set_filter_info(drc_val > 0 ? drc_val - 61 : drc_val);
        prev_drc_val = drc_val;
    }
    return 0;
}

static SMARTBOX_MISC_SETTING_OPT drc_setting_opt = {
    .misc_data_len = 2,
    .misc_setting_type = MISC_SETTING_DRC_VAL,
    .misc_syscfg_id = CFG_RCSP_MISC_DRC_SETTING,
    .misc_set_setting = drc_setting_set,
    .misc_get_setting = drc_setting_get,
    .misc_state_update = drc_state_update,
    .misc_custom_setting_init = NULL,
    .misc_custom_write_vm = NULL,
};
REGISTER_APP_MISC_SETTING_OPT(drc_setting_opt);

#endif
