#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "circular_buf.h"
#include "audio_fmtx.h"
#include "fm_emitter/fm_emitter_manage.h"

#if (TCFG_APP_FM_EMITTER_EN)

#define FMTX_DEBUG_ENABLE  	1
#if FMTX_DEBUG_ENABLE
#define fmtx_debug       printf
#else
#define fmtx_debug(...)
#endif

#define FMTX_TX_BUF_LEN 	(4*1024)
struct _fmtx_output {
    u32 in_sample_rate;
    u32 out_sample_rate;
    cbuffer_t cbuf;
    u8 buf[FMTX_TX_BUF_LEN];
    struct audio_src_handle *hw_src;
    struct audio_stream_entry entry;
    int sr_cal_timer;
    u32 points_total;
    u32 cnt_total;
    u8 ch : 4;
    u8 flag : 1;
    u8 need_resume : 1;
} *fmtx_output_hdl = NULL;

static u32 fmtx_srI_last = 0;


static int audio_stream_fmtx_data_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in, struct audio_data_frame *out)

{

    if (fmtx_srI_last  != in->sample_rate) {
        fmtx_debug("in:%d ---%d\n", fmtx_srI_last, in->sample_rate);
        audio_fmtx_output_set_srI(in->sample_rate);
    }

    u32 len = audio_fmtx_output_write(in->data, in->data_len);
    if (len != in->data_len) {
        local_irq_disable();
        fmtx_output_hdl->need_resume = 1;
        local_irq_enable();
    }
    return len;
}

static void audio_stream_fmtx_data_clear(struct audio_stream_entry *entry)
{
    cbuf_clear(&(fmtx_output_hdl->cbuf));
    fmtx_debug("fmtx clear\n");
}

static void audio_fmtx_src_irq_cb(void *hdl)
{
    if (fmtx_output_hdl) {
        audio_stream_resume(&(fmtx_output_hdl->entry));
    }
}

int audio_fmtx_src_output_handler(void *parm, s16 *data, int len)
{
    int wlen = fmtx_output_hdl->cbuf.total_len - fmtx_output_hdl->cbuf.data_len;
    if (wlen > len) {
        wlen = len;
    }
    wlen = cbuf_write(&(fmtx_output_hdl->cbuf), data, wlen);
    if (wlen != len) {
        local_irq_disable();
        fmtx_output_hdl->need_resume = 1;
        local_irq_enable();
    }
    return wlen;
}

int audio_fmtx_output_write(s16 *data, u32 len)
{
    u32 wlen = 0;
    if (fmtx_output_hdl) {
        if (fmtx_output_hdl->hw_src) {
            wlen = audio_src_resample_write(fmtx_output_hdl->hw_src, data, len);
        } else {
            wlen = fmtx_output_hdl->cbuf.total_len - fmtx_output_hdl->cbuf.data_len;
            if (wlen > len) {
                wlen = len;
            }
            wlen = cbuf_write(&(fmtx_output_hdl->cbuf), data, len);
        }
        if (wlen != len) {
            local_irq_disable();
            fmtx_output_hdl->need_resume = 1;
            local_irq_enable();
        }
    }
    return wlen;
}

void audio_fmtx_tx_isr(s16 *data, u32 len)
{
    u32 wlen = 0;
    u32 rlen = 0;
    if (fmtx_output_hdl) {
        local_irq_disable();
        if (fmtx_output_hdl->need_resume) {
            fmtx_output_hdl->need_resume = 0;
            local_irq_enable();
            audio_stream_resume(&(fmtx_output_hdl->entry));
        } else {
            local_irq_enable();
        }


        fmtx_output_hdl->points_total += len >> 2;

        rlen = cbuf_get_data_len(&(fmtx_output_hdl->cbuf));
        if ((fmtx_output_hdl->flag == 0) && (rlen < (FMTX_TX_BUF_LEN / 2))) {
            memset(data, 0x00, len);
            return;
        }
        fmtx_output_hdl->flag = 1;

        if (rlen >= len) {
            wlen = cbuf_read(&(fmtx_output_hdl->cbuf), data, len);
        } else {
            fmtx_debug("fmtx end\n");
            wlen = cbuf_read(&(fmtx_output_hdl->cbuf), data, rlen);
            memset((u8 *)data + rlen, 0x00, len - rlen);
            fmtx_output_hdl->flag = 0;
        }
    }
}

