#ifndef __EFFECT_DEBUG_H__
#define __EFFECT_DEBUG_H__

#include "system/includes.h"
// #include "reverb/reverb_api.h"
// #include "audio_mic/mic_stream.h"
// #include "pitchshifter/pitchshifter_api.h"
// #include "asm/noisegate.h"
#include "effect_cfg.h"

void mic_effect_reverb_parm_printf(REVERBN_PARM_SET *parm);
void mic_effect_pitch2_parm_printf(PITCH_PARM_SET2 *parm);
void mic_effect_pitch_parm_printf(PITCH_SHIFT_PARM *parm);
void mic_effect_noisegate_parm_printf(NOISE_PARM_SET *parm);
void mic_effect_shout_wheat_parm_printf(SHOUT_WHEAT_PARM_SET *parm);
void mic_effect_low_sound_parm_printf(LOW_SOUND_PARM_SET *parm);
void mic_effect_high_sound_parm_printf(HIGH_SOUND_PARM_SET *parm);

#endif// __EFFECT_DEBUG_H__
