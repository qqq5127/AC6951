/*****************************************************************
>file name : mic_rec.c
>author : lichao
>create time : Wed 26 Jun 2019 04:31:50 PM CST
*****************************************************************/
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "classic/tws_local_media_sync.h"
#include "3th_profile_api.h"
#include "classic/tws_api.h"
#include "btstack/avctp_user.h"
#include "bt_tws.h"

#include "btstack/third_party/baidu/dueros_dma.h"

#if (BT_MIC_EN)

enum {
    STANDARD_OPUS = 0 << 6,
    KUGOU_OPUS = 1 << 6,
};

enum {
    ENC_OPUS_16KBPS = 0,
    ENC_OPUS_32KBPS = 1,
    ENC_OPUS_64KBPS = 2,
};

typedef struct __ai_encode_info {
    const u8 *info;
    u32 enc_type;
    u8 opus_type;
} _ai_encode_info;
#define AI_ENC_INFO_REGISTER(name,enc_typ,opus_typ)\
	const _ai_encode_info name = {\
		.info = (const u8 *)#enc_typ" "#opus_typ,\
		.enc_type = enc_typ,\
		.opus_type = opus_typ,\
	}

#define LOG_TAG             "[MIC_REC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"
#define AUDIO_ENC_SYS_CLK_HZ     128 * 1000000L
#define MIC_REC_USE_AUDIO_BUFFER     1

extern u8 *dma_tws_mic_pool;
static struct server *ai_rec_server;
#if !MIC_REC_USE_AUDIO_BUFFER
#define MIC_BUFFER_OVERLAY_SIZE     3 * 1024
static u8 ai_mic_dma_buffer[MIC_BUFFER_OVERLAY_SIZE] sec(.opus_mem);
#endif

#define SOURCE_TYPE        0
#define SINK_TYPE_MASTER   1
#define SINK_TYPE_SLAVE    2

#if (DUEROS_DMA_EN)

#define AUDIO_SEND_MIN_BUF_LEN   200
#define AUDIO_SPEECH_BUF_LEN     (3*1024 + 512)
#define AI_PL_DATA_SEND			dma_speech_data_send
AI_ENC_INFO_REGISTER(ai_enc_info, AUDIO_CODING_OPUS, STANDARD_OPUS);
#elif (RCSP_BTMATE_EN)

#define AUDIO_SEND_MIN_BUF_LEN   200
#define AUDIO_SPEECH_BUF_LEN     (3*1024 + 512)
#define AI_PL_DATA_SEND				NULL
AI_ENC_INFO_REGISTER(ai_enc_info, AUDIO_CODING_OPUS, STANDARD_OPUS);
#elif (RCSP_ADV_EN)

#define AUDIO_SEND_MIN_BUF_LEN   200
#define AUDIO_SPEECH_BUF_LEN     (3*1024 + 512)
#define AI_PL_DATA_SEND				NULL
AI_ENC_INFO_REGISTER(ai_enc_info, AUDIO_CODING_OPUS, STANDARD_OPUS);
#elif (TME_EN)
#include "key_event_deal.h"
u16 TME_speech_data_send(u8 *buf, u16 len);
#define TME_OPUS_PACK_LEN        (46)
#define AUDIO_SEND_MIN_BUF_LEN   (TME_OPUS_PACK_LEN*4)
#define AUDIO_SPEECH_BUF_LEN     (2*1024 + 512)
#define AI_PL_DATA_SEND				TME_speech_data_send

AI_ENC_INFO_REGISTER(ai_enc_info, AUDIO_CODING_OPUS, KUGOU_OPUS);
#elif (GMA_EN)

#include "gma.h"
#define AUDIO_SEND_MIN_BUF_LEN      160
#define AUDIO_SPEECH_BUF_LEN        (3*1024 + 512)
#define AI_PL_DATA_SEND             gma_opus_voice_mic_send
AI_ENC_INFO_REGISTER(ai_enc_info, AUDIO_CODING_OPUS, STANDARD_OPUS);
#elif (XM_MMA_EN)
extern int XM_speech_data_send(u8 *buf, u16 len);
static u16 XM_send(u8 *buf, u16 len)
{
    //int to u16
    int ret = XM_speech_data_send(buf, len);

    return (ret == (-1)) ? (-1) : 1;
}
#define AUDIO_SEND_MIN_BUF_LEN   (200)
#define AUDIO_SPEECH_BUF_LEN     (2*1024 + 512)
#define AI_PL_DATA_SEND				XM_send

AI_ENC_INFO_REGISTER(ai_enc_info, AUDIO_CODING_SPEEX, 0);
#else

