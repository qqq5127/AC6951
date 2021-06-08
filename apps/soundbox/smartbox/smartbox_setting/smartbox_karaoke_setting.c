#include "smartbox/config.h"
#include "syscfg_id.h"
#include "le_smartbox_module.h"

#include "smartbox_setting_sync.h"
#include "smartbox_setting_opt.h"
#include "smartbox/function.h"

#if (SOUNDCARD_ENABLE && SMART_BOX_EN && RCSP_ADV_KARAOKE_SET_ENABLE)

#include "key_event_deal.h"
#include "mic_effect.h"
#include "JL_rcsp_packet.h"
#include "soundcard/soundcard.h"

// mask : 8byte
// 音效 : index(1byte)  + value(2byte)
// 气氛 : index(1byte)  + value(2byte)
// 参数 : [index(1byte)  + value(2byte)] * 7
#define KARAOKE_SETTING_SIZE	(8 + ((1 + 2) * 2) + ((1 + 2) * 7))
#define KARAOKE_SETTING_ATMOSPHERE_TYPE_SET		0

enum karaoke_setting_index {
    // 音效
    KARAOKE_SETTING_SOUND_EFFECT_NORMAL = 0,
    KARAOKE_SETTING_SOUND_EFFECT_BOY = 1,
    KARAOKE_SETTING_SOUND_EFFECT_GRIL = 2,
    KARAOKE_SETTING_SOUND_EFFECT_KIDS = 3,
    KARAOKE_SETTING_SOUND_EFFECT_MAGIC = 4,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_A_MAJOR = 5,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_Ashop_MAJOR = 6,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_B_MAJOR = 7,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_C_MAJOR = 8,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_Cshop_MAJOR = 9,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_D_MAJOR = 10,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_Dshop_MAJOR = 11,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_E_MAJOR = 12,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_F_MAJOR = 13,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_Fshop_MAJOR = 14,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_G_MAJOR = 15,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_Gshop_MAJOR = 16,
    // 气氛
    KARAOKE_SETTING_ATMOSPHERE_CHEERS = 17,
    KARAOKE_SETTING_ATMOSPHERE_EMBARRASSED = 18,
    KARAOKE_SETTING_ATMOSPHERE_GUNFIRE = 19,
    KARAOKE_SETTING_ATMOSPHERE_DESPISE = 20,
    KARAOKE_SETTING_ATMOSPHERE_OPEN = 21,
    KARAOKE_SETTING_ATMOSPHERE_KISS = 22,
    KARAOKE_SETTING_ATMOSPHERE_LAUGHTER = 23,
    KARAOKE_SETTING_ATMOSPHERE_APPLAUSE = 24,
    KARAOKE_SETTING_ATMOSPHERE_PAY_ATTENTION_TO = 25,
    KARAOKE_SETTING_ATMOSPHERE_MUA = 26,
    KARAOKE_SETTING_ATMOSPHERE_A_THIEF = 27,
    KARAOKE_SETTING_ATMOSPHERE_IF_YOU_ARE_THE_ONE = 28,
    // 参数
    KARAOKE_SETTING_MIC_PARAM_MIC_VOL = 29,
    KARAOKE_SETTING_MIC_PARAM_RECORD_VOL = 30,
    KARAOKE_SETTING_MIC_PARAM_REVERBERATION = 31,
    KARAOKE_SETTING_MIC_PARAM_HIGH = 32,
    KARAOKE_SETTING_MIC_PARAM_BASS = 33,
    KARAOKE_SETTING_MIC_PARAM_ACCOMPANIMENT_VOL = 34,
    KARAOKE_SETTING_MIC_PARAM_MONITOR_VOL = 35,
    // 其他
    KARAOKE_SETTING_OTHER_EFFECT_POPPING = 36,
    KARAOKE_SETTING_OTHER_EFFECT_SHOUT = 37,
    KARAOKE_SETTING_OTHER_EFFECT_DODGE = 38,

    KARAOKE_SETTING_INDEX_END,
};

enum {
    APP_PITCH_NONE = 0,
    APP_PITCH_BOY,
    APP_PITCH_GIRL,
    APP_PITCH_KIDS,
    APP_PITCH_MAGIC,
    /* 电音 */
    APP_SOUNDCARD_MODE_ELECTRIC_A_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_Ashop_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_B_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_C_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_Cshop_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_D_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_Dshop_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_E_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_F_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_Fshop_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_G_MAJOR,
    APP_SOUNDCARD_MODE_ELECTRIC_Gshop_MAJOR,

