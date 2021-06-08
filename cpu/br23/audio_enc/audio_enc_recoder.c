#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "asm/audio_src.h"
#include "audio_enc.h"
#include "app_main.h"
#include "app_task.h"
#include "encode/encode_write.h"
#include "clock_cfg.h"
#include "audio_config.h"
#include "dev_manager.h"
#include "audio_track.h"

#define LADC_MIC_BUF_NUM        2
#define LADC_MIC_CH_NUM         1
#define LADC_MIC_IRQ_POINTS     256
#define LADC_MIC_BUFS_SIZE      (LADC_MIC_CH_NUM * LADC_MIC_BUF_NUM * LADC_MIC_IRQ_POINTS)

struct ladc_mic_demo {
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    s16 adc_buf[LADC_MIC_BUFS_SIZE];    //align 4Bytes
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
    s16 tmp_buf[LADC_MIC_IRQ_POINTS * 2];
#endif
    int cut_timer;
};
struct ladc_mic_demo *ladc_mic = NULL;


extern struct audio_adc_hdl adc_hdl;
extern bool app_task_msg_post(int msg, int argc, ...);

static void adc_mic_demo_output(void *priv, s16 *data, int len)
{
    struct audio_adc_hdl *hdl = priv;
    //putchar('o');
    if (ladc_mic == NULL) {
        return;
    }
    //printf("mic:%x,len:%d,ch:%d",data,len,hdl->channel);
    putchar('.');
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
    //mono->dual
    for (u16 i = 0; i < len / 2; i++) {
        ladc_mic->tmp_buf[2 * i] = data[i];
        ladc_mic->tmp_buf[2 * i + 1] = data[i];
    }
    int wlen = app_audio_output_write(ladc_mic->tmp_buf, len * hdl->channel * 2);
#else
    int wlen = app_audio_output_write(data, len * hdl->channel);
    if (wlen != len) {
        //printf("wlen:%d-%d",wlen,len);
    }
#endif
}

static u8 mic_demo_idle_query()
{
    return (ladc_mic ? 0 : 1);
}
REGISTER_LP_TARGET(mic_demo_lp_target) = {
    .name = "mic_demo",
    .is_idle = mic_demo_idle_query,
};

