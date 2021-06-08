
#ifndef __EFFECT_STREAM_H__
#define __EFFECT_STREAM_H__

#include "system/includes.h"
#include "app_config.h"
#include "media/includes.h"
#include "application/audio_vocal_tract_synthesis.h"

void soundcard_mix_init(struct audio_mixer *mix, s16 *mix_buf, u16 buf_size, struct audio_vocal_tract_ch *tract_ch, u8 flag);

#endif//__EFFECT_STREAM_H__
