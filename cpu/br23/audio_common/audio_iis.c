#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "circular_buf.h"
#include "audio_iis.h"

#define IIS_DEBUG_ENABLE  	1
#if IIS_DEBUG_ENABLE
#define iis_debug       y_printf
#else
#define iis_debug(...)
#endif

#if (TCFG_IIS_ENABLE && TCFG_IIS_OUTPUT_EN)

#define IIS_RESAMPLE_USE_BUF_SIZE		1

#define IIS_TX_BUF_LEN 	(4*1024)
struct _iis_output {
    u32 in_sample_rate;
    u32 out_sample_rate;
    cbuffer_t cbuf;
    u8 buf[IIS_TX_BUF_LEN];
    struct audio_src_handle *hw_src;
    struct audio_stream_entry entry;
#if (IIS_RESAMPLE_USE_BUF_SIZE)
    int resample_dir;
#else
    int sr_cal_timer;
    u32 points_total;
    u16 cnt_total;
#endif
    u8 ch : 4;
    u8 flag : 1;
    u8 need_resume : 1;
} *iis_output_hdl = NULL;

static u32 iis_srI_last = 0;

struct audio_sample_sync *iis_samp_sync = NULL;
int iis_samp_sync_step = 0;

static void audio_stream_iis_src_open(void);
static void audio_stream_iis_src_close(void);

static int audio_stream_iis_data_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in, struct audio_data_frame *out)

{
    if (iis_srI_last != in->sample_rate) {
        iis_debug("in:%d ---%d\n", iis_srI_last, in->sample_rate);
        audio_iis_output_set_srI(in->sample_rate);
    }

#if (IIS_RESAMPLE_USE_BUF_SIZE)
    if (iis_output_hdl->hw_src) {
        u32 sample_rate = TCFG_IIS_OUTPUT_SR;
        u32 buf_size = cbuf_get_data_len(&(iis_output_hdl->cbuf));
        if (buf_size >= (IIS_TX_BUF_LEN * 3 / 4)) {
            /* putchar('-'); */
            sample_rate = TCFG_IIS_OUTPUT_SR - (TCFG_IIS_OUTPUT_SR * 5 / 10000);
            if (iis_output_hdl->resample_dir >= 0) {
                iis_output_hdl->resample_dir = -1;
            } else {
                iis_output_hdl->resample_dir --;
                int resr = (iis_output_hdl->resample_dir >> 3);
                if (resr < -10) {
                    resr = -10;
                }
                sample_rate += (resr * (TCFG_IIS_OUTPUT_SR * 5 / 10000));
                /* put_u16hex(sample_rate); */
            }
        } else if (buf_size <= (IIS_TX_BUF_LEN / 4)) {
            /* putchar('+'); */
            sample_rate = TCFG_IIS_OUTPUT_SR + (TCFG_IIS_OUTPUT_SR * 5 / 10000);
            if (iis_output_hdl->resample_dir <= 0) {
                iis_output_hdl->resample_dir = 1;
            } else {
                iis_output_hdl->resample_dir ++;
                int resr = (iis_output_hdl->resample_dir >> 3);
                if (resr > 10) {
                    resr = 10;
                }
                sample_rate += (resr * (TCFG_IIS_OUTPUT_SR * 5 / 10000));
                /* put_u16hex(sample_rate); */
            }
        } else {
            iis_output_hdl->resample_dir = 0;
            /* putchar('='); */
        }
        if (sample_rate != TCFG_IIS_OUTPUT_SR) {
            iis_output_hdl->out_sample_rate = sample_rate;
            audio_hw_src_set_rate(iis_output_hdl->hw_src, \
                                  iis_output_hdl->in_sample_rate, \
                                  iis_output_hdl->out_sample_rate);
        }
    }
#endif

    u32 len = audio_iis_output_sync_write(in->data, in->data_len, in->data_sync);
    if (len != in->data_len) {
        local_irq_disable();
        iis_output_hdl->need_resume = 1;
        local_irq_enable();
    }
    return len;
}

