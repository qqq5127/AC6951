#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "aec_user.h"
#include "asm/audio_src.h"
#include "audio_enc.h"
#include "app_main.h"
#include "btstack/avctp_user.h"
#include "audio_digital_vol.h"
#include "audio_sbc_codec.h"
#include "clock_cfg.h"
#include "media/pcm_decoder.h"
#include "audio_dec.h"

#define BT_USE_PCM_DEC         0   //多加一个解码，会多耗资源，时钟也会增加,用来解决完全切歌的时候出现杂音的问题

#define  SBC_RESERVED_LAST_PCM  1  //淡入淡出功能，耗资源很小，可以解决正常速度切歌杂音，但是快速连续切歌会有杂音


enum {
    SBC_RUN_STEP_NOR = 0,
    SBC_RUN_STEP_WAIT_STOP,
    SBC_RUN_STEP_STOP,
    SBC_RUN_STEP_WAIT_NOR,
};





extern struct audio_encoder_task *encode_task;
//static struct audio_encoder_task *encode_task = NULL;
#define SBC_ENC_WRITING                 BIT(1)
#define SBC_ENC_START                   BIT(2)
#define SBC_ENC_WAKEUP_SEND             BIT(3)  /*减少唤醒问题*/
#define SBC_ENC_PCM_MUTE_DATA           BIT(4)  /*发一包静音数据*/

static u8 sbc_buf_flag = 0;
#define ESCO_ADC_BUF_NUM        2
#define ESCO_ADC_IRQ_POINTS     256
#define ESCO_ADC_BUFS_SIZE      (ESCO_ADC_BUF_NUM * ESCO_ADC_IRQ_POINTS)

#define SBC_USE_MIC_CHANNEL     0
#define SBC_ENC_PACK_ENABLE		0	/*sbc数据包封装*/
#define SBC_ENC_IN_SIZE			512
#define SBC_ENC_OUT_SIZE		256

#define SBC_ENC_IN_CBUF_SIZE	(SBC_ENC_IN_SIZE * 8)
#define SBC_ENC_OUT_CBUF_SIZE	(SBC_ENC_OUT_SIZE * 8)

struct sbc_enc_hdl {
    struct audio_encoder encoder;
    OS_SEM pcm_frame_sem;
    u8 output_frame[SBC_ENC_OUT_SIZE];
    u8  pcm_frame[SBC_ENC_IN_SIZE];
    u8 frame_size;
    u8 backup_digital_vol;
    cbuffer_t output_cbuf;
    u8 out_cbuf_buf[SBC_ENC_OUT_CBUF_SIZE];
    cbuffer_t pcm_in_cbuf;/*要注意要跟DAC的buf大小一致*/
    u8 in_cbuf_buf[SBC_ENC_IN_CBUF_SIZE];
#if SBC_ENC_PACK_ENABLE
    u16 cp_type;
    u16 packet_head_sn;
#endif
    int mute_data_send_timer;
#if SBC_USE_MIC_CHANNEL
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    s16 adc_buf[ESCO_ADC_BUFS_SIZE];    //align 4Bytes
#endif

#if SBC_RESERVED_LAST_PCM
    u8 run_step;
    u8 run_cnt;
    u8 in_cnt;
    OS_SEM run_sem;
#endif



#if BT_USE_PCM_DEC
    struct audio_mixer_ch 		mix_ch;
    struct audio_res_wait 		wait;
    struct pcm_decoder 			pcm_dec;		// pcm解码句柄
    struct audio_stream 		*audio_stream;	// 音频流
#endif

    /* dvol_handle *dvol; */
};
static struct sbc_enc_hdl *sbc_enc = NULL;

__attribute__((weak))
void audio_sbc_enc_inbuf_resume(void)
{
    ;
}

__attribute__((weak))
void audio_sbc_enc_open_exit(void)
{
    ;
}

__attribute__((weak))
void audio_sbc_enc_close_enter(void)
{
    ;
}


