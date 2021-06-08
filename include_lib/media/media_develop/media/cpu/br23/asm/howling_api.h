#ifndef HOWLING_API_H
#define HOWLING_API_H

#include "asm/howling_pitchshifter_api.h"
// #include "shiftPhase_api.h"
// #include "pemafrow_api.h"
#include "media/audio_stream.h"
typedef struct s_howling_para {
    int threshold;  //初始化阈值
    int depth;  //陷波器深度
    int bandwidth;//陷波器带宽
    int attack_time; //门限噪声启动时间
    int release_time;//门限噪声释放时间
    int noise_threshold;//门限噪声阈值
    int low_th_gain; //低于门限噪声阈值增益
    int sample_rate;
    int channel;
} HOWLING_PARM_SET;

typedef struct _HOWLING_API_STRUCT_ {
    HOWLING_PARM_SET 	parm;  //陷波参数
    void				*ptr;    //运算buf指针

    HOWLING_PITCHSHIFT_PARM 	parm_2;  //移频参数
    HOWLING_PITCHSHIFT_FUNC_API *func_api;           //移频函数指针
    u8 mode;
    u16 run_points;
    struct audio_stream_entry entry;	// 音频流入口
    int out_len;
    int process_len;
    u8 run_en;
} HOWLING_API_STRUCT;

int get_howling_buf(void);
void howling_init(void *workbuf, int threshold, int depth, int bandwidth, int attackTime, int releaseTime, int Noise_threshold, int low_th_gain, int sampleRate, int channel);
int howling_run(void *workbuf, short *in, short *out, int len);

#endif
