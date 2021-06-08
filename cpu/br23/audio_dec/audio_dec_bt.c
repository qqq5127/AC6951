#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/a2dp_decoder.h"
#include "media/esco_decoder.h"
#include "classic/tws_api.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_dec.h"
#include "audio_digital_vol.h"
#include "clock_cfg.h"
#include "btstack/a2dp_media_codec.h"
#include "application/audio_eq_drc_apply.h"
#include "application/audio_equalloudness.h"
#include "application/audio_surround.h"
#include "application/audio_vbass.h"
#include "aec_user.h"
#include "audio_enc.h"
#include "bt_tws.h"

#if TCFG_ESCO_PLC
#include "PLC.h"
#define PLC_FRAME_LEN	60
#endif/*TCFG_ESCO_PLC*/

#if TCFG_ESCO_LIMITER
#include "limiter_noiseGate_api.h"
/*限幅器上限*/
#define LIMITER_THR	 -10000 /*-12000 = -12dB,放大1000倍,(-10000参考)*/
/*小于CONST_NOISE_GATE的当成噪声处理,防止清0近端声音*/
#define LIMITER_NOISE_GATE  -50000 /*-12000 = -12dB,放大1000倍,(-30000参考)*/
/*低于噪声门限阈值的增益 */
#define LIMITER_NOISE_GAIN  (0 << 30) /*(0~1)*2^30*/
#endif/*TCFG_ESCO_LIMITER*/

#define TCFG_ESCO_USE_SPEC_MIX_LEN		0

extern const int CONFIG_A2DP_DELAY_TIME;

extern const int config_mixer_en;

#define AUDIO_DEC_BT_MIXER_EN		config_mixer_en



//////////////////////////////////////////////////////////////////////////////


struct a2dp_dec_hdl {
    struct a2dp_decoder dec;		// a2dp解码句柄
    struct audio_res_wait wait;		// 资源等待句柄
    struct audio_mixer_ch mix_ch;	// 叠加句柄
#if (RECORDER_MIX_EN)
    struct audio_mixer_ch rec_mix_ch;	// 叠加句柄
#endif//RECORDER_MIX_EN

    struct audio_stream *stream;	// 音频流
    struct audio_eq_drc *eq_drc;    //eq drc句柄
#if AUDIO_EQUALLOUDNESS_CONFIG
    equal_loudness_hdl *loudness;   //等响度句柄
#endif
#if AUDIO_SURROUND_CONFIG
    surround_hdl *surround;         //环绕音效句柄
#endif
#if AUDIO_VBASS_CONFIG
    vbass_hdl *vbass;               //虚拟低音句柄
#endif
    struct audio_wireless_sync *sync;

#if TCFG_EQ_DIVIDE_ENABLE
    struct audio_eq_drc *eq_drc_rl_rr;//eq drc句柄
    struct audio_vocal_tract vocal_tract;//声道合并目标句柄
    struct audio_vocal_tract_ch synthesis_ch_fl_fr;//声道合并句柄
    struct audio_vocal_tract_ch synthesis_ch_rl_rr;//声道合并句柄
    struct channel_switch *ch_switch;//声道变换
#endif


    u8 closing;
    u8 preempted;
    u8 slience;
    int slience_time;
    u32 slience_end_time;
    struct audio_stream_entry *slience_entry;
};

struct esco_dec_hdl {
    struct esco_decoder dec;		// esco解码句柄
    struct audio_res_wait wait;		// 资源等待句柄
    struct audio_mixer_ch mix_ch;	// 叠加句柄
#if (RECORDER_MIX_EN)
    struct audio_mixer_ch rec_mix_ch;	// 叠加句柄
#endif//RECORDER_MIX_EN

    struct audio_stream *stream;	// 音频流
    struct audio_eq_drc *eq_drc;    //eq drc句柄

    struct channel_switch *ch_switch;//声道变换

    u32 tws_mute_en : 1;	// 静音
    u32 remain : 1;			// 未输出完成

#if TCFG_ESCO_PLC
    void *plc;				// 丢包修护
#endif

#if TCFG_ESCO_LIMITER
    void *limiter;			// 限福器
#endif
    struct audio_wireless_sync *sync;
};


//////////////////////////////////////////////////////////////////////////////

struct a2dp_dec_hdl *bt_a2dp_dec = NULL;
struct esco_dec_hdl *bt_esco_dec = NULL;

static u8 a2dp_suspend = 0;
static u16 drop_a2dp_timer = 0;

extern s16 mix_buff[];
extern s16 recorder_mix_buff[];

extern struct audio_dac_hdl dac_hdl;
extern struct audio_dac_channel default_dac;

//////////////////////////////////////////////////////////////////////////////

void *a2dp_eq_drc_open(u16 sample_rate, u8 ch_num);
void a2dp_eq_drc_close(struct audio_eq_drc *eq_drc);
void *a2dp_rl_rr_eq_drc_open(u16 sample_rate, u8 ch_num);
void a2dp_rl_rr_eq_drc_close(struct audio_eq_drc *eq_drc);
void *esco_eq_drc_open(u16 sample_rate, u8 ch_num);
void esco_eq_drc_close(struct audio_eq_drc *eq_drc);
struct audio_wireless_sync *a2dp_output_sync_open(int sample_rate, int output_sample_rate, u8 channel);
void a2dp_output_sync_close(struct audio_wireless_sync *a2dp_sync);
struct audio_wireless_sync *esco_output_sync_open(int sample_rate, int output_sample_rate, u8 channels);
void esco_output_sync_close(struct audio_wireless_sync *esco_sync);

extern int lmp_private_esco_suspend_resume(int flag);

surround_hdl *surround_open_demo(u8 ch_num);
void surround_close(surround_hdl *surround);
vbass_hdl *vbass_open_demo(u16 sample_rate, u8 ch_num);
void vbass_close_demo(vbass_hdl *vbass);


static void bt_dec_stream_run_stop(struct audio_decoder *decoder)
{
#if AUDIO_DAC_MULTI_CHANNEL_ENABLE
    struct audio_data_frame frame = {0};
    struct audio_data_frame output = {0};
    frame.stop = 1;
    frame.channel = audio_output_channel_num();
    frame.sample_rate = app_audio_output_samplerate_get();
    /* audio_stream_run(&default_dac.entry, &frame); */
    default_dac.entry.data_handler(&default_dac.entry, &frame, &output);
    audio_stream_del_entry(&default_dac.entry);
#else /*AUDIO_DAC_MULTI_CHANNEL_ENABLE*/
    audio_dac_stop(&dac_hdl);
#endif /*AUDIO_DAC_MULTI_CHANNEL_ENABLE*/
}

static int audio_dec_slience_data_handler(struct audio_stream_entry *entry,
        struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    out->data = in->data;
    out->data_len = in->data_len;

    if (in->offset == 0) {
        if (bt_a2dp_dec->slience == 0) {
            bt_a2dp_dec->slience_end_time = jiffies + msecs_to_jiffies(bt_a2dp_dec->slience_time);
            memset(in->data, 0, in->data_len);
            bt_a2dp_dec->slience = 1;
        } else if (bt_a2dp_dec->slience == 1) {
            if (time_after(jiffies, bt_a2dp_dec->slience_end_time)) {
                bt_a2dp_dec->slience = 2;
            }
            memset(in->data, 0, in->data_len);
        }
    }
    return in->data_len;
}

static void audio_dec_slience_process_len(struct audio_stream_entry *entry, int len)
{

}

static struct audio_stream_entry *audio_dec_slience_stream_entry(struct a2dp_dec_hdl *dec, int time)
{
    struct audio_stream_entry *entry = zalloc(sizeof(struct audio_stream_entry));
    entry->data_handler = audio_dec_slience_data_handler;
    entry->data_process_len = audio_dec_slience_process_len;
    dec->slience = 0;
    dec->slience_entry = entry;
    dec->slience_time = time;

    return entry;
}

