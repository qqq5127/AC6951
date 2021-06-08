/*
 ****************************************************************
 *File : audio_dec_fm.c
 *Note :
 *
 ****************************************************************
 */

#include "asm/includes.h"
#include "media/includes.h"
#include "media/pcm_decoder.h"
#include "system/includes.h"
#include "effectrs_sync.h"
#include "application/audio_eq_drc_apply.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_dec.h"
#include "app_config.h"
#include "app_main.h"
#include "audio_enc.h"
#include "audio_dec.h"
#include "clock_cfg.h"
#include "dev_manager.h"
#if TCFG_PCM_ENC2TWS_ENABLE
#include "bt_tws.h"
#endif

#if (RECORDER_MIX_EN)
#include "stream_sync.h"
#endif/*RECORDER_MIX_EN*/

#if TCFG_FM_ENABLE


//////////////////////////////////////////////////////////////////////////////

struct fm_dec_hdl {
    struct audio_stream *stream;	// 音频流
    struct pcm_decoder pcm_dec;		// pcm解码句柄
    struct audio_res_wait wait;		// 资源等待句柄
    struct audio_mixer_ch mix_ch;	// 叠加句柄

#if (RECORDER_MIX_EN)
    struct audio_mixer_ch rec_mix_ch;	// 叠加句柄
    struct __stream_sync *sync;
#endif/*RECORDER_MIX_EN*/

    struct audio_eq_drc *eq_drc;//eq drc句柄
#if TCFG_EQ_DIVIDE_ENABLE
    struct audio_eq_drc *eq_drc_rl_rr;//eq drc句柄
    struct audio_vocal_tract vocal_tract;//声道合并目标句柄
    struct audio_vocal_tract_ch synthesis_ch_fl_fr;//声道合并句柄
    struct audio_vocal_tract_ch synthesis_ch_rl_rr;//声道合并句柄
    struct channel_switch *ch_switch;//声道变换
#endif

    u32 id;				// 唯一标识符，随机值
    u32 start : 1;		// 正在解码
    u32 source : 8;		// fm音频源
    void *fm;			// 底层驱动句柄
};

//////////////////////////////////////////////////////////////////////////////

struct fm_dec_hdl *fm_dec = NULL;	// fm解码句柄


//////////////////////////////////////////////////////////////////////////////
void *fm_eq_drc_open(u16 sample_rate, u8 ch_num);
void fm_eq_drc_close(struct audio_eq_drc *eq_drc);
void *fm_rl_rr_eq_drc_open(u16 sample_rate, u8 ch_num);
void fm_rl_rr_eq_drc_close(struct audio_eq_drc *eq_drc);

int linein_sample_size(void *hdl);
int linein_sample_total(void *hdl);

//////////////////////////////////////////////////////////////////////////////

/*----------------------------------------------------------------------------*/
/**@brief    fm数据填充
   @param    *data: 数据
   @param    len: 数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void fm_sample_output_handler(s16 *data, int len)
{
    struct fm_dec_hdl *dec = fm_dec;
    if ((dec) && (dec->fm) && (dec->start)) {
        fm_inside_output_handler(dec->fm, data, len);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    fm解码释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void fm_dec_relaese(void)
{
    if (fm_dec) {
        audio_decoder_task_del_wait(&decode_task, &fm_dec->wait);
        clock_remove(DEC_FM_CLK);
        local_irq_disable();
        free(fm_dec);
        fm_dec = NULL;
        local_irq_enable();
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    fm解码事件处理
   @param    *decoder: 解码器句柄
   @param    argc: 参数个数
   @param    *argv: 参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void fm_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        if (!fm_dec) {
            log_i("fm_dec handle err ");
            break;
        }

        if (fm_dec->id != argv[1]) {
            log_w("fm_dec id err : 0x%x, 0x%x \n", fm_dec->id, argv[1]);
            break;
        }

        fm_dec_close();
        break;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    fm解码数据输出
   @param    *entry: 音频流句柄
   @param    *in: 输入信息
   @param    *out: 输出信息
   @return   输出长度
   @note     *out未使用
*/
/*----------------------------------------------------------------------------*/
static int fm_dec_data_handler(struct audio_stream_entry *entry,
                               struct audio_data_frame *in,
                               struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct pcm_decoder *pcm_dec = container_of(decoder, struct pcm_decoder, decoder);
    struct fm_dec_hdl *dec = container_of(pcm_dec, struct fm_dec_hdl, pcm_dec);
    if (!dec->start) {
        return 0;
    }
    audio_stream_run(&decoder->entry, in);
    return decoder->process_len;
}

