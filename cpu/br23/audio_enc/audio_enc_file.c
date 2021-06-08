
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "encode/encode_write.h"
#include "asm/audio_src.h"
#include "audio_enc.h"
#include "app_main.h"
#include "app_task.h"
#include "clock_cfg.h"
#include "dev_manager.h"
#include "btstack/btstack_task.h"
#include "btstack/avctp_user.h"

#if TCFG_ENC_WRITE_FILE_ENABLE

#define PCM_ENC2FILE_PCM_LEN				(2 * 1024)
#define PCM_ENC2FILE_FILE_LEN				(8 * 1024)

#define PCM_ENC2FILE_PCM_LEN_OVERLAY		(1 * 1024 + 512)
#define PCM_ENC2FILE_FILE_LEN_OVERLAY		(2 * 1024)

#define WAV_FILE_HEAD_LEN                   90

#define PCM2FILE_ENC_BUF_COUNT				1

//wav录音头部信息定时更新时间设置， 单位ms
#define UPDATA_WAV_HEAD_TIME                (5000)

//是否使用overlay 使能， 695暂时不支持overlay
#define PCM_ENC2FILE_USE_OVERLAY_EN			0


extern struct audio_encoder_task *encode_task;

struct pcm2file_enc_hdl {
    struct audio_encoder encoder;
    s16 output_frame[1152 / 2];             //align 4Bytes

    int pcm_frame[64];                 //align 4Bytes
    u8 file_head_frame[128];
    u8 file_head_len;
    /* u8 pcm_buf[PCM_ENC2FILE_PCM_LEN]; */
    u8 *pcm_buf;
    cbuffer_t pcm_cbuf;

    int out_file_frame[512 / 4];
    /* u8 out_file_buf[PCM_ENC2FILE_FILE_LEN]; */
    u8 *out_file_buf;
    cbuffer_t out_file_cbuf;

    void *whdl;
    OS_SEM 	 sem_wfile;
    OS_MUTEX mutex;

    volatile u32 status : 1;
        volatile u32 enc_err : 1;
        volatile u32 encoding : 1;
        volatile u32 write_head_busy : 1;

        u32 lost;

#if PCM2FILE_ENC_BUF_COUNT
        u16 pcm_buf_max;
        u16 out_file_max;
#endif

        u32 head_update_tick;

    };


    static volatile u8 pcm2file_used_overlay = 0;

#if (RECORDER_MIX_EN && PCM_ENC2FILE_USE_OVERLAY_EN)
    struct pcm2file_enc_hdl pcm2file_enc_overlay_handle sec(.enc_file_mem);
    u8 pcm_buf_overlay[PCM_ENC2FILE_PCM_LEN_OVERLAY] sec(.enc_file_mem);
    u8 out_file_buf_overlay[PCM_ENC2FILE_FILE_LEN_OVERLAY] sec(.enc_file_mem);
#endif/*RECORDER_MIX_EN*/



    static void pcm2file_enc_resume(struct pcm2file_enc_hdl *enc)
{
    audio_encoder_resume(&enc->encoder);
}

static void pcm2file_wfile_resume(struct pcm2file_enc_hdl *enc)
{
    os_sem_set(&enc->sem_wfile, 0);
    os_sem_post(&enc->sem_wfile);
    enc_write_file_resume(enc->whdl);
}

// 写pcm数据
int pcm2file_enc_write_pcm(void *priv, s16 *data, int len)
{
    struct pcm2file_enc_hdl *enc = (struct pcm2file_enc_hdl *)priv;
    if (!enc || enc->enc_err) {
        return 0;
    }

    u16 wlen = 0;
    os_mutex_pend(&enc->mutex, 0);
    if (enc->status) {
        wlen = cbuf_write(&enc->pcm_cbuf, data, len);
        if (!wlen) {
            enc->lost++;
            putchar('~');
        } else {
            /* putchar('G');//__G__ */
        }
#if PCM2FILE_ENC_BUF_COUNT
        if (enc->pcm_buf_max < enc->pcm_cbuf.data_len) {
            enc->pcm_buf_max = enc->pcm_cbuf.data_len;
        }
#endif
        /* printf("wl:%d ", wlen); */
        // 激活录音编码器
        pcm2file_enc_resume(enc);
    }
    os_mutex_post(&enc->mutex);
    return wlen;
}