#if SBC_RESERVED_LAST_PCM
static void sbc_enc_pcm_data_fade_out(s16 *data, int len)
{
    u16 samples = len >> 1;
    int down_step = (32767 + samples - 1) / samples + 1;
    int value = 32767;

    for (int i = 0; i < samples; i++) {
        if (value > down_step) {
            value -= down_step;
        } else {
            value = 0;
        }
        s32 tmp = *data;
        *data++ = tmp * value / 32767;
    }
}
static void sbc_enc_pcm_data_fade_in(s16 *data, int len)
{

    u16 samples = len >> 1;
    int down_step = (32767 + samples - 1) / samples + 1;
    int value = 0;

    for (int i = 0; i < samples; i++) {
        if (value + down_step < 32767) {
            value += down_step;
        } else {
            value = 32767;
        }
        s32 tmp = *data;
        *data++ = tmp * value / 32767;
    }

}

#endif /* SBC_RESERVED_LAST_PCM */



static void sbc_send_mute_data()
{
    sbc_buf_flag |= SBC_ENC_PCM_MUTE_DATA;
    audio_encoder_resume(&sbc_enc->encoder);
}
static int sbc_enc_pcm_get(struct audio_encoder *encoder, s16 **frame, u16 frame_len)
{
    u8 read_flag = 1;
    int pcm_len = 0;
    if (encoder == NULL) {
        r_printf("encoder NULL");
    }
    struct sbc_enc_hdl *enc = container_of(encoder, struct sbc_enc_hdl, encoder);

    if (enc == NULL) {
        r_printf("enc NULL");
    }
    int limit_size = SBC_ENC_IN_SIZE;
#if SBC_RESERVED_LAST_PCM
    if (enc->run_step == SBC_RUN_STEP_NOR) {
        limit_size += SBC_ENC_IN_SIZE;
    }
#endif


    if (cbuf_get_data_size(&enc->pcm_in_cbuf) < limit_size) {
        if (!(sbc_buf_flag & SBC_ENC_PCM_MUTE_DATA)) {
#if SBC_RESERVED_LAST_PCM
            if (enc->run_step == SBC_RUN_STEP_STOP) {
                if (enc->run_cnt) {
                    enc->run_cnt--;
                    if (enc->run_cnt == 0) {
                        os_sem_post(&enc->run_sem);
                    }
                    memset(enc->pcm_frame, 0, SBC_ENC_IN_SIZE);
                    *frame = enc->pcm_frame;
                    return SBC_ENC_IN_SIZE;
                }
            }
#endif



            return 0;
        } else {
            read_flag = 0;
        }
    }
    if (read_flag) {
        pcm_len = cbuf_read(&enc->pcm_in_cbuf, enc->pcm_frame, SBC_ENC_IN_SIZE);

#if SBC_RESERVED_LAST_PCM
        if (pcm_len && enc->run_step) {
            local_irq_disable();
            if (enc->run_step == SBC_RUN_STEP_WAIT_STOP) {
                enc->run_step = SBC_RUN_STEP_STOP;
                enc->run_cnt = 200; // 发射每次读取5包，至少保证发射一帧
                sbc_enc_pcm_data_fade_out(enc->pcm_frame, pcm_len);
            } else if (enc->run_step == SBC_RUN_STEP_STOP) {
                memset(enc->pcm_frame, 0, pcm_len);
                if (enc->run_cnt) {
                    enc->run_cnt--;
                    if (enc->run_cnt == 0) {
                        os_sem_post(&enc->run_sem);
                    }
                }
            } else if (sbc_enc->run_step == SBC_RUN_STEP_WAIT_NOR) {

                if (sbc_enc->in_cnt > 0) {
                    sbc_enc->in_cnt--;
                    memset(enc->pcm_frame, 0, pcm_len);

                } else {
                    sbc_enc->run_step = SBC_RUN_STEP_NOR;
                    sbc_enc_pcm_data_fade_in(enc->pcm_frame, pcm_len);

                }
            }

            local_irq_enable();
        } else {
        }
#endif


#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
        audio_stream_resume(&sbc_emitter.entry);
#endif
    } else {
        putchar('h');
        memset(enc->pcm_frame, 0, SBC_ENC_IN_SIZE);
        pcm_len = SBC_ENC_IN_SIZE;
    }
    if (pcm_len != SBC_ENC_IN_SIZE) {
        putchar('L');
    }

    *frame = enc->pcm_frame;
    return pcm_len;
}