#define AUDIO_SEND_MIN_BUF_LEN   200
#define AUDIO_SPEECH_BUF_LEN     (3*1024 + 512)
#define AI_PL_DATA_SEND				NULL
AI_ENC_INFO_REGISTER(ai_enc_info, AUDIO_CODING_OPUS, STANDARD_OPUS);
#endif

void bt_sniff_ready_clean(void);
extern int mic_coder_busy_flag(void);
extern void cbuf_init(cbuffer_t *cbuffer, void *buf, u32 size);
struct __speech_buf_ctl {
    cbuffer_t cbuffer;
    volatile u8 speech_buf_resend_flg;
    volatile u8 speech_init_flg;
    u8 speex_frame_cnt;
};
static u8 *speech_buf = NULL;
static u8 *speech_cbuf = NULL;
static struct __speech_buf_ctl _speech_buf_ctl;
#define __this (&_speech_buf_ctl)


static OS_MUTEX mutex_ai_mic;

static void speech_cbuf_init(void)
{
    __this->speech_init_flg = 1;
    __this->speech_buf_resend_flg = 0;

    ASSERT(speech_buf == NULL, "speech_buf is err\n");
#if (TME_EN)
    /* speech_buf = malloc(AUDIO_SEND_MIN_BUF_LEN+1); */
#else
    speech_buf = malloc(AUDIO_SEND_MIN_BUF_LEN);
    ASSERT(speech_buf, "speech_buf is not ok\n");
#endif

    ASSERT(speech_cbuf == NULL, "speech_cbuf is err\n");
    speech_cbuf = malloc(AUDIO_SPEECH_BUF_LEN);
    ASSERT(speech_cbuf, "speech_cbuf is not ok\n");

    cbuf_init(&(__this->cbuffer), speech_cbuf, AUDIO_SPEECH_BUF_LEN);
}

static void speech_cbuf_exit(void)
{
    __this->speech_init_flg = 0;
    __this->speech_buf_resend_flg = 0;
    cbuf_clear(&(__this->cbuffer));

    free(speech_cbuf);
    speech_cbuf = NULL;

    if (speech_buf) {
        free(speech_buf);
    }
    speech_buf = NULL;

}
#if (TME_EN)
static u16 tme_speech_data_send(u8 *buf, u16 len, u16(*send_data)(u8 *buf, u16 len))
{
    u16 res = 0;

    u8 temp_buf[AUDIO_SEND_MIN_BUF_LEN + 1];
    if (__this->speech_init_flg == 0) {
        return 0;
    }

    if (cbuf_write(&(__this->cbuffer), buf, len) != len) {
        res = (u16)(-1);
    }

    /* printf("cl %d\n",cbuf_get_data_size(&(__this->cbuffer))); */

    while (cbuf_get_data_size(&(__this->cbuffer)) >= AUDIO_SEND_MIN_BUF_LEN) {
        __this->speech_buf_resend_flg = 1;
        cbuf_read_alloc_len(&(__this->cbuffer), &temp_buf[1], AUDIO_SEND_MIN_BUF_LEN);
        temp_buf[0] = (AUDIO_SEND_MIN_BUF_LEN + TME_OPUS_PACK_LEN - 1) / TME_OPUS_PACK_LEN;
        if (send_data) {
            if (!send_data(temp_buf, AUDIO_SEND_MIN_BUF_LEN + 1)) {
                cbuf_read_alloc_len_updata(&(__this->cbuffer), AUDIO_SEND_MIN_BUF_LEN);
                __this->speech_buf_resend_flg = 0;
            } else {
                break;
            }
        }
    }

    return res;
}
#endif

static u16 speech_data_send(u8 *buf, u16 len, u16(*send_data)(u8 *buf, u16 len))
{
    u16 res = 0;

    if (__this->speech_init_flg == 0) {
        return 0;
    }

    if (cbuf_write(&(__this->cbuffer), buf, len) != len) {
        res = (u16)(-1);
    }

    if (__this->speech_buf_resend_flg) {
        __this->speech_buf_resend_flg = 1;
        if (send_data) {
            if (send_data(speech_buf, AUDIO_SEND_MIN_BUF_LEN) != (u16)(-1)) {
                __this->speech_buf_resend_flg = 0;
            }
        }
    }

    /* printf("cl %d\n",cbuf_get_data_size(&(__this->cbuffer))); */

    while ((__this->speech_buf_resend_flg == 0) && cbuf_get_data_size(&(__this->cbuffer)) >= AUDIO_SEND_MIN_BUF_LEN) {
        __this->speech_buf_resend_flg = 1;
        cbuf_read(&(__this->cbuffer), speech_buf, AUDIO_SEND_MIN_BUF_LEN);
        if (send_data) {
            putchar('S');
            if (send_data(speech_buf, AUDIO_SEND_MIN_BUF_LEN) != (u16)(-1)) {
                __this->speech_buf_resend_flg = 0;
            }
        }
    }

    return res;
}

