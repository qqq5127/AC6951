#include "audio_recorder_mix.h"
#include "audio_enc.h"
#include "audio_dec.h"
#include "media/pcm_decoder.h"
#include "btstack/btstack_task.h"
#include "btstack/avctp_user.h"
#include "audio_splicing.h"
#include "app_main.h"
#include "mic_effect.h"
#include "app_task.h"

#if (RECORDER_MIX_EN)

#if (TCFG_AEC_ENABLE)
//AEC回声消除使能情况下此配置必须为1， 由于资源局限蓝牙录音在高级音频和通话之前切换会打断正在进行的录音， 要继续录音需要按键再次触发
#define RECORDER_MIX_BREAK_EN				 1
#else
#define RECORDER_MIX_BREAK_EN				 0
#endif/*TCFG_AEC_ENABLE*/

#if (TCFG_MIC_EFFECT_ENABLE && RECORDER_MIX_BREAK_EN)
#error "TCFG_MIC_EFFECT_ENABLE && RECORDER_MIX_BREAK_EN no support!!\n"
#endif

#define RECORDER_MIX_PCM_INBUF_SIZE	 		 (2*1024L)//
#define RECORDER_MIX_PCM_SYNC_INBUF_SIZE	 (8*1024L)//
#define RECORDER_MIX_MAX_DATA_ONCE			 (512)//不建议修改
#define	RECORDER_MIX_SAMPLERATE				 (32000L)
#define	RECORDER_MIX_SAMPLERATE_LIMIT(sr)	 ((sr<16000)?16000:sr)

#if (TCFG_MIC_EFFECT_ENABLE)
//混响打开之后， 不支持AUDIO_CODING_MP3
#define RECORDER_MIX_CODING_TYPE			 AUDIO_CODING_WAV
#else
#define RECORDER_MIX_CODING_TYPE			 AUDIO_CODING_MP3
#endif

#define REC_ALIN(var,al)     ((((var)+(al)-1)/(al))*(al))

//填0数据流控制句柄
struct __zero_stream {
    struct audio_stream 			*audio_stream;
    struct audio_decoder 			decoder;
    struct audio_res_wait 			wait;
    struct pcm_decoder 				pcm_dec;
    struct audio_mixer_ch 			mix_ch;
    struct audio_mixer_ch 			rec_mix_ch;
};


//通话上行数据流控制句柄
struct __pcm_stream {
    struct audio_stream 			*audio_stream;
    struct audio_decoder 			decoder;
    struct audio_res_wait 			wait;
    struct pcm_decoder 				pcm_dec;
    struct audio_mixer_ch 			rec_mix_ch;
    cbuffer_t cbuf;
    u8 *pcm_buf;
    u8 start;
    u8 busy;
    u8 sync;
};

//混合录音总控制句柄
struct __recorder_mix {
    struct __zero_stream            *zero;
    struct __pcm_stream             *pcm;
    u8								phone_active;
    u8 								sync_en;
    u16								timer;
};

extern u8 mic_effect_get_status(void);

static u16 pcm_sample_rate = 8000L;
static u8 pcm_channel = 1;
static struct recorder_mix_stream *rec_mix = NULL;
struct __recorder_mix *recorder_hdl = NULL;
#define __this	recorder_hdl

void recorder_mix_pcm_set_info(u16 sample_rate, u8 channel)
{
    printf("[%s], sample_rate = %d, channel = %d\n", __FUNCTION__, sample_rate, channel);
    pcm_sample_rate = sample_rate;
    pcm_channel = channel;
}