static void sbc_enc_pcm_put(struct audio_encoder *encoder, s16 *frame)
{
}

static const struct audio_enc_input sbc_enc_input = {
    .fget = sbc_enc_pcm_get,
    .fput = sbc_enc_pcm_put,
};

static int sbc_enc_probe_handler(struct audio_encoder *encoder)
{
    return 0;
}


#if BT_USE_PCM_DEC

static int pcm_fread(void *hdl, void *buf, int len)
{
    len = len / 2;
    memset(buf, 0, len);
    return len;
}
static int pcm_dec_data_handler(struct audio_stream_entry *entry,
                                struct audio_data_frame *in,
                                struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct pcm_decoder *pcm_dec = container_of(decoder, struct pcm_decoder, decoder);
    struct sbc_enc_hdl *dec = container_of(pcm_dec, struct sbc_enc_hdl, pcm_dec);
    audio_stream_run(&decoder->entry, in);
    /* audio_mixer_ch_pause(&dec->mix_ch, 0); */
    return decoder->process_len;
}
static void pcm_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
}
static void dec_out_stream_resume(void *p)
{
    struct sbc_enc_hdl *dec = (struct sbc_enc_hdl *)p;

    audio_decoder_resume(&dec->pcm_dec.decoder);
}
static int pcm_dec_start(struct sbc_enc_hdl *stream)
{
    int err = 0;
    if (stream == NULL) {
        return -EINVAL;
    }
    err = pcm_decoder_open(&stream->pcm_dec, &decode_task);
    if (err) {
        return err;
    }
    pcm_decoder_set_event_handler(&stream->pcm_dec, pcm_dec_event_handler, 0);
    pcm_decoder_set_read_data(&stream->pcm_dec, pcm_fread, stream);
    pcm_decoder_set_data_handler(&stream->pcm_dec, pcm_dec_data_handler);

    audio_mixer_ch_open(&stream->mix_ch, &mixer);
    audio_mixer_ch_set_src(&stream->mix_ch, 0, 0);
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &stream->pcm_dec.decoder.entry;
    entries[entry_cnt++] = &stream->mix_ch.entry;
    stream->audio_stream = audio_stream_open(stream, dec_out_stream_resume);
    audio_stream_add_list(stream->audio_stream, entries, entry_cnt);

    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);
    err = audio_decoder_start(&stream->pcm_dec.decoder);
    if (err == 0) {
        printf("pcm_dec_start ok\n");
    }
    return err;
}
static void pcm_dec_stop(struct sbc_enc_hdl *stream)
{
    printf("mic stream dec stop \n\n");
    if (stream) {

        pcm_decoder_close(&stream->pcm_dec);
        audio_mixer_ch_close(&stream->mix_ch);
        if (stream->audio_stream) {
            audio_stream_close(stream->audio_stream);
            stream->audio_stream = NULL;
        }
    }
}

static int pcmdec_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    struct sbc_enc_hdl *stream = container_of(wait, struct sbc_enc_hdl, wait);
    if (event == AUDIO_RES_GET) {
        err = pcm_dec_start(stream);
    } else if (event == AUDIO_RES_PUT) {
        /* pcm_dec_stop(stream); */
    }

    return err;
}
static int pcm_dec_open(struct sbc_enc_hdl *stream, struct audio_fmt *pfmt)
{
    stream->pcm_dec.output_ch_num = stream->pcm_dec.ch_num = pfmt->channel;
    stream->pcm_dec.sample_rate = pfmt->sample_rate;
    if (stream->pcm_dec.output_ch_num == 2) {
        stream->pcm_dec.output_ch_type = AUDIO_CH_LR;
    } else {
        stream->pcm_dec.output_ch_type = AUDIO_CH_DIFF;
    }

    stream->wait.priority = 0;
    stream->wait.preemption = 0;
    stream->wait.protect = 1;
    stream->wait.handler = pcmdec_wait_res_handler;
    return audio_decoder_task_add_wait(&decode_task, &stream->wait);
}