static void audio_dec_slience_entry_free(struct a2dp_dec_hdl *dec)
{
    if (dec->slience_entry) {
        audio_stream_del_entry(dec->slience_entry);
        free(dec->slience_entry);
        dec->slience_entry = NULL;
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码close
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void a2dp_audio_res_close(void)
{
    if (bt_a2dp_dec->dec.start == 0) {
        log_i("bt_a2dp_dec->dec.start == 0");
        return ;
    }

    if (!bt_a2dp_dec->closing) {
        bt_a2dp_dec->preempted = 1;
    }
    bt_a2dp_dec->dec.start = 0;
    a2dp_decoder_close(&bt_a2dp_dec->dec);
    a2dp_eq_drc_close(bt_a2dp_dec->eq_drc);
#if TCFG_EQ_DIVIDE_ENABLE
    a2dp_rl_rr_eq_drc_close(bt_a2dp_dec->eq_drc_rl_rr);
    audio_vocal_tract_synthesis_close(&bt_a2dp_dec->synthesis_ch_fl_fr);
    audio_vocal_tract_synthesis_close(&bt_a2dp_dec->synthesis_ch_rl_rr);
    audio_vocal_tract_close(&bt_a2dp_dec->vocal_tract);
    channel_switch_close(&bt_a2dp_dec->ch_switch);
#endif
    a2dp_output_sync_close(bt_a2dp_dec->sync);
    bt_a2dp_dec->sync = NULL;
    if (AUDIO_DEC_BT_MIXER_EN) {
        audio_mixer_ch_close(&bt_a2dp_dec->mix_ch);
    } else {
        default_dac.entry.prob_handler = NULL;
        bt_dec_stream_run_stop(&bt_a2dp_dec->dec.decoder);
    }

    audio_dec_slience_entry_free(bt_a2dp_dec);

#if AUDIO_SURROUND_CONFIG
    surround_close(bt_a2dp_dec->surround);
#endif

#if AUDIO_VBASS_CONFIG
    vbass_close_demo(bt_a2dp_dec->vbass);
#endif
#if (RECORDER_MIX_EN)
    audio_mixer_ch_close(&bt_a2dp_dec->rec_mix_ch);
#endif//RECORDER_MIX_EN

#if SYS_DIGVOL_GROUP_EN
    sys_digvol_group_ch_close("music_a2dp");
#endif // SYS_DIGVOL_GROUP_EN



    // 先关闭各个节点，最后才close数据流
    if (bt_a2dp_dec->stream) {
        audio_stream_close(bt_a2dp_dec->stream);
        bt_a2dp_dec->stream = NULL;
    }

    app_audio_state_exit(APP_AUDIO_STATE_MUSIC);
}

/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void a2dp_dec_release()
{
    audio_decoder_task_del_wait(&decode_task, &bt_a2dp_dec->wait);
    a2dp_drop_frame_stop();

    if (bt_a2dp_dec->dec.coding_type == AUDIO_CODING_SBC) {
        clock_remove(DEC_SBC_CLK);
    } else if (bt_a2dp_dec->dec.coding_type == AUDIO_CODING_AAC) {
        clock_remove(DEC_AAC_CLK);
    }

    local_irq_disable();
    free(bt_a2dp_dec);
    bt_a2dp_dec = NULL;
    local_irq_enable();
}

/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码事件返回
   @param    *decoder: 解码器句柄
   @param    argc: 参数个数
   @param    *argv: 参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void a2dp_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        log_i("AUDIO_DEC_EVENT_END\n");
        a2dp_dec_close();
        break;
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码数据流激活
   @param    *p: 私有句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void a2dp_dec_out_stream_resume(void *p)
{
    struct a2dp_dec_hdl *dec = (struct a2dp_dec_hdl *)p;

    audio_decoder_resume(&dec->dec.decoder);
}

/*----------------------------------------------------------------------------*/
/**@brief    a2dp接受回调
   @param
   @return
   @note	 蓝牙库里面接受到了a2dp音频
*/
/*----------------------------------------------------------------------------*/
void a2dp_rx_notice_to_decode(void)
{
    if (bt_a2dp_dec && bt_a2dp_dec->dec.start) {
        a2dp_decoder_resume_from_bluetooth(&bt_a2dp_dec->dec);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码输出到dac时的预处理
   @param    *entry: 数据流节点
   @param    *in: 输入数据
   @return
   @note	 应用在没有使用mixer情况下传输sync标志
*/
/*----------------------------------------------------------------------------*/
static int a2dp_to_dac_probe_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in)
{
    if (!in->stop/* && in->data_len*/) {
        if (bt_a2dp_dec->sync) {
            in->data_sync = 1;
        }
    }

    return 0;
}

#if 0
// 解码输出数据流节点数据处理回调示例
static int demo_decoder_data_handler(struct audio_stream_entry *entry,
                                     struct audio_data_frame *in,
                                     struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    // 自定义数据处理
    // ...
    put_buf(in->data, 16);
    // 输出到数据流示例。
    // 如果不使用audio_stream流程，输出后，需要把实际输出长度更新到decoder->process_len
    decoder->process_len = 0;
    audio_stream_run(&decoder->entry, in);
    int wlen = decoder->process_len;
    return wlen;
}
#endif

#if 0
// 数据流预处理回调示例
static int demo_stream_mix_prob_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in)
{
    // 该函数中如果返回负数，数据流将截止运行，解码会被挂起
    // 对数据进行处理，如打印一下数据
    /* put_buf(in->data, in->data_len); */
    put_buf(in->data, 16);
    /* memset(in->data, 0, in->data_len); */
    return 0;
}
#endif

#if 0
// 保存原来的数据流数据处理
static void *demo_data_handler_save;
// 数据流data_handler处理
static int demo_new_data_handler(struct audio_stream_entry *entry,
                                 struct audio_data_frame *in,
                                 struct audio_data_frame *out)
{
    // 对数据进行处理
    put_buf(in->data, 8);
    // 调用原来的接口输出，这里就可以保存一下是否输出完，就可以做比较多的处理了
    int wlen = ((int (*)(struct audio_stream_entry *, struct audio_data_frame *, struct audio_data_frame *))demo_data_handler_save)(entry, in, out);
    return wlen;
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief    开始a2dp解码
   @param
   @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int a2dp_dec_start(void)
{
    int err;
    struct audio_fmt *fmt;
    struct a2dp_dec_hdl *dec = bt_a2dp_dec;
    u8 ch_num = 0;
    u8 ch_type = 0;

    if (!bt_a2dp_dec) {
        return -EINVAL;
    }

    log_i("a2dp_dec_start: in\n");

    // 打开a2dp解码
    err = a2dp_decoder_open(&dec->dec, &decode_task);
    if (err) {
        goto __err1;
    }
    // 使能事件回调
    audio_decoder_set_event_handler(&dec->dec.decoder, a2dp_dec_event_handler, 0);

    // 获取解码格式
    err = audio_decoder_get_fmt(&dec->dec.decoder, &fmt);
    if (err) {
        goto __err2;
    }

    // 设置输出声道数和声道类型
    ch_num = audio_output_channel_num();
    ch_type = audio_output_channel_type();
    a2dp_decoder_set_output_channel(&dec->dec, ch_num, ch_type);

    if (AUDIO_DEC_BT_MIXER_EN) {
        audio_mode_main_dec_open(AUDIO_MODE_MAIN_STATE_DEC_A2DP);
        // 配置mixer通道参数
        audio_mixer_ch_open_head(&dec->mix_ch, &mixer); // 挂载到mixer最前面
#if (SOUNDCARD_ENABLE)
        audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 0, BIT(0) | BIT(2) | BIT(3));
        audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 1, BIT(1) | BIT(2) | BIT(3));
#endif/*SOUNDCARD_ENABLE*/
        audio_mixer_ch_set_src(&dec->mix_ch, 1, 0);
        audio_mixer_ch_set_no_wait(&dec->mix_ch, 1, 20); // 超时自动丢数
        audio_mixer_ch_sample_sync_enable(&dec->mix_ch, 1); // 标志为sync通道
        audio_mixer_ch_set_sample_rate(&dec->mix_ch, fmt->sample_rate);
    }

#if (RECORDER_MIX_EN)
    audio_mixer_ch_open_head(&dec->rec_mix_ch, &recorder_mixer); // 挂载到mixer最前面
    audio_mixer_ch_set_src(&dec->rec_mix_ch, 1, 0);
    audio_mixer_ch_set_no_wait(&dec->rec_mix_ch, 1, 10); // 超时自动丢数
    /* audio_mixer_ch_sample_sync_enable(&dec->rec_mix_ch, 1); */
#endif//RECORDER_MIX_EN

    // eq、drc音效
    dec->eq_drc = a2dp_eq_drc_open(fmt->sample_rate, ch_num);

#if TCFG_EQ_DIVIDE_ENABLE
    dec->eq_drc_rl_rr = a2dp_rl_rr_eq_drc_open(fmt->sample_rate, ch_num);
    if (dec->eq_drc_rl_rr) {
        audio_vocal_tract_open(&dec->vocal_tract, AUDIO_SYNTHESIS_LEN);
        {
            u8 entry_cnt = 0;
            struct audio_stream_entry *entries[8] = {NULL};
            entries[entry_cnt++] = &dec->vocal_tract.entry;
            if (AUDIO_DEC_BT_MIXER_EN) {
                entries[entry_cnt++] = &dec->mix_ch.entry;
            } else {
                entries[entry_cnt++] = &default_dac.entry;
            }

            dec->vocal_tract.stream = audio_stream_open(&dec->vocal_tract, audio_vocal_tract_stream_resume);
            audio_stream_add_list(dec->vocal_tract.stream, entries, entry_cnt);
        }
        audio_vocal_tract_synthesis_open(&dec->synthesis_ch_fl_fr, &dec->vocal_tract, FL_FR);
        audio_vocal_tract_synthesis_open(&dec->synthesis_ch_rl_rr, &dec->vocal_tract, RL_RR);
    } else {
        dec->ch_switch = channel_switch_open(AUDIO_CH_QUAD, AUDIO_SYNTHESIS_LEN / 2);
    }

#ifdef CONFIG_MIXER_CYCLIC
    audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 0, BIT(0));
    audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 1, BIT(1));
    audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 2, BIT(2));
    audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 3, BIT(3));
