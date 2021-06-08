#include "soundcard/soundcard.h"
#include "soundcard/lamp.h"
#include "soundcard/notice.h"
#include "soundcard/peripheral.h"
#include "audio_mic/mic_effect.h"
#include "key_event_deal.h"
#include "btstack/avctp_user.h"
#include "audio_dec.h"
#include "application/audio_output_dac.h"
#include "linein/linein.h"
#include "app_power_manage.h"
#include "app_task.h"


#if SOUNDCARD_ENABLE


struct __soundcard {
    u8 mode;
    u8 electric;
    u8 pitch;
    u8 mic_status;
    u8 status;

    u16 mic_vol;
    u16 reverb_wet_vol;
    u16 high_sound_vol;
    u16 low_sound_vol;
    u16 rec_vol;
    u16 music_vol;
    u16 earphone_vol;

    u32 electric_mode_cancel_tick;
    u16 a2dp_stop_timerout;
};


enum {
    MIC_TYPE_NORMAL = 0x0,
    MIC_TYPE_EAR,
};



static const u32 electric_tab[] = {
    A_MAJOR,
    Ashop_MAJOR,
    B_MAJOR,
    C_MAJOR,
    Cshop_MAJOR,
    D_MAJOR,
    Dshop_MAJOR,
    E_MAJOR,
    F_MAJOR,
    Fshop_MAJOR,
    G_MAJOR,
    Gshop_MAJOR,
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

static const u16 pitch_tab[] = {
    EFFECT_REVERB_MODE_GIRL_TO_BOY,
    EFFECT_REVERB_MODE_BOY_TO_GIRL,
    EFFECT_REVERB_MODE_KIDS,
};

struct __soundcard soundcard = {
    .mode = SOUNDCARD_MODE_NORMAL,
    .mic_vol = (u16) - 1,
    .reverb_wet_vol = (u16) - 1,
    .high_sound_vol = (u16) - 1,
    .low_sound_vol = (u16) - 1,
    .rec_vol = (u16) - 1,
    .music_vol = (u16) - 1,
    .earphone_vol = (u16) - 1,
    .a2dp_stop_timerout = 0,
    .status = 0,
};



#define __this (&soundcard)


extern void pc_rrrl_output_enable(u8 onoff);
extern u8 pc_rrrl_output_enable_status(void);

static void soundcard_mic_status_update(void)
{
    if (__this->mic_status == 0) {
        //没有mic在线
        //关闭ear mic
        soundcard_ear_mic_mute(1);
        //关闭混响
        mic_effect_pause(1);
        return ;
    }

    if (__this->mic_status & BIT(MIC_TYPE_NORMAL)) {
        //普通mic在线
        //mute ear mic
        soundcard_ear_mic_mute(1);
    } else if (__this->mic_status & BIT(MIC_TYPE_EAR)) {
        //ear mic在线
        //unmute ear mic
        soundcard_ear_mic_mute(0);
    }
    //释放暂停状态
    mic_effect_pause(0);
}



static void soundcard_change_mic_effect_mode(u16 mode)
{
    mic_effect_change_mode(mode);
}

static u8 soundcard_electric_conversion(u32 value)
{
    for (u8 i = 0; i < sizeof(electric_tab) / sizeof(electric_tab[0]); i++) {
        if (value == electric_tab[i]) {
            return i;
        }
    }
    return 0;
}

static u8 get_soundcard_electric_conversion(u8 value)
{
    return value + APP_SOUNDCARD_MODE_ELECTRIC_A_MAJOR;
}

static void soundcard_app_set_pitch_mode(u8 mode)
{
    switch (mode) {
    case APP_PITCH_NONE:
        __this->mode = SOUNDCARD_MODE_NORMAL;
        soundcard_led_mode(SOUNDCARD_MODE_NORMAL, 1);
        soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_NORMAL);
        break;
    case APP_PITCH_GIRL:
    case APP_PITCH_BOY:
    case APP_PITCH_KIDS:
        __this->mode = SOUNDCARD_MODE_PITCH;
        __this->pitch = mode - 1;
        soundcard_led_mode(SOUNDCARD_MODE_PITCH, 1);
        soundcard_change_mic_effect_mode(pitch_tab[__this->pitch]);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_A_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(A_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_Ashop_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(Ashop_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_B_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(B_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_C_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(C_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_Cshop_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(Cshop_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_D_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(D_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_Dshop_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(Dshop_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_E_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(E_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_F_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(F_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_Fshop_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(Fshop_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_G_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(G_MAJOR);
        break;
    case APP_SOUNDCARD_MODE_ELECTRIC_Gshop_MAJOR:
        __this->mode = SOUNDCARD_MODE_ELECTRIC;
        __this->electric = soundcard_electric_conversion(Gshop_MAJOR);
        break;
    }

    if (SOUNDCARD_MODE_ELECTRIC == __this->mode) {
        soundcard_led_mode(SOUNDCARD_MODE_ELECTRIC, 1);
        //play notice
        soundcard_make_notice_electric(__this->electric);
        //切换模式为电音模式,设置完需要手动配置电音的pitch参数，工具不可配
        soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_ELECTRIC);
        PITCH_PARM_SET2 electric_parm = {0};
        //设置电音的大调级数
        electric_parm.formant_shift = electric_tab[__this->electric];
        //电音该参数固定为100
        electric_parm.pitch = 100;
        //因为电音参数是固定的不是通过工具
        mic_effect_pitch_parm_fill(&electric_parm, 1, 0);
    }
}

static u8 soundcard_pitch_conversion(u8 value)
{
    u8 ret = 0;
    switch (value) {
    case EFFECT_REVERB_MODE_BOY_TO_GIRL:
        ret = APP_PITCH_GIRL;
        break;
    case EFFECT_REVERB_MODE_GIRL_TO_BOY:
        ret = APP_PITCH_BOY;
        break;
    case EFFECT_REVERB_MODE_KIDS:
        ret = APP_PITCH_KIDS;
        break;
    }
    return ret;
}

u8 get_curr_soundcard_pitch(void)
{
    u8 ret = 0;
    if (__this->mode == SOUNDCARD_MODE_PITCH) {
        ret = soundcard_pitch_conversion(pitch_tab[__this->pitch]);
    }
    return ret;
}

u8 get_curr_soundcard_pitch_by_value(void)
{
    u8 ret = 0;
    if (!__this) {
        return 0;
    }
    switch (__this->mode) {
    case SOUNDCARD_MODE_NORMAL:
        ret = APP_PITCH_NONE;
        break;
    case SOUNDCARD_MODE_PITCH:
        ret = soundcard_pitch_conversion(pitch_tab[__this->pitch]);
        break;
    case SOUNDCARD_MODE_ELECTRIC:
        ret = get_soundcard_electric_conversion(__this->electric);
        break;
    }
    return ret;
}

u8 curr_soundcard_mode_is_normal(void)
{
    return (SOUNDCARD_MODE_NORMAL == __this->mode);
}

u8 get_curr_key_soundcard_pitch(void)
{
    return __this->pitch;
}

u16 soundcard_get_mic_vol(void)
{
    return __this->mic_vol;
}

u16 soundcard_get_reverb_wet_vol(void)
{
    return __this->reverb_wet_vol;
}

u16 soundcard_get_low_sound_vol(void)
{
    return __this->low_sound_vol;
}

u16 soundcard_get_high_sound_vol(void)
{
    return __this->high_sound_vol;
}

u16 soundcard_get_rec_vol(void)
{
    return __this->rec_vol;
}

u16 soundcard_get_music_vol(void)
{
    return __this->music_vol;
}

u16 soundcard_get_earphone_vol(void)
{
    return __this->earphone_vol;
}

void soundcard_event_deal(struct sys_event *event)
{
    struct key_event *key = &event->u.key;
    int key_event = event->u.key.event;
    int key_value = event->u.key.value;
    u8 mic_status = 0;

    if (key_event == KEY_NULL) {
        return ;
    }

    if (__this->status == 0) {
        return ;
    }

    switch (key_event) {
    //按键模式操作
    case KEY_SOUNDCARD_MODE_ELECTRIC:
        log_i("KEY_SOUNDCARD_MODE_ELECTRIC\n");
        if (__this->mode != SOUNDCARD_MODE_ELECTRIC) {
            __this->mode = SOUNDCARD_MODE_ELECTRIC;
            __this->electric = 0;
        } else {
            __this->electric++;
            if (__this->electric >= ARRAY_SIZE(electric_tab)) {
                __this->electric = 0;
                //__this->mode = SOUNDCARD_MODE_NORMAL;
                //soundcard_led_mode(SOUNDCARD_MODE_NORMAL, 1);
                //soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_NORMAL);
                //break;
            }
        }
        soundcard_led_mode(SOUNDCARD_MODE_ELECTRIC, 1);
        //play notice
        soundcard_make_notice_electric(__this->electric);
        //切换模式为电音模式,设置完需要手动配置电音的pitch参数，工具不可配
        soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_ELECTRIC);
        PITCH_PARM_SET2 electric_parm = {0};
        //设置电音的大调级数
        electric_parm.formant_shift = electric_tab[__this->electric];
        //电音该参数固定为100
        electric_parm.pitch = 100;
        //因为电音参数是固定的不是通过工具
        mic_effect_pitch_parm_fill(&electric_parm, 1, 0);
        break;
    case KEY_SOUNDCARD_MODE_ELECTRIC_CANCEL:
        if (__this->mode == SOUNDCARD_MODE_ELECTRIC) {
            if (__this->electric_mode_cancel_tick == 0) {
                __this->electric_mode_cancel_tick = timer_get_ms();
                break;
            }
            u32 cur_tick = timer_get_ms();
            if ((cur_tick - __this->electric_mode_cancel_tick) > 4000) {
                __this->electric_mode_cancel_tick = timer_get_ms();
                break;
            } else if ((cur_tick - __this->electric_mode_cancel_tick) > 3000) {
                log_i("KEY_SOUNDCARD_MODE_ELECTRIC_CANCEL\n");
                __this->electric_mode_cancel_tick = 0;
                __this->mode = SOUNDCARD_MODE_NORMAL;
                soundcard_led_mode(SOUNDCARD_MODE_NORMAL, 1);
                soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_NORMAL);
            }
        }
        break;
    case KEY_SOUNDCARD_MODE_PITCH:
        log_i("KEY_SOUNDCARD_MODE_PITCH\n");
        if (__this->mode != SOUNDCARD_MODE_PITCH) {
            __this->mode = SOUNDCARD_MODE_PITCH;
            __this->pitch = 0;
        } else {
            __this->pitch++;
            if (__this->pitch >= ARRAY_SIZE(pitch_tab)) {
                __this->mode = SOUNDCARD_MODE_NORMAL;
                soundcard_led_mode(SOUNDCARD_MODE_NORMAL, 1);
                soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_NORMAL);
                break;
            }
        }
        soundcard_led_mode(SOUNDCARD_MODE_PITCH, 1);
        soundcard_change_mic_effect_mode(pitch_tab[__this->pitch]);
        break;
    case KEY_SOUNDCARD_MODE_PITCH_BY_VALUE:
        log_i("KEY_SOUNDCARD_MODE_PITCH_BY_VALUE, %d\n", key_value);
        soundcard_app_set_pitch_mode(key_value);
        break;
    case KEY_SOUNDCARD_MODE_MAGIC:
        log_i("KEY_SOUNDCARD_MODE_MAGIC\n");
        if (__this->mode != SOUNDCARD_MODE_MAGIC) {
            __this->mode = SOUNDCARD_MODE_MAGIC;
            soundcard_led_mode(SOUNDCARD_MODE_MAGIC, 1);
            soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_MAGIC);
        } else {
            __this->mode = SOUNDCARD_MODE_NORMAL;
            soundcard_led_mode(SOUNDCARD_MODE_NORMAL, 1);
            soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_NORMAL);
        }
        break;
    case KEY_SOUNDCARD_MODE_BOOM:
        log_i("KEY_SOUNDCARD_MODE_BOOM\n");
        if (__this->mode != SOUNDCARD_MODE_BOOM) {
            __this->mode = SOUNDCARD_MODE_BOOM;
            soundcard_led_mode(SOUNDCARD_MODE_BOOM, 1);
            soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_BOOM);
        } else {
            __this->mode = SOUNDCARD_MODE_NORMAL;
            soundcard_led_mode(SOUNDCARD_MODE_NORMAL, 1);
            soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_NORMAL);
        }
        break;
    case KEY_SOUNDCARD_MODE_SHOUTING_WHEAT:
        log_i("KEY_SOUNDCARD_MODE_SHOUTING_WHEAT\n");
        if (__this->mode != SOUNDCARD_MODE_SHOUTING_WHEAT) {
            __this->mode = SOUNDCARD_MODE_SHOUTING_WHEAT;
            soundcard_led_mode(SOUNDCARD_MODE_SHOUTING_WHEAT, 1);
            soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_SHOUTING_WHEAT);
            mic_effect_cal_coef(MIC_EQ_MODE_SHOUT_WHEAT, 1);
        } else {
            __this->mode = SOUNDCARD_MODE_NORMAL;
            mic_effect_cal_coef(MIC_EQ_MODE_SHOUT_WHEAT, 0);
            soundcard_led_mode(SOUNDCARD_MODE_NORMAL, 1);
            soundcard_change_mic_effect_mode(EFFECT_REVERB_MODE_NORMAL);
        }
        break;
    case KEY_SOUNDCARD_MODE_DODGE:
#if(TCFG_MIC_DODGE_EN)
        mic_dodge_ctr();
        if (mic_dodge_get_status()) {
            soundcard_led_mode(SOUNDCARD_MODE_DODGE, 1);
        } else {
            soundcard_led_mode(SOUNDCARD_MODE_DODGE, 0);
        }
#endif
        break;

    case KEY_SOUNDCARD_USB_MIC_MUTE_SWICH:
        log_i("KEY_SOUNDCARD_USB_MIC_MUTE_SWICH\n");
        mem_stats();
        /* break; */
#if (TCFG_PC_ENABLE)
        if (pc_rrrl_output_enable_status()) {
            printf("pc_rrrl_output_enable\n");
            pc_rrrl_output_enable(0);
        } else {
            printf("pc_rrrl_output_disable\n");
            pc_rrrl_output_enable(1);
        }
#endif
        /* if (mic_effect_get_status()) { */
        /* mic_effect_stop(); */
        /* } else { */
        /* mic_effect_start(); */
        /* } */
        break;

    //按键提示音响应处理
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
        log_i("make noise index = %d\n", key_event - KEY_SOUNDCARD_MAKE_NOISE0);
        soundcard_make_some_noise(key_event - KEY_SOUNDCARD_MAKE_NOISE0);
        break;
    case KEY_SOUNDCARD_SLIDE_MIC:
        log_i("slide MIC:%d\n", key_value);
        __this->mic_vol = key_value;
        mic_effect_set_dvol(key_value);
        break;
    case KEY_SOUNDCARD_SLIDE_WET_GAIN:
        log_i("slide WET:%d\n", key_value);
        __this->reverb_wet_vol = key_value;
        key_value = key_value * 10; //key_value*300/30;
        mic_effect_set_reverb_wet(key_value);
        break;
    case KEY_SOUNDCARD_SLIDE_HIGH_SOUND:
        log_i("slide HIGH SOUND:%d\n", key_value);
        __this->high_sound_vol = key_value;
        mic_effect_cal_coef(MIC_EQ_MODE_HIGH_SOUND, key_value);
        break;
    case KEY_SOUNDCARD_SLIDE_LOW_SOUND:
        log_i("slide LOW SOUND:%d\n", key_value);
        __this->low_sound_vol = key_value;
        mic_effect_cal_coef(MIC_EQ_MODE_LOW_SOUND, key_value);
        break;
    case KEY_SOUNDCARD_SLIDE_RECORD_VOL:
        log_i("slide RECORD:%d\n", key_value);
        __this->rec_vol = key_value;
        //设置dac rr rl硬件数字音量
        audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(2) | BIT(3), (u32)__this->rec_vol * (u32)16384 / (u32)30, 1);
        break;
    case KEY_SOUNDCARD_SLIDE_EARPHONE_VOL:
        log_i("slide EARPHONE:%d\n", key_value);
        __this->earphone_vol =  key_value;
        //设置dac fr fl硬件数字音量
        audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0) | BIT(1), (u32)__this->earphone_vol * (u32)16384 / (u32)30, 1);
        break;
    case KEY_SOUNDCARD_SLIDE_MUSIC_VOL:
        log_i("slide MUSIC:%d\n", key_value);
        __this->music_vol = key_value;
        audio_dig_vol_set(audio_dig_vol_group_hdl_get(sys_digvol_group, "music_a2dp"), 	AUDIO_DIG_VOL_ALL_CH, __this->music_vol);
        audio_dig_vol_set(audio_dig_vol_group_hdl_get(sys_digvol_group, "music_linein"), AUDIO_DIG_VOL_ALL_CH, __this->music_vol);
        audio_dig_vol_set(audio_dig_vol_group_hdl_get(sys_digvol_group, "music_pc"), 	AUDIO_DIG_VOL_ALL_CH, __this->music_vol);
        break;

    case KEY_SOUNDCARD_NORMAL_MIC_STATUS_UPDATE:
        if (key_value) {
            __this->mic_status |= BIT(MIC_TYPE_NORMAL);
        } else {
            __this->mic_status &= ~BIT(MIC_TYPE_NORMAL);
        }
        soundcard_mic_status_update();
        break;
    case KEY_SOUNDCARD_EAR_MIC_STATUS_UPDATE:
        if (key_value) {
            __this->mic_status |= BIT(MIC_TYPE_EAR);
        } else {
            __this->mic_status &= ~BIT(MIC_TYPE_EAR);
        }
        soundcard_mic_status_update();
        break;

    case KEY_SOUNDCARD_AUX_STATUS_UPDATE:
        if (key_value) {

        } else {

        }
        break;
    default:
        break;
    }
}