static void audio_stream_iis_data_clear(struct audio_stream_entry *entry)
{
#if (IIS_RESAMPLE_USE_BUF_SIZE)
    if (iis_output_hdl->hw_src) {
        // 开关一次src，避免src中残留数据
        audio_stream_iis_src_close();
        iis_output_hdl->out_sample_rate = TCFG_IIS_OUTPUT_SR;
        iis_output_hdl->resample_dir = 0;
        audio_stream_iis_src_open();
    }
#endif
    if (iis_samp_sync && iis_samp_sync_step == 2) {
        iis_samp_sync_step = 1;
    }

    cbuf_clear(&(iis_output_hdl->cbuf));
    iis_debug("iis clear\n");
}

static void audio_iis_src_irq_cb(void *hdl)
{
    if (iis_output_hdl) {
        audio_stream_resume(&(iis_output_hdl->entry));
    }
}

int audio_iis_src_output_handler(void *parm, s16 *data, int len)
{
    int wlen = iis_output_hdl->cbuf.total_len - iis_output_hdl->cbuf.data_len;
    if (wlen > len) {
        wlen = len;
    }
    wlen = cbuf_write(&(iis_output_hdl->cbuf), data, wlen);
    if (wlen != len) {
        local_irq_disable();
        iis_output_hdl->need_resume = 1;
        local_irq_enable();
    }
    return wlen;
}

int audio_iis_output_sync_write(s16 *data, u32 len, u8 data_sync)
{
    u32 wlen = 0;

    if (data_sync) {
        if (len && iis_samp_sync && iis_samp_sync_step == 1 && iis_output_hdl) {
            iis_samp_sync_step = 2;
            audio_sample_sync_output_begin(iis_samp_sync, 0, iis_output_hdl->in_sample_rate);
        }
    } else {
        if (iis_samp_sync_step == 2) {
            iis_samp_sync_step = 1;
            if (iis_samp_sync) {
                audio_sample_sync_stop(iis_samp_sync);
            } else {
                iis_samp_sync_step = 0;
            }
        }
    }

    if (len) {
        wlen = audio_iis_output_write(data, len);
    }

    return wlen;
}


int audio_iis_output_write(s16 *data, u32 len)
{
    u32 wlen = 0;
    if (iis_output_hdl) {
        if (iis_output_hdl->hw_src) {
            wlen = audio_src_resample_write(iis_output_hdl->hw_src, data, len);
        } else {
            wlen = iis_output_hdl->cbuf.total_len - iis_output_hdl->cbuf.data_len;
            if (wlen > len) {
                wlen = len;
            }
            wlen = cbuf_write(&(iis_output_hdl->cbuf), data, len);
        }
        if (wlen != len) {
            local_irq_disable();
            iis_output_hdl->need_resume = 1;
            local_irq_enable();
        }
    }
    return wlen;
}

void audio_iis_tx_isr(u8 idx, s16 *data, u32 len)
{
    u32 wlen = 0;
    u32 rlen = 0;
    if (iis_output_hdl) {
        if (iis_samp_sync && iis_samp_sync_step == 2) {
            audio_irq_update_sample_sync_position(iis_samp_sync, (len >> 1) / 2);
        }
        local_irq_disable();
        if (iis_output_hdl->need_resume) {
            iis_output_hdl->need_resume = 0;
            local_irq_enable();
            audio_stream_resume(&(iis_output_hdl->entry));
        } else {
            local_irq_enable();
        }


#if (!IIS_RESAMPLE_USE_BUF_SIZE)
        iis_output_hdl->points_total += len >> 2;
#endif

        rlen = cbuf_get_data_len(&(iis_output_hdl->cbuf));
        if ((iis_output_hdl->flag == 0) && (rlen < (IIS_TX_BUF_LEN / 2))) {
            memset(data, 0x00, len);
            return;
        }
        iis_output_hdl->flag = 1;

        if (rlen >= len) {
            wlen = cbuf_read(&(iis_output_hdl->cbuf), data, len);
        } else {
            iis_debug("iis end\n");
            wlen = cbuf_read(&(iis_output_hdl->cbuf), data, rlen);
            memset((u8 *)data + rlen, 0x00, len - rlen);
            iis_output_hdl->flag = 0;
        }

    }
}