static void pcm_dec_close(struct sbc_enc_hdl *stream)
{
    pcm_dec_stop(stream);
    audio_decoder_task_del_wait(&decode_task, &stream->wait);
}

#endif /*BT_USE_PCM_DEC*/












#if SBC_ENC_PACK_ENABLE
/*
 struct Media_Playload_Header {
 	u8 Version: 2;
 	u8 Padding: 1;
 	u8 Extension: 1;
 	u8 CSRC_Count: 4;
 	u8 Marker: 1;
	u8 PayloadType: 7;
 	u16 SequenceNumber;
 	u32 TimeStamp;
	u32 SSRC;
 };
 SBC_FRAME_SUM defined within sbc_packet_head_buf used for Media Playload parameter:Number of Frames
 */
#define SBC_PACKET_HEADER_LEN   (14) /*包头Max Length*/
#define SBC_FRAME_SUM           (4)	 /*一包多少帧*/
static unsigned char sbc_packet_head_buf[SBC_PACKET_HEADER_LEN] = {
    0x80, 0x60,     		//Version:2 Playload Type:0x60
    0x00, 0x64,   			//Sequencenumber
    0x00, 0x00, 0x03, 0x20, //Timestamp
    0x00, 0x00, 0x00, 0x00  //Synchronization Source
    //SBC_FRAME_SUM			//defined for Media Payload
};

void sbc_enc_packet_pack(struct sbc_enc_hdl *hdl)
{
    unsigned int TS; /*TimeStamp*/
    /*SequenceNumber*/
    hdl->packet_head_sn++;
    /*Timestamp*/
    TS = hdl->packet_head_sn * (128 * SBC_FRAME_SUM);

    sbc_packet_head_buf[2] = hdl->packet_head_sn >> 8;
    sbc_packet_head_buf[3] = hdl->packet_head_sn & 0xff;
    sbc_packet_head_buf[4] = TS >> 24;
    sbc_packet_head_buf[5] = (TS >> 16) & 0xff;
    sbc_packet_head_buf[6] = (TS >> 8) & 0xff;
    sbc_packet_head_buf[7] = (TS) & 0xff;
    if (hdl->cp_type == 0x0002) {
        /*
         *Content Protection Header
         *L-bit	:bit0
         *Cp-bit:bit1
         *RFA	:bit2 to bit7
         */
        sbc_packet_head_buf[12] = 0;
        sbc_packet_head_buf[13] = SBC_FRAME_SUM;
    } else {
        sbc_packet_head_buf[12] = SBC_FRAME_SUM;
    }
}
#endif/*SBC_ENC_PACK_ENABLE*/

static int sbc_enc_output_handler(struct audio_encoder *encoder, u8 *frame, int len)
{
    if (encoder == NULL) {
        r_printf("encoder NULL");
    }
    struct sbc_enc_hdl *enc = container_of(encoder, struct sbc_enc_hdl, encoder);
    enc->frame_size = len;

    u16 wlen = cbuf_write(&enc->output_cbuf, frame, len);
    if (wlen != len) {
        putchar('F');
        return len;
    }
    //printf("sbc_enc out,frame:%x,out:%d[0x%x]\n", frame, len, enc->frame_size);
    if (cbuf_get_data_size(&sbc_enc->output_cbuf) >= enc->frame_size * 5) {
        //达到基本的数据量，唤醒蓝牙发数
        user_emitter_cmd_prepare(USER_CTRL_CMD_RESUME_STACK, 0, NULL);
    }
    return len;
}

