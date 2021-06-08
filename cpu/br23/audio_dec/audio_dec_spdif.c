/*
 ****************************************************************
 *File : audio_spdif.c
 *Note :
 *
 ****************************************************************
***********************   spdif_dec ******************************/
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "effectrs_sync.h"
#include "application/audio_eq_drc_apply.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_dec.h"
#include "app_config.h"
#include "app_main.h"
#include "audio_enc.h"
#include "clock_cfg.h"
#include "media/pcm_decoder.h"

#if TCFG_SPDIF_ENABLE

#define SPDIF_DEC_DATA_BUF_LEN 1024*4
struct s_spdif_decode {
    struct pcm_decoder pcm_dec;
    struct file_decoder file_dec;
    struct audio_stream *audio_stream;		// 音频流
    struct audio_res_wait wait;
    struct audio_mixer_ch mix_ch;
    u32 coding_type;
    struct audio_fmt fmt;
    volatile  u32 pcm_sr;
    cbuffer_t dec_cbuf;
    u8 dec_data_buf[SPDIF_DEC_DATA_BUF_LEN];
    u8 status;
    struct audio_eq_drc *eq_drc;
#if TCFG_EQ_DIVIDE_ENABLE
    struct audio_eq_drc *eq_drc_rl_rr;//eq drc句柄
    struct audio_vocal_tract vocal_tract;//声道合并目标句柄
    struct audio_vocal_tract_ch synthesis_ch_fl_fr;//声道合并句柄
    struct audio_vocal_tract_ch synthesis_ch_rl_rr;//声道合并句柄
    struct channel_switch *ch_switch;//声道变换
#endif

};

void *spdif_eq_drc_open(u16 sample_rate, u8 ch_num);
void spdif_eq_drc_close(struct audio_eq_drc *eq_drc);
void *spdif_rl_rr_eq_drc_open(u16 sample_rate, u8 ch_num);
void spdif_rl_rr_eq_drc_close(struct audio_eq_drc *eq_drc);


static void reset_pcm_sample_rate(u32 sr);
int spdif_dec_open(struct audio_fmt fmt);
void spdif_dec_close(void);

struct audio_spdif_hdl spdif_slave_hdl;
struct s_spdif_decode *spdif_dec_hdl = NULL;

int spdif_non_linear_packet(u16 *pkt, u16 pkt_len)
{
    enum IEC61937DataType data_type;
    u16 search_index = 0;
    int data_cp_begin_idx = 0;
    int data_cp_len = 0;
    u16 *data = pkt;
    u16 *data_ptr = NULL;
    int zero_cnt = 0;
    u32 state = 0;
    int pkt_size_bits, offset, skip_len;
    if (spdif_slave_hdl.length_code > 0) {
        if (spdif_slave_hdl.length_code >= SPDIF_DATA_DAM_LEN * 2) {
            data_cp_begin_idx = 0;
            data_cp_len       = SPDIF_DATA_DAM_LEN * 2;
            spdif_slave_hdl.length_code       -= data_cp_len;
        } else {
            data_cp_begin_idx    = 0;
            data_cp_len = spdif_slave_hdl.length_code;
            spdif_slave_hdl.length_code = 0;
        }
        //写入解码cbuf
        /* int spdif_dec_write_data(s16 *data, int len) */
        spdif_dec_write_data(&data[data_cp_begin_idx], data_cp_len * 2);
        if (spdif_slave_hdl.length_code != 0) {
            return 0;
        } else {
            search_index = data_cp_len;
        }
    }
    if (spdif_slave_hdl.find_sync_word != 0) {
        printf("[last:%d,%x]", search_index, data[search_index]);
        if (data[search_index] == SYNCWORD2) {
            puts("3");
        } else {
            puts("M");
            spdif_slave_hdl.find_sync_word = 0;
            return -1;
        }
    } else {
        while (search_index < pkt_len) {
            if (data[search_index] == SYNCWORD1) {
                spdif_slave_hdl.find_sync_word = 1;
            }
            if (++search_index >= pkt_len) {
                return -1;
            }
            if (spdif_slave_hdl.find_sync_word) {
                if ((data[search_index] == SYNCWORD2) && (data[search_index - 1] == SYNCWORD1)) {
                    break;
                } else {
                    spdif_slave_hdl.find_sync_word = 0;
                }
            }
        }
    }
    spdif_slave_hdl.find_sync_word = 0;

    data_type = data[++search_index];
    spdif_slave_hdl.decoder_type = data_type & 0xff;
    pkt_size_bits = data[++search_index];
    spdif_slave_hdl.length_code = (pkt_size_bits >> 3) >> 1; //16bit
    if (pkt_size_bits % 16) {
        printf("packet not endinf at a 16-bit boundary %d-%d \n", pkt_size_bits, search_index);
        for (int i = 0; i < pkt_len; i++) {
            printf("%x ", data[i]);
        }
    }
    ++search_index;
    data_cp_len    = pkt_len - search_index;
    skip_len = 2048 - (spdif_slave_hdl.length_code << 1) - BURST_HEADER_SZIE;
    if (data_cp_len) {
        spdif_dec_write_data(&data[search_index], data_cp_len * 2);
        data_cp_len = 0;
    }
    return 0;
}
void reset_data_buf(void)
{
    if (!spdif_dec_hdl) {
        return;
    }
    cbuf_clear(&spdif_dec_hdl->dec_cbuf);
}