#endif
#endif



#if AUDIO_SURROUND_CONFIG
    //环绕音效
    dec->surround = surround_open_demo(ch_num);
#endif
#if AUDIO_VBASS_CONFIG
    //虚拟低音
    dec->vbass = vbass_open_demo(fmt->sample_rate, ch_num);
#endif
    // sync初始化
    if (AUDIO_DEC_BT_MIXER_EN) {
        dec->sync = a2dp_output_sync_open(fmt->sample_rate, audio_mixer_get_sample_rate(&mixer), ch_num);
    } else {
        dec->sync = a2dp_output_sync_open(fmt->sample_rate, fmt->sample_rate, ch_num);
    }

    /*使能同步，配置延时时间*/
    a2dp_decoder_stream_sync_enable(&dec->dec, dec->sync->context, fmt->sample_rate, CONFIG_A2DP_DELAY_TIME);

#if 0
    // 获取解码输出数据示例
    // 可以重新实现data_handler，在data_handler中再调用数据流输出
    // 或者可以查看紧接着dec->dec.decoder.entry之后的数据流预处理数据
    dec->dec.decoder.entry.data_handler = demo_decoder_data_handler;
#endif
#if 0
    // 获取节点数据示例1
    // 获取传入数据流节点中的数据仅需实现prob_handler接口就可以了
    // 该方式仅用于查看节点数据，或者做一些清零之类的简单动作
    // 这里以获取传入mix节点的数据为例
    dec->mix_ch.entry.prob_handler = demo_stream_mix_prob_handler;
#endif
#if 0
    // 获取节点数据示例2
    // 可以用变量保存原来的数据处理接口，然后重新赋值新的数据处理
    // 这里以获取mix节点的数据为例
    demo_data_handler_save = (void *)dec->mix_ch.entry.data_handler;
    dec->mix_ch.entry.data_handler = demo_new_data_handler;
#endif

    // 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &dec->dec.decoder.entry;
    if (dec->sync) {
        entries[entry_cnt++] = dec->sync->entry;
        entries[entry_cnt++] = dec->sync->resample_entry;
    }
#if TCFG_EQ_ENABLE && TCFG_BT_MUSIC_EQ_ENABLE
    if (dec->eq_drc) {
        entries[entry_cnt++] = &dec->eq_drc->entry;
    }
#endif
    /* if (dec->sync) { */
    /* entries[entry_cnt++] = dec->sync->resample_entry; */
    /* } */

    /* if (dec->loudness){ */
    /* entries[entry_cnt++] = &dec->loudness->entry; */
    /* } */
#if AUDIO_SURROUND_CONFIG
    if (dec->surround) {
        entries[entry_cnt++] = &dec->surround->entry;
    }
#endif

#if AUDIO_VBASS_CONFIG
    if (dec->vbass) {
        entries[entry_cnt++] = &dec->vbass->entry;
    }
#endif
#if SYS_DIGVOL_GROUP_EN
    void *dvol_entry = sys_digvol_group_ch_open("music_a2dp", -1, NULL);
    entries[entry_cnt++] = dvol_entry;
#endif // SYS_DIGVOL_GROUP_EN

#if TCFG_USER_TWS_ENABLE
    if (dec->preempted) {
        dec->preempted = 0;
        entries[entry_cnt++] = audio_dec_slience_stream_entry(dec, 700);
    }
#endif
#if TCFG_EQ_DIVIDE_ENABLE
    if (dec->eq_drc_rl_rr) {
        entries[entry_cnt++] = &dec->synthesis_ch_fl_fr.entry;//四声道eq独立时，该节点后不接节点
    } else {
        if (dec->ch_switch) {
            entries[entry_cnt++] = &dec->ch_switch->entry;
        }
        if (AUDIO_DEC_BT_MIXER_EN) {
            entries[entry_cnt++] = &dec->mix_ch.entry;
        } else {
            entries[entry_cnt++] = &default_dac.entry;
        }
    }
#else
    if (AUDIO_DEC_BT_MIXER_EN) {
        entries[entry_cnt++] = &dec->mix_ch.entry;
    } else {
        default_dac.entry.prob_handler = a2dp_to_dac_probe_handler;
        entries[entry_cnt++] = &default_dac.entry;
    }
#endif

    // 创建数据流，把所有节点连接起来
    dec->stream = audio_stream_open(dec, a2dp_dec_out_stream_resume);
    audio_stream_add_list(dec->stream, entries, entry_cnt);

#if (RECORDER_MIX_EN)
    audio_stream_add_entry(entries[entry_cnt - 2], &dec->rec_mix_ch.entry);
#endif//RECORDER_MIX_EN

#if TCFG_EQ_DIVIDE_ENABLE
    if (dec->eq_drc_rl_rr) { //接在eq_drc的上一个节点
        u8 cnt = 0;
        if (dec->sync) {
            cnt = 2;
        }

        audio_stream_add_entry(entries[cnt], &dec->eq_drc_rl_rr->entry);
        audio_stream_add_entry(&dec->eq_drc_rl_rr->entry, &dec->synthesis_ch_rl_rr.entry);
    }
#endif

    // 设置音频输出类型
    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);

    log_i("dec->ch:%d, fmt->channel:%d\n", dec->dec.ch, fmt->channel);

    // 关闭丢数
    a2dp_drop_frame_stop();
#if TCFG_USER_TWS_ENABLE
    if (tws_network_audio_was_started()) {
        /*a2dp播放中副机加入*/
        tws_network_local_audio_start();
        a2dp_decoder_join_tws(&dec->dec);
    }
#endif

    // 开始解码
    dec->dec.start = 1;
    err = audio_decoder_start(&dec->dec.decoder);
    if (err) {
        goto __err3;
    }

    clock_set_cur();

    return 0;

__err3:
    dec->dec.start = 0;
    a2dp_eq_drc_close(dec->eq_drc);
#if TCFG_EQ_DIVIDE_ENABLE
    a2dp_rl_rr_eq_drc_close(dec->eq_drc_rl_rr);
    audio_vocal_tract_synthesis_close(&dec->synthesis_ch_fl_fr);
    audio_vocal_tract_synthesis_close(&dec->synthesis_ch_rl_rr);
    audio_vocal_tract_close(&dec->vocal_tract);
    channel_switch_close(&dec->ch_switch);
#endif