const static struct audio_enc_handler sbc_enc_handler = {
    .enc_probe = sbc_enc_probe_handler,
    .enc_output = sbc_enc_output_handler,
};
int a2dp_sbc_encoder_get_data(u8 *packet, u16 buf_len, int *frame_size)
{
    if (sbc_enc == NULL) {
        return 0;
    }

    if (cbuf_get_data_size(&sbc_enc->output_cbuf) < buf_len) {
        return 0;
    }
    int debug_frame_size = sbc_enc->frame_size;
    int number = buf_len / debug_frame_size;  /*取整数包*/
    *frame_size = debug_frame_size;
    u16 rlen = cbuf_read(&sbc_enc->output_cbuf, packet, *frame_size * number);
    if (rlen == 0) {
        putchar('N');
    }

#if SBC_RESERVED_LAST_PCM
    if (sbc_enc->run_step != SBC_RUN_STEP_NOR) {
        // 非正常状态，sbc数据取走后激活一下编码
        audio_encoder_resume(&sbc_enc->encoder);
    }
#endif

    return rlen;
}

static void sbc_enc_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    printf("sbc_enc_event_handler:0x%x,%d\n", argv[0], argv[0]);
    switch (argv[0]) {
    case AUDIO_ENC_EVENT_END:
        puts("AUDIO_ENC_EVENT_END\n");
        break;
    }
}
#include "sbc_enc.h"
sbc_t sbc_param = {
    .frequency = SBC_FREQ_44100,
    .blocks = SBC_BLK_16,
    .subbands = SBC_SB_8,
    .mode = SBC_MODE_STEREO,
    .allocation = 0,
    .endian = SBC_LE,
    .bitpool = 53,
};

int audio_sbc_enc_config(void *cfg)
{
    sbc_t *p = cfg;
    if (p) {
        printf("sbc_enc_param update:\n");
        printf("frequency:%d\n", p->frequency);
        printf("blocks:%d\n", p->blocks);
        printf("subbands:%d\n", p->subbands);
        printf("mode:%d\n", p->mode);
        printf("allocation:%d\n", p->allocation);
        printf("endian:%d\n", p->endian);
        printf("bitpool:%d\n", p->bitpool);
    }
    return 0;
}

int audio_sbc_enc_check_empty_len(void)
{
    if (!(sbc_buf_flag & SBC_ENC_START)) {
        return 0;
    }
    if (!sbc_enc) {
        return 0;
    }
    return sbc_enc->pcm_in_cbuf.total_len - sbc_enc->pcm_in_cbuf.data_len;
}

static int audio_sbc_enc_write_base(s16 *data, int len, u8 ch_i, u8 ch_o)
{
    if (!(sbc_buf_flag & SBC_ENC_START)) {
        return 0;
    }
    if (!sbc_enc) {
        return 0;
    }
    sbc_buf_flag |= SBC_ENC_WRITING;

    int wlen = 0;
    int ilen = 0;
    int remain_len = len;
    while (remain_len) {
        wlen = 0;
        void *obuf = cbuf_write_alloc(&sbc_enc->pcm_in_cbuf, &wlen);
        if (!wlen) {
            break;
        }
        if (ch_o == ch_i) {
            ilen = wlen;
            if (ilen > remain_len) {
                ilen = remain_len;
                wlen = ilen;
            }
            memcpy(obuf, data, wlen);
        } else if ((ch_o == 2) && (ch_i == 1)) {
            ilen = (wlen >> 2) << 1;
            if (ilen > remain_len) {
                ilen = remain_len;
                wlen = ilen << 1;
            }
            s16 *outbuf = obuf;
            s16 *inbuf = data;
            int run_len = ilen >> 1;
            while (run_len --) {
                *outbuf++ = *inbuf;
                *outbuf++ = *inbuf;
                inbuf++;
            }
        } else if ((ch_o == 1) && (ch_i == 2)) {
            ilen = wlen << 1;
            if (ilen > remain_len) {
                ilen = remain_len;
                wlen = ilen >> 1;
            }
            s16 *outbuf = obuf;
            s16 *inbuf = data;
            int run_len = ilen >> 2;
            while (run_len--) {
                *outbuf++ = data_sat_s16(inbuf[0] + inbuf[1]);
                inbuf += 2;
            }
        }
        cbuf_write_updata(&sbc_enc->pcm_in_cbuf, wlen);
        remain_len -= ilen;
        data += ilen / 2;
    };
    wlen = len - remain_len;

    if (cbuf_get_data_size(&sbc_enc->pcm_in_cbuf) >= SBC_ENC_IN_SIZE) {
        audio_encoder_resume(&sbc_enc->encoder);
    }
    sbc_buf_flag &= ~SBC_ENC_WRITING;
    return wlen;
}