#define SAMPLE_RATE_TABLE_SIZE 14
static u8 old_samplerate_index = 6;   /*位置记得要跟默认采样率对应*/
u16 spdif_sample_rate_table[SAMPLE_RATE_TABLE_SIZE] = {
    80,
    110,
    160,
    221,
    240,
    320,
    441,
    480,
    504,
    640,
    882,
    960,
    1764,
    1920,
};
#define SPDIF_DEFAULT_OUT_SAMPLE_RATE 44100
static u32 cal_samplerate(void)
{
    static u32 pre_sr_cnt = 0;
    static u32 sr_cnt = 0;
    sr_cnt = JL_SS->SR_CNT;
    u8  search_index = 0, i = 0;
    u32 tick_result = 0 ;
    tick_result = (((clk_get("lsb") / 100 * SPDIF_DATA_DAM_LEN)) / sr_cnt);
    for (i = 0; i < SAMPLE_RATE_TABLE_SIZE; i++) {
        if ((tick_result > (spdif_sample_rate_table[i] - 20)) && (tick_result < (spdif_sample_rate_table[i] + 20))) {
            search_index = i;
        }
    }
    if ((search_index != 0) && (old_samplerate_index != search_index)) {
        printf("\n\n spdif SET src param %d %d--- \n\n", spdif_sample_rate_table[search_index], tick_result);
        reset_data_buf();
        reset_pcm_sample_rate(spdif_sample_rate_table[search_index] * 100);
        old_samplerate_index = search_index;
    }
    return 0;
}

