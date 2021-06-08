#ifndef _AUDIO_NOISEGATE_API_H
#define _AUDIO_NOISEGATE_API_H

#include "media/audio_stream.h"
#include "asm/noisegate_api.h"
typedef struct _noiseGate {
    int attackTime;/*1~1500ms*/
    int releaseTime;/*1~300ms*/
    // QuasiFloat attFactor;
    // QuasiFloat relFactor;
    // QuasiFloat attFactor_sub;
    // QuasiFloat relFactor_sub;
    int threshold;/*-92~0 *(1000)db*/
    int low_th_gain;/*0.00~1.00 * (1<<3000)*/
    int channel;
    int sampleRate;

} NOISEGATE_PARM;

typedef struct _NOISEGATE_API_STRUCT_ {
    NOISEGATE_PARM 	parm;  //参数
    void				*ptr;    //运算buf指针

    struct audio_stream_entry entry;	// 音频流入口
    int out_len;
    int process_len;
    u8 run_en;
    u8 update;
} NOISEGATE_API_STRUCT;


NOISEGATE_API_STRUCT *open_noisegate(NOISEGATE_PARM *noisegate_parm, u16 sample_rate, u8 channel);
void run_noisegate(NOISEGATE_API_STRUCT *noisegate_hdl, short *in, short *out, int len);
void close_noisegate(NOISEGATE_API_STRUCT *noisegate_hdl);
void pause_noisegate(NOISEGATE_API_STRUCT *noisegate_hdl, u8 run_mark);
void update_noisegate(NOISEGATE_API_STRUCT *noisegate_hdl, NOISEGATE_PARM *parm);


#endif