/*----------------------------------------------------------------------------*/
/**@brief    fm解码数据流激活
   @param    *p: 私有句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
AT(.fm_data_code)
static void fm_dec_out_stream_resume(void *p)
{
    struct fm_dec_hdl *dec = p;
    audio_decoder_resume(&dec->pcm_dec.decoder);
}

/*----------------------------------------------------------------------------*/
/**@brief    fm解码激活
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
AT(.fm_data_code)
static void fm_dec_resume(void)
{
    if (fm_dec) {
        audio_decoder_resume(&fm_dec->pcm_dec.decoder);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    计算fm输入采样率
   @param
   @return   采样率
   @note
*/
/*----------------------------------------------------------------------------*/
AT(.fm_data_code)
static int audio_fm_input_sample_rate(void *priv)
{
    struct fm_dec_hdl *dec = (struct fm_dec_hdl *)priv;
    int sample_rate = linein_stream_sample_rate(dec->fm);
    int buf_size = linein_sample_size(dec->fm);
#if TCFG_PCM_ENC2TWS_ENABLE
    if (dec->pcm_dec.dec_no_out_sound) {
        /*TWS的上限在fm输入buffer，下限在tws push buffer*/
        if (buf_size >= linein_sample_total(dec->fm) / 2) {
            sample_rate += (sample_rate * 5 / 10000);
        } else if (tws_api_local_media_trans_check_ready_total() < 1024) {
            sample_rate -= (sample_rate * 5 / 10000);
        }
    }
#endif
    if (buf_size >= (linein_sample_total(dec->fm) * 3 / 4)) {
        sample_rate += (sample_rate * 5 / 10000);
    }
    if (buf_size <= (linein_sample_total(dec->fm) * 1 / 4)) {
        sample_rate -= (sample_rate * 5 / 10000);
    }
    return sample_rate;
}