___interrupt
static void spdif_isr_handler()
{
    int index = 0;
    s32 *ptr;
    u8 *info_ptr = NULL;
    s16 data_ptr[SPDIF_DATA_DAM_LEN * SPDIF_CHANNEL_NUMBER];
    u16 *u_data_ptr = (u16 *)data_ptr;
    /* putchar('A'); */
    if (I_PND) { //information pending
        I_PND_CLR;
        index = !(USING_INF_BUF_INDEX >> 13);
        info_ptr = (u8 *) &spdif_slave_hdl.p_spdif_info_buf[index];
        for (int i = 0; i < SPDIF_INFO_LEN; i++) {
            if (!(info_ptr[0]&INFO_VALIDITY_BIT)) {
                break;
            }
            spdif_slave_hdl.validity_bit_flag = 1;
        }
    }

    if (ERROR_VALUE) { // some error flag
        if (ERROR_VALUE & (BIT(8) | BIT(9))) {//块/帧错误
            memset(&spdif_slave_hdl.p_spdif_data_buf[0], 0x0, SPDIF_DATA_DAM_LEN * SPDIF_CHANNEL_NUMBER);
            memset(&spdif_slave_hdl.p_spdif_data_buf[1], 0x0, SPDIF_DATA_DAM_LEN * SPDIF_CHANNEL_NUMBER);
        }
        if (JL_SS->CON & BIT(11)) {//电平长度接收错误
            //模块复位
            putchar('E');
            audio_spdif_slave_close(&spdif_slave_hdl);
            spdif_slave_hdl.error_flag = 1;
            ERR_CLR;
            return;
        }
        ERR_CLR;
    }
    if (spdif_slave_hdl.error_flag) {
        return;
    }
    if (D_PND) { //data pending
        D_PND_CLR;
        if (spdif_slave_hdl.validity_bit_flag) { //DST
            spdif_slave_hdl.validity_bit_flag = 0;
            index = !(USING_BUF_INDEX >> 12);
            ptr = (s32 *)spdif_slave_hdl.p_spdif_data_buf[index];
            for (int i = 0; i < SPDIF_DATA_DAM_LEN * 2; i += 2) {
                u_data_ptr[i]   = (u16)(ptr[i] >> 8);
                u_data_ptr[i + 1] = (u16)(ptr[i + 1] >> 8);
            }
            if (!spdif_dec_hdl) {
#if 0
                struct audio_fmt fmt;
                fmt.channel = 2;
                fmt.coding_type = 	AUDIO_CODING_DTS;
                fmt.sample_rate = SPDIF_DEFAULT_OUT_SAMPLE_RATE;
                spdif_dec_open(fmt);
#endif
                audio_spdif_slave_close(&spdif_slave_hdl);
                spdif_slave_hdl.error_flag = 2;

            } else if (spdif_dec_hdl->fmt.coding_type != AUDIO_CODING_DTS) {
                putchar('d');
                //模块复位
                audio_spdif_slave_close(&spdif_slave_hdl);
                spdif_slave_hdl.error_flag = 2;
                return;
            }
            if (spdif_dec_hdl && (spdif_slave_hdl.status == SPDIF_STATE_START)) {
                spdif_non_linear_packet(u_data_ptr, SPDIF_DATA_DAM_LEN * 2);
            }

            /* putchar('D'); */
            return;
        } else { //PCM
            if (!spdif_dec_hdl) {
#if 0
                putchar('p');
                struct audio_fmt fmt;
                fmt.channel = 2;
                fmt.coding_type = 	AUDIO_CODING_PCM;
                fmt.sample_rate = SPDIF_DEFAULT_OUT_SAMPLE_RATE;
                spdif_dec_open(fmt);
#endif
                audio_spdif_slave_close(&spdif_slave_hdl);
                spdif_slave_hdl.error_flag = 1;

                return;
            } else if (spdif_dec_hdl->fmt.coding_type != AUDIO_CODING_PCM) {
                putchar('p');
                audio_spdif_slave_close(&spdif_slave_hdl);
                spdif_slave_hdl.error_flag = 1;
                return;
            }

            /* putchar('D'); */
            cal_samplerate();
            index = !(USING_BUF_INDEX >> 12);
            ptr = (s32 *) spdif_slave_hdl.p_spdif_data_buf[index];
            for (int i = 0; i < SPDIF_DATA_DAM_LEN * 2; i += 2) {
                data_ptr[i]   = (s16)(ptr[i] >> 8);
                data_ptr[i + 1] = (s16)(ptr[i + 1] >> 8);
            }
            if ((spdif_dec_hdl) && (spdif_slave_hdl.status == SPDIF_STATE_START)) {
                int wlen = spdif_dec_write_data(data_ptr, SPDIF_DATA_DAM_LEN * SPDIF_CHANNEL_NUMBER * 2);
                if (wlen == 0) {
                    /* printf(" wlen = 0"); */
                }
            }
            memset(spdif_slave_hdl.p_spdif_data_buf[index], 0, SPDIF_DATA_DAM_LEN * SPDIF_CHANNEL_NUMBER * 2);
        }
    }
    if (CSB_PND) {
        CSB_PND_CLR;
    }

}

