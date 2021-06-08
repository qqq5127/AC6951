#ifndef _AUDIO_FMTX_H_
#define _AUDIO_FMTX_H_



extern void *audio_fmtx_output_start(u16 fre);
extern void audio_fmtx_output_stop();
extern int audio_fmtx_output_write(s16 *data, u32 len);
extern void audio_fmtx_output_set_srI(u32 sample_rate);





#endif