u32 recorder_mix_pcm_write(u8 *data, u16 len)
{
    u32 wlen = 0;
    if (__this) {
        if (__this->pcm && __this->pcm->start) {
            __this->pcm->busy = 1;
            /* putchar('.'); */
            u32 wlen = cbuf_write(&__this->pcm->cbuf, data, len);
            if (!wlen) {
                putchar('s');
            }

            audio_decoder_resume(&__this->pcm->pcm_dec.decoder);
            __this->pcm->busy = 0;
        }
    }
    return len;
}
//*----------------------------------------------------------------------------*/
/**@brief    通话音频数据混合写接口
   @param
   		data:输入数据地址
		len:输入数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u32 recorder_mix_sco_data_write(u8 *data, u16 len)
{
#if (TCFG_MIC_EFFECT_ENABLE)
    if (mic_effect_get_status()) {
        return 0;
    }
#endif
    return recorder_mix_pcm_write(data, len);
}


static int recorder_mix_pcm_data_max(void *priv)
{
    cbuffer_t *cbuf = (cbuffer_t *)priv;
    return cbuf->total_len;
}

static int recorder_mix_pcm_data_size(void *priv)
{
    cbuffer_t *cbuf = (cbuffer_t *)priv;
    return cbuf_get_data_size(cbuf);
}

static int zero_pcm_fread(void *hdl, void *buf, u32 len)
{
    len = len / 2;
    memset(buf, 0, len);
    /* putchar('A'); */
    return len;
}

static int zero_pcm_dec_data_handler(struct audio_stream_entry *entry,
                                     struct audio_data_frame *in,
                                     struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct pcm_decoder *pcm_dec = container_of(decoder, struct pcm_decoder, decoder);
    audio_stream_run(&decoder->entry, in);
    return decoder->process_len;
}
static void zero_pcm_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
}
static void zero_pcm_dec_out_stream_resume(void *p)
{
    struct __zero_stream *zero = (struct __zero_stream *)p;
    if (zero) {
        audio_decoder_resume(&zero->pcm_dec.decoder);
    }
}
static int zero_pcm_dec_start(struct __zero_stream *zero)
{
    int err = 0;
    if (zero == NULL) {
        return -EINVAL;
    }
    err = pcm_decoder_open(&zero->pcm_dec, &decode_task);
    if (err) {
        return err;
    }
    pcm_decoder_set_event_handler(&zero->pcm_dec, zero_pcm_dec_event_handler, 0);
    pcm_decoder_set_read_data(&zero->pcm_dec, (void *)zero_pcm_fread, zero);
    pcm_decoder_set_data_handler(&zero->pcm_dec, zero_pcm_dec_data_handler);

    audio_mixer_ch_open(&zero->mix_ch, &mixer);
    audio_mixer_ch_set_src(&zero->mix_ch, 1, 0);
    audio_mixer_ch_set_no_wait(&zero->mix_ch, 1, 10); // 超时自动丢数

    audio_mixer_ch_open(&zero->rec_mix_ch, &recorder_mixer);
    //audio_mixer_ch_set_src(&zero->rec_mix_ch, 1, 0);
    audio_mixer_ch_set_no_wait(&zero->rec_mix_ch, 1, 10); // 超时自动丢数

// 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &zero->pcm_dec.decoder.entry;
    entries[entry_cnt++] = &zero->mix_ch.entry;
    zero->audio_stream = audio_stream_open(zero, zero_pcm_dec_out_stream_resume);
    audio_stream_add_list(zero->audio_stream, entries, entry_cnt);

    audio_stream_add_entry(entries[entry_cnt - 2], &zero->rec_mix_ch.entry);

    err = audio_decoder_start(&zero->pcm_dec.decoder);
    if (err == 0) {
        printf("zero_pcm_dec_start ok\n");
    }
    return err;
}
static void zero_pcm_dec_stop(struct __zero_stream *zero)
{
    printf("[%s, %d] \n\n", __FUNCTION__, __LINE__);
    if (zero) {
        /* audio_decoder_close(&zero->decoder); */
        /* audio_mixer_ch_close(&zero->mix_ch);	 */
        pcm_decoder_close(&zero->pcm_dec);
        audio_mixer_ch_close(&zero->mix_ch);
        audio_mixer_ch_close(&zero->rec_mix_ch);
        if (zero->audio_stream) {
            audio_stream_close(zero->audio_stream);
            zero->audio_stream = NULL;
        }
        /* audio_decoder_task_del_wait(&decode_task, &zero->wait); */
    }
}
static int zero_pcm_dec_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    struct __zero_stream *zero = container_of(wait, struct __zero_stream, wait);
    if (event == AUDIO_RES_GET) {
        err = zero_pcm_dec_start(zero);
    } else if (event == AUDIO_RES_PUT) {
        /* zero_pcm_dec_stop(zero); */
    }
    return err;
}
static void zero_pcm_dec_close(struct __zero_stream **hdl)
{
    if (hdl && (*hdl)) {
        struct __zero_stream *zero = *hdl;
        zero_pcm_dec_stop(zero);
        audio_decoder_task_del_wait(&decode_task, &zero->wait);
        local_irq_disable();
        free(zero);
        *hdl = NULL;
        local_irq_enable();
    }
}
static struct __zero_stream *zero_pcm_dec_open(void)
{
    struct __zero_stream *zero = zalloc(sizeof(struct __zero_stream));
    if (zero == NULL) {
        return NULL;
    }
    zero->pcm_dec.ch_num = 2;
    zero->pcm_dec.output_ch_num = audio_output_channel_num();
    zero->pcm_dec.output_ch_type = audio_output_channel_type();
#if (RECORDER_MIX_BREAK_EN == 0)
    zero->pcm_dec.sample_rate = RECORDER_MIX_SAMPLERATE;
#else
    u16 sr = audio_mixer_get_cur_sample_rate(&recorder_mixer);
    zero->pcm_dec.sample_rate = RECORDER_MIX_SAMPLERATE_LIMIT(sr);
#endif