#if AUDIO_SURROUND_CONFIG
    surround_close(dec->surround);
#endif
#if AUDIO_VBASS_CONFIG
    vbass_close_demo(dec->vbass);
#endif
    if (AUDIO_DEC_BT_MIXER_EN) {
        audio_mixer_ch_close(&dec->mix_ch);
    } else {
        bt_dec_stream_run_stop(&dec->dec.decoder);
    }
#if (RECORDER_MIX_EN)
    audio_mixer_ch_close(&dec->rec_mix_ch);
#endif//RECORDER_MIX_EN

    if (dec->sync) {
        a2dp_output_sync_close(dec->sync);
        dec->sync = NULL;
    }
#if SYS_DIGVOL_GROUP_EN
    sys_digvol_group_ch_close("music_a2dp");
#endif // SYS_DIGVOL_GROUP_EN


    // 先关闭各个节点，最后才close数据流
    if (dec->stream) {
        audio_stream_close(dec->stream);
        dec->stream = NULL;
    }

__err2:
    a2dp_decoder_close(&dec->dec);
__err1:
    a2dp_dec_release();

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    a2dp解码资源等待
   @param    *wait: 句柄
   @param    event: 事件
   @return   0：成功
   @note     用于多解码打断处理
*/
/*----------------------------------------------------------------------------*/
static int a2dp_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    log_i("a2dp_wait_res_handler: %d\n", event);

    if (event == AUDIO_RES_GET) {
        err = a2dp_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        if (bt_a2dp_dec->dec.start) {
            a2dp_audio_res_close();
            a2dp_drop_frame_start();
        }
    }
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    打开a2dp解码
   @param    media_type: 媒体类型
   @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
int a2dp_dec_open(int media_type)
{
    struct a2dp_dec_hdl *dec;

    if (bt_a2dp_dec) {
        return 0;
    }

    log_i("a2dp_dec_open: %d\n", media_type);

    if (a2dp_suspend) {
#if (TCFG_USER_TWS_ENABLE)
        if (tws_api_get_role() == TWS_ROLE_MASTER)
#endif//TCFG_USER_TWS_ENABLE
        {
            if (drop_a2dp_timer == 0) {
                drop_a2dp_timer = sys_timer_add(NULL,
                                                a2dp_media_clear_packet_before_seqn, 100);
            }
        }
        return 0;
    }

    dec = zalloc(sizeof(*dec));
    if (!dec) {
        return -ENOMEM;
    }

    switch (media_type) {
    case A2DP_CODEC_SBC:
        log_i("a2dp_media_type:SBC");
        dec->dec.coding_type = AUDIO_CODING_SBC;
        clock_add(DEC_SBC_CLK);
        break;
    case A2DP_CODEC_MPEG24:
        log_i("a2dp_media_type:AAC");
        dec->dec.coding_type = AUDIO_CODING_AAC;
        clock_add(DEC_AAC_CLK);
        break;
    default:
        log_i("a2dp_media_type unsupoport:%d", media_type);
        free(dec);
        return -EINVAL;
    }

    bt_a2dp_dec = dec;
    dec->wait.priority = 1;		// 解码优先级
    dec->wait.preemption = 0;	// 不使能直接抢断解码
    dec->wait.snatch_same_prio = 1;	// 可抢断同优先级解码
#if SOUNDCARD_ENABLE
    dec->wait.protect = 1;
#endif
    dec->wait.handler = a2dp_wait_res_handler;
    audio_decoder_task_add_wait(&decode_task, &dec->wait);

    if (bt_a2dp_dec && (bt_a2dp_dec->dec.start == 0)) {
        a2dp_drop_frame_start();
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    关闭a2dp解码
   @param
   @return   0: 没有a2dp解码
   @return   1: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
int a2dp_dec_close(void)
{
    if (drop_a2dp_timer) {
        sys_timer_del(drop_a2dp_timer);
        drop_a2dp_timer = 0;
    }
    if (!bt_a2dp_dec) {
        return 0;
    }
    bt_a2dp_dec->closing = 1;

    if (bt_a2dp_dec->dec.start) {
        a2dp_audio_res_close();
    }
    a2dp_dec_release();/*free bt_a2dp_dec*/
    clock_set_cur();
    log_i("a2dp_dec_close: exit\n");
    return 1;
}


/*----------------------------------------------------------------------------*/
/**@brief    a2dp挂起
   @param    *p: 私有参数
   @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
int a2dp_tws_dec_suspend(void *p)
{
    r_printf("a2dp_tws_dec_suspend\n");
    /*mem_stats();*/

    if (a2dp_suspend) {
        return -EINVAL;
    }
    a2dp_suspend = 1;

    if (bt_a2dp_dec) {
        a2dp_dec_close();
        a2dp_media_clear_packet_before_seqn(0);
#if (TCFG_USER_TWS_ENABLE)
        if (tws_api_get_role() == 0)
#endif//TCFG_USER_TWS_ENABLE
        {
            drop_a2dp_timer = sys_timer_add(NULL, a2dp_media_clear_packet_before_seqn, 100);
        }
    }

    int err = audio_decoder_fmt_lock(&decode_task, AUDIO_CODING_AAC);
    if (err) {
        log_e("AAC_dec_lock_faild\n");
    }

    return err;
}


/*----------------------------------------------------------------------------*/
/**@brief    a2dp挂起激活
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void a2dp_tws_dec_resume(void)
{
    r_printf("a2dp_tws_dec_resume\n");

    if (a2dp_suspend) {
        a2dp_suspend = 0;

        if (drop_a2dp_timer) {
            sys_timer_del(drop_a2dp_timer);
            drop_a2dp_timer = 0;
        }

        audio_decoder_fmt_unlock(&decode_task, AUDIO_CODING_AAC);

        int type = a2dp_media_get_codec_type();
        printf("codec_type: %d\n", type);
        if (type >= 0) {
#if (TCFG_USER_TWS_ENABLE)
            if (tws_api_get_role() == 0)
#endif//TCFG_USER_TWS_ENABLE
            {
                a2dp_media_clear_packet_before_seqn(0);
            }
            /* a2dp_resume_time = jiffies + msecs_to_jiffies(80); */
            a2dp_dec_open(type);
        }
    }
}

#if TCFG_ESCO_PLC
/*----------------------------------------------------------------------------*/
/**@brief    esco丢包修护初始化
   @param
   @return   丢包修护句柄
   @note
*/
/*----------------------------------------------------------------------------*/
static void *esco_plc_init(void)
{
    void *plc = malloc(PLC_query()); /*buf_size:1040*/
    //plc = zalloc_mux(PLC_query());
    log_i("PLC_buf:0x%x,size:%d\n", (int)plc, PLC_query());
    if (!plc) {
        return NULL;
    }
    int err = PLC_init(plc);
    if (err) {
        log_i("PLC_init err:%d", err);
        free(plc);
        plc = NULL;
    }
    return plc;
}
/*----------------------------------------------------------------------------*/
/**@brief    esco丢包修护关闭
   @param    *plc: 丢包修护句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_plc_close(void *plc)
{
    free(plc);
}
/*----------------------------------------------------------------------------*/
/**@brief    esco丢包修护运行
   @param    *data: 数据
   @param    len: 数据长度
   @param    repair: 修护标记
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_plc_run(s16 *data, u16 len, u8 repair)
{
    u16 repair_point, tmp_point;
    s16 *p_in, *p_out;
    p_in    = data;
    p_out   = data;
    tmp_point = len / 2;

#if 0	//debug
    static u16 repair_cnt = 0;
    if (repair) {
        repair_cnt++;
        y_printf("[E%d]", repair_cnt);
    } else {
        repair_cnt = 0;
    }
    //log_i("[%d]",point);
#endif/*debug*/

    while (tmp_point) {
        repair_point = (tmp_point > PLC_FRAME_LEN) ? PLC_FRAME_LEN : tmp_point;
        tmp_point = tmp_point - repair_point;
        PLC_run(p_in, p_out, repair_point, repair);
        p_in  += repair_point;
        p_out += repair_point;
    }
}
#endif