void audio_fmtx_sr_cal_timer(void *param)
{
    if (fmtx_output_hdl) {
        if (fmtx_output_hdl->cnt_total == 0) {
            fmtx_output_hdl->points_total = fmtx_output_hdl->out_sample_rate * 1000;
            fmtx_output_hdl->cnt_total = 999;
        }
        fmtx_output_hdl->cnt_total++;
        fmtx_output_hdl->out_sample_rate = fmtx_output_hdl->points_total / fmtx_output_hdl->cnt_total;

        if ((fmtx_output_hdl->points_total > 0xFFF00000) \
            && ((fmtx_output_hdl->cnt_total % 2) == 0)) {
            fmtx_output_hdl->points_total >>= 1;
            fmtx_output_hdl->cnt_total >>= 1;
        }


        if (fmtx_output_hdl->hw_src && fmtx_output_hdl->out_sample_rate) {
            /* fmtx_debug("fmtx cal:%d > %d\n", \ */
            /* fmtx_output_hdl->in_sample_rate, \ */
            /* fmtx_output_hdl->out_sample_rate); */
            audio_hw_src_set_rate(fmtx_output_hdl->hw_src, \
                                  fmtx_output_hdl->in_sample_rate, \
                                  fmtx_output_hdl->out_sample_rate);
        }
    }
}

void audio_fmtx_output_set_srI(u32 sample_rate)
{
    fmtx_debug("audio_fmtx_output_set_srI %d\n", sample_rate);
    fmtx_srI_last = sample_rate;
    if (fmtx_output_hdl) {
        fmtx_output_hdl->points_total = 0;
        fmtx_output_hdl->cnt_total = 0;
        fmtx_output_hdl->in_sample_rate = sample_rate;
        if (fmtx_output_hdl->hw_src) {
            fmtx_debug("fmtx set:%d > %d\n", \
                       fmtx_output_hdl->in_sample_rate, \
                       fmtx_output_hdl->out_sample_rate);
            audio_hw_src_set_rate(fmtx_output_hdl->hw_src, \
                                  fmtx_output_hdl->in_sample_rate, \
                                  fmtx_output_hdl->out_sample_rate);
        }
    }
}

void *audio_fmtx_output_start(u16 fre)
{
    if (fmtx_output_hdl == NULL) {
        fmtx_debug("audio_fmtx_output_start\n");
        mem_stats();
        fmtx_output_hdl = zalloc(sizeof(struct _fmtx_output));
        if (fmtx_output_hdl == NULL) {
            fmtx_debug("audio_fmtx_output_start malloc err !\n");
            return NULL;
        }
        /* clk_set("sys", 96 * 1000000L); */
        /* clk_set_en(0); */
        cbuf_init(&(fmtx_output_hdl->cbuf), fmtx_output_hdl->buf, FMTX_TX_BUF_LEN);

        fm_emitter_manage_init(fre, audio_fmtx_tx_isr);
        fm_emitter_manage_start();

        fmtx_output_hdl->in_sample_rate = fmtx_srI_last;
        fmtx_output_hdl->out_sample_rate = 41667;
        fmtx_output_hdl->sr_cal_timer = sys_s_hi_timer_add(NULL, audio_fmtx_sr_cal_timer, 1000);

        fmtx_output_hdl->hw_src = zalloc(sizeof(struct audio_src_handle));
        if (fmtx_output_hdl->hw_src) {
            audio_hw_src_open(fmtx_output_hdl->hw_src, 2, SRC_TYPE_RESAMPLE);
            audio_hw_src_set_rate(fmtx_output_hdl->hw_src, \
                                  fmtx_output_hdl->in_sample_rate, \
                                  fmtx_output_hdl->out_sample_rate);
            fmtx_debug("audio fmtx rate:%d -> %d\n", \
                       fmtx_output_hdl->in_sample_rate, \
                       fmtx_output_hdl->out_sample_rate);
            audio_src_set_output_handler(fmtx_output_hdl->hw_src, NULL, audio_fmtx_src_output_handler);
            audio_src_set_rise_irq_handler(fmtx_output_hdl->hw_src, NULL, audio_fmtx_src_irq_cb);
        } else {
            fmtx_debug("hw_src malloc err ==============\n");
        }

        fmtx_output_hdl->entry.data_handler = audio_stream_fmtx_data_handler;
        fmtx_output_hdl->entry.data_clear = audio_stream_fmtx_data_clear;
        /* mem_stats(); */
        return (&(fmtx_output_hdl->entry));
    }
    return NULL;
}

void audio_fmtx_output_stop()
{
    fmtx_debug("audio_fmtx_output_stop\n");
    if (fmtx_output_hdl) {
        fm_emitter_manage_stop();

        if (fmtx_output_hdl->sr_cal_timer) {
            sys_s_hi_timer_del(fmtx_output_hdl->sr_cal_timer);
        }

        if (fmtx_output_hdl->hw_src) {
            audio_hw_src_stop(fmtx_output_hdl->hw_src);
            audio_hw_src_close(fmtx_output_hdl->hw_src);
            free(fmtx_output_hdl->hw_src);
            fmtx_output_hdl->hw_src = NULL;
        }

        free(fmtx_output_hdl);
        fmtx_output_hdl = NULL;
        /* clk_set_en(1); */
    }
}

#endif