    zero->wait.priority = 0;
    zero->wait.preemption = 0;
    zero->wait.protect = 1;
    zero->wait.handler = zero_pcm_dec_wait_res_handler;
    printf("[%s], zero->pcm_dec.sample_rate:%d\n", __FUNCTION__, zero->pcm_dec.sample_rate);
    int err = audio_decoder_task_add_wait(&decode_task, &zero->wait);
    if (err) {
        printf("[%s, %d] fail\n\n", __FUNCTION__, __LINE__);
        zero_pcm_dec_close(&zero);
    }
    return zero;
}




static int pcm_fread(void *hdl, void *buf, u32 len)
{
    struct __pcm_stream *pcm = (struct __pcm_stream *)hdl;
    if (pcm) {
        if (pcm->start == 0) {
            return 0;
        }
        if (pcm_channel == 1) {
            int rlen = len >> 1;
            s16 *tmp = (s16 *)buf;
            tmp += (rlen >> 1);
            rlen = cbuf_read(&pcm->cbuf, tmp, rlen);
            if (rlen) {
                pcm_single_to_dual(buf, tmp, rlen);
                return len;
            }
        } else {
            return cbuf_read(&pcm->cbuf, buf, len);
        }
    }
    return 0;
}
static int pcm_dec_data_handler(struct audio_stream_entry *entry,
                                struct audio_data_frame *in,
                                struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct pcm_decoder *pcm_dec = container_of(decoder, struct pcm_decoder, decoder);
    audio_stream_run(&decoder->entry, in);
    return decoder->process_len;
}
static void pcm_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
}
static void pcm_dec_out_stream_resume(void *p)
{
    struct __pcm_stream *pcm = (struct __pcm_stream *)p;
    if (pcm) {
        audio_decoder_resume(&pcm->pcm_dec.decoder);
    }
}