    APP_SOUNDCARD_MODE_BOOM, // 爆音
    APP_SOUNDCARD_MODE_SHOUTING_WHEAT, // 喊麦
    APP_SOUNDCARD_MODE_DODGE, // 闪避
};

const static u8 g_mutex_type_table[] = {
    KARAOKE_SETTING_SOUND_EFFECT_NORMAL,
    KARAOKE_SETTING_SOUND_EFFECT_BOY,
    KARAOKE_SETTING_SOUND_EFFECT_GRIL,
    KARAOKE_SETTING_SOUND_EFFECT_KIDS,
    KARAOKE_SETTING_SOUND_EFFECT_MAGIC,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_A_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_Ashop_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_B_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_C_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_Cshop_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_D_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_Dshop_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_E_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_F_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_Fshop_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_G_MAJOR,
    KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_Gshop_MAJOR,
    KARAOKE_SETTING_OTHER_EFFECT_POPPING,
    KARAOKE_SETTING_OTHER_EFFECT_SHOUT
};

const static u8 g_atmosphere_type_table[] = {
    KARAOKE_SETTING_ATMOSPHERE_CHEERS,
    KARAOKE_SETTING_ATMOSPHERE_EMBARRASSED,
    KARAOKE_SETTING_ATMOSPHERE_GUNFIRE,
    KARAOKE_SETTING_ATMOSPHERE_DESPISE,
    KARAOKE_SETTING_ATMOSPHERE_OPEN,
    KARAOKE_SETTING_ATMOSPHERE_KISS,
    KARAOKE_SETTING_ATMOSPHERE_LAUGHTER,
    KARAOKE_SETTING_ATMOSPHERE_APPLAUSE,
    KARAOKE_SETTING_ATMOSPHERE_PAY_ATTENTION_TO,
    KARAOKE_SETTING_ATMOSPHERE_MUA,
    KARAOKE_SETTING_ATMOSPHERE_A_THIEF,
    KARAOKE_SETTING_ATMOSPHERE_IF_YOU_ARE_THE_ONE
};

const static u8 g_common_type_table[] = {
    KARAOKE_SETTING_OTHER_EFFECT_DODGE
};

const static u8 g_value_type_table[] = {
    KARAOKE_SETTING_MIC_PARAM_MIC_VOL,
    KARAOKE_SETTING_MIC_PARAM_RECORD_VOL,
    KARAOKE_SETTING_MIC_PARAM_REVERBERATION,
    KARAOKE_SETTING_MIC_PARAM_HIGH,
    KARAOKE_SETTING_MIC_PARAM_BASS,
    KARAOKE_SETTING_MIC_PARAM_ACCOMPANIMENT_VOL,
    KARAOKE_SETTING_MIC_PARAM_MONITOR_VOL
};

#pragma pack(1)
typedef struct {
    u8 mask[8];
    u8 mic_vol_index;
    u16 mic_vol_value;
    u8 mic_record_index;
    u16 mic_record_value;
    u8 mic_reververation_index;
    u16 mic_reververation_value;
    u8 mic_high_index;
    u16 mic_high_value;
    u8 mic_low_index;
    u16 mic_low_value;
    u8 mic_accompaniment_index;
    u16 mic_accompaniment_value;
    u8 mic_moitor_index;
    u16 mic_moitor_value;
} karaoke_setting;
#pragma pack()

static u8 g_update_index_record = -1;
static karaoke_setting g_karaoke_setting = {0};

extern u8 get_curr_soundcard_pitch_by_value(void);
extern u8 get_curr_soundcard_pitch(void);
extern u8 curr_soundcard_mode_is_normal(void);

static void karaoke_setting_mask_set_bit(u8 *mask, u8 index, u8 type)
{
    if (type) {
        mask[index / 8] |= BIT(index % 8);
    } else {
        mask[index / 8] &= ~BIT(index % 8);
    }
}

