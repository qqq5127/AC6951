#ifndef _AUDIO_PEMAFROW_API_H
#define _AUDIO_PEMAFROW_API_H

#include "asm/pemafrow_api.h"


PEMAFROW_API_STRUCT *open_pemafrow(void);
void run_pemafrow(PEMAFROW_API_STRUCT *pemafrow_hdl, short *in, short *pre_out, short *out);
// void run_pemafrow(PEMAFROW_API_STRUCT *pemafrow_hdl, short *in, short *out, int len);
void close_pemafrow(PEMAFROW_API_STRUCT *pemafrow_hdl);
void pause_pemafrow(PEMAFROW_API_STRUCT *pemafrow_hdl, u8 run_mark);

#endif

