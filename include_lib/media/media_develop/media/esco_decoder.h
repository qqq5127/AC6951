#ifndef ESCO_DECODER_H
#define ESCO_DECODER_H


#include "media/includes.h"


struct esco_decoder {
    u32 coding_type;	// 解码类型
    u16 sample_rate;	// 解码采样率
    u16 start : 1;		// 解码开始
    u16 enc_start : 1; 	// 采mic上行开始
    u8 ch_num;			// 解码声道数
    u8 out_ch_num;		// 解码输出声道数
    u8 offset;			// 帧数据偏移
    u8 data_len;		// 帧数据长度
    u8 frame_len;		// 帧数据解析总长
    u8 esco_len;		// 帧数据包长
    u8 frame_get;		// 帧数据获取到
    u8 sync_step;
    u8 join_tws;
    u8 preempt_state;
    volatile u8 wait_resume;
    u16 resume_tmr_id;
    u8 *frame;			// 帧数据
    u32 hash;			// 帧数据标签
    int data[15];		// 帧数据缓存 /*ALIGNED(4)*/
    struct audio_decoder decoder;	// 解码器
    struct audio_decoder_task *decode_task;	// 解码任务
    void *sync;
};

// 打开esco解码
int esco_decoder_open(struct esco_decoder *, struct audio_decoder_task *decode_task);
// 关闭esco解码
void esco_decoder_close(struct esco_decoder *dec);
// esco解码输出（如果输出是双声道，会有声道转换）
int esco_decoder_output_handler(struct esco_decoder *dec, struct audio_data_frame *in);

void esco_decoder_stream_sync_enable(struct esco_decoder *dec, void *sync, int sample_rate, int delay_time);

void esco_decoder_join_tws(struct esco_decoder *dec);
#endif /*ESCO_DECODER_H*/