static int pcm_dec_start(struct __pcm_stream *pcm)
{
    int err = 0;
    if (pcm == NULL) {
        return -EINVAL;
    }
    err = pcm_decoder_open(&pcm->pcm_dec, &decode_task);
    if (err) {
        return err;
    }
    pcm_decoder_set_event_handler(&pcm->pcm_dec, pcm_dec_event_handler, 0);
    pcm_decoder_set_read_data(&pcm->pcm_dec, (void *)pcm_fread, pcm);
    pcm_decoder_set_data_handler(&pcm->pcm_dec, pcm_dec_data_handler);

    /* audio_mixer_ch_open(&pcm->mix_ch, &mixer); */
    /* audio_mixer_ch_set_src(&pcm->mix_ch, 1, 0); */

    audio_mixer_ch_open(&pcm->rec_mix_ch, &recorder_mixer);
    /* audio_mixer_ch_set_src(&pcm->rec_mix_ch, 1, 0); */
    audio_mixer_ch_set_no_wait(&pcm->rec_mix_ch, 1, 10);

    if (pcm->sync) {
        struct audio_mixer_ch_sync_info info = {0};
        info.priv = &pcm->cbuf;
        info.get_total = recorder_mix_pcm_data_max;
        info.get_size = recorder_mix_pcm_data_size;
        audio_mixer_ch_set_sync(&pcm->rec_mix_ch, &info, 1, 1);
    } else {
        audio_mixer_ch_set_src(&pcm->rec_mix_ch, 1, 0);
    }

// 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &pcm->pcm_dec.decoder.entry;
    entries[entry_cnt++] = &pcm->rec_mix_ch.entry;
    pcm->audio_stream = audio_stream_open(pcm, pcm_dec_out_stream_resume);
    audio_stream_add_list(pcm->audio_stream, entries, entry_cnt);

    /* audio_stream_add_entry(entries[entry_cnt - 2], &pcm->rec_mix_ch.entry); */

    err = audio_decoder_start(&pcm->pcm_dec.decoder);
    if (err == 0) {
        pcm->start = 1;
        printf("pcm_dec_start ok\n");
    } else {
        printf("pcm_dec_start fail %d\n", err);
    }
    return err;
}
static void pcm_dec_stop(struct __pcm_stream *pcm)
{
    printf("[%s, %d] \n\n", __FUNCTION__, __LINE__);
    if (pcm) {
        pcm->start = 0;
        while (pcm->busy) {
            os_time_dly(2);
        }
        printf("[%s, %d] wait busy ok\n\n", __FUNCTION__, __LINE__);
        /* audio_decoder_close(&pcm->decoder); */
        /* audio_mixer_ch_close(&pcm->mix_ch);	 */
        pcm_decoder_close(&pcm->pcm_dec);
        /* audio_mixer_ch_close(&pcm->mix_ch); */
        audio_mixer_ch_close(&pcm->rec_mix_ch);
        if (pcm->audio_stream) {
            audio_stream_close(pcm->audio_stream);
            pcm->audio_stream = NULL;
        }
        /* audio_decoder_task_del_wait(&decode_task, &pcm->wait); */
    }
}
static int pcm_dec_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    struct __pcm_stream *pcm = container_of(wait, struct __pcm_stream, wait);
    if (event == AUDIO_RES_GET) {
        err = pcm_dec_start(pcm);
    } else if (event == AUDIO_RES_PUT) {
        /* pcm_dec_stop(pcm); */
    }
    return err;
}


static void pcm_dec_close(struct __pcm_stream **hdl)
{
    if (hdl && (*hdl)) {
        struct __pcm_stream *pcm = *hdl;
        pcm_dec_stop(pcm);
        audio_decoder_task_del_wait(&decode_task, &pcm->wait);
        local_irq_disable();
        free(pcm);
        *hdl = NULL;
        local_irq_enable();
    }
}

static struct __pcm_stream *pcm_dec_open(u8 sync)
{
    u32 offset = 0;
    u32 size = REC_ALIN(sizeof(struct __pcm_stream), 4);
    u32 cbuf_size;
    if (sync) {
        cbuf_size = RECORDER_MIX_PCM_SYNC_INBUF_SIZE;
    } else {
        cbuf_size = RECORDER_MIX_PCM_INBUF_SIZE;
    }
    size += REC_ALIN(cbuf_size, 4);
    u8 *buf = zalloc(size);
    if (buf == NULL) {
        return NULL;
    }
    struct __pcm_stream *pcm = (struct __pcm_stream *)(buf + offset);
    offset += REC_ALIN(sizeof(struct __pcm_stream), 4);

    pcm->pcm_buf = (buf + offset);
    offset += REC_ALIN(cbuf_size, 4);

