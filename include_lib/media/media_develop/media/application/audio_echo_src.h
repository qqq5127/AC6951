#ifndef _AUDIO_ECHO_SRC_API_H
#define _AUDIO_ECHO_SRC_API_H

#include "asm/Resample_api.h"
// #include "asm/howling_api.h"

typedef struct _ECHO_SRC_API_STRUCT_ {
    RS_PARA_STRUCT 	parm;  //参数
    void			*ptr;    //运算buf指针
    RS_STUCT_API     *src_api;
    struct audio_stream_entry entry;	// 音频流入口
    s16 *out_buf;
    int out_len;
    int process_len;
    u8 run_en;
    s16 src_adj_step;
    u32 original_out_src;
    u32 out_buf_len;
} ECHO_SRC_API_STRUCT;





ECHO_SRC_API_STRUCT *open_echo_src(int in_sr, int out_sr, u8 channel);
int run_echo_src(ECHO_SRC_API_STRUCT *echo_src_hdl, short *in, short *out, int len);
void close_echo_src(ECHO_SRC_API_STRUCT *echo_src_hdl);
void pause_echo_src(ECHO_SRC_API_STRUCT *echo_src_hdl, u8 run_mark);
#endif