// 编码器获取数据
static int pcm2file_enc_pcm_get(struct audio_encoder *encoder, s16 **frame, u16 frame_len)
{
    int rlen = 0;
    int dlen = 0;
    if (encoder == NULL) {
        r_printf("encoder NULL");
    }
    struct pcm2file_enc_hdl *enc = container_of(encoder, struct pcm2file_enc_hdl, encoder);

    if (enc == NULL) {
        r_printf("enc NULL");
    }

    /* printf("l:%d", frame_len); */

    if (!enc->status) {
        return 0;
    }
    if (enc->enc_err) {
        return 0;
    }

    dlen = cbuf_get_data_len(&enc->pcm_cbuf);
    if (dlen < frame_len) {
        /* putchar('T');//__T__ */
        return 0;
    }

    rlen = cbuf_read(&enc->pcm_cbuf, enc->pcm_frame, frame_len);

    *frame = enc->pcm_frame;

#if (RECORDER_MIX_EN)
    recorder_mix_stream_resume();
#endif/*RECORDER_MIX_EN*/

    return rlen;
}

static void pcm2file_enc_pcm_put(struct audio_encoder *encoder, s16 *frame)
{
}

static const struct audio_enc_input pcm2file_enc_input = {
    .fget = pcm2file_enc_pcm_get,
    .fput = pcm2file_enc_pcm_put,
};

static int pcm2file_enc_probe_handler(struct audio_encoder *encoder)
{
    return 0;
}
// 编码器输出
static int pcm2file_enc_output_handler(struct audio_encoder *encoder, u8 *frame, int len)
{
    struct pcm2file_enc_hdl *enc = container_of(encoder, struct pcm2file_enc_hdl, encoder);

    int wlen = cbuf_write(&enc->out_file_cbuf, frame, len);
#if PCM2FILE_ENC_BUF_COUNT
    if (enc->out_file_max < enc->out_file_cbuf.data_len) {
        enc->out_file_max = enc->out_file_cbuf.data_len;
    }
#endif
    pcm2file_wfile_resume(enc);
    /* if (wlen != len) { */
    /* printf("X"); */
    /* } */
    /* if (!enc->status) { */
    /* return 0; */
    /* } */
    if (enc->enc_err) {
        return 0;
    }

    return wlen;
}

static void pcm2file_enc_get_head_info(struct audio_encoder *encoder)
{
    struct pcm2file_enc_hdl *pcm2file = container_of(encoder, struct pcm2file_enc_hdl, encoder);
    u16 len = 0;
    u8 *ptr = (u8 *)audio_encoder_ioctrl(&pcm2file->encoder, 2, AUDIO_ENCODER_IOCTRL_CMD_GET_HEAD_INFO, &len);
    /* printf("%s, ptr = %x, len = %d\n", __FUNCTION__, ptr, len); */
    if (ptr) {
        if (len > sizeof(pcm2file->file_head_frame)) {
            printf("file_head_frame buf not enough\n");
            return;
        }
        memcpy(pcm2file->file_head_frame, ptr, len);
        /* put_buf(pcm2file->file_head_frame, len); */
        pcm2file->file_head_len = len;
    }
}

static int pcm2file_enc_close_handler(struct audio_encoder *encoder)
{
    //做一些编码关闭前的操作， 例如：adpcm写头操作
    pcm2file_enc_get_head_info(encoder);//写编码头部信息
    return 0;
}

const static struct audio_enc_handler pcm2file_enc_handler = {
    .enc_probe = pcm2file_enc_probe_handler,
    .enc_output = pcm2file_enc_output_handler,
    .enc_close = pcm2file_enc_close_handler,
};

static int enc_wfile_set_head(void *hdl, char **head)
{
    struct pcm2file_enc_hdl *enc = hdl;
    /* struct enc_write_test *tst = hdl; */
    *head = enc->file_head_frame;
    return enc->file_head_len;
}