static int karaoke_setting_setting_init(void)
{
    // 不读VM
    // 需要从接口获取初始值并填充
    // mic_vol_index
    g_karaoke_setting.mic_vol_index = KARAOKE_SETTING_MIC_PARAM_MIC_VOL;
    g_karaoke_setting.mic_vol_value = soundcard_get_mic_vol();

    // mic_record_index
    g_karaoke_setting.mic_record_index = KARAOKE_SETTING_MIC_PARAM_RECORD_VOL;
    g_karaoke_setting.mic_record_value = soundcard_get_rec_vol();

    // mic_reververation_index
    g_karaoke_setting.mic_reververation_index = KARAOKE_SETTING_MIC_PARAM_REVERBERATION;
    g_karaoke_setting.mic_reververation_value = soundcard_get_reverb_wet_vol();

    // mic_high_index
    g_karaoke_setting.mic_high_index = KARAOKE_SETTING_MIC_PARAM_HIGH;
    g_karaoke_setting.mic_high_value = soundcard_get_high_sound_vol();

    // mic_low_index
    g_karaoke_setting.mic_low_index = KARAOKE_SETTING_MIC_PARAM_BASS;
    g_karaoke_setting.mic_low_value = soundcard_get_low_sound_vol();

    // mic_accompaniment_index
    g_karaoke_setting.mic_accompaniment_index = KARAOKE_SETTING_MIC_PARAM_ACCOMPANIMENT_VOL;
    g_karaoke_setting.mic_accompaniment_value = soundcard_get_music_vol();

    // mic_moitor_index
    g_karaoke_setting.mic_moitor_index = KARAOKE_SETTING_MIC_PARAM_MONITOR_VOL;
    g_karaoke_setting.mic_moitor_value = soundcard_get_earphone_vol();

    // effect_index
    karaoke_setting_mask_set_bit(g_karaoke_setting.mask, KARAOKE_SETTING_SOUND_EFFECT_NORMAL, 1);

    // atomsphere_index
#if KARAOKE_SETTING_ATMOSPHERE_TYPE_SET
    karaoke_setting_mask_set_bit(g_karaoke_setting.mask, KARAOKE_SETTING_ATMOSPHERE_CHEERS, 1);
#endif

    return 0;
}

static void set_karaoke_setting_setting(u8 *karaoke_setting)
{
    memcpy(&g_karaoke_setting, karaoke_setting, sizeof(g_karaoke_setting));
}

static int get_karaoke_setting_setting(u8 *karaoke_setting)
{
    memcpy(karaoke_setting, &g_karaoke_setting, sizeof(g_karaoke_setting));
    return sizeof(g_karaoke_setting);
}

static int karaoke_setting_set_value(u8 *data, u8 index, u16 value)
{
    int offset = 0;
    data[offset++] = index;
    data[offset++] = ((u8 *)&value)[1];
    data[offset++] = ((u8 *)&value)[0];
    return offset;
}