void audio_adc_mic_demo(u16 sr)
{
    r_printf("audio_adc_mic_open:%d\n", sr);
    ladc_mic = zalloc(sizeof(struct ladc_mic_demo));
    if (ladc_mic) {
        audio_adc_mic_open(&ladc_mic->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
        audio_adc_mic_set_sample_rate(&ladc_mic->mic_ch, sr);
        audio_adc_mic_set_gain(&ladc_mic->mic_ch, 20);
        audio_adc_mic_set_buffs(&ladc_mic->mic_ch, ladc_mic->adc_buf, LADC_MIC_IRQ_POINTS * 2, LADC_MIC_BUF_NUM);
        ladc_mic->adc_output.handler = adc_mic_demo_output;
        ladc_mic->adc_output.priv = &adc_hdl;
        audio_adc_add_output_handler(&adc_hdl, &ladc_mic->adc_output);
        audio_adc_mic_start(&ladc_mic->mic_ch);

        app_audio_output_samplerate_set(sr);
        app_audio_output_start();
    }
}

int audio_adc_mic_init(u16 sr)
{
    //printf("ladc_mic_open:%d\n",sr);
    u8 ladc_mic_gain = 5;
    ASSERT(ladc_mic == NULL);
    ladc_mic = zalloc(sizeof(struct ladc_mic_demo));
    if (ladc_mic) {
        audio_adc_mic_open(&ladc_mic->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
        audio_adc_mic_set_sample_rate(&ladc_mic->mic_ch, sr);
        audio_adc_mic_set_gain(&ladc_mic->mic_ch, ladc_mic_gain);
        audio_adc_mic_set_buffs(&ladc_mic->mic_ch, ladc_mic->adc_buf, LADC_MIC_IRQ_POINTS * 2, LADC_MIC_BUF_NUM);
        audio_adc_mic_start(&ladc_mic->mic_ch);
        return 0;
    } else {
        return -1;
    }
}

void audio_adc_mic_exit(void)
{
    //printf("ladc_mic_close\n");
    if (ladc_mic) {
        audio_adc_mic_close(&ladc_mic->mic_ch);
        free(ladc_mic);
        ladc_mic = NULL;
    }
}

/******************************************************/
#define LADC_LINEIN_BUF_NUM        2
#define LADC_LINEIN_CH_NUM         2
#define LADC_LINEIN_IRQ_POINTS     32//256
#define LADC_LINEIN_BUFS_SIZE      (LADC_LINEIN_CH_NUM * LADC_LINEIN_BUF_NUM * LADC_LINEIN_IRQ_POINTS)
struct audio_adc_var {
    struct audio_adc_output_hdl adc_output;
    struct audio_adc_ch ch;
    s16 adc_buf[LADC_LINEIN_BUFS_SIZE];    //align 4Bytes
};
struct audio_adc_var *ladc_linein = NULL;

static void adc_linein_demo_output(void *priv, s16 *data, int len)
{
    struct audio_adc_hdl *hdl = priv;
    putchar('.');
    //printf("linein:%x,len:%d,ch:%d",data,len,hdl->channel);
    int wlen = app_audio_output_write(data, len * hdl->channel);
    if (wlen != len) {
        //printf("wlen:%d-%d",wlen,len);
    }
}

void audio_adc_linein_demo(void)
{
    u16 ladc_linein_sr = 44100;
    u8 ladc_linein_gain = 5;
    r_printf("audio_adc_linein_demo...");
    ladc_linein = zalloc(sizeof(*ladc_linein));
    if (ladc_linein) {
        audio_adc_linein_open(&ladc_linein->ch, AUDIO_ADC_LINE0_LR, &adc_hdl);
        audio_adc_linein_set_sample_rate(&ladc_linein->ch, ladc_linein_sr);
        audio_adc_linein_set_gain(&ladc_linein->ch, ladc_linein_gain);
        printf("adc_buf:%x size:%d", ladc_linein->adc_buf, sizeof(ladc_linein->adc_buf));
        audio_adc_set_buffs(&ladc_linein->ch, ladc_linein->adc_buf, LADC_LINEIN_CH_NUM * LADC_LINEIN_IRQ_POINTS * 2, LADC_LINEIN_BUF_NUM);
        ladc_linein->adc_output.handler = adc_linein_demo_output;
        ladc_linein->adc_output.priv = &adc_hdl;
        audio_adc_add_output_handler(&adc_hdl, &ladc_linein->adc_output);
        audio_adc_linein_start(&ladc_linein->ch);

        app_audio_output_samplerate_set(ladc_linein_sr);
        app_audio_output_start();
    }
}
#if 0
/*
 **************************************************************
 * Audio ADC 多通道使用demo
 * 数据结构：LINL LINR MIC LINL LINR MIC ...
 *
 **************************************************************
 */
#define LADC_BUF_NUM        2
#define LADC_CH_NUM         3
#if TCFG_REVERB_SAMPLERATE_DEFUALT >= 32000
#define LADC_IRQ_POINTS     48
#else
#define LADC_IRQ_POINTS     32
#endif
#define LADC_BUFS_SIZE      (LADC_CH_NUM * LADC_BUF_NUM * LADC_IRQ_POINTS)
#define  LADC_2_DAC_ENABLE	0

/*调试使用，推mic数据/linein数据/mic&line混合数据到dac*/
#define LADC_MIC_2_DAC		BIT(0)
#define LADC_LIN_2_DAC		BIT(1)
#define LADC_2_DAC			(LADC_MIC_2_DAC | LADC_LIN_2_DAC)

typedef struct {
    struct audio_adc_output_hdl output;
    struct audio_adc_ch linein_ch;
    struct adc_mic_ch mic_ch;
    s16 adc_buf[LADC_BUFS_SIZE];    //align 4Bytes
    s16 temp_buf[LADC_IRQ_POINTS * 2];
    u8 mic_gain;
    u8 ladc_gain;
    u8 mic_en: 2;
    u8 ladc_en: 2;
    u8 ladc_ch: 3;
    cbuffer_t *mic_pcm_cbuf;
    void (*mic_pcm_resume)(void *priv);
    void *mic_pcm_resume_priv;
    cbuffer_t *ladc_pcm_cbuf;
    void (*ladc_pcm_resume)(void *priv);
    void *ladc_pcm_resume_priv;
} audio_adc_t;
static audio_adc_t *ladc_var = NULL;

static void audio_adc_output_demo(void *priv, s16 *data, int len)
{
    struct audio_adc_hdl *hdl = priv;
    int wlen = 0;
    u16 i;
    /* putchar('.'); */
    if (ladc_var == NULL) {
        return;
    }
    /* printf("linein:%x,len:%d,ch:%d",data,len,hdl->channel); */
    if (ladc_var->ladc_ch == 2) {
        if (ladc_var->mic_en) {
            if (ladc_var->mic_pcm_cbuf) {
                for (i = 0; i < len / 2; i++) {
                    ladc_var->temp_buf[i] = data[i * 3 + 2];
                }
                wlen = cbuf_write(ladc_var->mic_pcm_cbuf, ladc_var->temp_buf, len);
                if (wlen != len) {
                    putchar('#');
                }
                if (ladc_var->mic_pcm_resume) {
                    ladc_var->mic_pcm_resume(ladc_var->mic_pcm_resume_priv);
                }
            }
        }
        if (ladc_var->ladc_en) {
            if (ladc_var->ladc_pcm_cbuf) {
                for (i = 1; i < len / 2; i++) {
                    data[i * 2] = data[i * 3];
                    data[i * 2 + 1] = data[i * 3 + 1];

                }
                wlen = cbuf_write(ladc_var->ladc_pcm_cbuf, data, len * 2);
                if (wlen != len * 2) {
                    putchar('e');
                }
                if (ladc_var->ladc_pcm_resume) {
                    ladc_var->ladc_pcm_resume(ladc_var->ladc_pcm_resume_priv);
                }
            }
        }
    } else {
        if (ladc_var->mic_en) {
            if (ladc_var->mic_pcm_cbuf) {
                for (i = 0; i < len / 2; i++) {
                    ladc_var->temp_buf[i] = data[i * 2 + 1];
                }
                wlen = cbuf_write(ladc_var->mic_pcm_cbuf, ladc_var->temp_buf, len);
                if (wlen != len) {
                    putchar('E');
                }
                if (ladc_var->mic_pcm_resume) {
                    ladc_var->mic_pcm_resume(ladc_var->mic_pcm_resume_priv);
                }
            }
        }
        if (ladc_var->ladc_en) {
            if (ladc_var->ladc_pcm_cbuf) {
                for (i = 1; i < len / 2; i++) {
                    data[i] = data[i * 2];
                }
                wlen = cbuf_write(ladc_var->ladc_pcm_cbuf, data, len);
                if (wlen != len) {
                    putchar('e');
                }
                if (ladc_var->ladc_pcm_resume) {
                    ladc_var->ladc_pcm_resume(ladc_var->ladc_pcm_resume_priv);
                }
            }
        }
    }
}



/* void audio_adc_open_demo(void) */
int audio_three_adc_open(void)
{
    u16 ladc_sr;
#if (defined(TCFG_REVERB_SAMPLERATE_DEFUALT))
    ladc_sr = TCFG_REVERB_SAMPLERATE_DEFUALT;
#else
    ladc_sr = 16000;
#endif
    u8 mic_gain = 5;
    u8 linein_gain = 3;
    u8 i, temp;
    r_printf("audio_adc_open_demo,sr:%d,mic_gain:%d,linein_gain:%d\n", ladc_sr, mic_gain, linein_gain);
    if (ladc_var) {
        r_printf("ladc already open \n");
        return 0;
    }
    ladc_var = zalloc(sizeof(audio_adc_t));
    if (ladc_var) {
        temp = (0x3F & TCFG_LINEIN_LR_CH);
        printf("linein ch [%d]\n\n\n", temp);
        ladc_var->ladc_ch = 0;
        for (i = 0; i < 8; i++) {
            if ((temp >> i)&BIT(0)) {
                ladc_var->ladc_ch++;
            }
        }
        printf("linein ch [%d]\n\n\n", ladc_var->ladc_ch);
        if ((ladc_var->ladc_ch > 2) || (ladc_var->ladc_ch == 0)) {
            printf(" err ladc ch \n\n");
            free(ladc_var);
            return -1;
        }
        audio_adc_mic_open(&ladc_var->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
        audio_adc_mic_set_sample_rate(&ladc_var->mic_ch, ladc_sr);
        audio_adc_mic_set_gain(&ladc_var->mic_ch, mic_gain);
        /* audio_adc_linein_open(&ladc_var->linein_ch, AUDIO_ADC_LINE0_LR, &adc_hdl); */
        audio_adc_linein_open(&ladc_var->linein_ch, TCFG_LINEIN_LR_CH << 2, &adc_hdl);

        audio_adc_linein_set_sample_rate(&ladc_var->linein_ch, ladc_sr);
        audio_adc_linein_set_gain(&ladc_var->linein_ch, linein_gain);

        printf("adc_buf_size:%d", sizeof(ladc_var->adc_buf));
        /* audio_adc_set_buffs(&ladc_var->linein_ch, ladc_var->adc_buf, LADC_CH_NUM * LADC_IRQ_POINTS * 2, LADC_BUF_NUM); */
        audio_adc_set_buffs(&ladc_var->linein_ch, ladc_var->adc_buf, (ladc_var->ladc_ch + 1) * LADC_IRQ_POINTS * 2, LADC_BUF_NUM);
        ladc_var->output.handler = audio_adc_output_demo;
        ladc_var->output.priv = &adc_hdl;
        audio_adc_add_output_handler(&adc_hdl, &ladc_var->output);
        audio_adc_start(&ladc_var->linein_ch, &ladc_var->mic_ch);

#if LADC_2_DAC_ENABLE
        app_audio_output_samplerate_set(ladc_sr);
        app_audio_output_start();
#endif/*LADC_2_DAC_ENABLE*/
        return 0;
    } else {
        return -1;
    }
}

void audio_three_adc_close()
{
    if (ladc_var) {
        if (ladc_var->mic_en == 0) {
            ladc_var->mic_pcm_cbuf = NULL;
        }
        if (ladc_var->ladc_en == 0) {
            ladc_var->ladc_pcm_cbuf = NULL;
        }

        if (ladc_var->mic_en || ladc_var->ladc_en) {
            return;
        }
        audio_adc_close(&ladc_var->linein_ch, &ladc_var->mic_ch);
        audio_adc_del_output_handler(&adc_hdl, &ladc_var->output);
        free(ladc_var);
        ladc_var = NULL;
    }
}
void three_adc_mic_enable(u8 mark)
{
    if (ladc_var) {
        ladc_var->mic_en = mark ? 1 : 0;
    }
}

void three_adc_ladc_enable(u8 mark)
{
    if (ladc_var) {
        ladc_var->ladc_en = mark ? 1 : 0;
    }
}

void set_mic_cbuf_hdl(cbuffer_t *mic_cbuf)
{
    if (ladc_var && mic_cbuf) {
        ladc_var->mic_pcm_cbuf = mic_cbuf;
    }
}
void set_mic_resume_hdl(void (*resume)(void *priv), void *priv)
{
    if (ladc_var) {
        ladc_var->mic_pcm_resume = resume;
        ladc_var->mic_pcm_resume_priv = priv;
    }
}
void set_ladc_cbuf_hdl(cbuffer_t *ladc_cbuf)
{
    if (ladc_var && ladc_cbuf) {
        ladc_var->ladc_pcm_cbuf = ladc_cbuf;
    }
}
void set_ladc_resume_hdl(void (*resume)(void *priv), void *priv)
{
    if (ladc_var) {
        ladc_var->ladc_pcm_resume = resume;
        ladc_var->ladc_pcm_resume_priv = priv;
    }
}
void three_adc_mic_set_gain(u8 level)
{
    if (ladc_var) {
        audio_adc_mic_set_gain(&ladc_var->mic_ch, level);
    }
}
#endif
/***********************************************************************************************************/
#define ADC_BUF_NUM        2
#define ADC_CH_NUM         2
#define ADC_IRQ_POINTS     256
#define ADC_BUFS_SIZE      (ADC_BUF_NUM *ADC_CH_NUM* ADC_IRQ_POINTS)

#if SOUNDCARD_ENABLE
#define ADC_STORE_PCM_SIZE	(ADC_BUFS_SIZE * 4)
#else
#define ADC_STORE_PCM_SIZE	(ADC_BUFS_SIZE * 16)
#endif

//#define ADC_STORE_PCM_MAX	(ADC_STORE_PCM_SIZE	* 80 / 100)

struct linein_sample_hdl {
    OS_SEM sem;
    struct audio_adc_ch linein_ch;
    struct audio_adc_output_hdl sample_output;
#if SOUNDCARD_ENABLE
    s16 adc_buf[4];
#else
    s16 adc_buf[ADC_BUFS_SIZE];
#endif
    /* s16 *store_pcm_buf[ADC_STORE_PCM_SIZE]; */
    s16 *store_pcm_buf;
    cbuffer_t cbuf;
    void (*resume)(void);
    u8 channel_num;
    volatile u8 wait_resume;
    u16 output_fade_in_gain;
    u8 output_fade_in;
    u16 sample_rate;
    void *audio_track;
};

/* struct linein_sample_hdl g_linein_sample_hdl sec(.linein_pcm_mem); */
/* static s16 linein_store_pcm_buf[ADC_STORE_PCM_SIZE] sec(.linein_pcm_mem); */

void linein_sample_set_resume_handler(void *priv, void (*resume)(void))
{
    struct linein_sample_hdl *linein = (struct linein_sample_hdl *)priv;

    if (linein) {
        linein->resume = resume;
    }
}

void fm_inside_output_handler(void *priv, s16 *data, int len)
{
    struct linein_sample_hdl *linein = (struct linein_sample_hdl *)priv;

    if (linein->audio_track) {
        audio_local_sample_track_in_period(linein->audio_track, (len >> 1) / linein->channel_num);
    }

    int wlen = cbuf_write(&linein->cbuf, data, len);
    os_sem_post(&linein->sem);
    if (wlen != len) {
        putchar('W');
    }
    if (linein->resume) {
        if (linein->wait_resume) {
            linein->wait_resume = 0;
            linein->resume();
        }
    }
}

static void linein_sample_output_handler(void *priv, s16 *data, int len)
{
    struct linein_sample_hdl *linein = (struct linein_sample_hdl *)priv;


    if (linein->audio_track) {
        audio_local_sample_track_in_period(linein->audio_track, (len >> 1));
    }


    if (linein->output_fade_in)

    {
        s32 tmp_data;
        //printf("fade:%d\n",aec_hdl->output_fade_in_gain);
        for (int i = 0; i < len * linein->channel_num / 2; i++) {
            tmp_data = data[i];
            data[i] = tmp_data * linein->output_fade_in_gain >> 7;

        } //end of for

        linein->output_fade_in_gain += 2 ;
        if (linein->output_fade_in_gain >= 128) {
            linein->output_fade_in = 0;
        }

    }

    int wlen = cbuf_write(&linein->cbuf, data, len * linein->channel_num);
    /* os_sem_post(&linein->sem); */
    if (wlen != len * linein->channel_num) {
        putchar('W');
    }
    if (linein->resume) {
        if (linein->wait_resume) {
            linein->wait_resume = 0;
            linein->resume();
        }
    }
}

AT(.fm_data_code)
int linein_stream_sample_rate(void *hdl)
{
    struct linein_sample_hdl *linein = (struct linein_sample_hdl *)hdl;

    if (linein->audio_track) {
        int sr =  audio_local_sample_track_rate(linein->audio_track);
        if ((sr < (linein->sample_rate + 200)) && (sr > linein->sample_rate - 200)) {
            return sr;
        }
        printf("linein audio_track reset \n");
        local_irq_disable();
        audio_local_sample_track_close(linein->audio_track);
        linein->audio_track = audio_local_sample_track_open(linein->channel_num, linein->sample_rate, 1000);
        local_irq_enable();
    }

    return linein->sample_rate;
}

AT(.fm_data_code)
int linein_sample_read(void *hdl, void *data, int len)
{
    struct linein_sample_hdl *linein = (struct linein_sample_hdl *)hdl;
#if 0
// no wait
    u8 count = 0;
__retry:
    if (cbuf_get_data_size(&linein->cbuf) < len) {
        local_irq_disable();
        os_sem_set(&linein->sem, 0);
        local_irq_enable();
        os_sem_pend(&linein->sem, 2);
        if (cbuf_get_data_size(&linein->cbuf) < len) {
            if (++count > 4) {
                return 0;
            }
            goto __retry;
        }

    }
#endif
    int rlen = cbuf_read(&linein->cbuf, data, len);
    local_irq_disable();
    if (rlen < len) {
        linein->wait_resume = 1;
    }
    local_irq_enable();

    return rlen;
}

AT(.fm_data_code)
int linein_sample_size(void *hdl)
{
    struct linein_sample_hdl *linein = (struct linein_sample_hdl *)hdl;
    return cbuf_get_data_size(&linein->cbuf);
}

AT(.fm_data_code)
int linein_sample_total(void *hdl)
{
    struct linein_sample_hdl *linein = (struct linein_sample_hdl *)hdl;
    return linein->cbuf.total_len;
}

static void linein_sample_resume(void *priv)
{
    struct linein_sample_hdl *linein = priv;
    if (linein->resume) {
        linein->resume();
    }
}

void *linein_sample_open(u8 source, u16 sample_rate)
{
    struct linein_sample_hdl *linein = NULL;

    /* linein = &g_linein_sample_hdl;// zalloc(sizeof(struct linein_sample_hdl)); */
    linein =  zalloc(sizeof(struct linein_sample_hdl));
    if (!linein) {
        return NULL;
    }

    memset(linein, 0x0, sizeof(struct linein_sample_hdl));

    linein->store_pcm_buf = malloc(ADC_STORE_PCM_SIZE);
    if (!linein->store_pcm_buf) {
        return NULL;
    }

    linein->output_fade_in_gain = 0;

    linein->output_fade_in = 1;
    if (source != 0xff) {
#if 0
#ifndef THREE_ADC_ENABLE
        audio_adc_linein_open(&linein->linein_ch, (source << 2), &adc_hdl);
#endif
#endif
    }

    cbuf_init(&linein->cbuf, linein->store_pcm_buf, ADC_STORE_PCM_SIZE);
    os_sem_create(&linein->sem, 0);
    if (source == 0xff) {
        return linein;
    }
#if 0
#ifndef THREE_ADC_ENABLE
    linein->channel_num = adc_hdl.channel;
    audio_adc_linein_set_sample_rate(&linein->linein_ch, sample_rate);
    audio_adc_linein_set_gain(&linein->linein_ch, 3);
    audio_adc_set_buffs(&linein->linein_ch, linein->adc_buf, ADC_CH_NUM * ADC_IRQ_POINTS * 2, ADC_BUF_NUM);

    linein->sample_output.handler = linein_sample_output_handler;
    linein->sample_output.priv = linein;

    audio_adc_add_output_handler(&adc_hdl, &linein->sample_output);
    audio_adc_linein_start(&linein->linein_ch);
#else
    if (audio_three_adc_open() == 0) {
        printf("mic adc open success \n");
        set_ladc_cbuf_hdl(&linein->cbuf);
        set_ladc_resume_hdl(linein_sample_resume, linein);
        three_adc_ladc_enable(1);
    }
#endif
#else

    if (audio_linein_open(&linein->linein_ch, sample_rate, 3) == 0) {
        linein->sample_output.handler = linein_sample_output_handler;
        linein->sample_output.priv = linein;
        linein->channel_num = get_audio_linein_ch_num();
        printf("-------------linein->channel_num:%d ----------\n", linein->channel_num);
        audio_linein_add_output(&linein->sample_output);
        audio_linein_start(&linein->linein_ch);
    }
#endif

    linein->sample_rate = sample_rate;
    /* linein->audio_track = audio_local_sample_track_open(linein->channel_num, sample_rate, 1000); */



    return linein;
}

void linein_sample_close(void *hdl)
{
    struct linein_sample_hdl *linein = (struct linein_sample_hdl *)hdl;

    if (!linein) {
        return;
    }
#if 0
#ifndef THREE_ADC_ENABLE
    audio_adc_linein_close(&linein->linein_ch);
    audio_adc_del_output_handler(&adc_hdl, &linein->sample_output);
#else
    three_adc_ladc_enable(0);
    audio_three_adc_close();
#endif
#else
    audio_linein_close(&linein->linein_ch, &linein->sample_output);
#endif
    if (linein->audio_track) {
        audio_local_sample_track_close(linein->audio_track);
    }

    free(linein->store_pcm_buf);
    free(linein);
}


void *fm_sample_open(u8 source, u16 sample_rate)
{

    struct linein_sample_hdl *linein = NULL;
    /* linein = &g_linein_sample_hdl;// zalloc(sizeof(struct linein_sample_hdl)); */
    linein =  zalloc(sizeof(struct linein_sample_hdl));
    if (!linein) {
        return NULL;
    }
    memset(linein, 0x0, sizeof(struct linein_sample_hdl));

    linein->store_pcm_buf = zalloc(ADC_STORE_PCM_SIZE);
    if (!linein->store_pcm_buf) {
        return NULL;
    }

    cbuf_init(&linein->cbuf, linein->store_pcm_buf, ADC_STORE_PCM_SIZE);
    os_sem_create(&linein->sem, 0);

    linein->sample_rate = sample_rate;
    linein->audio_track = audio_local_sample_track_open(linein->channel_num, sample_rate, 1000);

    linein->channel_num = 2;
    if (source == 1) { //line1
        audio_adc_linein_open(&linein->linein_ch, AUDIO_ADC_LINE1_LR, &adc_hdl);
    } else if (source == 0) { //linein0
        audio_adc_linein_open(&linein->linein_ch, AUDIO_ADC_LINE0_LR, &adc_hdl);
    } else if (source == 2) {
        audio_adc_linein_open(&linein->linein_ch, AUDIO_ADC_LINE2_LR, &adc_hdl);
    } else {
        return linein;
    }

    audio_adc_linein_set_sample_rate(&linein->linein_ch, sample_rate);
    audio_adc_linein_set_gain(&linein->linein_ch, 1);

    audio_adc_set_buffs(&linein->linein_ch, linein->adc_buf, ADC_CH_NUM * ADC_IRQ_POINTS * 2, ADC_BUF_NUM);

    linein->sample_output.handler = linein_sample_output_handler;
    linein->sample_output.priv = linein;

    audio_adc_add_output_handler(&adc_hdl, &linein->sample_output);
    audio_adc_linein_start(&linein->linein_ch);
    return linein;
}

void fm_sample_close(void *hdl, u8 source)
{
    struct linein_sample_hdl *linein = (struct linein_sample_hdl *)hdl;

    if (!linein) {
        return;
    }

    if (source <= 2) {
        audio_adc_linein_close(&linein->linein_ch);
        audio_adc_del_output_handler(&adc_hdl, &linein->sample_output);
    }
    if (linein->audio_track) {
        audio_local_sample_track_close(linein->audio_track);
    }

    free(linein->store_pcm_buf);
    free(linein);
}


////>>>>>>>>>>>>>>record_player api录音接口<<<<<<<<<<<<<<<<<<<<<///
#if 1

/* #define RECORD_PLAYER_DEFULT_SAMPLERATE	(44100L) */
#define RECORD_PLAYER_DEFULT_SAMPLERATE	(16000L)
#define RECORD_PLAYER_DEFULT_BITRATE	(128L)
#define RECORD_PLAYER_DEFULT_ADPCM_BLOCKSIZE (1024L) //256/512/1024/2048
#define RECORD_PLAYER_MIXER_CHANNELS		 1     //mixer record single channel

#define REC_ALIN(var,al)     ((((var)+(al)-1)/(al))*(al))

struct record_hdl {
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch    mic_ch;
    struct audio_adc_ch linein_ch;
    struct __dev *dev;
    void *file;
    //s16 adc_buf[ADC_BUFS_SIZE];
    s16 *adc_buf;
    u32 magic;
    u32 start_sys_tick;
    enum enc_source source;
    u16	sample_rate;
    u16 cut_head_timer;
    u8  cut_head_flag;
    u8  stop_req;
    u8  nchl;
    u8  coding_type;
    volatile u8  busy;
    volatile u8  status;
    void (*err_callback)(void);
};

extern u32 timer_get_ms(void);
extern u32 audio_output_channel_num(void);

extern struct audio_mixer mixer;

static struct record_hdl *rec_hdl = NULL;

int recorder_get_enc_time()
{
    int ret = 0;
    if (rec_hdl && rec_hdl->file) {
        ret = pcm2file_enc_get_time(rec_hdl->file);
    }
//	log_d("get ret %d s\n", ret);
    return ret;
}

static void recorder_encode_clock_add(u32 coding_type)
{
    if ((coding_type == AUDIO_CODING_WAV) || (coding_type == AUDIO_CODING_PCM)) {
        clock_add(ENC_WAV_CLK);
    } else if (coding_type == AUDIO_CODING_G726) {
        clock_add(ENC_G726_CLK);
    } else if (coding_type == AUDIO_CODING_MP3) {
        clock_add(ENC_MP3_CLK);
    }
    clock_set_cur();
}

static void recorder_encode_clock_remove(u32 coding_type)
{
    if ((coding_type == AUDIO_CODING_WAV) || (coding_type == AUDIO_CODING_PCM)) {
        clock_remove(ENC_WAV_CLK);
    } else if (coding_type == AUDIO_CODING_G726) {
        clock_remove(ENC_G726_CLK);
    } else if (coding_type == AUDIO_CODING_MP3) {
        clock_remove(ENC_MP3_CLK);
    }
    clock_set_cur();
}

void recorder_encode_stop(void)
{
    if (rec_hdl) {
        switch (rec_hdl->source) {
        case ENCODE_SOURCE_MIX:
            break;
        case ENCODE_SOURCE_MIC:
#if 0
            audio_adc_mic_close(&rec_hdl->mic_ch);
            audio_adc_del_output_handler(&adc_hdl, &rec_hdl->adc_output);
#else
            audio_mic_close(&rec_hdl->mic_ch, &rec_hdl->adc_output);
#endif
            break;
        case ENCODE_SOURCE_LINE0_LR:
        case ENCODE_SOURCE_LINE1_LR:
        case ENCODE_SOURCE_LINE2_LR:
            audio_adc_linein_close(&rec_hdl->linein_ch);
            audio_adc_del_output_handler(&adc_hdl, &rec_hdl->adc_output);
            break;
        default:
            break;
        }
        rec_hdl->magic++;

        rec_hdl->status = 0;
        printf("[%s], wait busy\n", __FUNCTION__);
        while (rec_hdl->busy) {
            os_time_dly(2);
        }
        printf("[%s], wait busy ok\n", __FUNCTION__);
        pcm2file_enc_close(&rec_hdl->file);
        recorder_encode_clock_remove(rec_hdl->coding_type);
        rec_hdl->file = NULL;
        sys_timeout_del(rec_hdl->cut_head_timer);
        local_irq_disable();
        free(rec_hdl);
        rec_hdl = NULL;
        local_irq_enable();
    }
}

static void recorder_encode_event_handler(struct audio_encoder *hdl, int argc, int *argv)
{
    if (rec_hdl == NULL) {
        return ;
    }

    struct audio_encoder *enc = get_pcm2file_encoder_hdl(rec_hdl->file);
    printf("%s, argv[]:%d, %d , hdl = %x, enc = %x", __FUNCTION__,  argv[0], argv[1], hdl, enc);
    if ((hdl != NULL) && (hdl == enc)) {
        if (rec_hdl->err_callback) {
            rec_hdl->err_callback();
        } else {
            recorder_encode_stop();
        }
    } else {
        printf("err enc handle !!\n");
    }
}



static void adc_output_to_enc(void *priv, s16 *data, int len)
{
    /* putchar('o'); */
    if (rec_hdl == NULL) {
        return ;
    }

    if (rec_hdl->cut_head_flag) {
        return ;
    }

    switch (rec_hdl->source) {
    case ENCODE_SOURCE_MIC:
    case ENCODE_SOURCE_LINE0_LR:
    case ENCODE_SOURCE_LINE1_LR:
    case ENCODE_SOURCE_LINE2_LR:
        pcm2file_enc_write_pcm(rec_hdl->file, data, len);
        break;
    default:
        break;
    }
}

void record_cut_head_timeout(void *priv)
{
    if (rec_hdl) {
        rec_hdl->cut_head_flag = 0;
        printf("record_cut_head_timeout \n");
    }
}

u32 recorder_get_encoding_time()
{
    return recorder_get_enc_time();
//    u32 time_sec = 0;
//    if (rec_hdl) {
//        time_sec = (timer_get_ms() - rec_hdl->start_sys_tick) / 1000;
//        /* printf("rec time sec = %d\n",time_sec); */
//    }
//    return time_sec;
}

///检查录音是否正在进行
int recorder_is_encoding(void)
{
    if (rec_hdl) {
        return 1;
    }
    return 0;
}



int recorder_encode_start(struct record_file_fmt *f)
{
    struct audio_fmt afmt;
    struct record_file_fmt fmt;
    u32 cut_tail_size = 0;
    struct record_hdl *rec = NULL;

    memcpy((u8 *)&fmt, (u8 *)f, sizeof(struct record_file_fmt));
    if ((fmt.coding_type == AUDIO_CODING_WAV) || fmt.coding_type == AUDIO_CODING_G726) {
        fmt.bit_rate    = RECORD_PLAYER_DEFULT_ADPCM_BLOCKSIZE;
    } else {
        fmt.bit_rate    = RECORD_PLAYER_DEFULT_BITRATE;
    }
    afmt.coding_type = fmt.coding_type;
    afmt.channel = fmt.channel;
    afmt.sample_rate = fmt.sample_rate;
    afmt.bit_rate = fmt.bit_rate;

    if (rec_hdl) {
        recorder_encode_stop();
    }

    u32 offset = 0;
    u32 size = REC_ALIN(sizeof(struct record_hdl), 4);
    if ((fmt.source == ENCODE_SOURCE_LINE0_LR)
        || (fmt.source == ENCODE_SOURCE_LINE1_LR)
        || (fmt.source == ENCODE_SOURCE_LINE2_LR)
       ) {
        size += REC_ALIN((sizeof(s16) * ADC_BUFS_SIZE), 4);
    }

    u8 *buf = zalloc(size);
    if (!buf) {
        return -1;
    }
    rec = (struct record_hdl *)(buf + offset);
    offset += REC_ALIN(sizeof(struct record_hdl), 4);

    rec->source = fmt.source;
    rec->dev = dev_manager_find_spec(fmt.dev, 0);
    if (rec->dev == NULL) {
        free(rec);
        return -1;
    }

    rec->sample_rate = fmt.sample_rate;
    rec->nchl = fmt.channel;
    rec->coding_type = fmt.coding_type;
    recorder_encode_clock_add(fmt.coding_type);
    rec->file = pcm2file_enc_open(&afmt, fmt.dev, fmt.folder, fmt.filename);
    if (!rec->file) {
        free(rec);
        recorder_encode_clock_remove(fmt.coding_type);
        return -1;
    }

    rec->cut_head_flag = 1;
    rec->cut_head_timer = sys_timeout_add(NULL, record_cut_head_timeout, fmt.cut_head_time);

    if ((fmt.coding_type == AUDIO_CODING_WAV) || (fmt.coding_type == AUDIO_CODING_G726)) {
        cut_tail_size = (4 * fmt.sample_rate * fmt.channel) * fmt.cut_tail_time / 1000 / 8;
        cut_tail_size = ((cut_tail_size + fmt.bit_rate - 1) / fmt.bit_rate) * fmt.bit_rate;
    } else {
        cut_tail_size = fmt.bit_rate * fmt.cut_tail_time / 8;
    }


    rec->err_callback = NULL;
    if (fmt.err_callback) {
        rec->err_callback = fmt.err_callback;
    }

    last_enc_file_codeing_type_save(afmt.coding_type);
    pcm2file_enc_set_evt_handler(rec->file, recorder_encode_event_handler, rec->magic);
    pcm2file_enc_write_file_set_limit(rec->file, cut_tail_size, fmt.limit_size);
    pcm2file_enc_start(rec->file);

    switch (rec->source) {
    case ENCODE_SOURCE_MIX:
        break;
    case ENCODE_SOURCE_MIC:
#if 0
        audio_adc_mic_open(&rec->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
        audio_adc_mic_set_sample_rate(&rec->mic_ch, fmt.sample_rate);
        audio_adc_mic_set_gain(&rec->mic_ch, fmt.gain);///调节录音mic的增益
        audio_adc_mic_set_buffs(&rec->mic_ch, rec->adc_buf, LADC_MIC_IRQ_POINTS * 2, LADC_MIC_BUF_NUM);

        rec->adc_output.handler = adc_output_to_enc;
        audio_adc_add_output_handler(&adc_hdl, &rec->adc_output);
        audio_adc_mic_start(&rec->mic_ch);
#else
        rec->adc_output.handler = adc_output_to_enc;
        rec->adc_output.priv    = rec;
        if (audio_mic_open(&rec->mic_ch, fmt.sample_rate, fmt.gain) == 0) {
            audio_mic_add_output(&rec->adc_output);
            audio_mic_start(&rec->mic_ch);
        }

#endif
        break;
    case ENCODE_SOURCE_LINE0_LR:
    case ENCODE_SOURCE_LINE1_LR:
    case ENCODE_SOURCE_LINE2_LR:
        rec->adc_buf = (s16 *)(buf + offset);
        audio_adc_linein_open(&rec->linein_ch, AUDIO_ADC_LINE0_LR + (rec->source - ENCODE_SOURCE_LINE0_LR), &adc_hdl);
        audio_adc_linein_set_sample_rate(&rec->linein_ch, fmt.sample_rate);
        audio_adc_linein_set_gain(&rec->linein_ch, fmt.gain);
        audio_adc_set_buffs(&rec->linein_ch, rec->adc_buf, ADC_CH_NUM * ADC_IRQ_POINTS * 2, ADC_BUF_NUM);
        rec->adc_output.handler = adc_output_to_enc;
        rec->adc_output.priv = &adc_hdl;
        audio_adc_add_output_handler(&adc_hdl, &rec->adc_output);
        audio_adc_linein_start(&rec->linein_ch);
        break;
    default:
        break;
    }

    rec->start_sys_tick = timer_get_ms();
    rec_hdl = rec;

    rec_hdl->status = 1;
    rec_hdl->busy = 0;

    return 0;
}

int recorder_userdata_to_enc(s16 *data, int len)
{
    if ((rec_hdl == NULL) || (rec_hdl->status == 0)) {
        return 0;
    }
    rec_hdl->busy = 1;
    if (rec_hdl->cut_head_flag) {
        rec_hdl->busy = 0;
        return 0;
    }
    int wlen = pcm2file_enc_write_pcm(rec_hdl->file, data, len);
    rec_hdl->busy = 0;
    return wlen;
}

void recorder_device_offline_check(char *logo)
{
    if (rec_hdl) {
        if (!strcmp(dev_manager_get_logo(rec_hdl->dev), logo)) {
            ///当前录音正在使用的设备掉线， 应该停掉录音
            printf("is the recording dev = %s\n", logo);
            recorder_encode_stop();
        }
    }
}

#endif//