#if TCFG_ESCO_LIMITER
/*----------------------------------------------------------------------------*/
/**@brief    esco限福器初始化
   @param    sample_rate: 解码采样率
   @return   限福器句柄
   @note
*/
/*----------------------------------------------------------------------------*/
static void *esco_limiter_init(u16 sample_rate)
{
    void *limiter = malloc(need_limiter_noiseGate_buf(1));
    log_i("limiter size:%d\n", need_limiter_noiseGate_buf(1));
    if (!limiter) {
        return NULL;
    }
    //限幅器启动因子 int32(exp(-0.65/(16000 * 0.005))*2^30)   16000为采样率  0.005 为启动时间(s)
    int limiter_attfactor = 1065053018;
    //限幅器释放因子 int32(exp(-0.15/(16000 * 0.1))*2^30)     16000为采样率  0.1   为释放时间(s)
    int limiter_relfactor = 1073641165;
    //限幅器阈值(mdb)
    //int limiter_threshold = CONST_LIMITER_THR;

    //噪声门限启动因子 int32(exp(-1/(16000 * 0.1))*2^30)       16000为采样率  0.1   为释放时间(s)
    int noiseGate_attfactor = 1073070945;
    //噪声门限释放因子 int32(exp(-1/(16000 * 0.005))*2^30)     16000为采样率  0.005 为启动时间(s)
    int noiseGate_relfactor = 1060403589;
    //噪声门限(mdb)
    //int noiseGate_threshold = -25000;
    //低于噪声门限阈值的增益 (0~1)*2^30
    //int noise
    //Gate_low_thr_gain = 0 << 30;

    if (sample_rate == 8000) {
        limiter_attfactor = 1056434522;
        limiter_relfactor =  1073540516;
        noiseGate_attfactor = 1072400485;
        noiseGate_relfactor =  1047231044;
    }

    limiter_noiseGate_init(limiter,
                           limiter_attfactor,
                           limiter_relfactor,
                           noiseGate_attfactor,
                           noiseGate_relfactor,
                           LIMITER_THR,
                           LIMITER_NOISE_GATE,
                           LIMITER_NOISE_GAIN,
                           sample_rate, 1);

    return limiter;
}
/*----------------------------------------------------------------------------*/
/**@brief    esco限福器关闭
   @param    *limiter: 限福器句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_limiter_close(void *limiter)
{
    free(limiter);
}
#endif /* TCFG_ESCO_LIMITER */


/*----------------------------------------------------------------------------*/
/**@brief    esco解码输出数据处理
   @param    *entry: 数据流入口
   @param    *in: 输入数据
   @param    *out: 输出数据
   @return   处理了多长数据
   @note
*/
/*----------------------------------------------------------------------------*/
static int esco_dec_data_handler(struct audio_stream_entry *entry,
                                 struct audio_data_frame *in,
                                 struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct esco_decoder *esco_dec = container_of(decoder, struct esco_decoder, decoder);
    struct esco_dec_hdl *dec = container_of(esco_dec, struct esco_dec_hdl, dec);

    if (dec->remain == 0) {
        if (dec->tws_mute_en) {
            memset(in->data, 0, in->data_len);
        }

#if TCFG_ESCO_PLC
        if (dec->plc && out && out->data) {
            esco_plc_run(in->data, in->data_len, *(u8 *)out->data);
        }
#endif/*TCFG_ESCO_PLC*/

#if TCFG_ESCO_LIMITER
        if (dec->limiter) {
            limiter_noiseGate_run(dec->limiter, in->data, in->data, in->data_len / 2);
        }
#endif/*TCFG_ESCO_LIMITER*/

    }

    out->no_subsequent = 1;

    struct audio_data_frame out_frame;
    memcpy(&out_frame, in, sizeof(struct audio_data_frame));
    audio_stream_run(&decoder->entry, &out_frame);
    int wlen = decoder->process_len;
    /* int wlen = esco_decoder_output_handler(&dec->dec, in); */


    if (in->data_len != wlen) {
        dec->remain = 1;
    } else {
        dec->remain = 0;
    }
    return wlen;
}


/*----------------------------------------------------------------------------*/
/**@brief    esco解码释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void esco_dec_release()
{
    audio_decoder_task_del_wait(&decode_task, &bt_esco_dec->wait);

    if (bt_esco_dec->dec.coding_type == AUDIO_CODING_MSBC) {
        clock_remove(DEC_MSBC_CLK);
    } else if (bt_esco_dec->dec.coding_type == AUDIO_CODING_CVSD) {
        clock_remove(DEC_CVSD_CLK);
    }

    local_irq_disable();
    free(bt_esco_dec);
    bt_esco_dec = NULL;
    local_irq_enable();
}

/*----------------------------------------------------------------------------*/
/**@brief    esco解码事件返回
   @param    *decoder: 解码器句柄
   @param    argc: 参数个数
   @param    *argv: 参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        log_i("AUDIO_DEC_EVENT_END\n");
        esco_dec_close();
        break;
    }
}

static int esco_to_dac_probe_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in)
{
    if (!in->stop) {
        if (bt_esco_dec->sync) {
            in->data_sync = 1;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    esco解码close
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_audio_res_close(void)
{
    /*
     *先关闭aec，里面有复用到enc的buff，再关闭enc，
     *如果没有buf复用，则没有先后顺序要求。
     */
    if (!bt_esco_dec->dec.start) {
        return ;
    }
    bt_esco_dec->dec.start = 0;
    bt_esco_dec->dec.enc_start = 0;
    audio_aec_close();
    esco_enc_close();
    esco_decoder_close(&bt_esco_dec->dec);
    esco_eq_drc_close(bt_esco_dec->eq_drc);
    spectrum_switch_demo(1);//打开频谱计算
    channel_switch_close(&bt_esco_dec->ch_switch);
    esco_output_sync_close(bt_esco_dec->sync);
    bt_esco_dec->sync = NULL;
    if (AUDIO_DEC_BT_MIXER_EN) {
        audio_mixer_ch_close(&bt_esco_dec->mix_ch);
    } else {
        default_dac.entry.prob_handler = NULL;
        bt_dec_stream_run_stop(&bt_esco_dec->dec.decoder);
    }
#if (RECORDER_MIX_EN)
    audio_mixer_ch_close(&bt_esco_dec->rec_mix_ch);
#endif//RECORDER_MIX_EN


#if TCFG_ESCO_PLC
    if (bt_esco_dec->plc) {
        esco_plc_close(bt_esco_dec->plc);
        bt_esco_dec->plc = NULL;
    }
#endif/*TCFG_ESCO_PLC*/

#if TCFG_ESCO_LIMITER
    if (bt_esco_dec->limiter) {
        esco_limiter_close(bt_esco_dec->limiter);
        bt_esco_dec->limiter = NULL;
    }
#endif /*TCFG_ESCO_LIMITER*/

#if SYS_DIGVOL_GROUP_EN
    sys_digvol_group_ch_close("call_esco");
#endif // SYS_DIGVOL_GROUP_EN


    // 先关闭各个节点，最后才close数据流
    if (bt_esco_dec->stream) {
        audio_stream_close(bt_esco_dec->stream);
        bt_esco_dec->stream = NULL;
    }

#if TCFG_ESCO_USE_SPEC_MIX_LEN
    /*恢复mix_buf的长度*/
    audio_mixer_set_output_buf(&mixer, mix_buff, AUDIO_MIXER_LEN);
#endif /*TCFG_ESCO_USE_SPEC_MIX_LEN*/
    app_audio_state_exit(APP_AUDIO_STATE_CALL);
    bt_esco_dec->dec.start = 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    esco解码数据流激活
   @param    *p: 私有句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_dec_out_stream_resume(void *p)
{
    struct esco_dec_hdl *dec = (struct esco_dec_hdl *)p;

    audio_decoder_resume(&dec->dec.decoder);
}