    cbuf_init(&pcm->cbuf, pcm->pcm_buf, cbuf_size);
    pcm->sync = sync;
    pcm->pcm_dec.ch_num = 2;
    pcm->pcm_dec.output_ch_num = audio_output_channel_num();
    pcm->pcm_dec.output_ch_type = audio_output_channel_type();
    pcm->pcm_dec.sample_rate = pcm_sample_rate;
    pcm->wait.priority = 0;
    pcm->wait.preemption = 0;
    pcm->wait.protect = 1;
    pcm->wait.handler = pcm_dec_wait_res_handler;
    printf("[%s], pcm->pcm_dec.sample_rate:%d\n", __FUNCTION__, pcm->pcm_dec.sample_rate);
    int err = audio_decoder_task_add_wait(&decode_task, &pcm->wait);
    if (err) {
        printf("[%s, %d] fail\n\n", __FUNCTION__, __LINE__);
        pcm_dec_close(&pcm);
    }
    return pcm;
}

static int recorder_mix_stream_data_handler(struct audio_stream_entry *entry,
        struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    struct recorder_mix_stream *hdl = container_of(entry, struct recorder_mix_stream, entry);
    if (in->data_len == 0) {
        /* printf("data = 0\n"); */
        return 0;
    }
    /* putchar('$'); */

    s16 *data = in->data;//(s16 *)(in->data + in->offset / 2);
    u16 data_len = in->data_len;

    if (data_len >= RECORDER_MIX_MAX_DATA_ONCE) {
        data_len = RECORDER_MIX_MAX_DATA_ONCE;
    }
    int wlen = recorder_userdata_to_enc(data, data_len);
    if (wlen == 0) {
        //数据无法输出直接丢掉， 保证数据流能够顺利激活
        return in->data_len;
    }
    return wlen;
}

static void recorder_mix_stream_output_data_process_len(struct audio_stream_entry *entry,  int len)
{
}

static struct recorder_mix_stream *recorder_mix_stream_open(u8 source)
{
    struct recorder_mix_stream *hdl;
    hdl = zalloc(sizeof(struct recorder_mix_stream));
    hdl->source = source;
    hdl->entry.data_process_len = recorder_mix_stream_output_data_process_len;
    hdl->entry.data_handler = recorder_mix_stream_data_handler;

    return hdl;
}

static void recorder_mix_stream_close(struct recorder_mix_stream *hdl)
{
}

//*----------------------------------------------------------------------------*/
/**@brief    录音数据流节点激活接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void recorder_mix_stream_resume(void)
{
    if (rec_mix)	{
        audio_stream_resume(&rec_mix->entry);
    }
}

static void audio_last_out_stream_resume(void *p)
{
}

static u32 recorder_mix_audio_mixer_check_sr(struct audio_mixer *mixer, u32 sr)
{
    u16 o_sr;
#if (RECORDER_MIX_BREAK_EN == 0)
    o_sr = RECORDER_MIX_SAMPLERATE;
#else
    o_sr = RECORDER_MIX_SAMPLERATE_LIMIT(sr);
    ///对于一些特殊采样率的音频， 如FM的37500, 默认变采样为RECORDER_MIX_SAMPLERATE
    if (o_sr == 37500) {
        o_sr = RECORDER_MIX_SAMPLERATE;
    }
#endif
    //printf("o_sr ============================= %d\n", o_sr);
    return o_sr;
}

//*----------------------------------------------------------------------------*/
/**@brief    混合录音mix节点初始化
   @param
   			mix:通道混合控制句柄
			mix_buf:通道混合后输出的缓存
			buf_size:通道混合输出缓存大小
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void recorder_mix_init(struct audio_mixer *mix, s16 *mix_buf, u16 buf_size)
{
    if (mix == NULL) {
        return ;
    }
    audio_mixer_open(mix);
    audio_mixer_set_event_handler(mix, NULL);
    audio_mixer_set_check_sr_handler(mix, recorder_mix_audio_mixer_check_sr);
    /*初始化mix_buf的长度*/
    audio_mixer_set_output_buf(mix, mix_buf, buf_size);
#ifdef CONFIG_MIXER_CYCLIC
    audio_mixer_set_min_len(mix, buf_size / 2);
