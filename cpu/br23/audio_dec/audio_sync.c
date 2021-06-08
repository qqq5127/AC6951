#include "board_config.h"
#include "asm/dac.h"
#include "audio_common/audio_iis.h"
#include "media/includes.h"
#include "effectrs_sync.h"
#include "asm/gpio.h"


extern struct audio_dac_hdl dac_hdl;
extern int bt_media_sync_master(u8 type);
extern u8 bt_media_device_online(u8 dev);
extern void *bt_media_sync_open(void);
extern void bt_media_sync_close(void *);
extern int bt_send_audio_sync_data(void *, void *buf, u32 len);
extern void bt_media_sync_set_handler(void *, void *priv,
                                      void (*event_handler)(void *, int *, int));
extern u32 get_bt_slot_time(u8 type, u32 time, int *ret_time, int (*local_us_time)(void));


const static struct audio_tws_conn_ops tws_conn_ops = {
    .open = bt_media_sync_open,
    .set_handler = bt_media_sync_set_handler,
    .close = bt_media_sync_close,
    .master = bt_media_sync_master,
    .online = bt_media_device_online,
    .send = bt_send_audio_sync_data,
};


extern struct audio_dac_channel default_dac;
struct audio_wireless_sync *a2dp_output_sync_open(int sample_rate, int output_sample_rate, u8 channels)
{
    struct audio_wireless_sync_info info = {0};
    struct audio_sample_sync *sample_sync;
    struct audio_wireless_sync *a2dp_sync;

    a2dp_sync = (struct audio_wireless_sync *)zalloc(sizeof(struct audio_wireless_sync));
    if (!a2dp_sync) {
        return NULL;
    }

    sample_sync = audio_sample_sync_open(0);

    /*选择SYNC模块接入到DAC内部*/
    /*audio_dac_add_sample_sync(&dac_hdl, sample_sync, 1);*/

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_IIS)
    audio_iis_channel_sync_enable(sample_sync);
    info.target = AUDIO_SYNC_TARGET_IIS;
#else
    audio_dac_channel_sync_enable(&default_dac, sample_sync);
    info.target = AUDIO_SYNC_TARGET_DAC;
#endif

    info.tws_ops = &tws_conn_ops;
    info.sample_ch = sample_sync;
    info.protocol = WL_PROTOCOL_RTP;
    info.sample_rate = sample_rate;
    info.output_rate = output_sample_rate;
    info.channel = channels;

    a2dp_sync->context = audio_wireless_sync_open(&info, &a2dp_sync->entry);

    a2dp_sync->sample_sync = sample_sync;
    a2dp_sync->resample_entry = &sample_sync->entry;

    return a2dp_sync;
}

void a2dp_output_sync_close(struct audio_wireless_sync *a2dp_sync)
{
    if (a2dp_sync) {
        audio_stream_del_entry(a2dp_sync->entry);
        audio_stream_del_entry(a2dp_sync->resample_entry);
        audio_wireless_sync_close(a2dp_sync->context);
        /*audio_dac_remove_sample_sync(&dac_hdl, a2dp_sync->sample_sync);*/

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_IIS)
        audio_iis_channel_sync_disable(a2dp_sync->sample_sync);
#else
        audio_dac_channel_sync_disable(&default_dac, a2dp_sync->sample_sync);
#endif

        audio_sample_sync_close(a2dp_sync->sample_sync);
        free(a2dp_sync);
    }
}

struct audio_wireless_sync *esco_output_sync_open(int sample_rate, int output_sample_rate, u8 channels)
{
    struct audio_wireless_sync_info info = {0};
    struct audio_sample_sync *sample_sync;
    struct audio_wireless_sync *esco_sync;

    esco_sync = (struct audio_wireless_sync *)zalloc(sizeof(struct audio_wireless_sync));
    if (!esco_sync) {
        return NULL;
    }

    sample_sync = audio_sample_sync_open(0);

    /*选择SYNC模块接入到DAC内部*/
    /*audio_dac_add_sample_sync(&dac_hdl, sample_sync, 1);*/

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_IIS)
    audio_iis_channel_sync_enable(sample_sync);
    info.target = AUDIO_SYNC_TARGET_IIS;