// 写入spdif数据
/* int spdif_dec_write_data(s16 *data, int len) */
int spdif_dec_write_data(void *data, int len)
{
    if (!spdif_dec_hdl) {
        return 0;
    }
    u16 wlen = cbuf_write(&spdif_dec_hdl->dec_cbuf, data, len);
    if (wlen) {
        audio_decoder_resume(&spdif_dec_hdl->pcm_dec.decoder);
    }
    /* printf("wl:%d ", wlen); */
    return wlen;
}

static void spdif_dec_release(void)
{
    audio_decoder_task_del_wait(&decode_task, &spdif_dec_hdl->wait);
    if (spdif_dec_hdl) {
        clock_remove(SPDIF_CLK);
        free(spdif_dec_hdl);
        spdif_dec_hdl = NULL;
    }
}
static void file_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        printf("\n--func=%s\n", __FUNCTION__);
        spdif_dec_close();
        break;
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    文件指针定位
   @param    *decoder: 解码器句柄
   @param    offset: 定位偏移
   @param    seek_mode: 定位类型
   @return   0：成功
   @return   非0：错误
   @note
*/
/*----------------------------------------------------------------------------*/
static int spdif_file_seek(struct audio_decoder *decoder, u32 offset, int seek_mode)
{
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    读取文件数据
   @param    *decoder: 解码器句柄
   @param    *buf: 数据
   @param    len: 数据长度
   @return   >=0：读到的数据长度
   @return   <0：错误
   @note
*/
/*----------------------------------------------------------------------------*/
static int spdif_file_read(struct audio_decoder *decoder, void *buf, u32 len)
{
    int rlen;
    rlen = cbuf_read(&spdif_dec_hdl->dec_cbuf, buf, len);
    return rlen;
}

static const struct audio_dec_input file_input = {
    .coding_type = 	AUDIO_CODING_DTS,
    .data_type   = AUDIO_INPUT_FILE,
    .ops = {
        .file = {
            .fread = spdif_file_read,
            .fseek = spdif_file_seek,
        }
    }
};


static int spdif_dec_read(void *hdl, void *buf, int len)
{
    int rlen = 0;
    rlen = cbuf_read(&spdif_dec_hdl->dec_cbuf, buf, len);
    /* printf("rlen:%d\n",rlen); */
    return rlen;
}

int spdif_data_size(void *hdl)
{
    struct s_spdif_decode *spdif_hdl = (struct s_spdif_decode *)hdl;
    return cbuf_get_data_size(&spdif_hdl->dec_cbuf);
}

int spdif_data_total(void *hdl)
{
    struct s_spdif_decode *spdif_hdl = (struct s_spdif_decode *)hdl;
    return spdif_hdl->dec_cbuf.total_len;
}
static void reset_pcm_sample_rate(u32 sr)
{
    spdif_dec_hdl->pcm_sr = sr;
}
static u32 get_pcm_sample_rate(void)
{
    /* printf("[%d]",spdif_dec_hdl->pcm_sr); */
    return spdif_dec_hdl->pcm_sr;
}
/*----------------------------------------------------------------------------*/
/**@brief    pcm解码数据输出
   @param    *entry: 音频流句柄
   @param    *in: 输入信息
   @param    *out: 输出信息
   @return   输出长度
   @note     *out未使用
*/
/*----------------------------------------------------------------------------*/
static int pcm_dec_data_handler(struct audio_stream_entry *entry,
                                struct audio_data_frame *in,
                                struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct pcm_decoder *pcm_dec = container_of(decoder, struct pcm_decoder, decoder);
    if (in->offset == 0) {
        in->sample_rate = get_pcm_sample_rate();
    }
    audio_stream_run(&decoder->entry, in);
    return decoder->process_len;
}
static void pcm_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
}
/*----------------------------------------------------------------------------*/
/**@brief    pcm解码数据流激活
   @param    *p: 私有句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void dec_out_stream_resume(void *p)
{
    struct s_spdif_decode *dec = (struct s_spdif_decode *)p;

    audio_decoder_resume(&dec->pcm_dec.decoder);
}


int spdif_dec_start(void)
{
    int err = 0;
    u8 ch_num = 0;
    struct audio_fmt f;
    if (!spdif_dec_hdl) {
        return -EINVAL;
    }
    if (spdif_dec_hdl->fmt.coding_type == AUDIO_CODING_DTS) {
        puts("dts \n");//DTS未测试
        return -EINVAL;
        err = file_decoder_open(&spdif_dec_hdl->file_dec, &file_input, &decode_task, NULL, 0);
        if (err) {
            return err;
        }
        file_decoder_set_event_handler(&spdif_dec_hdl->file_dec, file_dec_event_handler, 0);

    } else {
        puts("pcm \n");
        err = pcm_decoder_open(&spdif_dec_hdl->pcm_dec, &decode_task);
        if (err) {
            return err;
        }
        pcm_decoder_set_event_handler(&spdif_dec_hdl->pcm_dec, pcm_dec_event_handler, 0);
        pcm_decoder_set_read_data(&spdif_dec_hdl->pcm_dec, spdif_dec_read, spdif_dec_hdl);
        pcm_decoder_set_data_handler(&spdif_dec_hdl->pcm_dec, pcm_dec_data_handler);
    }
    printf("\n--func=%s\n", __FUNCTION__);

    audio_mode_main_dec_open(AUDIO_MODE_MAIN_STATE_DEC_SPDIF);

    audio_mixer_ch_open_head(&spdif_dec_hdl->mix_ch, &mixer);
    audio_mixer_ch_set_no_wait(&spdif_dec_hdl->mix_ch, 1, 10); // 超时自动丢数
    /* audio_mixer_ch_set_src(&spdif_dec_hdl->mix_ch, 1, 0); */