#endif/*CONFIG_MIXER_CYCLIC*/
    u8 ch_num = audio_output_channel_num();
    audio_mixer_set_channel_num(mix, ch_num);
    // 固定采样率输出
    audio_mixer_set_sample_rate(mix, MIXER_SR_SPEC, RECORDER_MIX_SAMPLERATE);

    rec_mix = recorder_mix_stream_open(ENCODE_SOURCE_MIX);

    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;

    entries[entry_cnt++] = &mix->entry;
    entries[entry_cnt++] = &rec_mix->entry;

    /* mix->stream = audio_stream_open(NULL, audio_last_out_stream_resume); */
    mix->stream = audio_stream_open(mix, audio_mixer_stream_resume);
    audio_stream_add_list(mix->stream, entries, entry_cnt);

}

static void recorder_mix_err_callback(void)
{
    recorder_mix_stop();
}

static void recorder_mix_destroy(struct __recorder_mix **hdl)
{
    if (hdl == NULL || (*hdl == NULL)) {
        return ;
    }
    struct __recorder_mix *recorder = *hdl;

    zero_pcm_dec_close(&recorder->zero);
    pcm_dec_close(&recorder->pcm);

    local_irq_disable();
    free(*hdl);
    *hdl = NULL;
    local_irq_enable();
}

static struct __recorder_mix *recorder_mix_creat(void)
{
    struct __recorder_mix *recorder = (struct __recorder_mix *)zalloc(sizeof(struct __recorder_mix));
    return recorder;
}

static int __recorder_mix_start(struct __recorder_mix *recorder)
{
    struct record_file_fmt fmt = {0};
    /* char logo[] = {"sd0"}; */		//可以指定设备
    char folder[] = {REC_FOLDER_NAME};         //录音文件夹名称
    char filename[] = {"AC69****"};     //录音文件名，不需要加后缀，录音接口会根据编码格式添加后缀

#if (TCFG_NOR_REC)
    char logo[] = {"rec_nor"};		//外挂flash录音
#elif (FLASH_INSIDE_REC_ENABLE)
    char logo[] = {"rec_sdfile"};		//内置flash录音
#else
    char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));//普通设备录音，获取最后活动设备
#endif

    fmt.dev = logo;
    fmt.folder = folder;
    fmt.filename = filename;
    fmt.channel = audio_output_channel_num();//跟mix通道数一致
    u16 sr = audio_mixer_get_cur_sample_rate(&recorder_mixer);
#if (RECORDER_MIX_BREAK_EN == 0)
    fmt.coding_type = RECORDER_MIX_CODING_TYPE;
    fmt.sample_rate = sr;
#else
    if (recorder->phone_active) {
        fmt.coding_type = AUDIO_CODING_WAV;
        fmt.sample_rate = pcm_sample_rate;
        audio_mixer_set_sample_rate(&recorder_mixer, MIXER_SR_SPEC, pcm_sample_rate);
    } else {
        fmt.coding_type = RECORDER_MIX_CODING_TYPE;
        fmt.sample_rate = RECORDER_MIX_SAMPLERATE;
        audio_mixer_set_sample_rate(&recorder_mixer, MIXER_SR_SPEC, RECORDER_MIX_SAMPLERATE);
    }
#endif/*RECORDER_MIX_BREAK_EN*/

    printf("[%s], fmt.sample_rate = %d\n", __FUNCTION__, fmt.sample_rate);
    fmt.cut_head_time = 300;            //录音文件去头时间,单位ms
    fmt.cut_tail_time = 300;            //录音文件去尾时间,单位ms
    fmt.limit_size = 3000;              //录音文件大小最小限制， 单位byte
    fmt.source = ENCODE_SOURCE_MIX;     //录音输入源
    fmt.err_callback = recorder_mix_err_callback;

    int ret = recorder_encode_start(&fmt);
    if (ret) {
        log_e("[%s] fail !!, dev = %s\n", __FUNCTION__, logo);
    } else {
        log_e("[%s] succ !!, dev = %s\n", __FUNCTION__, logo);
    }
    return ret;
}

static void __recorder_mix_stop(struct __recorder_mix *recorder)
{
    recorder_encode_stop();
}

static void __recorder_mix_timer(void *priv)
{
    u32 sec = recorder_get_encoding_time();
    printf("%d\n", sec);
}