int audio_sbc_enc_write(s16 *data, int len)
{
    if (!(sbc_buf_flag & SBC_ENC_START)) {
        return 0;
    }
    if (!sbc_enc) {
        return 0;
    }
    sbc_buf_flag |= SBC_ENC_WRITING;

#if 0
    u16 wlen = cbuf_write(&sbc_enc->pcm_in_cbuf, data, len);
    if (wlen != len) {
        putchar('@');
    }
#else
    int wlen = 0;
    int remain_len = len;
    while (remain_len) {
        wlen = 0;
        void *obuf = cbuf_write_alloc(&sbc_enc->pcm_in_cbuf, &wlen);
        if (!wlen) {
            break;
        }
        if (wlen > remain_len) {
            wlen = remain_len;
        }
        memcpy(obuf, data, wlen);
        /* audio_digital_vol_run(sbc_enc->dvol, obuf, wlen); */
        cbuf_write_updata(&sbc_enc->pcm_in_cbuf, wlen);
        remain_len -= wlen;
        data += wlen / 2;
    };
    wlen = len - remain_len;
#endif

    if (cbuf_get_data_size(&sbc_enc->pcm_in_cbuf) >= SBC_ENC_IN_SIZE) {
        audio_encoder_resume(&sbc_enc->encoder);
    }
    sbc_buf_flag &= ~SBC_ENC_WRITING;
    return wlen;
}

#if SBC_USE_MIC_CHANNEL
s16 temp_data[ESCO_ADC_IRQ_POINTS * 2];
extern struct audio_adc_hdl adc_hdl;
static void adc_mic_output_handler(void *priv, s16 *data, int len)
{
    u16 points = len >> 1;
    if (!(sbc_buf_flag & SBC_ENC_START)) {
        return 0;
    }
    if (sbc_enc) {
        //单变双
        for (int i = 0; i < points ; i++) {
            temp_data[i * 2] = data[i];
            temp_data[i * 2 + 1] = data[i];
        }
        //app_audio_output_write(temp_data, len*2);
        //memset(temp_data, 0 , sizeof(temp_data));
        audio_sbc_enc_write(temp_data, len * 2);
    }
}
#endif