static void soundcard_change_mode_timerout(void *p)
{
    if (app_get_curr_task() == APP_PC_TASK) {
        return ;
    }
    if (pc_app_check() == true) {
        app_task_switch_to(APP_PC_TASK);
    }
}

static void soundcard_switch_pc_mode_start(u32 msec)
{
#if (TCFG_PC_ENABLE)
    if (__this->a2dp_stop_timerout) {
        sys_timeout_del(__this->a2dp_stop_timerout);
    }
    __this->a2dp_stop_timerout = sys_timeout_add(NULL, soundcard_change_mode_timerout, msec);
#endif
}

static void soundcard_switch_pc_mode_stop(void)
{
#if (TCFG_PC_ENABLE)
    if (__this->a2dp_stop_timerout) {
        sys_timeout_del(__this->a2dp_stop_timerout);
        __this->a2dp_stop_timerout = 0;
    }
#endif
}



void soundcard_bt_connect_status_event(struct bt_event *e)
{
    switch (e->event) {
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        soundcard_switch_pc_mode_start(5000);
        soundcard_led_self_set(UI_LED_SELF_BLUE_FLASH, 0);
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        soundcard_led_self_set(UI_LED_SELF_BLUE_FLASH, 1);
        break;
    case BT_STATUS_PHONE_INCOME:
        break;
    case BT_STATUS_PHONE_OUT:
        break;
    case BT_STATUS_PHONE_ACTIVE:
        break;
    case BT_STATUS_PHONE_HANGUP:
        break;
    case BT_STATUS_PHONE_NUMBER:
        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
        break;
    case BT_STATUS_VOICE_RECOGNITION:
        break;
    case BT_STATUS_A2DP_MEDIA_START:
        soundcard_switch_pc_mode_stop();
        break;
    case BT_STATUS_INIT_OK:
        soundcard_switch_pc_mode_start(5000);
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        soundcard_switch_pc_mode_start(2000);
        break;
    default:
        break;
    }
}

void soundcard_power_event(struct device_event *dev)
{
#if(TCFG_SYS_LVD_EN == 1)
    switch (dev->event) {
    case POWER_EVENT_POWER_NORMAL:
        printf("POWER_EVENT_POWER_NORMAL\n");
        soundcard_low_power_led(0);
        break;
    case POWER_EVENT_POWER_WARNING:
        printf("POWER_EVENT_POWER_WARNING\n");
        soundcard_low_power_led(1);
        break;
    }
#endif
}

void soundcard_start(void)
{
    mic_effect_start();
    audio_mic_0dB_en(1);
    audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(0) | BIT(1) | BIT(2) | BIT(3), 0, 1);//模拟音量范围0~30
    __this->status = 1;
}

void soundcard_close(void)
{
    __this->status = 0;
    mic_effect_stop();
}

#endif/*SOUNDCARD_ENABLE*/