extern u8 mic_get_data_source(void);
extern u8 get_ble_connect_type(void);


#if (DUEROS_DMA_EN || GMA_EN || XM_MMA_EN)
void tws_api_local_media_sync_rx_handler_notify()
{
    u8 *tws_buf = NULL;
    int len = 0;

    if (mic_get_data_source() == SINK_TYPE_SLAVE) {
        if (dma_tws_mic_pool) {
            tws_buf = tws_api_local_media_trans_pop(&len);
            /* log_info("sf %d %0x\n",len, tws_buf); */
            if (tws_buf) {
                tws_api_local_media_trans_free(tws_buf);
            }
        }
    }
}

static u16 tws_data_send_slave_to_master(u8 *buf, u16 len)
{
    u8 *tws_buf = NULL;

    if (dma_tws_mic_pool == 0) {
        return -1;
    }

    tws_buf = tws_api_local_media_trans_alloc(len);
    if (tws_buf == NULL) {
        return -1;
    }
    /* log_info("sd  %0x %d\n",tws_buf,len); */
    memcpy(tws_buf, buf, len);
    /* printf("tdlen\n"); */
    /* put_buf(tws_buf,len); */
    tws_api_local_media_trans_push(tws_buf, len);
    return 0;

}
#endif

int audio_mic_enc_open(int (*mic_output)(void *priv, void *buf, int len), u32 code_type, u8 ai_type);
int audio_mic_enc_close();
static int rec_enc_output(void *priv, void *buf, int len)
{
    bt_sniff_ready_clean();
    if (clk_get("sys") != AUDIO_ENC_SYS_CLK_HZ) {
        //printf(">>>sys clk:%d \n", clk_get("sys"));
    }

#if (XM_MMA_EN)
    u8 *send_buf = (u8 *)buf;
    int send_len = len;

    //xiaomi header
    const u8 xiaomi_speech_frame_head[4] = {0xAA, 0XEA, 0XBD, 0XAC};
    u16 frame_len = (u16)len;
    u8 _send_buf[len + sizeof(xiaomi_speech_frame_head) + sizeof(frame_len)];
    if (__this->speex_frame_cnt == 0) {
        send_buf = _send_buf;
        send_len = len + sizeof(xiaomi_speech_frame_head) + sizeof(frame_len);
        //push header
        memcpy(send_buf, xiaomi_speech_frame_head, sizeof(xiaomi_speech_frame_head));
        //push frame length
        memcpy(&send_buf[sizeof(xiaomi_speech_frame_head)], (u8 *)&frame_len, sizeof(frame_len));
        //push data
        memcpy(&send_buf[sizeof(xiaomi_speech_frame_head) + sizeof(frame_len)], buf, len);
    }

    //add header every 5 frames
    __this->speex_frame_cnt++;
    if (__this->speex_frame_cnt >= 5) {
        __this->speex_frame_cnt = 0;
    }

#else
    u8 *send_buf = (u8 *)buf;
    int send_len = len;
#endif

    u8 *tws_buf = NULL;
    int get_len = 0;
#if (DUEROS_DMA_EN || GMA_EN || XM_MMA_EN)
    if (mic_get_data_source() == SINK_TYPE_MASTER) {
        if (dma_tws_mic_pool) {
            tws_buf = tws_api_local_media_trans_pop(&get_len);
            if (tws_buf) {
                /* log_info("md %d\n",get_len); */
                /* printf("sdlen\n"); */
                /* put_buf(tws_buf,get_len); */
                /* if (AI_PL_DATA_SEND) { */
                /*     ((u16(*)(u8 *, u16))AI_PL_DATA_SEND)(tws_buf, get_len); */
                /* } */
                if (speech_data_send(tws_buf, get_len, AI_PL_DATA_SEND) == (u16)(-1)) {
                    log_info("opus data miss !!! line:%d \n", __LINE__);
                }

                tws_api_local_media_trans_free(tws_buf);
            }
        }
    } else if (mic_get_data_source() == SINK_TYPE_SLAVE) {

        /* printf("mdlen\n"); */
        if (speech_data_send(send_buf, send_len, tws_data_send_slave_to_master) == (u16)(-1)) {
            log_info("opus data miss !!! line:%d \n", __LINE__);
        }

    } else {
        /* printf("ddlen\n"); */
        /* put_buf(send_buf,send_len); */
        if (get_ble_connect_type() == TYPE_SLAVE_BLE) {
            return 0;
        }

        if (speech_data_send(send_buf, send_len, AI_PL_DATA_SEND) == (u16)(-1)) {
            log_info("opus data miss !!! line:%d \n", __LINE__);
        }
    }
#elif (TME_EN)
    if (tme_speech_data_send(send_buf, send_len, AI_PL_DATA_SEND) == (u16)(-1)) {
        void TME_key_msg_notif(u16 msg, u8 * data, u16 len);
        TME_key_msg_notif(KEY_SEND_SPEECH_STOP, NULL, 0);
        log_info("opus data miss !!! line:%d \n", __LINE__);
    }
#else
    if (speech_data_send(send_buf, send_len, AI_PL_DATA_SEND) == (u16)(-1)) {
        log_info("opus data miss !!! line:%d \n", __LINE__);
    }
#endif

    return 0;
}