static int karaoke_get_setting_extra_handle(void *karaoke_setting, void *param)
{
    // 全局变量g_karaoke_setting已经把值都存好，只需要进行大小端转化即可
    int offset = 0;
    u8 *data = (u8 *)karaoke_setting;
    for (; offset < sizeof(g_karaoke_setting.mask); offset ++) {
        data[sizeof(g_karaoke_setting.mask) - offset - 1] = g_karaoke_setting.mask[offset];
    }

    if ((u8) - 1 == g_update_index_record) {
        // mic_vol_value
        offset += karaoke_setting_set_value(data + offset, g_karaoke_setting.mic_vol_index, g_karaoke_setting.mic_vol_value);
        // mic_record_value
        offset += karaoke_setting_set_value(data + offset, g_karaoke_setting.mic_record_index, g_karaoke_setting.mic_record_value);
        // mic_reververation_value
        offset += karaoke_setting_set_value(data + offset, g_karaoke_setting.mic_reververation_index, g_karaoke_setting.mic_reververation_value);
        // mic_high_value
        offset += karaoke_setting_set_value(data + offset, g_karaoke_setting.mic_high_index, g_karaoke_setting.mic_high_value);
        // mic_low_value
        offset += karaoke_setting_set_value(data + offset, g_karaoke_setting.mic_low_index, g_karaoke_setting.mic_low_value);
        // mic_accompaniment_value
        offset += karaoke_setting_set_value(data + offset, g_karaoke_setting.mic_accompaniment_index, g_karaoke_setting.mic_accompaniment_value);
        // mic_moitor_value
        offset += karaoke_setting_set_value(data + offset, g_karaoke_setting.mic_moitor_index, g_karaoke_setting.mic_moitor_value);
    } else {
        switch (g_update_index_record) {
        case KARAOKE_SETTING_MIC_PARAM_MIC_VOL:
            offset += karaoke_setting_set_value(data + offset, g_update_index_record, g_karaoke_setting.mic_vol_value);
            break;
        case KARAOKE_SETTING_MIC_PARAM_RECORD_VOL:
            offset += karaoke_setting_set_value(data + offset, g_update_index_record, g_karaoke_setting.mic_record_value);
            break;
        case KARAOKE_SETTING_MIC_PARAM_REVERBERATION:
            offset += karaoke_setting_set_value(data + offset, g_update_index_record, g_karaoke_setting.mic_reververation_value);
            break;
        case KARAOKE_SETTING_MIC_PARAM_HIGH:
            offset += karaoke_setting_set_value(data + offset, g_update_index_record, g_karaoke_setting.mic_high_value);
            break;
        case KARAOKE_SETTING_MIC_PARAM_BASS:
            offset += karaoke_setting_set_value(data + offset, g_update_index_record, g_karaoke_setting.mic_low_value);
            break;
        case KARAOKE_SETTING_MIC_PARAM_ACCOMPANIMENT_VOL:
            offset += karaoke_setting_set_value(data + offset, g_update_index_record, g_karaoke_setting.mic_accompaniment_value);
            break;
        case KARAOKE_SETTING_MIC_PARAM_MONITOR_VOL:
            offset += karaoke_setting_set_value(data + offset, g_update_index_record, g_karaoke_setting.mic_moitor_value);
            break;
        }
        g_update_index_record = -1;
    }
    return offset;
}

static bool karaoke_setting_table_check(u8 index, u8 *table, u8 table_len)
{
    for (u8 i = 0; i < table_len; i++) {
        if (index == table[i]) {
            return true;
        }
    }
    return false;
}

static bool karaoke_setting_mask_set(u8 index, u8 *table, u8 table_len)
{
    if (karaoke_setting_table_check(index, table, table_len)) {
        for (u8 i = 0; i < table_len; i++) {
            karaoke_setting_mask_set_bit(g_karaoke_setting.mask, table[i], 0);
        }
        karaoke_setting_mask_set_bit(g_karaoke_setting.mask, index, 1);
        return true;
    }
    return false;
}

static int karaoke_set_setting_extra_handle(void *karaoke_setting, void *param)
{
    u16 data_len = *((u16 *) param) - 1;
    u8 *data = (u8 *) karaoke_setting;
    u8 i = 0;
    while (i < data_len) {
        u8 index = data[i++];
        u16 value = data[i++] << 8 | data[i++];
        karaoke_setting_mask_set(index, g_mutex_type_table, sizeof(g_mutex_type_table));
#if KARAOKE_SETTING_ATMOSPHERE_TYPE_SET
        karaoke_setting_mask_set(index, g_atmosphere_type_table, sizeof(g_atmosphere_type_table));
#endif
        if (karaoke_setting_table_check(index, g_common_type_table, sizeof(g_common_type_table))) {
            karaoke_setting_mask_set_bit(g_karaoke_setting.mask, index, 1);
        }
        karaoke_setting_mask_set(index, g_value_type_table, sizeof(g_value_type_table));
        g_update_index_record = index;
        switch (index) {
        case KARAOKE_SETTING_MIC_PARAM_MIC_VOL:
            g_karaoke_setting.mic_vol_index = index;
            g_karaoke_setting.mic_vol_value = value;
            break;
        case KARAOKE_SETTING_MIC_PARAM_RECORD_VOL:
            g_karaoke_setting.mic_record_index = index;
            g_karaoke_setting.mic_record_value = value;
            break;
        case KARAOKE_SETTING_MIC_PARAM_REVERBERATION:
            g_karaoke_setting.mic_reververation_index = index;
            g_karaoke_setting.mic_reververation_value = value;
            break;
        case KARAOKE_SETTING_MIC_PARAM_HIGH:
            g_karaoke_setting.mic_high_index = index;
            g_karaoke_setting.mic_high_value = value;
            break;
        case KARAOKE_SETTING_MIC_PARAM_BASS:
            g_karaoke_setting.mic_low_index = index;
            g_karaoke_setting.mic_low_value = value;
            break;
        case KARAOKE_SETTING_MIC_PARAM_ACCOMPANIMENT_VOL:
            g_karaoke_setting.mic_accompaniment_index = index;
            g_karaoke_setting.mic_accompaniment_value = value;
            break;
        case KARAOKE_SETTING_MIC_PARAM_MONITOR_VOL:
            g_karaoke_setting.mic_moitor_index = index;
            g_karaoke_setting.mic_moitor_value = value;
            break;
        }
    }
    return 0;
}