/*----------------------------------------------------------------------------*/
/**@brief    fm解码开始
   @param
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int fm_dec_start(void)
{
    int err;
    struct fm_dec_hdl *dec = fm_dec;
    struct audio_mixer *p_mixer = &mixer;

    if (!fm_dec) {
        return -EINVAL;
    }

    err = pcm_decoder_open(&dec->pcm_dec, &decode_task);
    if (err) {
        goto __err1;
    }

    // 打开fm驱动
    dec->fm = fm_sample_open(dec->source, dec->pcm_dec.sample_rate);
    linein_sample_set_resume_handler(dec->fm, fm_dec_resume);

    pcm_decoder_set_event_handler(&dec->pcm_dec, fm_dec_event_handler, dec->id);
    pcm_decoder_set_read_data(&dec->pcm_dec, linein_sample_read, dec->fm);
    pcm_decoder_set_data_handler(&dec->pcm_dec, fm_dec_data_handler);

#if TCFG_PCM_ENC2TWS_ENABLE
    {
        // localtws使用sbc等编码转发
        struct audio_fmt enc_f;
        memcpy(&enc_f, &dec->pcm_dec.decoder.fmt, sizeof(struct audio_fmt));
        enc_f.coding_type = AUDIO_CODING_SBC;
        if (dec->pcm_dec.ch_num == 2) { // 如果是双声道数据，localtws在解码时才变成对应声道
            enc_f.channel = 2;
        }
        int ret = localtws_enc_api_open(&enc_f, LOCALTWS_ENC_FLAG_STREAM);
        if (ret == true) {
            dec->pcm_dec.dec_no_out_sound = 1;
            // 重定向mixer
            p_mixer = &g_localtws.mixer;
            // 关闭资源等待。最终会在localtws解码处等待
            audio_decoder_task_del_wait(&decode_task, &dec->wait);
            if (dec->pcm_dec.output_ch_num != enc_f.channel) {
                dec->pcm_dec.output_ch_num = dec->pcm_dec.decoder.fmt.channel = enc_f.channel;
                if (enc_f.channel == 2) {
                    dec->pcm_dec.output_ch_type = AUDIO_CH_LR;
                } else {
                    dec->pcm_dec.output_ch_type = AUDIO_CH_DIFF;
                }
            }
        }
    }
#endif

    if (!dec->pcm_dec.dec_no_out_sound) {
        audio_mode_main_dec_open(AUDIO_MODE_MAIN_STATE_DEC_FM);
    }

    // 设置叠加功能
    audio_mixer_ch_open_head(&dec->mix_ch, p_mixer);
    audio_mixer_ch_set_no_wait(&dec->mix_ch, 1, 10); // 超时自动丢数
#if (RECORDER_MIX_EN)
    audio_mixer_ch_open_head(&dec->rec_mix_ch, &recorder_mixer);
    audio_mixer_ch_set_no_wait(&dec->rec_mix_ch, 1, 10); // 超时自动丢数
#endif/*RECORDER_MIX_EN*/

#if 0
    if (dec->pcm_dec.dec_no_out_sound) {
        // 自动变采样
        audio_mixer_ch_set_src(&dec->mix_ch, 1, 0);
#if (RECORDER_MIX_EN)
        audio_mixer_ch_set_src(&dec->rec_mix_ch, 1, 0);
#endif/*RECORDER_MIX_EN*/
    } else {
        // 根据buf数据量动态变采样
#if (RECORDER_MIX_EN && (TCFG_MIC_EFFECT_ENABLE == 0))
        struct stream_sync_info info = {0};
        info.i_sr = dec->pcm_dec.sample_rate;
        info.o_sr = recorder_mix_get_samplerate();
        /* info.o_sr = audio_output_rate(info.i_sr); */
        info.ch_num = audio_output_channel_num();
        info.priv = dec->fm;
        info.get_total = linein_sample_total;
        info.get_size = linein_sample_size;
        printf("info.i_sr = %d, info.o_sr = %d, ch_num = %d\n", info.i_sr, info.o_sr, info.ch_num);
        dec->sync = stream_sync_open(&info, 1);
#else
        struct audio_mixer_ch_sync_info info = {0};
        info.priv = dec->fm;
        info.get_total = linein_sample_total;
        info.get_size = linein_sample_size;
        audio_mixer_ch_set_sync(&dec->mix_ch, &info, 1, 1);
#endif/*RECORDER_MIX_EN*/
    }
#else /*0*/

    if (dec->pcm_dec.dec_no_out_sound) {
        // 自动变采样
        audio_mixer_ch_follow_resample_enable(&dec->mix_ch, dec, audio_fm_input_sample_rate);
#if (RECORDER_MIX_EN)
        audio_mixer_ch_follow_resample_enable(&dec->rec_mix_ch, dec, audio_fm_input_sample_rate);
#endif/*RECORDER_MIX_EN*/
    } else {
        // 根据buf数据量动态变采样
#if (RECORDER_MIX_EN && (TCFG_MIC_EFFECT_ENABLE == 0))
        struct stream_sync_info info = {0};
        info.i_sr = dec->pcm_dec.sample_rate;
        info.o_sr = recorder_mix_get_samplerate();
        /* info.o_sr = audio_output_rate(info.i_sr); */
        info.ch_num = audio_output_channel_num();
        info.priv = dec->fm;
        info.get_total = linein_sample_total;
        info.get_size = linein_sample_size;
        printf("info.i_sr = %d, info.o_sr = %d, ch_num = %d\n", info.i_sr, info.o_sr, info.ch_num);
        dec->sync = stream_sync_open(&info, 1);
#else
        audio_mixer_ch_follow_resample_enable(&dec->mix_ch, dec, audio_fm_input_sample_rate);
#endif/*RECORDER_MIX_EN*/
    }