static void updata_wav_head_info(struct audio_encoder *encoder, void *priv)
{
    int len;
    char *head;
    int cur_write_pos;
    struct audio_fmt *enc_fmt;
    struct pcm2file_enc_hdl *enc = (struct pcm2file_enc_hdl *)priv;

    /* extern int audio_encoder_get_fmt(struct audio_encoder * enc, struct audio_fmt **fmt); */
    audio_encoder_get_fmt(encoder, &enc_fmt);
    if ((enc_fmt->coding_type != AUDIO_CODING_WAV)
        && (enc_fmt->coding_type != AUDIO_CODING_G726)
       ) {
        return;
    }

    cur_write_pos = fpos(get_wfil_file(enc->whdl));
    pcm2file_enc_get_head_info(encoder);
    len = enc_wfile_set_head(get_wfil_head_hdl(enc->whdl), &head);
    if (len) {
        fseek(get_wfil_file(enc->whdl), 0, SEEK_SET);
        fwrite(get_wfil_file(enc->whdl), head, len);
    }
    fseek(get_wfil_file(enc->whdl), cur_write_pos, SEEK_SET);
    //printf("update head!!\n");
}

static void pcm2file_enc_w_evt(void *hdl, int evt, int parm)
{
    struct pcm2file_enc_hdl *enc = hdl;
    printf("evt: %d ", evt);
    if (evt == ENC_WRITE_FILE_EVT_WRITE_ERR) {
        enc->enc_err = 1;
        pcm2file_wfile_resume(enc);
        pcm2file_enc_resume(enc);
        audio_encoder_stop(&enc->encoder);
    } else if (evt == ENC_WRITE_FILE_EVT_FILE_CLOSE) {
        printf("sclust: %d ", parm);
    }
}

static int pcm2file_enc_w_get(void *hdl, s16 **frame, u16 frame_len)
{
    int rlen;
    struct pcm2file_enc_hdl *enc = hdl;
    os_sem_set(&enc->sem_wfile, 0);
    /* printf("r:%d", frame_len); */
    do {
        rlen = cbuf_read(&enc->out_file_cbuf, enc->out_file_frame, frame_len);
        if (rlen == frame_len) {
//            if (cbuf_get_data_len(&enc->out_file_cbuf) <= (PCM_ENC2FILE_FILE_LEN / 2)) {
//                ///当写文件有数据取走达到一定程度主动激活编码线程
//                pcm2file_enc_resume(enc);
//            }
            break;
        }

        if (!enc->status) {
            rlen = cbuf_get_data_len(&enc->out_file_cbuf);
            rlen = cbuf_read(&enc->out_file_cbuf, enc->out_file_frame, rlen);
            break;
        }

        if (enc->enc_err) {
            return 0;
        }
        pcm2file_enc_resume(enc);
        os_sem_pend(&enc->sem_wfile, 2);
    } while (1);

    *frame = enc->out_file_frame;
    return rlen;
}
static void pcm2file_enc_w_put(void *hdl, s16 *frame)
{
#if TCFG_NOR_FS_ENABLE || FLASH_INSIDE_REC_ENABLE
    return ;
#endif

    struct pcm2file_enc_hdl *enc = (struct pcm2file_enc_hdl *)hdl;
    u32 cur_sys_time = timer_get_ms();
    os_mutex_pend(&enc->mutex, 0);
    if (enc->status == 0) {
        os_mutex_post(&enc->mutex);
        return ;
    }
    if (cur_sys_time - enc->head_update_tick >= UPDATA_WAV_HEAD_TIME) {
        updata_wav_head_info(&(enc->encoder), enc);
        enc->head_update_tick = cur_sys_time;
    }
    os_mutex_post(&enc->mutex);
}

const struct audio_enc_write_input pcm2file_enc_w_input = {
    .get = pcm2file_enc_w_get,
    .put = pcm2file_enc_w_put,
};





void pcm2file_enc_write_file_set_limit(void *hdl, u32 cut_size, u32 limit_size)
{
    struct pcm2file_enc_hdl *pcm2file = hdl;
    enc_write_file_set_limit(pcm2file->whdl, cut_size, limit_size);
}

void pcm2file_enc_set_evt_handler(void *hdl, void (*handler)(struct audio_encoder *, int, int *), u32 maigc)
{
    struct pcm2file_enc_hdl *pcm2file = hdl;
    audio_encoder_set_event_handler(&pcm2file->encoder, handler, maigc);
}