#else
    audio_dac_channel_sync_enable(&default_dac, sample_sync);
    info.target = AUDIO_SYNC_TARGET_DAC;
#endif

    info.tws_ops = &tws_conn_ops;
    info.sample_ch = sample_sync;
    info.protocol = WL_PROTOCOL_SCO;
    info.sample_rate = sample_rate;
    info.output_rate = output_sample_rate;
    info.channel = channels;

    esco_sync->context = audio_wireless_sync_open(&info, &esco_sync->entry);

    esco_sync->sample_sync = sample_sync;
    esco_sync->resample_entry = &sample_sync->entry;

    return esco_sync;
}

void esco_output_sync_close(struct audio_wireless_sync *esco_sync)
{
    if (esco_sync) {
        audio_stream_del_entry(esco_sync->entry);
        audio_stream_del_entry(esco_sync->resample_entry);
        audio_wireless_sync_close(esco_sync->context);

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_IIS)
        audio_iis_channel_sync_disable(esco_sync->sample_sync);
#else
        audio_dac_channel_sync_disable(&default_dac, esco_sync->sample_sync);
#endif

        /*audio_dac_remove_sample_sync(&dac_hdl, esco_sync->sample_sync);*/
        audio_sample_sync_close(esco_sync->sample_sync);
        free(esco_sync);
    }
}

#if TCFG_DEC2TWS_ENABLE

#include "classic/tws_api.h"

extern int tws_api_local_media_trans_get_total_buffer_size(void);

extern u8 is_tws_active_device(void);

static u8 local_tws_media_sync_no_check = 0;
void local_tws_sync_no_check_data_buf(u8 no_check)
{
    local_tws_media_sync_no_check = no_check;
}

u8 local_tws_sync_no_check_data_buf_status(void)
{
    return local_tws_media_sync_no_check;
}

int local_bt_media_sync_master(u8 type)
{
    int state = tws_api_get_tws_state();
    if (!(state & TWS_STA_SIBLING_CONNECTED)) {
        return 1;
    }
    return is_tws_active_device() ? 1 : 0;
}

u8 local_bt_media_device_online(u8 type)
{
    return bt_media_device_online(type);
}

const static struct audio_tws_conn_ops local_tws_conn_ops = {
    .open = bt_media_sync_open,
    .set_handler = bt_media_sync_set_handler,
    .close = bt_media_sync_close,
    .master = local_bt_media_sync_master,
    .online = local_bt_media_device_online,
    .send = bt_send_audio_sync_data,
};

struct audio_wireless_sync *audio_localtws_sync_open(int sample_rate, int output_sample_rate, u8 channels)
{
    struct audio_wireless_sync_info info = {0};
    struct audio_sample_sync *sample_sync;
    struct audio_wireless_sync *localtws_sync;

    localtws_sync = (struct audio_wireless_sync *)zalloc(sizeof(struct audio_wireless_sync));
    if (!localtws_sync) {
        return NULL;
    }

    sample_sync = audio_sample_sync_open(0);
    /*选择SYNC模块接入到DAC内部*/
    /*audio_dac_add_sample_sync(&dac_hdl, sample_sync, 1);*/

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_IIS)
    audio_iis_channel_sync_enable(sample_sync);
    info.target = AUDIO_SYNC_TARGET_IIS;
#else
    audio_dac_channel_sync_enable(&default_dac, sample_sync);
    info.target = AUDIO_SYNC_TARGET_DAC;
#endif

    info.tws_ops = &local_tws_conn_ops;
    info.sample_ch = sample_sync;
    info.protocol = WL_PROTOCOL_RTP;
    info.sample_rate = sample_rate;
    info.output_rate = output_sample_rate;
    info.channel = channels;

    localtws_sync->context = audio_wireless_sync_open(&info, &localtws_sync->entry);

    localtws_sync->sample_sync = sample_sync;
    localtws_sync->resample_entry = &sample_sync->entry;

    return localtws_sync;
}