#endif  /*0*/

    dec->eq_drc = fm_eq_drc_open(dec->pcm_dec.sample_rate, dec->pcm_dec.output_ch_num);
#if TCFG_EQ_DIVIDE_ENABLE
    dec->eq_drc_rl_rr = fm_rl_rr_eq_drc_open(dec->pcm_dec.sample_rate, dec->pcm_dec.output_ch_num);
    if (dec->eq_drc_rl_rr) {
        audio_vocal_tract_open(&dec->vocal_tract, AUDIO_SYNTHESIS_LEN);
        {
            u8 entry_cnt = 0;
            struct audio_stream_entry *entries[8] = {NULL};
            entries[entry_cnt++] = &dec->vocal_tract.entry;
            entries[entry_cnt++] = &dec->mix_ch.entry;
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

    // 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &dec->pcm_dec.decoder.entry;
#if TCFG_EQ_ENABLE && TCFG_FM_MODE_EQ_ENABLE
    if (dec->eq_drc) {
        entries[entry_cnt++] = &dec->eq_drc->entry;
    }
#endif

#if (RECORDER_MIX_EN)
    if (dec->sync) {
        entries[entry_cnt++] = &dec->sync->entry;
    }
#endif

#if SYS_DIGVOL_GROUP_EN
    void *dvol_entry = sys_digvol_group_ch_open("music_fm", -1, NULL);
    entries[entry_cnt++] = dvol_entry;
#endif // SYS_DIGVOL_GROUP_EN

#if TCFG_EQ_DIVIDE_ENABLE
    if (dec->eq_drc_rl_rr) {
        entries[entry_cnt++] = &dec->synthesis_ch_fl_fr.entry;//四声道eq独立时，该节点后不接节点
    } else {
        if (dec->ch_switch) {
            entries[entry_cnt++] = &dec->ch_switch->entry;
        }
        entries[entry_cnt++] = &dec->mix_ch.entry;
    }
#else
    entries[entry_cnt++] = &dec->mix_ch.entry;
#endif


    // 创建数据流，把所有节点连接起来
    dec->stream = audio_stream_open(dec, fm_dec_out_stream_resume);
    audio_stream_add_list(dec->stream, entries, entry_cnt);
#if TCFG_EQ_DIVIDE_ENABLE
    if (dec->eq_drc_rl_rr) { //接在eq_drc的上一个节点
        audio_stream_add_entry(entries[0], &dec->eq_drc_rl_rr->entry);
        audio_stream_add_entry(&dec->eq_drc_rl_rr->entry, &dec->synthesis_ch_rl_rr.entry);
    }
#endif

#if (RECORDER_MIX_EN)
    audio_stream_add_entry(entries[entry_cnt - 2], &dec->rec_mix_ch.entry);
#endif/*RECORDER_MIX_EN*/

    // 设置音频输出音量
    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);

    // 开始解码
    dec->start = 1;
    err = audio_decoder_start(&dec->pcm_dec.decoder);
    if (err) {
        goto __err3;
    }
    clock_set_cur();
    return 0;
__err3:
    dec->start = 0;
    fm_eq_drc_close(dec->eq_drc);
#if TCFG_EQ_DIVIDE_ENABLE
    fm_rl_rr_eq_drc_close(dec->eq_drc_rl_rr);
    audio_vocal_tract_synthesis_close(&dec->synthesis_ch_fl_fr);
    audio_vocal_tract_synthesis_close(&dec->synthesis_ch_rl_rr);
    audio_vocal_tract_close(&dec->vocal_tract);
    channel_switch_close(&dec->ch_switch);
#endif
    if (dec->fm) {
        local_irq_disable();
        fm_sample_close(dec->fm, dec->source);
        dec->fm = NULL;
        local_irq_enable();
    }


    audio_mixer_ch_close(&dec->mix_ch);
#if (RECORDER_MIX_EN)
    audio_mixer_ch_close(&dec->rec_mix_ch);
    if (dec->sync) {
        stream_sync_close(&dec->sync);
    }
#endif/*RECORDER_MIX_EN*/
#if TCFG_PCM_ENC2TWS_ENABLE
    if (dec->pcm_dec.dec_no_out_sound) {
        dec->pcm_dec.dec_no_out_sound = 0;
        localtws_enc_api_close();
    }
#endif

#if SYS_DIGVOL_GROUP_EN
    sys_digvol_group_ch_close("music_fm");
#endif // SYS_DIGVOL_GROUP_EN


    if (dec->stream) {
        audio_stream_close(dec->stream);
        dec->stream = NULL;
    }

    pcm_decoder_close(&dec->pcm_dec);
__err1:
    fm_dec_relaese();
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    fm解码关闭
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void __fm_dec_close(void)
{
    if (fm_dec && fm_dec->start) {
        fm_dec->start = 0;

        pcm_decoder_close(&fm_dec->pcm_dec);

        local_irq_disable();
        fm_sample_close(fm_dec->fm, fm_dec->source);
        fm_dec->fm = NULL;
        local_irq_enable();

        fm_eq_drc_close(fm_dec->eq_drc);
#if TCFG_EQ_DIVIDE_ENABLE
        fm_rl_rr_eq_drc_close(fm_dec->eq_drc_rl_rr);
        audio_vocal_tract_synthesis_close(&fm_dec->synthesis_ch_fl_fr);
        audio_vocal_tract_synthesis_close(&fm_dec->synthesis_ch_rl_rr);
        audio_vocal_tract_close(&fm_dec->vocal_tract);
        channel_switch_close(&fm_dec->ch_switch);
#endif


        audio_mixer_ch_close(&fm_dec->mix_ch);
#if (RECORDER_MIX_EN)
        audio_mixer_ch_close(&fm_dec->rec_mix_ch);
        if (fm_dec->sync) {
            stream_sync_close(&fm_dec->sync);
        }
#endif/*RECORDER_MIX_EN*/
#if TCFG_PCM_ENC2TWS_ENABLE
        if (fm_dec->pcm_dec.dec_no_out_sound) {
            fm_dec->pcm_dec.dec_no_out_sound = 0;
            localtws_enc_api_close();
        }
#endif
#if SYS_DIGVOL_GROUP_EN
        sys_digvol_group_ch_close("music_fm");
#endif // SYS_DIGVOL_GROUP_EN


        // 先关闭各个节点，最后才close数据流
        if (fm_dec->stream) {
            audio_stream_close(fm_dec->stream);
            fm_dec->stream = NULL;
        }

    }

}

/*----------------------------------------------------------------------------*/
/**@brief    fm解码资源等待
   @param    *wait: 句柄
   @param    event: 事件
   @return   0：成功
   @note     用于多解码打断处理
*/
/*----------------------------------------------------------------------------*/
static int fm_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    log_i("fm_wait_res_handler, event:%d\n", event);
    if (event == AUDIO_RES_GET) {
        // 启动解码
        err = fm_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        // 被打断
        __fm_dec_close();
    }

    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    暂停/启动 fm解码mix ch输出
   @param    pause : 1:暂停   0：启动
   @return   NULL
   @note
*/
/*----------------------------------------------------------------------------*/
void fm_dec_pause_out(u8 pause)
{
    if (!fm_dec) {
        return;
    }
#if 0
    audio_mixer_ch_pause(&fm_dec->mix_ch, pause);
#if (RECORDER_MIX_EN)
    audio_mixer_ch_pause(&fm_dec->rec_mix_ch, pause);
#endif/*RECORDER_MIX_EN*/
    audio_decoder_resume_all(&decode_task);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    打开fm解码
   @param    source: 音频源
   @param    sample_rate: 采样率
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int fm_dec_open(u8 source, u32 sample_rate)
{
    int err;
    struct fm_dec_hdl *dec;
    dec = zalloc(sizeof(*dec));
    if (!dec) {
        return -ENOMEM;
    }
    fm_dec = dec;

    dec->id = rand32();

    dec->source = source;

    dec->pcm_dec.ch_num = 2;
    dec->pcm_dec.output_ch_num = audio_output_channel_num();
    dec->pcm_dec.output_ch_type = audio_output_channel_type();
    dec->pcm_dec.sample_rate = sample_rate;

    dec->wait.priority = 2;
    dec->wait.preemption = 0;
    dec->wait.snatch_same_prio = 1;
    dec->wait.handler = fm_wait_res_handler;

    clock_add(DEC_FM_CLK);


#if TCFG_DEC2TWS_ENABLE
    // 设置localtws重播接口
    localtws_globle_set_dec_restart(fm_dec_push_restart);
#endif

    err = audio_decoder_task_add_wait(&decode_task, &dec->wait);
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    关闭fm解码
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void fm_dec_close(void)
{
    if (!fm_dec) {
        return;
    }

    __fm_dec_close();
    fm_dec_relaese();
    clock_set_cur();
    log_i("fm dec close \n\n ");
}

/*----------------------------------------------------------------------------*/
/**@brief    fm解码重新开始
   @param    id: 文件解码id
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int fm_dec_restart(int id)
{
    if ((!fm_dec) || (id != fm_dec->id)) {
        return -1;
    }
    u8 source = fm_dec->source;
    u32 sample_rate = fm_dec->pcm_dec.sample_rate;
    fm_dec_close();
    int err = fm_dec_open(source, sample_rate);
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    推送fm解码重新开始命令
   @param
   @return   true：成功
   @return   false：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int fm_dec_push_restart(void)
{
    if (!fm_dec) {
        return false;
    }
    int argv[3];
    argv[0] = (int)fm_dec_restart;
    argv[1] = 1;
    argv[2] = (int)fm_dec->id;
    os_taskq_post_type(os_current_task(), Q_CALLBACK, ARRAY_SIZE(argv), argv);
    return true;
}


/*----------------------------------------------------------------------------*/
/**@brief    FM模式 eq drc 打开
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
void *fm_eq_drc_open(u16 sample_rate, u8 ch_num)
{
#if TCFG_EQ_ENABLE

    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

#if TCFG_FM_MODE_EQ_ENABLE
    effect_parm.eq_en = 1;

#if TCFG_DRC_ENABLE
#if TCFG_FM_MODE_DRC_ENABLE
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
#endif
    return NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    FM模式 eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void fm_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
#if TCFG_FM_MODE_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
#if TCFG_DRC_ENABLE
#if TCFG_FM_MODE_DRC_ENABLE
        clock_remove(EQ_DRC_CLK);
#endif
#endif
    }
#endif
#endif
    return;
}
/*----------------------------------------------------------------------------*/
/**@brief    fm模式 RL RR 通道eq drc 打开
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
void *fm_rl_rr_eq_drc_open(u16 sample_rate, u8 ch_num)
{

#if TCFG_EQ_ENABLE

    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};
#if TCFG_FM_MODE_EQ_ENABLE
    effect_parm.eq_en = 1;

#if TCFG_DRC_ENABLE
#if TCFG_FM_MODE_DRC_ENABLE
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
/**@brief    FM模式 RL RR 通道eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void fm_rl_rr_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
#if TCFG_FM_MODE_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
#if TCFG_DRC_ENABLE
#if TCFG_FM_MODE_DRC_ENABLE
        clock_remove(EQ_DRC_CLK);
#endif
#endif
    }
#endif
#endif
    return;
}


#endif