#if (!IIS_RESAMPLE_USE_BUF_SIZE)
void audio_iis_sr_cal_timer(void *param)
{
    if (iis_output_hdl) {
        if (iis_output_hdl->cnt_total == 0) {
            iis_output_hdl->points_total = iis_output_hdl->out_sample_rate;
        }
        iis_output_hdl->cnt_total++;
        if (iis_output_hdl->cnt_total == 0) {
            iis_output_hdl->cnt_total++;
        }
        iis_output_hdl->out_sample_rate = iis_output_hdl->points_total / iis_output_hdl->cnt_total;

        if ((iis_output_hdl->points_total > 0xFFF00000) \
            && ((iis_output_hdl->cnt_total % 2) == 0)) {
            iis_output_hdl->points_total >>= 1;
            iis_output_hdl->cnt_total >>= 1;
        }


        if (iis_output_hdl->hw_src && iis_output_hdl->out_sample_rate) {
            /* iis_debug("iis cal:%d > %d\n", \ */
            /* iis_output_hdl->in_sample_rate, \ */
            /* iis_output_hdl->out_sample_rate); */
            audio_hw_src_set_rate(iis_output_hdl->hw_src, \
                                  iis_output_hdl->in_sample_rate, \
                                  iis_output_hdl->out_sample_rate);
        }
    }
}
#endif /* (!IIS_RESAMPLE_USE_BUF_SIZE) */

void audio_iis_output_set_srI(u32 sample_rate)
{
    iis_debug("audio_iis_output_set_srI %d\n", sample_rate);
    iis_srI_last = sample_rate;
    if (iis_output_hdl) {
        /* iis_output_hdl->points_total = 0; */
        /* iis_output_hdl->cnt_total = 0; */
        iis_output_hdl->in_sample_rate = sample_rate;
        if (iis_output_hdl->hw_src) {
            iis_debug("iis set:%d > %d\n", \
                      iis_output_hdl->in_sample_rate, \
                      iis_output_hdl->out_sample_rate);
            audio_hw_src_set_rate(iis_output_hdl->hw_src, \
                                  iis_output_hdl->in_sample_rate, \
                                  iis_output_hdl->out_sample_rate);
        }
    }
}

static void audio_stream_iis_src_open(void)
{
    if (iis_output_hdl->hw_src) {
        return ;
    }
    iis_output_hdl->hw_src = zalloc(sizeof(struct audio_src_handle));
    if (iis_output_hdl->hw_src) {
        audio_hw_src_open(iis_output_hdl->hw_src, 2, SRC_TYPE_RESAMPLE);
        audio_hw_src_set_rate(iis_output_hdl->hw_src, \
                              iis_output_hdl->in_sample_rate, \
                              iis_output_hdl->out_sample_rate);
        iis_debug("audio iis rate:%d -> %d\n", \
                  iis_output_hdl->in_sample_rate, \
                  iis_output_hdl->out_sample_rate);
        audio_src_set_output_handler(iis_output_hdl->hw_src, NULL, audio_iis_src_output_handler);
        audio_src_set_rise_irq_handler(iis_output_hdl->hw_src, NULL, audio_iis_src_irq_cb);
    } else {
        iis_debug("hw_src malloc err ==============\n");
    }
}

static void audio_stream_iis_src_close(void)
{
    if (iis_output_hdl->hw_src) {
        audio_hw_src_stop(iis_output_hdl->hw_src);
        audio_hw_src_close(iis_output_hdl->hw_src);
        free(iis_output_hdl->hw_src);
        iis_output_hdl->hw_src = NULL;
    }
}