//*----------------------------------------------------------------------------*/
/**@brief    混合录音开始
   @param
   @return   0成功， 非0失败
   @note
   			混合录音支持录制内容：
				BT sbc(高级音频）
				BT sco（蓝牙通话）
				FM（内置FM）
				Linein(外部音源输入)
			录音参数配置：
				请在__recorder_mix_start函数内部修改参数
				1、支持设备选择, 如：sd0、udisk0等
				2、修改文件名称及文件夹名称, 默认文件夹名称为JL_REC，文件名AC69****
				3、编码格式(资源受限，通话支持adpcm wav)
				4、支持砍头砍尾处理
			说明：
				1、录音允许打断配置, 通过RECORDER_MIX_BREAK_EN来配置
					1）录音过程中， 蓝牙音乐播放与通话切换过程， 自动打断， 如需继续录音需要手动启动
						A、该配置支持AEC回声消除，因为回声消除占用cpu及ram资源比较多，所以录音会被打断
						B、编码类型可选， SDK默认是除通话情况下使用wav格式，其他使用mp3
						C、采样率随当前dac的采样率
					2) 录音过程中， 蓝牙音乐播放与通话切换过程， 不允许打断， 录音继续
						A、该配置不支持AEC回声消除,因为该过程固定了编码采样率， 需要较大的ram及cpu资源
						B、编码类型可以选， 开混响情况下，只可以选择WAV， 不开混响可选MP3
						C、编码采样率固定，SDK默认配置采样率为32000, 不建议高于此采样率
						D、录制混响时，会录制混响+背景音乐
				2、混合录音支持蓝牙、FM、LINEIN模式, 其他模式不支持
*/
/*----------------------------------------------------------------------------*/
int recorder_mix_start(void)
{
    if (__this) {
        recorder_mix_stop();
    }

    u8 sync_en = 0;
    u8 pcm_en = 0;
    u8 zero_pcm_en = 0;
    u8 phone_active = 0;
    u8 cur_app = app_get_curr_task();

    switch (cur_app) {
    case APP_BT_TASK:
        sync_en = 0;
        zero_pcm_en = 1;
        break;
    case APP_FM_TASK:
#if (TCFG_MIC_EFFECT_ENABLE)
        printf("record & mic effect, not support!!\n");
        return -1;
#endif
        sync_en = 0;
        zero_pcm_en = 0;
        break;
    case APP_LINEIN_TASK:
        sync_en = 0;
        zero_pcm_en = 1;
        break;
    default:
        log_e("cur app not support recorder mix\n");
        return -1;
    }
    u8 call_status = get_call_status();

#if (RECORDER_MIX_BREAK_EN)
    if ((call_status != BT_CALL_HANGUP) && (call_status == BT_CALL_INCOMING)) {
        printf("phone incomming, no active, no record!!!!\n");
        return -1;
    }

    if (app_var.siri_stu) {
        printf("siri active, no record!!!\n");
        return -1;
    }

    if (call_status != BT_CALL_HANGUP) {
        sync_en = 0;
    }
#endif/*RECORDER_MIX_BREAK_EN*/

    if (call_status != BT_CALL_HANGUP) {
        phone_active = 1;
    }

    if (phone_active
#if (TCFG_MIC_EFFECT_ENABLE)
        || mic_effect_get_status()
#endif/*TCFG_MIC_EFFECT_ENABLE*/
       ) {
        pcm_en = 1;
    }

    struct __recorder_mix *recorder = recorder_mix_creat();
    if (recorder == NULL) {
        return -1;
    }

    recorder->phone_active = phone_active;
    recorder->sync_en = sync_en;

    if (zero_pcm_en) {
        //启动时间轴解码(填充0)， 用于确保混合录音始终有数据写入编码器
        recorder->zero = zero_pcm_dec_open();
        if (recorder->zero == NULL) {
            recorder_mix_destroy(&recorder);
            return -1;
        }
    }

    if (pcm_en) {
        recorder->pcm = pcm_dec_open(recorder->sync_en);
        if (recorder->pcm == NULL) {
            recorder_mix_destroy(&recorder);
            return -1;
        }
    }

    int ret = __recorder_mix_start(recorder);
    if (ret) {
        recorder_mix_destroy(&recorder);
        return -1;
    }


    local_irq_disable();
    __this = recorder;
    local_irq_enable();

    __this->timer = sys_timer_add((void *)__this, __recorder_mix_timer, 1000);

    printf("[%s] start ok\n", __FUNCTION__);
    printf("pcm_en = %d\n", pcm_en);
    printf("zero_pcm_en = %d\n", zero_pcm_en);
    printf("sync_en = %d\n", sync_en);
    printf("phone_active = %d\n", phone_active);

    mem_stats();

    return 0;
}