void pcm2file_enc_start(void *hdl)
{
    struct pcm2file_enc_hdl *pcm2file = hdl;

    pcm2file->status = 1;

    audio_encoder_start(&pcm2file->encoder);

    enc_write_file_start(pcm2file->whdl);

    //记录录音启动时的tick， 后面定时更新头部信息用到
    pcm2file->head_update_tick = timer_get_ms();
}

int pcm2file_enc_get_time(void *hdl)
{
    struct pcm2file_enc_hdl *pcm2file = hdl;
    if (!pcm2file) {
        return 0;
    }
    os_mutex_pend(&pcm2file->mutex, 0);
    if (pcm2file->status == 0) {
        os_mutex_post(&pcm2file->mutex);
        return 0;
    }
    int ret = audio_encoder_ioctrl(&pcm2file->encoder, 1, AUDIO_ENCODER_IOCTRL_CMD_GET_TIME);
    os_mutex_post(&pcm2file->mutex);
    return ret;
}
void *pcm2file_enc_open(struct audio_fmt *pfmt, char *logo, char *folder, char *filename)
{
    int err;
    struct pcm2file_enc_hdl *pcm2file = NULL;
    char *temp_filename = NULL;
    u32 out_file_buf_size = 0;
    u32 pcm_buf_size = 0;

    if (pfmt->coding_type != AUDIO_CODING_MP3 && pfmt->coding_type != AUDIO_CODING_WAV && pfmt->coding_type != AUDIO_CODING_G726) {
        return NULL;
    }

    ///创建录音编码线程
    audio_encoder_task_open();

#if (RECORDER_MIX_EN && PCM_ENC2FILE_USE_OVERLAY_EN)
    if (BT_CALL_HANGUP != get_call_status()) {
        printf("[%s], used overlay!!\n", __FUNCTION__);
        pcm2file_used_overlay = 1;
        pcm2file = &pcm2file_enc_overlay_handle;
        memset(&pcm2file_enc_overlay_handle, 0, sizeof(struct pcm2file_enc_hdl));
        pcm2file->pcm_buf = pcm_buf_overlay;
        pcm2file->out_file_buf = out_file_buf_overlay;
        pcm_buf_size = PCM_ENC2FILE_PCM_LEN_OVERLAY;
        out_file_buf_size = PCM_ENC2FILE_FILE_LEN_OVERLAY;
        memset(pcm2file->pcm_buf, 0, PCM_ENC2FILE_PCM_LEN_OVERLAY);
        memset(pcm2file->out_file_buf, 0, PCM_ENC2FILE_FILE_LEN_OVERLAY);
    } else
#endif/*RECORDER_MIX_EN*/
    {
        printf("[%s], used malloc!!\n", __FUNCTION__);
        pcm2file_used_overlay = 0;
        pcm2file = zalloc(sizeof(struct pcm2file_enc_hdl));
        if (!pcm2file) {
            goto __out;
        }
        pcm_buf_size = PCM_ENC2FILE_PCM_LEN;
        out_file_buf_size = PCM_ENC2FILE_FILE_LEN;
        pcm2file->pcm_buf = zalloc(pcm_buf_size);
        if (pcm2file->pcm_buf == NULL) {
            goto __out;
        }
        pcm2file->out_file_buf = zalloc(out_file_buf_size);
        if (pcm2file->out_file_buf == NULL) {
            goto __out;
        }

    }

    os_sem_create(&pcm2file->sem_wfile, 0);
    os_mutex_create(&pcm2file->mutex);
    cbuf_init(&pcm2file->out_file_cbuf, pcm2file->out_file_buf, out_file_buf_size);

    temp_filename = zalloc(strlen(filename) + 5);
    if (temp_filename == NULL) {
        goto __out;
    }
    if (pfmt->coding_type == AUDIO_CODING_MP3) {
        strcat(temp_filename, filename);
        strcat(temp_filename, ".mp3");
    } else if (pfmt->coding_type == AUDIO_CODING_WAV || pfmt->coding_type == AUDIO_CODING_G726) {
        strcat(temp_filename, filename);
        strcat(temp_filename, ".wav");
    }
    pcm2file->whdl = enc_write_file_open(logo, folder, temp_filename);
    free(temp_filename);
    if (!pcm2file->whdl) {
        goto __out;
    }

    enc_write_file_set_evt_handler(pcm2file->whdl, pcm2file_enc_w_evt, pcm2file);
    enc_write_file_set_input(pcm2file->whdl, &pcm2file_enc_w_input, pcm2file, sizeof(pcm2file->out_file_frame));
    if ((pfmt->coding_type == AUDIO_CODING_WAV) || (pfmt->coding_type == AUDIO_CODING_G726)) {
        pcm2file->file_head_len = WAV_FILE_HEAD_LEN;
        enc_write_file_set_head_handler(pcm2file->whdl, enc_wfile_set_head, pcm2file);
    }
    if (pfmt->coding_type == AUDIO_CODING_MP3) {
        if ((pfmt->sample_rate < 16000) && (pfmt->bit_rate > 64)) {
            pfmt->bit_rate = 64;
        }
    }
    cbuf_init(&pcm2file->pcm_cbuf, pcm2file->pcm_buf, pcm_buf_size);
    audio_encoder_open(&pcm2file->encoder, &pcm2file_enc_input, encode_task);
    audio_encoder_set_handler(&pcm2file->encoder, &pcm2file_enc_handler);
    audio_encoder_set_fmt(&pcm2file->encoder, pfmt);
    audio_encoder_set_output_buffs(&pcm2file->encoder, pcm2file->output_frame,
                                   sizeof(pcm2file->output_frame), 1);

    return pcm2file;
__out:
    if (pcm2file_used_overlay == 0) {
        if (pcm2file->pcm_buf) {
            free(pcm2file->pcm_buf);
        }
        if (pcm2file->out_file_buf) {
            free(pcm2file->out_file_buf);
        }
        if (pcm2file) {
            free(pcm2file);
        }
    }
    return NULL;
}