void audio_localtws_sync_close(struct audio_wireless_sync *localtws_sync)
{
    if (localtws_sync) {
        audio_stream_del_entry(localtws_sync->resample_entry);
        audio_stream_del_entry(localtws_sync->entry);
        audio_wireless_sync_close(localtws_sync->context);
        /*audio_dac_remove_sample_sync(&dac_hdl, localtws_sync->sample_sync);*/

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_IIS)
        audio_iis_channel_sync_disable(localtws_sync->sample_sync);
#else
        audio_dac_channel_sync_disable(&default_dac, localtws_sync->sample_sync);
#endif
        audio_sample_sync_close(localtws_sync->sample_sync);
        free(localtws_sync);
    }
}
#if 0
void *local_tws_play_sync_open(struct audio_decoder *dec, u8 channel, u32 sample_rate, u32 output_rate)
{
    struct audio_wireless_sync_info sync_param;
    /*int total_size = tws_api_local_media_trans_get_total_buffer_size();*/

    sync_param.channel = channel;
    sync_param.tws_ops = &local_tws_conn_ops;
    sync_param.sample_rate = sample_rate;
    sync_param.output_rate = output_rate;
    sync_param.reset_enable = 0;

#if 0
    if (local_tws_media_sync_no_check) {
        sync_param.data_top = total_size;
        sync_param.data_bottom = 0;
    } else {
        sync_param.data_top = 70 * total_size / 100;
        sync_param.data_bottom = 60 * total_size / 100;
    }
    sync_param.begin_size = 50 * total_size / 100;
    sync_param.tws_together_time = SYNC_START_TIME;
    printf("tws sync begin:%d, top:%d, bottom:%d \n", sync_param.begin_size, sync_param.data_top, sync_param.data_bottom);
#endif

    sync_param.protocol = WL_PROTOCOL_RTP;
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
    sync_param.target = AUDIO_SYNC_TARGET_FM;
    sync_param.dev = NULL;
#else
    sync_param.target = AUDIO_SYNC_TARGET_DAC;
    sync_param.dev = &dac_hdl;
#endif

    return audio_wireless_sync_open(&sync_param);
}
#endif
#endif


#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
#define FM_EMIT_BOTTOM  40
#define FM_EMIT_TOP     70
#define MAX_STEP        5
#define ONE_STEP_SIZE   512

#define FM_SYNC_SRC_OUTPUT_SIZE     (256 + 16)
#define FM_SYNC_SRC_INPUT_SIZE      (256)
#define MAX_COUNT_FOR_ADJUST        1
#define MAX_ADJUST_STEP             20
#define ABS(x)                      (x > 0 ? x : (-x))

struct local_fm_sync_handle {
    int top_size;
    int bottom_size;
    u16 sample_rate;
    u16 out_rate;
    u16 base_out_rate;
    u8 channel;
    s8 step_dir_count;
    void *src;
    void *output_addr;
    void *input_addr;
    int output_len;
};

static int local_fm_sync_event_handler(void *priv, enum audio_src_event event, void *arg)
{
    struct local_fm_sync_handle *fm_sync = (struct local_fm_sync_handle *)priv;
    struct audio_src_buffer *b = (struct audio_src_buffer *)arg;

    switch (event) {
    case SRC_EVENT_GET_OUTPUT_BUF:
        b->addr = fm_sync->output_addr;
        b->len = FM_SYNC_SRC_OUTPUT_SIZE;
        break;
    case SRC_EVENT_OUTPUT_DONE:
    case SRC_EVENT_INPUT_DONE:
    case SRC_EVENT_ALL_DONE:
        ASSERT(b->len < FM_SYNC_SRC_OUTPUT_SIZE, ", fm sync output err : %d\n", b->len);
        fm_sync->output_len = b->len;
        break;
    default:
        break;
    }

    return 0;
}

