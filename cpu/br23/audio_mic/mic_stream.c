#include "mic_stream.h"
#include "app_config.h"
#include "system/includes.h"
#include "audio_splicing.h"
#include "audio_config.h"
#include "asm/dac.h"
#include "audio_enc.h"
#include "audio_dec.h"
#include "media/includes.h"
#include "application/audio_dig_vol.h"
#include "media/pcm_decoder.h"


#if (TCFG_MIC_EFFECT_ENABLE)

#define MIC_STREAM_TASK_NAME				"mic_stream"


static struct __mic_stream *mic = NULL;
#define __this mic

#define MIC_SIZEOF_ALIN(var,al)     ((((var)+(al)-1)/(al))*(al))

/* extern struct audio_dac_hdl dac_hdl; */
/* extern struct audio_mixer mixer; */

/*----------------------------------------------------------------------------*/
/**@brief    唤醒mic数据处理任务
  @param
  @return
  @note
  */
/*----------------------------------------------------------------------------*/
void mic_stream_adc_resume(void *priv)
{
    struct __mic_stream *stream = (struct __mic_stream *)priv;
    if (stream != NULL && (stream->release == 0)) {
        os_sem_set(&stream->sem, 0);
        os_sem_post(&stream->sem);
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    mic数据处理函数
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

static int mic_stream_effects_run(struct __mic_stream *stream)
{
    int res = os_sem_pend(&stream->sem, 0);
    if (res) {
        return -1;
    }
    if (stream->release) {
        return -1;
    }
    s16 *read_buf = (s16 *)(stream->adc_buf);
    if (stream->out.func) {
        stream->out.func(stream->out.priv, read_buf, NULL, stream->parm->point_unit * 2, stream->parm->point_unit * 2 * 2);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    mic数据处理任务
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_stream_task_deal(void *p)
{
    int res = 0;
    struct __mic_stream *stream = (struct __mic_stream *)p;
    stream->busy = 1;
    while (1) {
        res = mic_stream_effects_run(stream);
        if (res) {
            ///等待删除线程
            stream->busy = 0;
            while (1) {
                os_time_dly(10000);
            }
        }
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    创建mic数据流
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
struct __mic_stream *mic_stream_creat(struct __mic_stream_parm *parm)
{
    int err = 0;
    struct __mic_stream_parm *p = parm;
    if (parm == NULL) {
        printf("%s parm err\n", __FUNCTION__);
        return NULL;
    }
    printf("p->dac_delay = %d\n", p->dac_delay);
    printf("p->point_unit = %d\n", p->point_unit);
    printf("p->sample_rate = %d\n", p->sample_rate);

    u32 offset = 0;
    u32 buf_size = MIC_SIZEOF_ALIN(sizeof(struct __mic_stream), 4);

    u8 *buf = zalloc(buf_size);
    if (buf == NULL) {
        return NULL;
    }

    struct __mic_stream *stream = (struct __mic_stream *)buf;
    offset += MIC_SIZEOF_ALIN(sizeof(struct __mic_stream), 4);

    stream->parm = p;
    os_sem_create(&stream->sem, 0);
    err = task_create(mic_stream_task_deal, (void *)stream, MIC_STREAM_TASK_NAME);
    if (err != OS_NO_ERR) {
        printf("%s creat fail %x\n", __FUNCTION__,  err);
        free(stream);
        return NULL;
    }

    local_irq_disable();
    __this = stream;
    local_irq_enable();

    printf("mic stream creat ok\n");
    return stream;
}
/*----------------------------------------------------------------------------*/
/**@brief    设置mic处理函数的回调处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_stream_set_output(struct __mic_stream  *stream, void *priv, u32(*func)(void *priv, void *in, void *out, u32 inlen, u32 outlen))
{
    if (stream) {
        stream->out.priv = priv;
        stream->out.func = func;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    mic中断数据输出回调函数
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void adc_output_to_buf(void *priv, s16 *data, int len)
{
    struct __mic_stream *stream = (struct __mic_stream *)priv;
    int wlen = 0;
    if (stream != NULL && (stream->release == 0)) {
        stream->adc_buf = data;
        stream->adc_buf_len = len;
        os_sem_set(&stream->sem, 0);
        os_sem_post(&stream->sem);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    打开mic
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
bool mic_stream_start(struct __mic_stream  *stream)
{
    if (stream) {
        if (audio_mic_open(&stream->mic_ch, stream->parm->sample_rate, stream->parm->mic_gain) == 0) {
            stream->adc_output.handler = adc_output_to_buf;
            stream->adc_output.priv = stream;
            audio_mic_add_output(&stream->adc_output);
            audio_mic_start(&stream->mic_ch);
            /* audio_mic_0dB_en(1); */
            log_i("mic_stream_start ok 11\n");
            return true;
        }
    }
    return false;
}
/*----------------------------------------------------------------------------*/
/**@brief    关闭mic数据流
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_stream_destroy(struct __mic_stream **hdl)
{
    int err = 0;
    if ((hdl == NULL) || (*hdl == NULL)) {
        return ;
    }

    struct __mic_stream *stream = *hdl;
    stream->release = 1;
    audio_mic_close(&stream->mic_ch, &stream->adc_output);

    os_sem_set(&stream->sem, 0);
    os_sem_post(&stream->sem);

    while (stream->busy) {
        os_time_dly(5);
    }

    printf("%s wait busy ok!!!\n", __FUNCTION__);

    err = task_kill(MIC_STREAM_TASK_NAME);
    os_sem_del(&stream->sem, 0);

    local_irq_disable();
    free(*hdl);
    *hdl = NULL;
    __this = NULL;
    local_irq_enable();
}

#endif//TCFG_MIC_EFFECT_ENABLE