#if 1
    struct audio_mixer_ch_sync_info info = {0};
    info.priv = spdif_dec_hdl;
    info.get_total = spdif_data_total;
    info.get_size = spdif_data_size;
    audio_mixer_ch_set_sync(&spdif_dec_hdl->mix_ch, &info, 1, 1);
#endif

    ch_num = audio_output_channel_num();
    u16 sample_rate = spdif_dec_hdl->fmt.sample_rate;
    spdif_dec_hdl->eq_drc = spdif_eq_drc_open(sample_rate, ch_num);

#if TCFG_EQ_DIVIDE_ENABLE
    spdif_dec_hdl->eq_drc_rl_rr = spdif_rl_rr_eq_drc_open(sample_rate, ch_num);
    if (spdif_dec_hdl->eq_drc_rl_rr) {
        audio_vocal_tract_open(&spdif_dec_hdl->vocal_tract, AUDIO_SYNTHESIS_LEN);
        {
            u8 entry_cnt = 0;
            struct audio_stream_entry *entries[8] = {NULL};
            entries[entry_cnt++] = &spdif_dec_hdl->vocal_tract.entry;
            entries[entry_cnt++] = &spdif_dec_hdl->mix_ch.entry;
            spdif_dec_hdl->vocal_tract.stream = audio_stream_open(&spdif_dec_hdl->vocal_tract, audio_vocal_tract_stream_resume);
            audio_stream_add_list(spdif_dec_hdl->vocal_tract.stream, entries, entry_cnt);
        }
        audio_vocal_tract_synthesis_open(&spdif_dec_hdl->synthesis_ch_fl_fr, &spdif_dec_hdl->vocal_tract, FL_FR);
        audio_vocal_tract_synthesis_open(&spdif_dec_hdl->synthesis_ch_rl_rr, &spdif_dec_hdl->vocal_tract, RL_RR);
    } else {
        spdif_dec_hdl->ch_switch = channel_switch_open(AUDIO_CH_QUAD, AUDIO_SYNTHESIS_LEN / 2);
    }
#ifdef CONFIG_MIXER_CYCLIC
    audio_mixer_ch_set_aud_ch_out(&spdif_dec_hdl->mix_ch, 0, BIT(0));
    audio_mixer_ch_set_aud_ch_out(&spdif_dec_hdl->mix_ch, 1, BIT(1));
    audio_mixer_ch_set_aud_ch_out(&spdif_dec_hdl->mix_ch, 2, BIT(2));
    audio_mixer_ch_set_aud_ch_out(&spdif_dec_hdl->mix_ch, 3, BIT(3));