void pcm2file_enc_close(void **hdl)
{
    struct pcm2file_enc_hdl *pcm2file = *hdl;
    if (!pcm2file) {
        return;
    }
    os_mutex_pend(&pcm2file->mutex, 0);
    pcm2file->status = 0;
    os_mutex_post(&pcm2file->mutex);
    audio_encoder_close(&pcm2file->encoder);
    enc_write_file_stop(pcm2file->whdl, 1000);
    enc_write_file_close(pcm2file->whdl);

    printf("pcm2file_enc_close, lost:%d ", pcm2file->lost);

#if PCM2FILE_ENC_BUF_COUNT
    printf("pcm_buf_max:%d,%d/100; out_file_max:%d,%d/100 ",
           pcm2file->pcm_buf_max, pcm2file->pcm_buf_max * 100 / PCM_ENC2FILE_PCM_LEN,
           pcm2file->out_file_max, pcm2file->out_file_max * 100 / PCM_ENC2FILE_FILE_LEN
          );
#endif
    if (pcm2file_used_overlay == 0) {
        printf("[%s], used malloc, need free!!\n", __FUNCTION__);
        if (pcm2file->pcm_buf) {
            free(pcm2file->pcm_buf);
        }
        if (pcm2file->out_file_buf) {
            free(pcm2file->out_file_buf);
        }
        OS_ENTER_CRITICAL();
        free(pcm2file);
        *hdl = NULL;
        OS_EXIT_CRITICAL();
    } else {
        printf("[%s], used overlay, noneed free!!\n", __FUNCTION__);
    }

    audio_encoder_task_close();
}

int pcm2file_enc_is_work(void *hdl)
{
    struct pcm2file_enc_hdl *enc = hdl;
    if (!enc || !enc->status || enc->enc_err) {
        return false;
    }
    return true;
}

int get_pcm2file_enc_file_len(void *hdl)
{
    struct pcm2file_enc_hdl *pcm2file = hdl;
    return get_enc_file_len(pcm2file->whdl);
}

struct audio_encoder *get_pcm2file_encoder_hdl(void *hdl)
{
    struct pcm2file_enc_hdl *enc = hdl;
    return &(enc->encoder);
}


#endif






