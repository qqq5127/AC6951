#ifndef __SOUNDCARD_H__
#define __SOUNDCARD_H__

#include "generic/typedef.h"
#include "app_config.h"
#include "system/includes.h"

enum {
    SOUNDCARD_MODE_NORMAL = 0x0,
    SOUNDCARD_MODE_ELECTRIC,
    SOUNDCARD_MODE_PITCH,
    SOUNDCARD_MODE_MAGIC,
    SOUNDCARD_MODE_BOOM,
    SOUNDCARD_MODE_SHOUTING_WHEAT,
    SOUNDCARD_MODE_DODGE,
};

u16 soundcard_get_mic_vol(void);
u16 soundcard_get_reverb_wet_vol(void);
u16 soundcard_get_low_sound_vol(void);
u16 soundcard_get_high_sound_vol(void);
u16 soundcard_get_rec_vol(void);
u16 soundcard_get_music_vol(void);
u16 soundcard_get_earphone_vol(void);

void soundcard_event_deal(struct sys_event *event);
void soundcard_bt_connect_status_event(struct bt_event *e);
void soundcard_power_event(struct device_event *dev);
void soundcard_start(void);
void soundcard_close(void);

#endif//__SOUNDCARD_H__