#endif

#endif



    // 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    if (spdif_dec_hdl->fmt.coding_type == AUDIO_CODING_DTS) {
        entries[entry_cnt++] = &spdif_dec_hdl->file_dec.decoder.entry;
    } else {

        entries[entry_cnt++] = &spdif_dec_hdl->pcm_dec.decoder.entry;
    }
#if TCFG_EQ_ENABLE && TCFG_SPDIF_MODE_EQ_ENABLE
    if (spdif_dec_hdl->eq_drc) {
        entries[entry_cnt++] = &spdif_dec_hdl->eq_drc->entry;
    } else {
        printf("------eq_drc NULL------\n\n\n");
    }
#endif
#if TCFG_EQ_DIVIDE_ENABLE
    if (spdif_dec_hdl->eq_drc_rl_rr) {
        entries[entry_cnt++] = &spdif_dec_hdl->synthesis_ch_fl_fr.entry;//四声道eq独立时，该节点后不接节点
    } else {
        if (spdif_dec_hdl->ch_switch) {
            entries[entry_cnt++] = &spdif_dec_hdl->ch_switch->entry;
        }
        entries[entry_cnt++] = &spdif_dec_hdl->mix_ch.entry;
    }
#else
    entries[entry_cnt++] = &spdif_dec_hdl->mix_ch.entry;
#endif

    spdif_dec_hdl->audio_stream = audio_stream_open(spdif_dec_hdl, dec_out_stream_resume);
    audio_stream_add_list(spdif_dec_hdl->audio_stream, entries, entry_cnt);
#if TCFG_EQ_DIVIDE_ENABLE
    if (spdif_dec_hdl->eq_drc_rl_rr) { //接在eq_drc的上一个节点
        audio_stream_add_entry(entries[0], &spdif_dec_hdl->eq_drc_rl_rr->entry);
        audio_stream_add_entry(&spdif_dec_hdl->eq_drc_rl_rr->entry, &spdif_dec_hdl->synthesis_ch_rl_rr.entry);
    }
#endif

    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);
    err = audio_decoder_start(&spdif_dec_hdl->pcm_dec.decoder);
    if (err) {
        spdif_eq_drc_close(spdif_dec_hdl->eq_drc);
#if TCFG_EQ_DIVIDE_ENABLE
        spdif_rl_rr_eq_drc_close(spdif_dec_hdl->eq_drc_rl_rr);
        audio_vocal_tract_synthesis_close(&spdif_dec_hdl->synthesis_ch_fl_fr);
        audio_vocal_tract_synthesis_close(&spdif_dec_hdl->synthesis_ch_rl_rr);
        audio_vocal_tract_close(&spdif_dec_hdl->vocal_tract);
        channel_switch_close(&spdif_dec_hdl->ch_switch);
#endif
        return err;
    }
    clock_set_cur();
    printf("\n--func=%s\n", __FUNCTION__);
    spdif_dec_hdl->status = SPDIF_STATE_START;
    return 0;
}

int spdif_dec_stop(void)
{
    if (spdif_dec_hdl) {
        spdif_dec_hdl->status = SPDIF_STATE_STOP;
        pcm_decoder_close(&spdif_dec_hdl->pcm_dec);
        spdif_eq_drc_close(spdif_dec_hdl->eq_drc);
#if TCFG_EQ_DIVIDE_ENABLE
        spdif_rl_rr_eq_drc_close(spdif_dec_hdl->eq_drc_rl_rr);
        audio_vocal_tract_synthesis_close(&spdif_dec_hdl->synthesis_ch_fl_fr);
        audio_vocal_tract_synthesis_close(&spdif_dec_hdl->synthesis_ch_rl_rr);
        audio_vocal_tract_close(&spdif_dec_hdl->vocal_tract);
        channel_switch_close(&spdif_dec_hdl->ch_switch);
#endif

        audio_mixer_ch_close(&spdif_dec_hdl->mix_ch);
        if (spdif_dec_hdl->audio_stream) {
            audio_stream_close(spdif_dec_hdl->audio_stream);
            spdif_dec_hdl->audio_stream = 0;
        }
    }
    return 0;
}
static int spdif_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    if (event == AUDIO_RES_GET) {
        err = spdif_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        err = spdif_dec_stop();
    }
    return err;
}


