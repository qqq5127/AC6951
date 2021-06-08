#include "stream_src.h"

static void stream_src_irq_cb(void *hdl)
{
    struct __stream_src *stream = hdl;
    audio_stream_resume(&stream->entry);
}

static int stream_src_output_handler(void *hdl, void *buf, int len)
{
    struct __stream_src *stream = hdl;
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


static int stream_src_run(struct __stream_src *stream, s16 *data, int len)
{
    if (stream->src_hdl == NULL) {
        return 0;
    }
    return audio_src_resample_write(stream->src_hdl, data, len);
}

static void stream_src_output_data_process_len(struct audio_stream_entry *entry, int len)
{
    struct __stream_src *stream = container_of(entry, struct __stream_src, entry);
    if (stream) {
        stream->out_points += len / 2;
    }
}

static void stream_src_check(struct __stream_src *stream, u16 sr, u8 ch_num)
{
    if ((stream->src_always == 0) && (stream->target_sr == sr)) {
        audio_hw_src_stop(stream->src_hdl);
        audio_hw_src_close(stream->src_hdl);
        free(stream->src_hdl);
        stream->src_hdl = NULL;
        return ;
    }
    if (stream->src_hdl == NULL) {
        stream->src_hdl = zalloc(sizeof(struct audio_src_handle));
        ASSERT(stream->src_hdl);
        audio_hw_src_open(stream->src_hdl, ch_num, SRC_TYPE_RESAMPLE);
        audio_hw_src_set_rate(stream->src_hdl, sr, stream->target_sr);
        audio_src_set_output_handler(stream->src_hdl, stream, stream_src_output_handler);
        audio_src_set_rise_irq_handler(stream->src_hdl, stream, stream_src_irq_cb);
    } else {
        audio_hw_src_set_rate(stream->src_hdl, sr, stream->target_sr);
    }
}

static int stream_src_data_handler(struct audio_stream_entry *entry,
                                   struct audio_data_frame *in,
                                   struct audio_data_frame *out)
{
    struct __stream_src *stream = container_of(entry, struct __stream_src, entry);
    out->sample_rate = stream->target_sr;
    if (in->data_len) {
        out->no_subsequent = 1;
    }
    //输出剩余数据
    if (stream->out_buf && (stream->out_points < stream->out_total)) {
        struct audio_data_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.channel = in->channel;
        frame.sample_rate = stream->target_sr;
        frame.data_sync = in->data_sync;
        while (1) {
            frame.data_len = (stream->out_total - stream->out_points) * 2;
            frame.data = &stream->out_buf[stream->out_points];
            int len = stream->out_points;
            audio_stream_run(entry, &frame);
            break;
        }
        //未输出完退出
        if (stream->out_points < stream->out_total) {
            return 0;
        }
    } else {
        //printf("=========\n");
    }

    //同步检查
    stream_src_check(stream, in->sample_rate, in->channel);

    int len = 0;
    u8 cnt = 0;
    len = stream_src_run(stream, in->data, in->data_len);

    if (stream->out_buf && (stream->out_points < stream->out_total)) {
        struct audio_data_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.channel = in->channel;
        frame.sample_rate = stream->target_sr;
        frame.data_sync = in->data_sync;
        while (1) {
            frame.data_len = (stream->out_total - stream->out_points) * 2;
            frame.data = &stream->out_buf[stream->out_points];
            int len = stream->out_points;
            audio_stream_run(entry, &frame);
            break;
        }
        if (stream->out_points < stream->out_total) {
            return len;
        }
        if (!len) {
            len = stream_src_run(stream, in->data, in->data_len);
        }

    } else {
        out->data_len = 0;
        //printf("^^^^^^^^^^/n");
    }

    return len;
}


void stream_src_set_target_rate(struct __stream_src *hdl, u16 sr)
{
    if (hdl) {
        hdl->target_sr = sr;
    }
}

struct __stream_src *stream_src_open(u16 target_sr, u8 always)
{
    struct __stream_src *stream = zalloc(sizeof(struct __stream_src));
    ASSERT(stream);

    stream->src_always = always;
    stream->target_sr = target_sr;

    stream->entry.data_process_len = stream_src_output_data_process_len;
    stream->entry.data_handler = stream_src_data_handler;

    return stream;
}

void stream_src_close(struct __stream_src **hdl)
{
    if (hdl && (*hdl)) {
        struct __stream_src *stream = *hdl;
        audio_stream_del_entry(&stream->entry);
        if (stream->src_hdl) {
            audio_hw_src_stop(stream->src_hdl);
            audio_hw_src_close(stream->src_hdl);
            free(stream->src_hdl);
            stream->src_hdl = NULL;
        }
        local_irq_disable();
        free(stream);
        *hdl = NULL;
        local_irq_enable();
    }
}

void stream_src_resume(struct __stream_src *hdl)
{
    if (hdl) {
        audio_stream_resume(&hdl->entry);
    }
}