/*----------------------------------------------------------------------------*/
/**@brief    esco接受回调
   @param
   @return
   @note	 蓝牙库里面接受到了esco音频
*/
/*----------------------------------------------------------------------------*/
void esco_rx_notice_to_decode(void)
{
    if (bt_esco_dec && bt_esco_dec->dec.start) {
        /* audio_decoder_resume(&bt_esco_dec->dec.decoder); */
        if (bt_esco_dec->dec.wait_resume) {
            bt_esco_dec->dec.wait_resume = 0;
            audio_decoder_resume(&bt_esco_dec->dec.decoder);
        }
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    开始esco解码
   @param
   @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int esco_dec_start()
{
    int err;
    struct esco_dec_hdl *dec = bt_esco_dec;

    if (!bt_esco_dec) {
        return -EINVAL;
    }
    // 打开esco解码
    err = esco_decoder_open(&dec->dec, &decode_task);
    if (err) {
        goto __err1;
    }
    // 使能事件回调
    audio_decoder_set_event_handler(&dec->dec.decoder, esco_dec_event_handler, 0);

#if TCFG_ESCO_USE_SPEC_MIX_LEN
    /*
     *(1)bt_esco_dec输出是120或者240，所以通话的时候，修改mix_buff的长度，提高效率
     *(2)其他大部分时候解码输出是512的倍数，通话结束，恢复mix_buff的长度，提高效率
     */
    audio_mixer_set_output_buf(&mixer, mix_buff, AUDIO_MIXER_LEN / 240 * 240);
#endif /*TCFG_ESCO_USE_SPEC_MIX_LEN*/

    if (AUDIO_DEC_BT_MIXER_EN) {
        audio_mode_main_dec_open(AUDIO_MODE_MAIN_STATE_DEC_ESCO);
        // 配置mixer通道参数
        audio_mixer_ch_open_head(&dec->mix_ch, &mixer); // 挂载到mixer最前面
        audio_mixer_ch_set_src(&dec->mix_ch, 1, 0);
        audio_mixer_ch_set_no_wait(&dec->mix_ch, 1, 10); // 超时自动丢数
        audio_mixer_ch_sample_sync_enable(&dec->mix_ch, 1);
        audio_mixer_ch_set_sample_rate(&dec->mix_ch, dec->dec.sample_rate);
    }

#if (RECORDER_MIX_EN)
    audio_mixer_ch_open_head(&dec->rec_mix_ch, &recorder_mixer); // 挂载到mixer最前面
    audio_mixer_ch_set_src(&dec->rec_mix_ch, 1, 0);
    audio_mixer_ch_set_no_wait(&dec->rec_mix_ch, 1, 10); // 超时自动丢数
    audio_mixer_ch_sample_sync_enable(&dec->rec_mix_ch, 1);
    audio_mixer_ch_set_sample_rate(&dec->mix_ch, dec->dec.sample_rate);
    /* audio_mixer_ch_set_sample_rate(&dec->rec_mix_ch, dec->dec.sample_rate); */
    printf("[%s], dec->dec.sample_rate = %d\n", __FUNCTION__, dec->dec.sample_rate);
#endif//RECORDER_MIX_EN

    // eq、drc音效
    dec->eq_drc = esco_eq_drc_open(dec->dec.sample_rate, dec->dec.ch_num);
    spectrum_switch_demo(0);//关闭频谱计算
#ifdef CONFIG_MIXER_CYCLIC
    // mixer_cyclic 中自带声道处理
    audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 0, BIT(0) | BIT(1) | BIT(2) | BIT(3));
#if (RECORDER_MIX_EN)
    audio_mixer_ch_set_aud_ch_out(&dec->rec_mix_ch, 0, BIT(0) | BIT(1) | BIT(2) | BIT(3));
#endif

#else
    if (dec->dec.ch_num != dec->dec.out_ch_num) {
        u8 out_ch_type = AUDIO_CH_DIFF;
        if (dec->dec.out_ch_num == 4) {
            out_ch_type = AUDIO_CH_QUAD;
        } else if (dec->dec.out_ch_num == 2) {
            out_ch_type = AUDIO_CH_LR;
        }
        dec->ch_switch = channel_switch_open(out_ch_type, AUDIO_SYNTHESIS_LEN / 2);
    }
#endif

    dec->dec.decoder.entry.data_handler = esco_dec_data_handler;

    // sync初始化
    if (AUDIO_DEC_BT_MIXER_EN) {
        dec->sync = esco_output_sync_open(dec->dec.sample_rate, audio_mixer_get_sample_rate(&mixer), dec->dec.ch_num);
    } else {
        dec->sync = esco_output_sync_open(dec->dec.sample_rate, dec->dec.sample_rate, dec->dec.ch_num);

    }
    /*使能同步，配置延时时间*/
    esco_decoder_stream_sync_enable(&dec->dec, dec->sync->context, dec->dec.sample_rate, 25);
    // 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &dec->dec.decoder.entry;
    if (dec->sync) {
        entries[entry_cnt++] = dec->sync->entry;
    }
#if TCFG_EQ_ENABLE && TCFG_PHONE_EQ_ENABLE
    if (dec->eq_drc) {
        entries[entry_cnt++] = &dec->eq_drc->entry;
    }
#endif
    if (dec->sync) {
        entries[entry_cnt++] = dec->sync->resample_entry;
    }

    if (dec->ch_switch) {
        entries[entry_cnt++] = &dec->ch_switch->entry;
    }

#if SYS_DIGVOL_GROUP_EN
    void *dvol_entry = sys_digvol_group_ch_open("call_esco", -1, NULL);
    entries[entry_cnt++] = dvol_entry;
#endif // SYS_DIGVOL_GROUP_EN

    if (AUDIO_DEC_BT_MIXER_EN) {
        entries[entry_cnt++] = &dec->mix_ch.entry;
    } else {
        default_dac.entry.prob_handler = esco_to_dac_probe_handler;
        entries[entry_cnt++] = &default_dac.entry;
    }
    // 创建数据流，把所有节点连接起来
    dec->stream = audio_stream_open(dec, esco_dec_out_stream_resume);
    audio_stream_add_list(dec->stream, entries, entry_cnt);

#if (RECORDER_MIX_EN)
    audio_stream_add_entry(entries[entry_cnt - 2], &dec->rec_mix_ch.entry);
#endif//RECORDER_MIX_EN
    // 设置音频输出类型
    audio_output_set_start_volume(APP_AUDIO_STATE_CALL);

#if TCFG_ESCO_PLC
    // 丢包修护
    dec->plc = esco_plc_init();
#endif

#if TCFG_ESCO_LIMITER
    // 限福器
    dec->limiter = esco_limiter_init(dec->dec.sample_rate);
#endif /* TCFG_ESCO_LIMITER */

#if TCFG_USER_TWS_ENABLE
    if (tws_network_audio_was_started()) {
        tws_network_local_audio_start();
        esco_decoder_join_tws(&dec->dec);
    }
#endif
    lmp_private_esco_suspend_resume(2);
    // 开始解码
    dec->dec.start = 1;
    err = audio_decoder_start(&dec->dec.decoder);
    if (err) {
        goto __err3;
    }
    dec->dec.frame_get = 0;

    err = audio_aec_init(dec->dec.sample_rate);
    if (err) {
        log_i("audio_aec_init failed:%d", err);
        //goto __err3;
    }
    err = esco_enc_open(dec->dec.coding_type, dec->dec.esco_len);
    if (err) {
        log_i("audio_enc_open failed:%d", err);
        //goto __err3;
    }
    dec->dec.enc_start = 1;

    clock_set_cur();
    return 0;

__err3:
    dec->dec.start = 0;
    esco_eq_drc_close(dec->eq_drc);
    spectrum_switch_demo(1);//打开频谱计算
    channel_switch_close(&dec->ch_switch);
    if (AUDIO_DEC_BT_MIXER_EN) {
        audio_mixer_ch_close(&dec->mix_ch);
    } else {
        bt_dec_stream_run_stop(&dec->dec.decoder);
    }
#if (RECORDER_MIX_EN)
    audio_mixer_ch_close(&dec->rec_mix_ch);
#endif//RECORDER_MIX_EN

    if (dec->sync) {
        esco_output_sync_close(dec->sync);
        dec->sync = NULL;
    }
#if SYS_DIGVOL_GROUP_EN
    sys_digvol_group_ch_close("call_esco");
#endif // SYS_DIGVOL_GROUP_EN


    if (dec->stream) {
        audio_stream_close(dec->stream);
        dec->stream = NULL;
    }

__err2:
    esco_decoder_close(&dec->dec);
__err1:
    esco_dec_release();
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    esco解码资源等待
   @param    *wait: 句柄
   @param    event: 事件
   @return   0：成功
   @note     用于多解码打断处理
*/
/*----------------------------------------------------------------------------*/
static int esco_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    log_d("esco_wait_res_handler %d\n", event);
    if (event == AUDIO_RES_GET) {
        err = esco_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        if (bt_esco_dec->dec.start) {
            lmp_private_esco_suspend_resume(1);
            esco_audio_res_close();
        }
    }

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    打开esco解码
   @param    *param: 媒体信息
   @param    mutw: 静音
   @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
int esco_dec_open(void *param, u8 mute)
{
    int err;
    struct esco_dec_hdl *dec;
    u32 esco_param = *(u32 *)param;
    int esco_len = esco_param >> 16;
    int codec_type = esco_param & 0x000000ff;

    log_i("esco_dec_open, type=%d,len=%d\n", codec_type, esco_len);

    dec = zalloc(sizeof(*dec));
    if (!dec) {
        return -ENOMEM;
    }

#if TCFG_DEC2TWS_ENABLE
    localtws_media_disable();
#endif

    bt_esco_dec = dec;

    dec->tws_mute_en = mute;
    dec->dec.esco_len = esco_len;
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_LR)

#if !TCFG_EQ_DIVIDE_ENABLE
    dec->dec.out_ch_num = 2;
#else
    dec->dec.out_ch_num = 4;
#endif
#else
    dec->dec.out_ch_num = audio_output_channel_num();
#endif

    printf("dec->dec.out_ch_num %d\n", dec->dec.out_ch_num);
    if (codec_type == 3) {
        dec->dec.coding_type = AUDIO_CODING_MSBC;
        dec->dec.sample_rate = 16000;
        dec->dec.ch_num = 1;
        clock_add(DEC_MSBC_CLK);
    } else if (codec_type == 2) {
        dec->dec.coding_type = AUDIO_CODING_CVSD;
        dec->dec.sample_rate = 8000;
        dec->dec.ch_num = 1;
        clock_add(DEC_CVSD_CLK);
    }

    dec->wait.priority = 2;		// 解码优先级
    dec->wait.preemption = 0;	// 不使能直接抢断解码
    dec->wait.snatch_same_prio = 1;	// 可抢断同优先级解码
    dec->wait.handler = esco_wait_res_handler;
    err = audio_decoder_task_add_wait(&decode_task, &dec->wait);
    if (bt_esco_dec->dec.start == 0) {
        lmp_private_esco_suspend_resume(1);
    }

#if AUDIO_OUTPUT_AUTOMUTE
    extern void mix_out_automute_skip(u8 skip);
    mix_out_automute_skip(1);
#endif

    return err;
}


/*----------------------------------------------------------------------------*/
/**@brief    关闭esco解码
   @param    :
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void esco_dec_close()
{
    if (!bt_esco_dec) {
        return;
    }

    esco_audio_res_close();
    esco_dec_release();

    log_i("esco_dec_close: exit\n");
#if AUDIO_OUTPUT_AUTOMUTE
    extern void mix_out_automute_skip(u8 skip);
    mix_out_automute_skip(0);
#endif
    clock_set_cur();
#if TCFG_DEC2TWS_ENABLE
    localtws_media_enable();
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙音频正在运行
   @param
   @return   1: 正在运行
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_audio_is_running(void)
{
    return (bt_a2dp_dec || bt_esco_dec);
}
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙播放正在运行
   @param
   @return   1: 正在运行
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_media_is_running(void)
{
    return bt_a2dp_dec != NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙电话正在运行
   @param
   @return   1: 正在运行
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_phone_dec_is_running()
{
    return bt_esco_dec != NULL;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙解码idle判断
   @param
   @return   1: idle
   @return   0: busy
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 bt_dec_idle_query()
{
    if (bt_audio_is_running()) {
        return 0;
    }

    return 1;
}
REGISTER_LP_TARGET(bt_dec_lp_target) = {
    .name = "bt_dec",
    .is_idle = bt_dec_idle_query,
};



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 eq drc 打开
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
void *a2dp_eq_drc_open(u16 sample_rate, u8 ch_num)
{

#if TCFG_EQ_ENABLE
    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

#if TCFG_BT_MUSIC_EQ_ENABLE
    effect_parm.eq_en = 1;

#if TCFG_DRC_ENABLE
#if TCFG_BT_MUSIC_DRC_ENABLE
    effect_parm.drc_en = 1;
    effect_parm.drc_cb = drc_get_filter_info;

#endif
#endif


    if (effect_parm.eq_en) {
        effect_parm.async_en = 1;
        effect_parm.out_32bit = 1;
        effect_parm.online_en = 1;
        effect_parm.mode_en = 1;
    }

    effect_parm.eq_name = song_eq_mode;
#if TCFG_EQ_DIVIDE_ENABLE
    effect_parm.divide_en = 1;
#endif


    effect_parm.ch_num = ch_num;
    effect_parm.sr = sample_rate;
    effect_parm.eq_cb = eq_get_filter_info;

    eq_drc = audio_eq_drc_open(&effect_parm);

    clock_add(EQ_CLK);
    if (effect_parm.drc_en) {
        clock_add(EQ_DRC_CLK);
    }
#endif
    return eq_drc;
#endif//TCFG_EQ_ENABLE

    return NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void a2dp_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
#if TCFG_BT_MUSIC_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
#if TCFG_DRC_ENABLE
#if TCFG_BT_MUSIC_DRC_ENABLE
        clock_remove(EQ_DRC_CLK);
#endif
#endif
    }
#endif
#endif
    return;
}


void *a2dp_rl_rr_eq_drc_open(u16 sample_rate, u8 ch_num)
{

#if TCFG_EQ_ENABLE

    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};
#if TCFG_BT_MUSIC_EQ_ENABLE
    effect_parm.eq_en = 1;

#if TCFG_DRC_ENABLE
#if TCFG_BT_MUSIC_DRC_ENABLE
    effect_parm.drc_en = 1;
    effect_parm.drc_cb = drc_get_filter_info;
#endif
#endif

    if (effect_parm.eq_en) {
        effect_parm.async_en = 1;
        effect_parm.out_32bit = 1;
        effect_parm.online_en = 1;
        effect_parm.mode_en = 1;
    }

#if TCFG_EQ_DIVIDE_ENABLE
    effect_parm.divide_en = 1;
    effect_parm.eq_name = rl_eq_mode;
    /* #else */
    /* effect_parm.eq_name = fr_eq_mode; */
#endif

    effect_parm.ch_num = ch_num;
    effect_parm.sr = sample_rate;
    effect_parm.eq_cb = eq_get_filter_info;
    printf("ch_num %d\n,sr %d\n", ch_num, sample_rate);
    eq_drc = audio_eq_drc_open(&effect_parm);

    clock_add(EQ_CLK);
    if (effect_parm.drc_en) {
        clock_add(EQ_DRC_CLK);
    }
#endif
    return eq_drc;
#endif
    return NULL;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 RL RR 通道eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void a2dp_rl_rr_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
#if TCFG_BT_MUSIC_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
#if TCFG_DRC_ENABLE
#if TCFG_BT_MUSIC_DRC_ENABLE
        clock_remove(EQ_DRC_CLK);
#endif
#endif
    }
