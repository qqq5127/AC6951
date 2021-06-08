#include "effect_debug.h"

#define LOG_TAG     "[APP-REVERB]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#if (TCFG_MIC_EFFECT_ENABLE)

void mic_effect_reverb_parm_printf(REVERBN_PARM_SET *parm)
{
#if TCFG_MIC_EFFECT_DEBUG
    if (parm) {
        log_info("<reverb_parm>\n");
        log_info("dry        :%d\n", parm->dry);
        log_info("wet        :%d\n", parm->wet);
        log_info("delay      :%d\n", parm->delay);
        log_info("rot60      :%d\n", parm->rot60);
        log_info("Erwet 	 :%d\n", parm->Erwet);
        log_info("Erfactor   :%d\n", parm->Erfactor);
        log_info("Ewidth     :%d\n", parm->Ewidth);
        log_info("Ertolate   :%d\n", parm->Ertolate);
        log_info("predelay   :%d\n", parm->predelay);
        log_info("width      :%d\n", parm->width);
        log_info("diffusion  :%d\n", parm->diffusion);
        log_info("dampinglpf :%d\n", parm->dampinglpf);
        log_info("basslpf    :%d\n", parm->basslpf);
        log_info("bassB      :%d\n", parm->bassB);
        log_info("inputlpf   :%d\n", parm->inputlpf);
        log_info("outputlpf  :%d\n", parm->outputlpf);
    }
#endif
}


void mic_effect_echo_parm_parintf(ECHO_PARM_SET *parm)
{
#if TCFG_MIC_EFFECT_DEBUG
    if (parm) {
        log_info("delay %d\n", parm->delay);
        log_info("decayval %d\n", parm->decayval);
        log_info("direct_sound_enable %d\n", parm->direct_sound_enable);
        log_info("filt_enable %d\n", parm->filt_enable);
    }
#endif

}
void mic_effect_pitch2_parm_printf(PITCH_PARM_SET2 *parm)
{
#if TCFG_MIC_EFFECT_DEBUG
    if (parm) {
        log_info("<pitch2_parm>\n");
        log_info("effect_v 	 :%d\n", parm->effect_v);
        log_info("shiftv	 :%d\n", parm->pitch);
        log_info("formant 	 :%d\n", parm->formant_shift);
    }
#endif
}

void mic_effect_pitch_parm_printf(PITCH_SHIFT_PARM *parm)
{
#if TCFG_MIC_EFFECT_DEBUG
    if (parm) {
        log_info("<pitch_parm>\n");
        log_info("effect_v 	 :%d\n", parm->effect_v);
        log_info("shiftv	 :%d\n", parm->shiftv);
        log_info("formant 	 :%d\n", parm->formant_shift);
    }
#endif
}



void mic_effect_noisegate_parm_printf(NOISE_PARM_SET *parm)
{
#if TCFG_MIC_EFFECT_DEBUG
    if (parm) {
        log_info("<noisegate_parm>\n");
        log_info("attackTime :%d\n", parm->attacktime);
        log_info("releaseTime:%d\n", parm->releasetime);
        log_info("threshold  :%d\n", parm->threadhold);
        log_info("gain 		 :%d\n", parm->gain);
    }
#endif
}

void mic_effect_shout_wheat_parm_printf(SHOUT_WHEAT_PARM_SET *parm)
{
#if TCFG_MIC_EFFECT_DEBUG
    if (parm) {
        log_info("<shout_wheat_parm>\n");
        log_info("center_frequency :%d\n", 	parm->center_frequency);
        log_info("bandwidth:%d\n", 			parm->bandwidth);
        log_info("occupy  :%d\n", 			parm->occupy);
    }
#endif
}


void mic_effect_low_sound_parm_printf(LOW_SOUND_PARM_SET *parm)
{
#if TCFG_MIC_EFFECT_DEBUG
    if (parm) {
        log_info("<low_sound_parm>\n");
        log_info("cutoff_frequency :%d\n", 		parm->cutoff_frequency);
        log_info("highest_gain:%d\n", 			parm->highest_gain);
        log_info("lowest_gain  :%d\n", 			parm->lowest_gain);
    }
#endif
}


void mic_effect_high_sound_parm_printf(HIGH_SOUND_PARM_SET *parm)
{
#if TCFG_MIC_EFFECT_DEBUG
    if (parm) {
        log_info("<high_sound_parm>\n");
        log_info("cutoff_frequency :%d\n", 		parm->cutoff_frequency);
        log_info("highest_gain:%d\n", 			parm->highest_gain);
        log_info("lowest_gain  :%d\n", 			parm->lowest_gain);
    }
#endif
}
#endif//TCFG_MIC_EFFECT_ENABLE
