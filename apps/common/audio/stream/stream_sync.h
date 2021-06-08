#ifndef __STREAM_SYNC_H__
#define __STREAM_SYNC_H__

#include "system/includes.h"
#include "media/includes.h"

struct stream_sync_info {
    u16 i_sr;
    u16 o_sr;
    u8	ch_num;
    int begin_per;		// 起始百分比
    int top_per;		// 最大百分比
    int bottom_per;		// 最小百分比
    u8 inc_step;		// 每次调整增加步伐
    u8 dec_step;		// 每次调整减少步伐
    u8 max_step;		// 最大调整步伐
    void *priv;			// get_total,get_size私有句柄
    int (*get_total)(void *priv);	// 获取buf总长
    int (*get_size)(void *priv);	// 获取buf数据长度
};

struct __stream_sync_cb {
    void *priv;			// get_total,get_size私有句柄
    int (*get_total)(void *priv);	// 获取buf总长
    int (*get_size)(void *priv);	// 获取buf数据长度
};


struct __stream_sync {
    s16 *out_buf;
    int out_points;
    int out_total;
    u16 sample_rate;
    struct __stream_sync_cb cb;
    struct audio_buf_sync_hdl sync;
    struct audio_stream_entry entry;
};

struct __stream_sync *stream_sync_open(struct stream_sync_info *info, u8 always);
void stream_sync_close(struct __stream_sync **hdl);
void stream_sync_resume(struct __stream_sync *hdl);

#endif// __STREAM_SYNC_H__