#endif
#endif
    return;
}


/*----------------------------------------------------------------------------*/
/**@brief    通话模式 eq drc 打开
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
void *esco_eq_drc_open(u16 sample_rate, u8 ch_num)
{
    mix_out_high_bass_dis(AUDIO_EQ_HIGH_BASS_DIS, 1);
#if TCFG_EQ_ENABLE
    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

#if TCFG_PHONE_EQ_ENABLE
    effect_parm.eq_en = 1;

    if (effect_parm.eq_en) {
        effect_parm.async_en = 1;
        effect_parm.online_en = 1;
        effect_parm.mode_en = 0;
    }

    effect_parm.eq_name = call_eq_mode;

    effect_parm.ch_num = ch_num;
    effect_parm.sr = sample_rate;
    effect_parm.eq_cb = eq_phone_get_filter_info;

    eq_drc = audio_eq_drc_open(&effect_parm);

    clock_add(EQ_CLK);
#endif
    return eq_drc;
#endif//TCFG_EQ_ENABLE

    return NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    通话模式 eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void esco_eq_drc_close(struct audio_eq_drc *eq_drc)
{

#if TCFG_EQ_ENABLE
#if TCFG_PHONE_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
    }
#endif
#endif
    mix_out_high_bass_dis(AUDIO_EQ_HIGH_BASS_DIS, 0);
    return;
}



/*----------------------------------------------------------------------------*/
/**@brief    环绕音效切换测试例子
   @param   surround:句柄
   @param   effect_type:
   						EFFECT_3D_PANORAMA,             //3d全景
    					EFFECT_3D_ROTATES,                  //3d环绕
    					EFFECT_FLOATING_VOICE,              //流动人声
    					EFFECT_GLORY_OF_KINGS,              //王者荣耀
    					EFFECT_FOUR_SENSION_BATTLEFIELD,    //四季战场
    					EFFECT_OFF,//原声
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void surround_effect_switch(surround_hdl *surround, u32 effect_type)
{
#if AUDIO_SURROUND_CONFIG
    if (surround) {
        surround_update_parm parm = {0};
        parm.rotatestep = 2;//旋转速度
        parm.damping = 120;//高频衰减速度
        parm.feedback = 110;//整体衰减速度
        parm.roomsize = 128;//空间大小
        if (EFFECT_OFF == effect_type) {
            //中途关测试
            /* u8 en = 0; */
            /* audio_surround_parm_update(surround, effect_type, (surround_update_parm *)en); */

            //此处关闭音效处理使用surround_type = 0的方式
            u8 en = 1;
            audio_surround_parm_update(surround, effect_type, (surround_update_parm *)en);
            parm.surround_type = 0;
            audio_surround_parm_update(surround, EFFECT_3D_PANORAMA, &parm);
        } else {
            //音效切换测试
            u8 en = 1;
            audio_surround_parm_update(surround, EFFECT_OFF, (surround_update_parm *)en);
            if (effect_type == EFFECT_3D_PANORAMA) {
                parm.surround_type = EFFECT_3D_TYPE1;
                audio_surround_parm_update(surround, EFFECT_3D_PANORAMA, &parm);
            } else {
                audio_surround_parm_update(surround, effect_type, NULL);
            }
        }
    }

