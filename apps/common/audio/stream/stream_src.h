#ifndef __STREAM_SRC_H__
#define __STREAM_SRC_H__

#include "system/includes.h"
#include "media/includes.h"

struct __stream_src {
    s16 *out_buf;
    int out_points;
    int out_total;
    u8 src_always;
    u16 target_sr;
    struct audio_src_handle   *src_hdl;
    struct audio_stream_entry entry;
};

struct __stream_src *stream_src_open(u16 target_sr, u8 always);
void stream_src_close(struct __stream_src **hdl);
void stream_src_resume(struct __stream_src *hdl);
void stream_src_set_target_rate(struct __stream_src *hdl, u16 sr);

#endif// __STREAM_SRC_H__