static void update_karaoke_setting_vm_value(u8 *karaoke_setting)
{
    // 不写VM
}

static void karaoke_setting_sync(u8 *color_led_setting_info)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_smartbox_setting(ATTR_TYPE_KARAOKE_SETTING);
    }
#endif
}

static void karaoke_setting_update(void)
{
    switch (g_update_index_record) {
    case KARAOKE_SETTING_MIC_PARAM_MIC_VOL:
        app_task_put_key_msg(KEY_SOUNDCARD_SLIDE_MIC, g_karaoke_setting.mic_vol_value);
        break;
    case KARAOKE_SETTING_MIC_PARAM_RECORD_VOL:
        app_task_put_key_msg(KEY_SOUNDCARD_SLIDE_RECORD_VOL, g_karaoke_setting.mic_record_value);
        break;
    case KARAOKE_SETTING_MIC_PARAM_REVERBERATION:
        app_task_put_key_msg(KEY_SOUNDCARD_SLIDE_WET_GAIN, g_karaoke_setting.mic_reververation_value);
        break;
    case KARAOKE_SETTING_MIC_PARAM_HIGH:
        app_task_put_key_msg(KEY_SOUNDCARD_SLIDE_HIGH_SOUND, g_karaoke_setting.mic_high_value);
        break;
    case KARAOKE_SETTING_MIC_PARAM_BASS:
        app_task_put_key_msg(KEY_SOUNDCARD_SLIDE_LOW_SOUND, g_karaoke_setting.mic_low_value);
        break;
    case KARAOKE_SETTING_MIC_PARAM_ACCOMPANIMENT_VOL:
        app_task_put_key_msg(KEY_SOUNDCARD_SLIDE_MUSIC_VOL, g_karaoke_setting.mic_accompaniment_value);
        break;
    case KARAOKE_SETTING_MIC_PARAM_MONITOR_VOL:
        app_task_put_key_msg(KEY_SOUNDCARD_SLIDE_EARPHONE_VOL, g_karaoke_setting.mic_moitor_value);
        break;
    case KARAOKE_SETTING_OTHER_EFFECT_POPPING:
        app_task_put_key_msg(KEY_SOUNDCARD_MODE_BOOM, 0);
        break;
    case KARAOKE_SETTING_SOUND_EFFECT_MAGIC:
        app_task_put_key_msg(KEY_SOUNDCARD_MODE_MAGIC, 0);
        break;
    case KARAOKE_SETTING_OTHER_EFFECT_SHOUT:
        app_task_put_key_msg(KEY_SOUNDCARD_MODE_SHOUTING_WHEAT, 0);
        break;
    case KARAOKE_SETTING_OTHER_EFFECT_DODGE:
        app_task_put_key_msg(KEY_SOUNDCARD_MODE_DODGE, 0);
        break;
    default:
        if (karaoke_setting_table_check(g_update_index_record, g_mutex_type_table, sizeof(g_mutex_type_table))) {
            app_task_put_key_msg(KEY_SOUNDCARD_MODE_PITCH_BY_VALUE, g_update_index_record);
            break;
        } else if (karaoke_setting_table_check(g_update_index_record, g_atmosphere_type_table, sizeof(g_atmosphere_type_table))) {
            app_task_put_key_msg(KEY_SOUNDCARD_MAKE_NOISE0 + g_update_index_record - KARAOKE_SETTING_ATMOSPHERE_CHEERS, 0);
            break;
        }
        break;
    }
    /* g_update_index_record = -1; */

}