#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    环绕音效模块打开例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
surround_hdl *surround_open_demo(u8 ch_num)
{
    surround_hdl *surround = NULL;
#if AUDIO_SURROUND_CONFIG
    surround_open_parm parm = {0};
    if (ch_num == 1) {
        ch_num = EFFECT_CH_L;//单声道时，默认做左声道
    }
    parm.channel = ch_num;
    parm.surround_effect_type = EFFECT_3D_PANORAMA;//打开时默认使用3d全景音,使用者，根据需求修改
    surround = audio_surround_open(&parm);

    surround_effect_switch(surround, EFFECT_OFF);//不做处理,使用者根据需求是否关闭该动作
    clock_add(DEC_3D_CLK);
#endif
    return surround;
}
/*----------------------------------------------------------------------------*/
/**@brief    环绕音效关闭例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void surround_close(surround_hdl *surround)
{
#if AUDIO_SURROUND_CONFIG
    if (surround) {
        audio_surround_close(surround);
        surround = NULL;
    }
    clock_remove(DEC_3D_CLK);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    虚拟低音参数更新例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

void vbass_switch(vbass_hdl *vbass, u32 vbass_switch)
{
#if AUDIO_VBASS_CONFIG
    if (vbass) {
        //关虚拟低音例子
        u8 en = vbass_switch;
        audio_vbass_parm_update(vbass, VBASS_SW, (void *)en);
    }
#endif
}


/*----------------------------------------------------------------------------*/
/**@brief    虚拟低音打开例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
vbass_hdl *vbass_open_demo(u16 sample_rate, u8 ch_num)
{
    vbass_hdl *vbass = NULL;

#if AUDIO_VBASS_CONFIG
    vbass_open_parm parm = {0};
    parm.sr = sample_rate;
    parm.channel = ch_num;
    vbass = audio_vbass_open(&parm);

    vbass_update_parm def_parm = {0};
    def_parm.bass_f = 200;
    def_parm.level = 8192;
    audio_vbass_parm_update(vbass, VBASS_UPDATE_PARM, &def_parm);
    clock_add(DEC_VBASS_CLK);
    vbass_switch(vbass, 0);//默认不做处理，使用者根据需要是否关闭该动作
#endif

    return vbass;
}

/*----------------------------------------------------------------------------*/
/**@brief    虚拟低音关闭例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void vbass_close_demo(vbass_hdl *vbass)
{
#if AUDIO_VBASS_CONFIG
    if (vbass) {
        audio_vbass_close(vbass);
        vbass = NULL;
    }
    clock_remove(DEC_VBASS_CLK);
#endif
}



/*----------------------------------------------------------------------------*/
/**@brief    环绕音效、虚拟低音切换例子
   @param    eff_mode:
			KARAOKE_SPK_OST,//原声
			KARAOKE_SPK_DBB,//重低音
			KARAOKE_SPK_SURROUND,//全景环绕
			KARAOKE_SPK_3D,//3d环绕
			KARAOKE_SPK_FLOW_VOICE,//流动人声
			KARAOKE_SPK_KING,//王者荣耀
			KARAOKE_SPK_WAR,//四季战场
			KARAOKE_SPK_MAX,

   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_bt_effect_switch(u8 eff_mode)
{
    if (!bt_a2dp_dec) {
        return;
    }
    log_i("SPK eff***************:%d ", eff_mode);
#if AUDIO_SURROUND_CONFIG
    surround_hdl *surround = bt_a2dp_dec->surround;
#if AUDIO_VBASS_CONFIG
    vbass_hdl *vbass = bt_a2dp_dec->vbass;
    if (eff_mode != KARAOKE_SPK_DBB) {
        vbass_switch(vbass, 0);
    }
#endif
    switch (eff_mode) {
    case KARAOKE_SPK_OST://原声
        surround_effect_switch(surround, EFFECT_OFF);
        log_i("spk_OST\n");
        break;
    case KARAOKE_SPK_DBB://重低音
        /* tone_play_index(IDEX_TONE_VABSS, 1); */
        surround_effect_switch(surround, EFFECT_OFF);//这里要用回原声处理的地方不一样

#if AUDIO_VBASS_CONFIG
        vbass_switch(vbass, 1);
#endif
        log_i("spk_DDB\n");
        break;
    case KARAOKE_SPK_SURROUND://全景环绕
        /* tone_play_index(IDEX_TONE_SURROUND, 1); */
        surround_effect_switch(surround, EFFECT_3D_PANORAMA);
        log_i("spk_SURRROUND\n");
        break;
    case KARAOKE_SPK_3D://3d旋转
        /* tone_play_index(IDEX_TONE_3D, 1); */
        surround_effect_switch(surround, EFFECT_3D_ROTATES);
        log_i("spk_3D\n");
        break;
    case KARAOKE_SPK_FLOW_VOICE://流动人声
        /* tone_play_index(IDEX_TONE_FLOW, 1); */
        surround_effect_switch(surround, EFFECT_FLOATING_VOICE);
        log_i("spk_FLOW\n");
        break;
    case KARAOKE_SPK_KING://王者荣耀
        /* tone_play_index(IDEX_TONE_KING, 1); */
        surround_effect_switch(surround, EFFECT_GLORY_OF_KINGS);
        log_i("spk_KING\n");
        break;
    case KARAOKE_SPK_WAR://四季战场
        /* tone_play_index(IDEX_TONE_WAR, 1); */
        surround_effect_switch(surround, EFFECT_FOUR_SENSION_BATTLEFIELD);
        log_i("spk_WAR\n");
        break;
    default:
        log_i("spk_ERROR\n");
        break;
    }
#endif
}

void audio_file_effect_switch(u8 eff_mode);
//效果切换 测试
void effect_test(void *p)
{
    static u8 cnt = 0;
    audio_file_effect_switch(cnt);
    audio_bt_effect_switch(cnt);
    cnt++;
    if (cnt == KARAOKE_SPK_MAX) {
        cnt = 0;
    }
}