static void spdif_status_check(void *priv)
{
    static u8 cnt = 0;
    if (spdif_dec_hdl && (spdif_slave_hdl.status == SPDIF_STATE_START || spdif_slave_hdl.status == SPDIF_STATE_STOP)) {
        /* printf("spdif status \n"); */
        if (spdif_slave_hdl.error_flag == 1) {
            printf("spdif reset \n");
            /* audio_spdif_slave_close(&spdif_slave_hdl); */
            spdif_dec_close();
            struct audio_fmt fmt;
            fmt.channel = 2;
            fmt.coding_type = 	AUDIO_CODING_PCM;
            fmt.sample_rate = SPDIF_DEFAULT_OUT_SAMPLE_RATE;
            spdif_dec_open(fmt);
            audio_spdif_slave_open(&spdif_slave_hdl);
            audio_spdif_slave_start(&spdif_slave_hdl);
            spdif_slave_hdl.error_flag = 0;
        } else if (spdif_slave_hdl.error_flag == 2) {
            printf("spdif reset \n");
            /* audio_spdif_slave_close(&spdif_slave_hdl); */
            spdif_dec_close();
            struct audio_fmt fmt;
            fmt.channel = 2;
            fmt.coding_type = 	AUDIO_CODING_DTS;
            fmt.sample_rate = SPDIF_DEFAULT_OUT_SAMPLE_RATE;
            spdif_dec_open(fmt);
            audio_spdif_slave_open(&spdif_slave_hdl);
            audio_spdif_slave_start(&spdif_slave_hdl);
            spdif_slave_hdl.error_flag = 0;
        } else {
#if 10
            if (spdif_dec_hdl->status == SPDIF_STATE_START) {
                if (spdif_data_size(spdif_dec_hdl) == 0) {
                    if (cnt < 6) {
                        cnt++;
                    }
                    if (cnt == 5) {
                        audio_mixer_ch_pause(&spdif_dec_hdl->mix_ch, 1);
                        audio_decoder_resume_all(&decode_task);
                    }
                } else {
                    if (cnt > 4) {
                        audio_mixer_ch_pause(&spdif_dec_hdl->mix_ch, 0);
                        audio_decoder_resume_all(&decode_task);
                    }
                    cnt = 0;
                }
            }
#endif
        }
    }
}
///*----------------------------------------------------------------------------*/
/**@brief    spdif 硬件设置
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void _spdif_open(void)
{
#if	TCFG_SPDIF_OUTPUT_ENABLE
    spdif_slave_hdl.output_port = SPDIF_OUT_PORT_A;//PB11
#endif
    spdif_slave_hdl.input_port  = SPDIF_IN_PORT_A;//PA9
    audio_spdif_slave_open(&spdif_slave_hdl);

    audio_spdif_slave_start(&spdif_slave_hdl);
#if TCFG_HDMI_ARC_ENABLE
    extern void hdmi_cec_init(void);
    hdmi_cec_init();
#endif
}

void spdif_init(void)
{
    audio_spdif_slave_init(&spdif_slave_hdl);
    request_irq(IRQ_SPDIF_IDX, 2, spdif_isr_handler, 0);
    sys_timer_add(NULL, spdif_status_check, 250);

#if (TCFG_APP_SPDIF_EN == 0)
    spdif_dec_init();
    _spdif_open();  //for test
#endif

}

void spdif_dec_init(void)
{
    struct audio_fmt fmt;
    fmt.channel = 2;
    fmt.coding_type = 	AUDIO_CODING_PCM;
    fmt.sample_rate = SPDIF_DEFAULT_OUT_SAMPLE_RATE;
    spdif_dec_open(fmt);
}

int spdif_dec_open(struct audio_fmt fmt)
{
    int err;
    if (spdif_dec_hdl) {
        return -1;
    }
    spdif_dec_hdl = zalloc(sizeof(struct s_spdif_decode));
    if (!spdif_dec_hdl) {
        return -ENOMEM;
    }

    clock_add(SPDIF_CLK);
    printf("\n--func=%s\n", __FUNCTION__);
    cbuf_init(&spdif_dec_hdl->dec_cbuf, spdif_dec_hdl->dec_data_buf, SPDIF_DEC_DATA_BUF_LEN);
    spdif_dec_hdl->fmt.channel = fmt.channel;
    spdif_dec_hdl->fmt.coding_type = fmt.coding_type;
    spdif_dec_hdl->fmt.sample_rate = fmt.sample_rate;
    spdif_dec_hdl->fmt.coding_type = fmt.coding_type;
    spdif_dec_hdl->coding_type = fmt.coding_type;
    if (fmt.coding_type == AUDIO_CODING_DTS) {
        spdif_dec_hdl->file_dec.ch_type = AUDIO_CH_MAX;
        spdif_dec_hdl->file_dec.output_ch_num = audio_output_channel_num();
        spdif_dec_hdl->file_dec.output_ch_type = audio_output_channel_type();
    } else {
        old_samplerate_index = 6;
        spdif_dec_hdl->pcm_dec.ch_num = fmt.channel;
        spdif_dec_hdl->pcm_dec.output_ch_num = audio_output_channel_num();
        spdif_dec_hdl->pcm_dec.output_ch_type = audio_output_channel_type();
        spdif_dec_hdl->pcm_dec.sample_rate = fmt.sample_rate;
        spdif_dec_hdl->pcm_sr = fmt.sample_rate;
    }
    spdif_dec_hdl->wait.priority = 4;
    spdif_dec_hdl->wait.preemption = 0;
    spdif_dec_hdl->wait.protect = 1;//1 叠加模式
    spdif_dec_hdl->wait.snatch_same_prio = 1;
    spdif_dec_hdl->wait.handler = spdif_wait_res_handler;
    spdif_dec_hdl->status = SPDIF_STATE_OPEN;
    err = audio_decoder_task_add_wait(&decode_task, &spdif_dec_hdl->wait);
    return err;
}

void spdif_dec_close(void)
{
    printf("\n--func1=%s\n", __FUNCTION__);
    if (!spdif_dec_hdl) {
        return;
    }
    spdif_dec_stop();
    spdif_dec_release();
    clock_set_cur();
}

bool spdif_dec_check(void)
{
    if (spdif_dec_hdl) {

        return true;
    }
    return false;
}

/*----------------------------------------------------------------------------*/
/**@brief    spdi模式 eq drc 打开
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
void *spdif_eq_drc_open(u16 sample_rate, u8 ch_num)
{

#if TCFG_EQ_ENABLE
    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

#if TCFG_SPDIF_MODE_EQ_ENABLE
    effect_parm.eq_en = 1;

#if TCFG_DRC_ENABLE
#if TCFG_SPDIF_MODE_DRC_ENABLE
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
/**@brief    spdif式 eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void spdif_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
#if TCFG_SPDIF_MODE_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
#if TCFG_DRC_ENABLE
#if TCFG_SPDIF_MODE_DRC_ENABLE
        clock_remove(EQ_DRC_CLK);
#endif
#endif
    }
#endif
#endif
    return;
}


void *spdif_rl_rr_eq_drc_open(u16 sample_rate, u8 ch_num)
{

#if TCFG_EQ_ENABLE

    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};
#if TCFG_SPDIF_MODE_EQ_ENABLE
    effect_parm.eq_en = 1;

#if TCFG_DRC_ENABLE
#if TCFG_SPDIF_MODE_DRC_ENABLE
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
/**@brief    spdif模式 RL RR 通道eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void spdif_rl_rr_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
#if TCFG_SPDIF_MODE_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
#if TCFG_DRC_ENABLE
#if TCFG_SPDIF_MODE_DRC_ENABLE
        clock_remove(EQ_DRC_CLK);
#endif
#endif
    }
#endif
#endif
    return;
}


#endif      // TCFG_SPDIF_ENABLE
/***********************spdif dec end*****************************/