int audio_sbc_enc_open()
{
    int err;
    struct audio_fmt fmt;
    u16 frame_len = 512;

    if (encode_task) {
        return -1;
    }
    printf("sbc_enc_open,frame_len:%d\n", frame_len);
    sbc_buf_flag &= ~SBC_ENC_START;
    fmt.channel = 1;
    fmt.frame_len = frame_len;
    fmt.coding_type = AUDIO_CODING_SBC;

    fmt.priv = &sbc_param;

    audio_encoder_task_open();
    if (!sbc_enc) {
        sbc_enc = zalloc(sizeof(*sbc_enc));
#if SBC_RESERVED_LAST_PCM
        os_sem_create(&sbc_enc->run_sem, 0);
#endif


    }
    printf("init vol:%d \n", app_var.music_volume);
    /* sbc_enc->dvol = audio_digital_vol_open(app_var.music_volume, 30, 7); */
    cbuf_init(&sbc_enc->output_cbuf, sbc_enc->out_cbuf_buf, SBC_ENC_OUT_CBUF_SIZE);
    cbuf_init(&sbc_enc->pcm_in_cbuf, sbc_enc->in_cbuf_buf, SBC_ENC_IN_CBUF_SIZE);
    os_sem_create(&sbc_enc->pcm_frame_sem, 0);
    audio_encoder_open(&sbc_enc->encoder, &sbc_enc_input, encode_task);
    audio_encoder_set_handler(&sbc_enc->encoder, &sbc_enc_handler);
    audio_encoder_set_fmt(&sbc_enc->encoder, &fmt);
    audio_encoder_set_event_handler(&sbc_enc->encoder, sbc_enc_event_handler, 0);
    audio_encoder_set_output_buffs(&sbc_enc->encoder, sbc_enc->output_frame,
                                   sizeof(sbc_enc->output_frame), 1);
    int start_err = audio_encoder_start(&sbc_enc->encoder);
    printf("sbc_enc_open ok %d\n", start_err);

#if SBC_USE_MIC_CHANNEL
    fmt.sample_rate = 44100;
    audio_adc_mic_open(&sbc_enc->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
    audio_adc_mic_set_sample_rate(&sbc_enc->mic_ch, fmt.sample_rate);
    audio_adc_mic_set_gain(&sbc_enc->mic_ch, app_var.aec_mic_gain);
    audio_adc_mic_set_buffs(&sbc_enc->mic_ch, sbc_enc->adc_buf,
                            ESCO_ADC_IRQ_POINTS * 2, ESCO_ADC_BUF_NUM);
    sbc_enc->adc_output.handler = adc_mic_output_handler;
    audio_adc_add_output_handler(&adc_hdl, &sbc_enc->adc_output);


    //app_audio_output_samplerate_set(44100);
    //app_audio_output_start();
    audio_adc_mic_start(&sbc_enc->mic_ch);
#endif


#if BT_USE_PCM_DEC

    clock_add(DEC_PCM_CLK);
    pcm_dec_open(sbc_enc, &fmt);
#endif




    clock_add(ENC_SBC_CLK);

    clock_set_cur();
    sbc_buf_flag |= SBC_ENC_START;
    audio_sbc_enc_open_exit();
    return 0;

}
int audio_sbc_enc_reset_buf(u8 flag)
{
    if (!sbc_enc) {
        return -1;
    }
    r_printf("audio_sbc_enc_reset_buf %d\n", flag);
    if (flag) {
        //有些情况（提示音）没有足够的参数去fade out，使得下次fade in受影响。
        //所以默认每次都reset一下
        /* audio_digital_vol_reset_fade(sbc_enc->dvol); */
        /* audio_digital_vol_set(sbc_enc->dvol, sbc_enc->backup_digital_vol); */
        if (sbc_enc->mute_data_send_timer) {
            sys_hi_timer_del(sbc_enc->mute_data_send_timer);
            sbc_enc->mute_data_send_timer = 0;
            sbc_buf_flag &= ~SBC_ENC_PCM_MUTE_DATA;
        }
    } else {
        /* audio_digital_vol_set(sbc_enc->dvol, 0); */
        if (sbc_enc->mute_data_send_timer == 0) {
            //sbc_enc->mute_data_send_timer= sys_hi_timer_add((void *)0, sbc_send_mute_data, 12);
        }
    }
    return 0;
}
int audio_sbc_enc_close()
{
    if (!sbc_enc) {
        return -1;
    }
    printf("audio_sbc_enc_close\n");
    audio_sbc_enc_close_enter();
    sbc_buf_flag &= ~SBC_ENC_START;

#if BT_USE_PCM_DEC
    pcm_dec_close(sbc_enc);
    clock_remove(DEC_PCM_CLK);
#endif


#if SBC_RESERVED_LAST_PCM
    os_sem_post(&sbc_enc->run_sem);
#endif
    while (sbc_buf_flag & SBC_ENC_WRITING) {
        os_time_dly(1);

    }



    /* audio_digital_vol_close(sbc_enc->dvol); */
#if SBC_USE_MIC_CHANNEL
    audio_adc_mic_close(&sbc_enc->mic_ch);
    audio_adc_del_output_handler(&adc_hdl, &sbc_enc->adc_output);
#endif
    audio_encoder_close(&sbc_enc->encoder);
    free(sbc_enc);
    sbc_enc = NULL;

    audio_encoder_task_close();

    clock_remove(ENC_SBC_CLK);

    clock_set_cur();
    printf("audio_sbc_enc_close end\n");
    return 0;
}

void audio_sbc_enc_init(void)
{
    /* audio_digital_vol_open(0, 30, 4); */
    //printf("init vol:%d \n", app_var.music_volume);
    //audio_digital_vol_open(app_var.music_volume, 30, 7);
}

int audio_sbc_enc_is_work(void)
{
    if ((sbc_buf_flag & SBC_ENC_START) &&  sbc_enc) {
        return true;
    }
    return false;
}

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
#include "application/audio_dig_vol.h"
extern void *bt_digvol_last;
#endif
void bt_emitter_set_vol(u8 vol)
{
    if (!(sbc_buf_flag & SBC_ENC_START)) {
        return ;
    }
    if (!sbc_enc) {
        return ;
    }
    sbc_enc->backup_digital_vol = vol;
    r_printf("=======set vol %d\n", vol);
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
    if (bt_digvol_last) {
        audio_dig_vol_set(bt_digvol_last, AUDIO_DIG_VOL_ALL_CH, vol);
    }
#endif
}

int audio_sbc_enc_get_rate(void)
{
    int sr = 44100;
    switch (sbc_param.frequency) {
    case SBC_FREQ_16000:
        sr = 16000;
        break;
    case SBC_FREQ_32000:
        sr = 32000;
        break;
    case SBC_FREQ_44100:
        sr = 44100;
        break;
    case SBC_FREQ_48000:
        sr = 48000;
        break;
    }
    return sr;
}
int audio_sbc_enc_get_channel_num(void)
{
    return sbc_param.mode == SBC_MODE_MONO ? 1 : 2;
}

int a2dp_sbc_encoder_init(void *sbc_struct)
{
    extern void audio_mixer_reset_sample_rate(u8 flag, u32 sr);
    if (sbc_struct) {
        //更新连接过程中的a2dp source参数
        //audio_sbc_enc_config(&sbc_param);
        g_printf("sbc enc open");
        memcpy(&sbc_param, sbc_struct, sizeof(sbc_t));
        audio_mixer_reset_sample_rate(1, audio_sbc_enc_get_rate());
        //audio_sbc_enc_config(&sbc_param);
        audio_sbc_enc_open();
    } else {
        g_printf("sbc enc close");
        audio_mixer_reset_sample_rate(0, audio_sbc_enc_get_rate());
        audio_sbc_enc_close();
    }
    return 0;
}


#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)

struct sbc_emitter_hdl sbc_emitter = {0};

static int audio_stream_sbc_emitter_data_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in,
        struct audio_data_frame *out)
{