//*----------------------------------------------------------------------------*/
/**@brief    混合录音停止
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void recorder_mix_stop(void)
{
    if (__this && __this->timer) {
        sys_timer_del(__this->timer);
    }
    __recorder_mix_stop(__this);
    recorder_mix_destroy(&__this);
    printf("[%s] stop ok\n", __FUNCTION__);

    mem_stats();
}
//*----------------------------------------------------------------------------*/
/**@brief    获取混合录音状态
   @param
   @return
  			1:正在录音状态
			0:录音停止状态
   @note
*/
/*----------------------------------------------------------------------------*/
int recorder_mix_get_status(void)
{
    return (__this ? 1 : 0);
}

u16 recorder_mix_get_samplerate(void)
{
    return RECORDER_MIX_SAMPLERATE;
}

u32 recorder_mix_get_coding_type(void)
{
    return RECORDER_MIX_CODING_TYPE;
}

void recorder_mix_call_status_change(u8 active)
{
    printf("[%s] = %d\n", __FUNCTION__, active);
    if (active) {
        if (__this && (__this->phone_active == 0)) {
#if (RECORDER_MIX_BREAK_EN)
            printf("phone rec unactive !!, stop cur rec first\n");
            recorder_mix_stop();
#else
            if (__this->pcm == NULL) {
                __this->pcm = pcm_dec_open(__this->sync_en);
            }
#endif/*RECORDER_MIX_BREAK_EN*/
        }
    } else {
        if (__this) {
#if (RECORDER_MIX_BREAK_EN == 0)
            if (__this->pcm) {
#if (TCFG_MIC_EFFECT_ENABLE)
                if (mic_effect_get_status() == 0)
#endif
                {
                    pcm_dec_close(&__this->pcm);
                }
            }
#endif/*RECORDER_MIX_BREAK_EN*/
        }
    }
}

void recorder_mix_bt_status_event(struct bt_event *e)
{
    switch (e->event) {
    case BT_STATUS_PHONE_INCOME:
        break;
    case BT_STATUS_PHONE_OUT:
        break;
    case BT_STATUS_PHONE_ACTIVE:
        break;
    case BT_STATUS_PHONE_HANGUP:
#if (RECORDER_MIX_BREAK_EN)
        recorder_mix_stop();
#endif
        break;
    case BT_STATUS_PHONE_NUMBER:
        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
        if (e->value != 0xff) {
            //电话激活， 先停止当前录音
#if (RECORDER_MIX_BREAK_EN)
            recorder_mix_stop();
#endif
        } else {

        }
        break;
    case BT_STATUS_VOICE_RECOGNITION:
        if (e->value) { //如果是siri语音状态，停止录音
#if (RECORDER_MIX_BREAK_EN)
            recorder_mix_stop();
#endif
        }
        break;
    default:
        break;
    }
}

int recorder_mix_pcm_stream_open(u16 sample_rate, u8 channel)
{
    recorder_mix_pcm_set_info(sample_rate, channel);
    if (__this && (__this->pcm == NULL))	{
        __this->pcm = pcm_dec_open(__this->sync_en);
        if (__this->pcm) {
            return 0;
        }
    }
    return -1;
}

void recorder_mix_pcm_stream_close(void)
{
    if (__this && __this->pcm)	{
        if (get_call_status() != BT_CALL_HANGUP) {
            ///如果通话还在，不要关闭pcm解码，因为通话录音还要使用
            return ;
        }
        pcm_dec_close(&__this->pcm);
    }
}

#endif//RECORDER_MIX_EN


