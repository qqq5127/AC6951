
#ifndef __PERIPHERAL_H__
#define __PERIPHERAL_H__

#include "generic/typedef.h"
#include "app_config.h"

void soundcard_pa_init(void);
void soundcard_pa_umute(void);
void soundcard_pa_mute(void);
void soundcard_ear_mic_mute(u8 mute);
void soundcard_peripheral_init(void);

#endif//__PERIPHERAL_H__