void *audio_iis_output_start(ALINK_PORT port, u8 ch)
{
    if (iis_output_hdl == NULL) {
        iis_debug("audio_iis_output_start\n");
        mem_stats();
        iis_output_hdl = zalloc(sizeof(struct _iis_output));
        if (iis_output_hdl == NULL) {
            iis_debug("audio_iis_output_start malloc err !\n");
            return NULL;
        }
        /* clk_set("sys", 96 * 1000000L); */
        /* clk_set_en(0); */
        cbuf_init(&(iis_output_hdl->cbuf), iis_output_hdl->buf, IIS_TX_BUF_LEN);
        iis_output_hdl->in_sample_rate = iis_srI_last;
        iis_output_hdl->out_sample_rate = TCFG_IIS_OUTPUT_SR;
        iis_output_hdl->ch = ch;

#if (!IIS_RESAMPLE_USE_BUF_SIZE)
        iis_output_hdl->sr_cal_timer = sys_hi_timer_add(NULL, audio_iis_sr_cal_timer, 1000);
#endif

        /* audio_stream_iis_src_open(); */

        audio_link_init(port);
        alink_channel_init(port, ch, ALINK_DIR_TX, audio_iis_tx_isr);

        iis_output_hdl->entry.data_handler = audio_stream_iis_data_handler;
        iis_output_hdl->entry.data_clear = audio_stream_iis_data_clear;
        /* mem_stats(); */
        return (&(iis_output_hdl->entry));
    }
    return NULL;
}

void audio_iis_output_stop(ALINK_PORT port)
{
    iis_debug("audio_iis_output_stop\n");
    if (iis_output_hdl) {
        alink_channel_close(port, iis_output_hdl->ch);
        audio_link_uninit(port);

#if (!IIS_RESAMPLE_USE_BUF_SIZE)
        if (iis_output_hdl->sr_cal_timer) {
            sys_hi_timer_del(iis_output_hdl->sr_cal_timer);
        }
#endif

        audio_stream_iis_src_close();

        free(iis_output_hdl);
        iis_output_hdl = NULL;
        /* clk_set_en(1); */
    }
}

static int audio_iis_channel_sync_state_query(void *priv)
{
    int state = SAMPLE_SYNC_DEVICE_IDLE;

    if (iis_output_hdl) {
        state = SAMPLE_SYNC_DEVICE_START;
    }

    return state;
}

static int audio_iis_channel_buf_samples(void *_ch)
{
    if (!iis_output_hdl) {
        return 0;
    }

    /* int timeout = (1000000 / iis_output_hdl->in_sample_rate + 1) * (clk_get("sys") / 1000000); */
    int pcm_num = iis_output_hdl->cbuf.data_len / 2 / 2;

    return pcm_num << PCM_PHASE_BIT;
}


int audio_iis_channel_sync_enable(struct audio_sample_sync *samp_sync)
{
    if (!samp_sync) {
        return -EINVAL;
    }

    iis_samp_sync = samp_sync;
    iis_samp_sync_step = 1;

    struct sample_sync_device_ops device;
    device.data_len = audio_iis_channel_buf_samples;
    device.state_query = audio_iis_channel_sync_state_query;

    audio_sample_sync_set_device(iis_samp_sync, NULL, &device);

    return 0;
}

int audio_iis_channel_sync_disable(struct audio_sample_sync *samp_sync)
{
    if (samp_sync == iis_samp_sync) {
        iis_samp_sync = NULL;
    }
    return 0;
}
#endif //(TCFG_IIS_ENABLE && TCFG_IIS_OUTPUT_EN)





#if (TCFG_IIS_ENABLE && TCFG_IIS_INPUT_EN)

u8 iis_input_state = 0;

void audio_iis_input_start(ALINK_PORT port, u8 ch, void (*handle)(u8 ch, s16 *buf, u32 len))
{
    if (iis_input_state == 0) {
        audio_link_init(port);
        alink_channel_init(port, ch, ALINK_DIR_RX, handle);
        iis_input_state = 1;
    }
}


void audio_iis_input_stop(ALINK_PORT port, u8 ch)
{
    if (iis_input_state) {
        alink_channel_close(port, ch);
        audio_link_uninit(port);
        iis_input_state = 0;
    }
}



#endif //(TCFG_IIS_ENABLE && TCFG_IIS_INPUT_EN)