static void deal_karaoke_setting(u8 *karaoke_setting, u8 write_vm, u8 tws_sync)
{
    if (karaoke_setting) {
        set_karaoke_setting_setting(karaoke_setting);
    }
    if (write_vm) {
        update_karaoke_setting_vm_value(karaoke_setting);
    }
    if (tws_sync) {
        karaoke_setting_sync(karaoke_setting);
    }
    karaoke_setting_update();
}

static int karaoke_setting_key_event_update(int event, void *param)
{
    int ret = false;
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL || 0 == JL_rcsp_get_auth_flag()) {
        return ret;
    }
    switch (event) {
    case KEY_SOUNDCARD_MAKE_NOISE0:
    case KEY_SOUNDCARD_MAKE_NOISE1:
    case KEY_SOUNDCARD_MAKE_NOISE2:
    case KEY_SOUNDCARD_MAKE_NOISE3:
    case KEY_SOUNDCARD_MAKE_NOISE4:
    case KEY_SOUNDCARD_MAKE_NOISE5:
    case KEY_SOUNDCARD_MAKE_NOISE6:
    case KEY_SOUNDCARD_MAKE_NOISE7:
    case KEY_SOUNDCARD_MAKE_NOISE8:
    case KEY_SOUNDCARD_MAKE_NOISE9:
    case KEY_SOUNDCARD_MAKE_NOISE10:
    case KEY_SOUNDCARD_MAKE_NOISE11:
        g_update_index_record = KARAOKE_SETTING_ATMOSPHERE_CHEERS + event - KEY_SOUNDCARD_MAKE_NOISE0;
#if KARAOKE_SETTING_ATMOSPHERE_TYPE_SET
        karaoke_setting_mask_set(g_update_index_record, g_atmosphere_type_table, sizeof(g_atmosphere_type_table));
#endif
        ret = true;
        break;
    case KEY_SOUNDCARD_MODE_PITCH:
        g_update_index_record = get_curr_soundcard_pitch();
        karaoke_setting_mask_set(g_update_index_record, g_mutex_type_table, sizeof(g_mutex_type_table));
        ret = true;
        break;
    case KEY_SOUNDCARD_MODE_PITCH_BY_VALUE:
        g_update_index_record = get_curr_soundcard_pitch_by_value();
        switch (g_update_index_record) {
        case APP_SOUNDCARD_MODE_BOOM:
            karaoke_setting_mask_set(KARAOKE_SETTING_OTHER_EFFECT_POPPING, g_mutex_type_table, sizeof(g_mutex_type_table));
            break;
        case APP_SOUNDCARD_MODE_SHOUTING_WHEAT:
            karaoke_setting_mask_set(KARAOKE_SETTING_OTHER_EFFECT_SHOUT, g_mutex_type_table, sizeof(g_mutex_type_table));
            break;
        default:
            if (g_update_index_record >= APP_SOUNDCARD_MODE_ELECTRIC_A_MAJOR && g_update_index_record <= APP_SOUNDCARD_MODE_ELECTRIC_Gshop_MAJOR) {
                karaoke_setting_mask_set(g_update_index_record - APP_SOUNDCARD_MODE_ELECTRIC_A_MAJOR + KARAOKE_SETTING_SOUND_EFFECT_ELECTRIC_A_MAJOR, g_mutex_type_table, sizeof(g_mutex_type_table));
                break;
            }
            karaoke_setting_mask_set(g_update_index_record, g_mutex_type_table, sizeof(g_mutex_type_table));
            break;
        }
        ret = true;
        break;
    case KEY_SOUNDCARD_MODE_BOOM:
        g_update_index_record = KARAOKE_SETTING_OTHER_EFFECT_POPPING;
        karaoke_setting_mask_set(g_update_index_record, g_mutex_type_table, sizeof(g_mutex_type_table));
        if (curr_soundcard_mode_is_normal()) {
            karaoke_setting_mask_set_bit(g_karaoke_setting.mask, g_update_index_record, 0);
        }
        ret = true;
        break;
    case KEY_SOUNDCARD_MODE_SHOUTING_WHEAT:
        g_update_index_record = KARAOKE_SETTING_OTHER_EFFECT_SHOUT;
        karaoke_setting_mask_set(g_update_index_record, g_mutex_type_table, sizeof(g_mutex_type_table));
        if (curr_soundcard_mode_is_normal()) {
            karaoke_setting_mask_set_bit(g_karaoke_setting.mask, g_update_index_record, 0);
        }
        ret = true;
    case KEY_SOUNDCARD_MODE_DODGE:
        g_update_index_record = KARAOKE_SETTING_OTHER_EFFECT_DODGE;
        if (mic_dodge_get_status()) {
            karaoke_setting_mask_set_bit(g_karaoke_setting.mask, g_update_index_record, 1);
        } else {
            karaoke_setting_mask_set_bit(g_karaoke_setting.mask, g_update_index_record, 0);
        }
        ret = true;
        break;
    case KEY_SOUNDCARD_MODE_ELECTRIC:
        g_update_index_record = get_curr_soundcard_pitch_by_value();
        karaoke_setting_mask_set(g_update_index_record, g_mutex_type_table, sizeof(g_mutex_type_table));
        ret = true;
        break;
    case KEY_SOUNDCARD_MODE_MAGIC:
        g_update_index_record = KARAOKE_SETTING_SOUND_EFFECT_MAGIC;
        karaoke_setting_mask_set(g_update_index_record, g_mutex_type_table, sizeof(g_mutex_type_table));
        if (curr_soundcard_mode_is_normal()) {
            karaoke_setting_mask_set_bit(g_karaoke_setting.mask, g_update_index_record, 0);
        }
        ret = true;
        break;
    case KEY_SOUNDCARD_SLIDE_MIC:
        g_update_index_record = KARAOKE_SETTING_MIC_PARAM_MIC_VOL;
        g_karaoke_setting.mic_vol_value = soundcard_get_mic_vol();
        ret = true;
        break;
    case KEY_SOUNDCARD_SLIDE_LOW_SOUND:
        g_update_index_record = KARAOKE_SETTING_MIC_PARAM_BASS;
        // 更新对应全局变量
        g_karaoke_setting.mic_low_value = soundcard_get_low_sound_vol();
        ret = true;
        break;
    case KEY_SOUNDCARD_SLIDE_HIGH_SOUND:
        g_update_index_record = KARAOKE_SETTING_MIC_PARAM_HIGH;
        // 更新对应全局变量
        g_karaoke_setting.mic_high_value = soundcard_get_high_sound_vol();
        ret = true;
        break;
    case KEY_SOUNDCARD_SLIDE_WET_GAIN:
        g_update_index_record = KARAOKE_SETTING_MIC_PARAM_REVERBERATION;
        g_karaoke_setting.mic_reververation_value = soundcard_get_reverb_wet_vol();
        ret = true;
        break;
    case KEY_SOUNDCARD_SLIDE_RECORD_VOL:
        g_update_index_record = KARAOKE_SETTING_MIC_PARAM_RECORD_VOL;
        // 更新对应全局变量
        g_karaoke_setting.mic_record_value = soundcard_get_rec_vol();
        ret = true;
        break;
    case KEY_SOUNDCARD_SLIDE_MUSIC_VOL:
        g_update_index_record = KARAOKE_SETTING_MIC_PARAM_ACCOMPANIMENT_VOL;
        // 更新对应全局变量
        g_karaoke_setting.mic_accompaniment_value = soundcard_get_music_vol();
        ret = true;
        break;
    case KEY_SOUNDCARD_SLIDE_EARPHONE_VOL:
        g_update_index_record = KARAOKE_SETTING_MIC_PARAM_MONITOR_VOL;
        // 更新对应全局变量
        g_karaoke_setting.mic_moitor_value = soundcard_get_earphone_vol();
        ret = true;
        break;
    }
    if (ret) {
        SMARTBOX_UPDATE(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_KARAOKE_SETTING_INFO));
    }
    return ret;
}

static SMARTBOX_SETTING_OPT karaoke_setting_opt = {
    .data_len = KARAOKE_SETTING_SIZE,
    .setting_type = ATTR_TYPE_KARAOKE_SETTING,
    .deal_opt_setting = deal_karaoke_setting,
    .set_setting = set_karaoke_setting_setting,
    .get_setting = get_karaoke_setting_setting,
    .custom_setting_init = karaoke_setting_setting_init,
    .set_setting_extra_handle = karaoke_set_setting_extra_handle,
    .get_setting_extra_handle = karaoke_get_setting_extra_handle,
    .custom_setting_key_event_update = karaoke_setting_key_event_update,
};
REGISTER_APP_SETTING_OPT(karaoke_setting_opt);

#endif
