#include "stream_sync.h"

static void stream_sync_irq_cb(void *hdl)
{
    struct __stream_sync *stream = hdl;
    audio_stream_resume(&stream->entry);
}

static int stream_sync_output_handler(void *hdl, void *buf, int len)
{
    struct __stream_sync *stream = hdl;
    if (!stream) {
        return 0;
    }

    if (stream->out_buf) {
        if (stream->out_points < stream->out_total) {
            return 0;
        } else {
            stream->out_buf = NULL;
            return len;
        }
    }

    stream->out_buf = buf;
    stream->out_points = 0;
    stream->out_total = len / 2;
    return 0;
}


static int stream_sync_run(struct __stream_sync *stream, s16 *data, int len)
{
    int data_size = stream->cb.get_size(stream->cb.priv);
#if 0
    if (!stream->sync.start) {
        if (data_size < stream->sync.begin_size) {
            return 0;
        }
        stream->sync.start = 1;
    }
#endif
    stream->sync.start = 1;
    audio_buf_sync_adjust(&stream->sync, data_size);
    return audio_src_resample_write(stream->sync.src_sync, data, len);
}

static void stream_sync_output_data_process_len(struct audio_stream_entry *entry, int len)
{
    struct __stream_sync *stream = container_of(entry, struct __stream_sync, entry);
    if (stream) {
        stream->out_points += len / 2;
    }
}

static int stream_sync_data_handler(struct audio_stream_entry *entry,
                                    struct audio_data_frame *in,
                                    struct audio_data_frame *out)
{
    struct __stream_sync *stream = container_of(entry, struct __stream_sync, entry);
    out->sample_rate = stream->sample_rate;
    if (in->data_len) {
        out->no_subsequent = 1;
    }
    //输出剩余数据
    if (stream->out_buf && (stream->out_points < stream->out_total)) {
        struct audio_data_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.channel = in->channel;
        frame.sample_rate = stream->sample_rate;
        frame.data_sync = in->data_sync;
        while (1) {
            frame.data_len = (stream->out_total - stream->out_points) * 2;
            frame.data = &stream->out_buf[stream->out_points];
            int len = stream->out_points;
            audio_stream_run(entry, &frame);
#if 0
            if (len == stream->out_points) {
                break;
            }
            if (stream->out_points == stream->out_total) {
                break;
            }
#else
            break;
#endif
        }
        //未输出完退出
        if (stream->out_points < stream->out_total) {
            return 0;
        }
    } else {
        //printf("=========\n");
    }

    int len = 0;
    u8 cnt = 0;
    len = stream_sync_run(stream, in->data, in->data_len);

    if (stream->out_buf && (stream->out_points < stream->out_total)) {
        struct audio_data_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.channel = in->channel;
        frame.sample_rate = stream->sample_rate;
        frame.data_sync = in->data_sync;
        while (1) {
            frame.data_len = (stream->out_total - stream->out_points) * 2;
            frame.data = &stream->out_buf[stream->out_points];
            int len = stream->out_points;
            audio_stream_run(entry, &frame);
#if 0
            if (len == stream->out_points) {
                break;
            }
            if (stream->out_points == stream->out_total) {
                break;
            }
#else
            break;
#endif
        }
        if (stream->out_points < stream->out_total) {
            return len;
        }
        if (!len) {
            len = stream_sync_run(stream, in->data, in->data_len);
        }

    } else {
        out->data_len = 0;
        //printf("^^^^^^^^^^/n");
    }

    return len;
}


struct __stream_sync *stream_sync_open(struct stream_sync_info *info, u8 always)
{
    struct audio_buf_sync_open_param param = {0};

    struct __stream_sync *stream = zalloc(sizeof(struct __stream_sync));
    ASSERT(stream);

    param.total_len = info->get_total(info->priv);
    param.begin_per = info->begin_per;
    param.top_per = info->top_per;
    param.bottom_per = info->bottom_per;
    param.inc_step = info->inc_step;
    param.dec_step = info->dec_step;
    param.max_step = info->max_step;
    param.in_sr = info->i_sr;
    param.out_sr = info->o_sr;
    param.ch_num = info->ch_num;
    if (always) {
        param.check_in_out = 0;
    } else {
        param.check_in_out = 1;
    }
    param.output_hdl = stream;
    param.output_handler = stream_sync_output_handler;
    int ret = audio_buf_sync_open(&stream->sync, &param);
    ASSERT(ret == true);
    audio_src_set_rise_irq_handler(stream->sync.src_sync, stream, stream_sync_irq_cb);

    stream->sample_rate = info->o_sr;
    stream->cb.priv = info->priv;
    stream->cb.get_size = info->get_size;
    stream->cb.get_total = info->get_total;

    stream->entry.data_process_len = stream_sync_output_data_process_len;
    stream->entry.data_handler = stream_sync_data_handler;

    return stream;
}

void stream_sync_close(struct __stream_sync **hdl)
{
    if (hdl && (*hdl)) {
        struct __stream_sync *stream = *hdl;
        audio_stream_del_entry(&stream->entry);
        stream->sync.start = 0;
        audio_buf_sync_close(&stream->sync);
        local_irq_disable();
        free(stream);
        *hdl = NULL;
        local_irq_enable();
    }
}

void stream_sync_resume(struct __stream_sync *hdl)
{
    if (hdl) {
        audio_stream_resume(&hdl->entry);
    }
}