void *local_fm_sync_open(u16 sample_rate, u16 output_rate, u8 channel, u32 total_size)
{
    struct local_fm_sync_handle *fm_sync;

    fm_sync = (struct local_fm_sync_handle *)zalloc(sizeof(struct local_fm_sync_handle));
    if (!fm_sync) {
        return NULL;
    }

    fm_sync->top_size = total_size * FM_EMIT_TOP / 100;
    fm_sync->bottom_size = total_size * FM_EMIT_BOTTOM / 100;
    fm_sync->sample_rate = sample_rate;
    fm_sync->out_rate = output_rate;
    fm_sync->base_out_rate = output_rate;
    fm_sync->channel = channel;
    if (!fm_sync->src) {
        fm_sync->src = zalloc(sizeof(struct audio_src_base_handle));

        if (!fm_sync->src) {
            goto __err;
        }
    }
    fm_sync->input_addr = zalloc(FM_SYNC_SRC_INPUT_SIZE);
    if (!fm_sync->input_addr) {
        goto __err1;
    }
    fm_sync->output_addr = zalloc(FM_SYNC_SRC_OUTPUT_SIZE);
    if (!fm_sync->output_addr) {
        goto __err2;
    }

    audio_src_base_open(fm_sync->src, channel, SRC_TYPE_AUDIO_SYNC);

    audio_src_base_set_rate(fm_sync->src, sample_rate, sample_rate);

    audio_src_base_set_event_handler(fm_sync->src, fm_sync, local_fm_sync_event_handler);

    audio_src_base_set_input_buff(fm_sync->src, fm_sync->input_addr, FM_SYNC_SRC_INPUT_SIZE);

    return fm_sync;

__err2:
    free(fm_sync->input_addr);
__err1:
    free(fm_sync->src);
__err:
    free(fm_sync);
    return NULL;
}

int local_fm_sync_by_emitter(void *sync, void *data, int len, int emitter_data_len)
{
    struct local_fm_sync_handle *fm_sync = (struct local_fm_sync_handle *)sync;
    int diff = 0;
    s8 step = 0;

    if (emitter_data_len < fm_sync->bottom_size) {
        diff = fm_sync->bottom_size - emitter_data_len;
        step = 1;
    } else if (emitter_data_len > fm_sync->top_size) {
        diff = fm_sync->top_size - emitter_data_len;
        step = -1;
    } else {
        if (fm_sync->out_rate < fm_sync->base_out_rate) {
            step = 1;
        } else if (fm_sync->out_rate > fm_sync->base_out_rate) {
            step = -1;
        }
    }

    step += diff / 512;
    if (step < 0) {
        fm_sync->step_dir_count--;
    } else if (step > 0) {
        fm_sync->step_dir_count++;
    } else {
        if (fm_sync->step_dir_count < 0) {
            fm_sync->step_dir_count++;
        } else if (fm_sync->step_dir_count > 0) {
            fm_sync->step_dir_count--;
        }
    }

    if (ABS(fm_sync->step_dir_count) >= MAX_COUNT_FOR_ADJUST) {
        fm_sync->out_rate += step;
        if (fm_sync->out_rate < fm_sync->base_out_rate - MAX_ADJUST_STEP) {
            fm_sync->out_rate = fm_sync->base_out_rate - MAX_ADJUST_STEP;
        } else if (fm_sync->out_rate > fm_sync->base_out_rate + MAX_ADJUST_STEP) {
            fm_sync->out_rate = fm_sync->base_out_rate + MAX_ADJUST_STEP;
        }
        /*printf("--%d--\n", fm_sync->out_rate);*/
        audio_src_base_set_rate(fm_sync->src, fm_sync->sample_rate, fm_sync->out_rate);
        fm_sync->step_dir_count = 0;
    }

    audio_src_base_write(fm_sync->src, data, len);
    audio_src_base_data_flush_out(fm_sync->src);

    /*printf("%d - %d - %d\n", len, fm_sync->output_len, step);*/
    return fm_sync->output_len;
}

void *local_fm_sync_output_addr(void *sync)
{
    struct local_fm_sync_handle *fm_sync = (struct local_fm_sync_handle *)sync;

    return fm_sync->output_addr;
}

void local_fm_sync_close(void *sync)
{
    struct local_fm_sync_handle *fm_sync = (struct local_fm_sync_handle *)sync;
    if (!fm_sync) {
        return;
    }

    if (fm_sync->src) {
        audio_src_base_stop(fm_sync->src);
        audio_src_base_close(fm_sync->src);
        free(fm_sync->src);
    }

    if (fm_sync->input_addr) {
        free(fm_sync->input_addr);
    }

    if (fm_sync->output_addr) {
        free(fm_sync->output_addr);
    }

    free(fm_sync);
}

#endif