    local_irq_disable();
    if (false == audio_sbc_enc_is_work()) {
        local_irq_enable();
        return in->data_len;
    }
#if SBC_RESERVED_LAST_PCM
    if (in->stop) {
        if (sbc_enc->run_step == SBC_RUN_STEP_NOR) {
            sbc_enc->run_step = SBC_RUN_STEP_WAIT_STOP;
        }
        audio_encoder_resume(&sbc_enc->encoder);
        os_sem_set(&sbc_enc->run_sem, 0);
        local_irq_enable();
        if ((sbc_enc->run_step == SBC_RUN_STEP_WAIT_STOP) || (sbc_enc->run_cnt)) {
            os_sem_pend(&sbc_enc->run_sem, 3);
            if ((sbc_enc->run_step == SBC_RUN_STEP_WAIT_STOP) || (sbc_enc->run_cnt)) {
                log_w("sbc wait stop err, step:%d, cnt:%d \n\n", sbc_enc->run_step, sbc_enc->run_cnt);
            }
        }
        return in->data_len;
    } else {
        if (sbc_enc->run_step != SBC_RUN_STEP_NOR && (sbc_enc->run_step != SBC_RUN_STEP_WAIT_NOR)) {
            cbuf_clear(&sbc_enc->pcm_in_cbuf);

            sbc_enc->run_step = SBC_RUN_STEP_WAIT_NOR;

            sbc_enc->in_cnt = 100;     //丢掉前面几包



        }

    }
#endif
    /* return audio_sbc_enc_write(in->data, in->data_len); */
    local_irq_enable();
    if (in->data_len == 0) {
        return 0;
    }

    return audio_sbc_enc_write_base(in->data, in->data_len, in->channel, audio_sbc_enc_get_channel_num());
}

void audio_sbc_emitter_init(void)
{
    sbc_emitter.entry.data_handler = audio_stream_sbc_emitter_data_handler;
}

#endif