_WEAK_ int a2dp_tws_dec_suspend(void *p)
{
    return 0;
}


_WEAK_ void a2dp_tws_dec_resume(void)
{
}

_WEAK_ void bt_sniff_ready_clean(void)
{
}

//_WEAK_ void clk_set_en(u8 en)
//{
//
//}

_WEAK_ void mic_rec_clock_set(void)
{
}

_WEAK_ void mic_rec_clock_recover(void)
{
}


void a2dp_drop_frame_start();
void a2dp_drop_frame_stop();
void tws_slave_a2dp_drop_frame_ctl(u8 en);
bool get_tws_sibling_connect_state(void);
int a2dp_tws_dec_suspend(void *p);
void a2dp_tws_dec_resume(void);
//static int sys_clk_before_rec;
static int ai_mic_busy_flg = 0;
int ai_mic_rec_start(void)
{
    os_mutex_pend(&mutex_ai_mic, 0);

    if (ai_mic_busy_flg) {
        os_mutex_post(&mutex_ai_mic);
        return -1;
    }

    __this->speex_frame_cnt = 0;

    user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);

#if TCFG_USER_TWS_ENABLE
    if (get_tws_sibling_connect_state() && (tws_api_get_role() == TWS_ROLE_MASTER)) {
        tws_slave_a2dp_drop_frame_ctl(1);
    }
#endif

    int err = a2dp_tws_dec_suspend(NULL);
    if (err == 0) {
        printf("opus init \n");

        mic_rec_clock_set();

        speech_cbuf_init();

        printf("%s \n", ai_enc_info.info);
        audio_mic_enc_open(rec_enc_output, ai_enc_info.enc_type, ai_enc_info.opus_type);
        ai_mic_busy_flg = 1;
    }

    os_mutex_post(&mutex_ai_mic);

    return 0;
}

int ai_mic_rec_stop(void)
{

    return 0;
}

int ai_mic_is_busy(void)
{
    return ai_mic_busy_flg;
}

static void ai_mic_tws_stop_opus()
{
    if (ai_mic_busy_flg == 0) {
        a2dp_tws_dec_resume();
    }
}

TWS_SYNC_CALL_REGISTER(ai_tws_stop_opus) = {
    .uuid = 0x3890AB12,
    .func = ai_mic_tws_stop_opus,
    .task_name = "app_core",
};

static int ai_mic_resume_a2dp(void)
{
    int err = 0;
    int msg[8];
    msg[0] = (int)ai_mic_tws_stop_opus;
    msg[1] = 1;
    msg[2] = 0;

    while (1) {
        err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        if (err != OS_Q_FULL) {
            break;
        }
        os_time_dly(2);
    }

    return err;
}

int ai_mic_rec_close(void)
{
    os_mutex_pend(&mutex_ai_mic, 0);

    if (ai_mic_busy_flg) {
        printf(">>>>opus close: %d, %s\n", cpu_in_irq(), os_current_task());
        audio_mic_enc_close();

        speech_cbuf_exit();

        mic_rec_clock_recover();

        ai_mic_busy_flg = 0;
    }

    os_mutex_post(&mutex_ai_mic);

#if TCFG_USER_TWS_ENABLE
    if ((tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) &&
        tws_api_get_role() == TWS_ROLE_MASTER) {
        tws_api_sync_call_by_uuid(0x3890AB12, 0, 400);
    } else {
        /*a2dp_tws_dec_resume();*/
        ai_mic_resume_a2dp();
    }
#else
    ai_mic_resume_a2dp();
    /*a2dp_tws_dec_resume();*/
#endif

    return 0;
}

static u8 ai_rec_is_running(void)
{
#if MIC_REC_USE_AUDIO_BUFFER
    return ai_rec_server ? 1 : 0;
#else
    return 0;
#endif
}

static u8 ai_rec_is_suspend(void)
{
    return 0;
}

static void ai_rec_suspend(void)
{
    ai_mic_rec_stop();
}

static void ai_rec_resume(void)
{

}

static int ai_mic_mutex_init(void)
{
    os_mutex_create(&mutex_ai_mic);
    return 0;
}
late_initcall(ai_mic_mutex_init);

#endif

