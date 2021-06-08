#include "effect_tool.h"
#include "effect_debug.h"
/* #include "reverb/reverb_api.h" */
/* #include "application/audio_dig_vol.h" */
#include "audio_splicing.h"
/* #include "effect_config.h" */
#include "application/audio_bfilt.h"
/* #include "audio_effect/audio_eq.h" */
#include "application/audio_eq_drc_apply.h"
#include "application/audio_echo_src.h"
#include "application/audio_energy_detect.h"
#include "clock_cfg.h"
#include "media/audio_stream.h"
#include "media/includes.h"
#include "mic_effect.h"
#include "asm/dac.h"
#include "audio_enc.h"
#include "audio_dec.h"
#include "stream_entry.h"
#include "effect_linein.h"
#include "audio_recorder_mix.h"
#define LOG_TAG     "[APP-REVERB]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#if (TCFG_MIC_EFFECT_ENABLE)

extern struct audio_dac_hdl dac_hdl;
extern int *get_outval_addr(u8 mode);

enum {
    MASK_REVERB = 0x0,
    MASK_PITCH,
    MASK_ECHO,
    MASK_NOISEGATE,
    MASK_SHOUT_WHEAT,
    MASK_LOW_SOUND,
    MASK_HIGH_SOUND,
    MASK_EQ,
    MASK_EQ_SEG,
    MASK_EQ_GLOBAL_GAIN,
    MASK_MIC_GAIN,
    MASK_MAX,
};

typedef struct _BFILT_API_STRUCT_ {
    SHOUT_WHEAT_PARM_SET 	shout_wheat;
    LOW_SOUND_PARM_SET 		low_sound;
    HIGH_SOUND_PARM_SET 	high_sound;
    audio_bfilt_hdl             *hdl;     //系数计算句柄
} BFILT_API_STRUCT;

struct __fade {
    int wet;
    u32 delay;
    u32 decayval;
};
extern struct audio_mixer mixer;
struct __mic_effect {
    OS_MUTEX				 		mutex;
    struct __mic_effect_parm     	parm;
    struct __fade	 				fade;
    volatile u32					update_mask;
    mic_stream 						*mic;
    /* PITCH_SHIFT_PARM        		*p_set; */
    /* NOISEGATE_API_STRUCT    		*n_api; */
    BFILT_API_STRUCT 				*filt;
    struct audio_eq_drc             *eq_drc;    //eq drc句柄

    struct audio_stream *stream;		// 音频流
    struct audio_stream_entry entry;	// effect 音频入口
    int out_len;
    int process_len;
    u8 input_ch_num;                      //mic输入给混响数据流的源声道数

    struct audio_mixer_ch mix_ch;//for test

    REVERBN_API_STRUCT 		*p_reverb_hdl;
    ECHO_API_STRUCT 		*p_echo_hdl;
    s_pitch_hdl 			*p_pitch_hdl;
    NOISEGATE_API_STRUCT	*p_noisegate_hdl;
    void            		*d_vol;
    HOWLING_API_STRUCT 		*p_howling_hdl;
    ECHO_SRC_API_STRUCT 	*p_echo_src_hdl;
    struct channel_switch   *channel_zoom;
    struct audio_dac_channel *dac;
#if (RECORDER_MIX_EN)
    struct __stream_entry 	*rec_hdl;
#endif
#if (TCFG_USB_MIC_DATA_FROM_MICEFFECT||TCFG_USB_MIC_DATA_FROM_DAC)
    struct __stream_entry 	*usbmic_hdl;
    u8    usbmic_start;
#endif
    void *energy_hdl;     //能量检测的hdl
    u8 dodge_en;          //能量检测运行过程闪避是否使能

    struct __effect_linein *linein;

    u8  pause_mark;
};

struct __mic_stream_parm *g_mic_parm = NULL;
static struct __mic_effect *p_effect = NULL;
#define __this  p_effect
#define R_ALIN(var,al)     ((((var)+(al)-1)/(al))*(al))

void *mic_eq_drc_open(u32 sample_rate, u8 ch_num);
void mic_eq_drc_close(struct audio_eq_drc *eq_drc);
void mic_eq_drc_update();
BFILT_API_STRUCT *mic_high_bass_coeff_cal_init(u32 sample_rate);
void mic_high_bass_coeff_cal_uninit(BFILT_API_STRUCT *filt);
void mic_effect_echo_parm_parintf(ECHO_PARM_SET *parm);


void *mic_energy_detect_open(u32 sr, u8 ch_num);
void mic_energy_detect_close(void *hdl);

/*----------------------------------------------------------------------------*/
/**@brief    mic数据流音效处理参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_parm_update(struct __mic_effect *effect)
{
    for (int i = 0; i < MASK_MAX; i++) {
        if (effect->update_mask & BIT(i)) {
            effect->update_mask &= ~BIT(i);
            switch (i) {
            case MASK_REVERB:
                break;
            case MASK_PITCH:
                break;
            case MASK_ECHO:
                break;
            case MASK_NOISEGATE:
                break;
            case MASK_SHOUT_WHEAT:
                break;
            case MASK_LOW_SOUND:
                break;
            case MASK_HIGH_SOUND:
                break;
            case MASK_EQ:
                break;
            case MASK_EQ_SEG:
                break;
            case MASK_EQ_GLOBAL_GAIN:
                break;
            case MASK_MIC_GAIN:
                break;
            }
        }
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    mic混响参数淡入淡出处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_fade_run(struct __mic_effect *effect)
{
    if (effect == NULL) {
        return ;
    }
    u8 update = 0;
#if 10
    if (effect->p_reverb_hdl) {
        int wet = effect->p_reverb_hdl->parm.wet;
        if (wet != effect->fade.wet) {
            update = 1;
            if (wet > effect->fade.wet) {
                wet --;
            } else {
                wet ++;
            }
        }
        if (update) {
            effect->p_reverb_hdl->parm.wet = wet;
            update_reverb_parm(effect->p_reverb_hdl, &effect->p_reverb_hdl->parm);
        }
    }
    update = 0;
    if (effect->p_echo_hdl) {
        int delay = effect->p_echo_hdl->echo_parm_obj.delay;
        int decayval = effect->p_echo_hdl->echo_parm_obj.decayval;
        if (delay != effect->fade.delay) {
            update = 1;
            delay = effect->fade.delay;
        }
        if (decayval != effect->fade.decayval) {
            update = 1;
            if (decayval > effect->fade.decayval) {
                decayval --;
            } else {
                decayval ++;
            }
        }
        if (update) {
            effect->p_echo_hdl->echo_parm_obj.delay = delay;
            effect->p_echo_hdl->echo_parm_obj.decayval = decayval;
            update_echo_parm(effect->p_echo_hdl, &effect->p_echo_hdl->echo_parm_obj);
        }
    }
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief    mic数据流串接入口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
/* #define TEST_PORT IO_PORTA_00 */
static u32 mic_effect_effect_run(void *priv, void *in, void *out, u32 inlen, u32 outlen)
{
    struct __mic_effect *effect = (struct __mic_effect *)priv;
    if (effect == NULL) {
        return 0;
    }
    struct audio_data_frame frame = {0};
    frame.channel = effect->input_ch_num;
    frame.sample_rate = effect->parm.sample_rate;
    frame.data_len = inlen;
    frame.data = in;
    effect->out_len = 0;
    effect->process_len = inlen;

    if (effect->pause_mark) {
        memset(in, 0, inlen);
        /* return outlen; */
    }
    mic_effect_fade_run(effect);//reverb 或者echo 参数淡入淡出
    while (1) {
        audio_stream_run(&effect->entry, &frame);
        if (effect->out_len >= effect->process_len) {
            break;
        }
        frame.data = (s16 *)((u8 *)in + effect->out_len);
        frame.data_len = inlen - effect->out_len;
    }
    return outlen;
}

/*----------------------------------------------------------------------------*/
/**@brief   释放mic数据流资源
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_destroy(struct __mic_effect **hdl)
{
    if (hdl == NULL || *hdl == NULL) {
        return ;
    }
    struct __mic_effect *effect = *hdl;
    if (effect->mic) {
        log_i("mic_stream_destroy\n\n\n");
        mic_stream_destroy(&effect->mic);
    }

#if TCFG_MIC_DODGE_EN
    if (effect->energy_hdl) {
        mic_energy_detect_close(effect->energy_hdl);
    }
#endif

    if (effect->p_noisegate_hdl) {
        log_i("close_noisegate\n\n\n");
        close_noisegate(effect->p_noisegate_hdl);
    }
    if (effect->p_howling_hdl) {
        log_i("close_howling\n\n\n");
        close_howling(effect->p_howling_hdl);
    }

    if (effect->eq_drc) {
        log_i("mic_eq_drc_close\n\n\n");
        mic_eq_drc_close(effect->eq_drc);
    }
    if (effect->p_pitch_hdl) {
        log_i("close_pitch\n\n\n");
        close_pitch(effect->p_pitch_hdl);
    }

    if (effect->p_reverb_hdl) {
        log_i("close_reverb\n\n\n");
        close_reverb(effect->p_reverb_hdl);
    }
    if (effect->p_echo_hdl) {
        log_i("close_echo\n\n\n");
        close_echo(effect->p_echo_hdl);
    }


    if (effect->filt) {
        log_i("mic_high_bass_coeff_cal_uninit\n\n\n");
        mic_high_bass_coeff_cal_uninit(effect->filt);
    }
    if (effect->d_vol) {
        audio_stream_del_entry(audio_dig_vol_entry_get(effect->d_vol));
#if SYS_DIGVOL_GROUP_EN
        sys_digvol_group_ch_close("mic_mic");
#else
        audio_dig_vol_close(effect->d_vol);
#endif/*SYS_DIGVOL_GROUP_EN*/
    }
    if (effect->linein) {
        effect_linein_close(&effect->linein);
    }
    if (effect->p_echo_src_hdl) {
        log_i("close_echo src\n\n\n");
        close_echo_src(effect->p_echo_src_hdl);
    }
    if (effect->channel_zoom) {
        channel_switch_close(&effect->channel_zoom);
        /*effect->channel_zoom = NULL;*/
    }
    if (effect->dac) {
        audio_stream_del_entry(&effect->dac->entry);
        audio_dac_free_channel(effect->dac);
        free(effect->dac);
        effect->dac = NULL;
    }

#if (RECORDER_MIX_EN)
    if (effect->rec_hdl) {
        stream_entry_close(&effect->rec_hdl);
    }
#endif
#if (TCFG_USB_MIC_DATA_FROM_MICEFFECT||TCFG_USB_MIC_DATA_FROM_DAC)
    if (effect->usbmic_hdl) {
        stream_entry_close(&effect->usbmic_hdl);
    }
#endif

    if (effect->stream) {
        audio_stream_close(effect->stream);
    }
    local_irq_disable();
    free(effect);
    *hdl = NULL;
    local_irq_enable();

    mem_stats();
    clock_remove_set(REVERB_CLK);
}
/*----------------------------------------------------------------------------*/
/**@brief    串流唤醒
   @param
   @return
   @note 暂未使用
*/
/*----------------------------------------------------------------------------*/
static void mic_stream_resume(void *p)
{
    struct __mic_effect *effect = (struct __mic_effect *)p;
    /* audio_decoder_resume_all(&decode_task); */
}

/*----------------------------------------------------------------------------*/
/**@brief    串流数据处理长度回调
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_data_process_len(struct audio_stream_entry *entry, int len)
{

    struct __mic_effect *effect = container_of(entry, struct __mic_effect, entry);
    effect->out_len += len;
    /* printf("out len[%d]",effect->out_len); */
}

static int mic_effect_record_stream_callback(void *priv, struct audio_data_frame *in)
{
    struct __mic_effect *effect = (struct __mic_effect *)priv;

    s16 *data = in->data;
    u32 len = in->data_len;

    return recorder_mix_pcm_write((u8 *)data, len);
}
/* extern int usb_audio_mic_write(void *data, u16 len); */
extern int usb_audio_mic_write_do(void *data, u16 len);
static int mic_effect_otherout_stream_callback(void *priv, struct audio_data_frame *in)
{
    struct __mic_effect *effect = (struct __mic_effect *)priv;
    s16 *data = in->data;
    u32 len = in->data_len;

#if ((TCFG_USB_MIC_DATA_FROM_MICEFFECT||TCFG_USB_MIC_DATA_FROM_DAC) && (TCFG_MIC_EFFECT_DEBUG == 0))
    if (effect->usbmic_start) {
        /* putchar('A');		 */
        if (len) {
            usb_audio_mic_write_do(data, len);
        }
    } else {
        /* putchar('B');		 */
    }
#endif
    return len;
}

void mic_effect_to_usbmic_onoff(u8 mark)
{
#if (TCFG_USB_MIC_DATA_FROM_MICEFFECT||TCFG_USB_MIC_DATA_FROM_DAC)
    if (__this) {
        __this->usbmic_start = mark ? 1 : 0;
    }
#endif
}

static void  inline rl_rr_mix_to_rl_rr(short *data, int len)
{
    s32 tmp32_1;
    s32 tmp32_2;
    s16 *inbuf = data;
    inbuf = inbuf + 2;  //定位到第三通道
    len >>= 3;
    __asm__ volatile(
        "1:                      \n\t"
        "rep %0 {                \n\t"
        "  %2 = h[%1 ++= 2](s)     \n\t"  //取第三通道值，并地址偏移两个字节指向第四通道数据
        " %3 = h[%1 ++= -2](s)   \n\t"   //取第四通道值，并地址偏移两个字节指向第三通道数据
        " %2 = %2 + %3           \n\t"
        " %2 = sat16(%2)(s)      \n\t"  //饱和处理
        " h[%1 ++= 2] = %2      \n\t"  //存取第三通道数据，并地址偏移两个字节指向第四通道数据
        " h[%1 ++= 6] = %2      \n\t"  //存取第四通道数据，并地址偏移六个字节指向第三通道相邻的数据
        "}                      \n\t"
        "if(%0 != 0) goto 1b    \n\t"
        :
        "=&r"(len),
        "=&r"(inbuf),
        "=&r"(tmp32_1),
        "=&r"(tmp32_2)
        :
        "0"(len),
        "1"(inbuf),
        "2"(tmp32_1),
        "3"(tmp32_2)
        :
    );
}
static int effect_to_dac_data_pro_handle(struct audio_stream_entry *entry,  struct audio_data_frame *in)
{
#if (SOUNDCARD_ENABLE)
    if (in->data_len == 0) {
        return 0;
    }
#if 1
    rl_rr_mix_to_rl_rr(in->data, in->data_len);//汇编加速
#else
    s16 *outbuf = in->data;
    s16 *inbuf = in->data;
    s32 tmp32;
    u16 len = in->data_len;
    len >>= 3;
    while (len--) {
        *outbuf++ = inbuf[0];
        *outbuf++ = inbuf[1];
        tmp32 = (inbuf[2] + inbuf[3]);
        if (tmp32 < -32768) {
            tmp32 = -32768;
        } else if (tmp32 > 32767) {
            tmp32 = 32767;
        }
        *outbuf++ = tmp32;
        *outbuf++ = tmp32;
        inbuf += 4;
    }
#endif
#endif
    return 0;
}

#if TCFG_APP_RECORD_EN
static int prob_handler_to_record(struct audio_stream_entry *entry,  struct audio_data_frame *in)
{
    if (in->data_len == 0) {
        return 0;
    }
    if (recorder_is_encoding() == 0) {
        return 0;
    }
    int  wlen = recorder_userdata_to_enc(in->data, in->data_len);
    if (wlen != in->data_len) {
        putchar('N');
    }
    return 0;
}
#endif

#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR || TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_DUAL_LR_DIFF)
#define DAC_OUTPUT_CHANNELS     2
#elif (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_L || TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_R || TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR_DIFF)
#define DAC_OUTPUT_CHANNELS     1
#elif (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_LR)
#define DAC_OUTPUT_CHANNELS     4
#else
#define DAC_OUTPUT_CHANNELS     3
#endif
/*----------------------------------------------------------------------------*/
/**@brief    (mic数据流)混响打开接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
bool mic_effect_start(void)
{
    bool ret = false;
    printf("\n--func=%s\n", __FUNCTION__);
    if (__this) {
        log_e("reverb is already start \n");
        return ret;
    }
    struct __mic_effect *effect = (struct __mic_effect *)zalloc(sizeof(struct __mic_effect));
    if (effect == NULL) {
        return false;
    }
    clock_add_set(REVERB_CLK);
    os_mutex_create(&effect->mutex);
    memcpy(&effect->parm, &effect_parm_default, sizeof(struct __mic_effect_parm));
    struct __mic_stream_parm *mic_parm = (struct __mic_stream_parm *)&effect_mic_stream_parm_default;
    if (g_mic_parm) {
        mic_parm = g_mic_parm;
    }

    if ((effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_REVERB))
        && (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_ECHO))) {
        log_e("effect config err ?? !!!, cann't support echo && reverb at the same time\n");
        mic_effect_destroy(&effect);
        return false;
    }
    u8 ch_num = 1; //??
    effect->input_ch_num = ch_num;
#if TCFG_MIC_DODGE_EN
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_ENERGY_DETECT))	{
        effect->energy_hdl = mic_energy_detect_open(effect->parm.sample_rate, ch_num);
        effect->dodge_en = 0;//默认关闭， 需要通过按键触发打开
    }
#endif

    ///声音门限初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_NOISEGATE)) {
        effect->p_noisegate_hdl = open_noisegate((NOISEGATE_PARM *)&effect_noisegate_parm_default, 0, 0);
    }
    ///啸叫抑制初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_HOWLING)) {
        log_i("open_howling\n\n\n");
        effect->p_howling_hdl = open_howling(NULL, effect->parm.sample_rate, 0, 1);
    }
    ///滤波器初始化
    ///eq初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_EQ)) {
        log_i("effect->parm.sample_rate %d\n", effect->parm.sample_rate);
        effect->filt = mic_high_bass_coeff_cal_init(effect->parm.sample_rate);
        if (!effect->filt) {
            log_e("mic filt malloc err\n");
        }
        effect->eq_drc = mic_eq_drc_open(effect->parm.sample_rate, ch_num);
    }
    ///pitch 初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_PITCH)) {
        log_i("open_pitch\n\n\n");
        effect->p_pitch_hdl = open_pitch((PITCH_SHIFT_PARM *)&effect_pitch_parm_default);
        pause_pitch(effect->p_pitch_hdl, 1);
    }
    ///reverb 初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_REVERB)) {
        ch_num = 2;
        effect->fade.wet = effect_reverb_parm_default.wet;
        effect->p_reverb_hdl = open_reverb((REVERBN_PARM_SET *)&effect_reverb_parm_default, effect->parm.sample_rate);
        pause_reverb(effect->p_reverb_hdl, 1);
    }
    ///echo 初始化
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_ECHO)) {
        effect->fade.decayval = effect_echo_parm_default.decayval;
        effect->fade.delay = effect_echo_parm_default.delay;
        log_i("open_echo\n\n\n");
        effect->p_echo_hdl = open_echo((ECHO_PARM_SET *)&effect_echo_parm_default, (EF_REVERB_FIX_PARM *)&effect_echo_fix_parm_default);
    }

    ///初始化数字音量
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_DVOL)) {
        effect_dvol_default_parm.ch_total = ch_num;
        struct audio_stream_entry *dvol_entry;
#if SYS_DIGVOL_GROUP_EN
        dvol_entry = sys_digvol_group_ch_open("mic_mic", -1, &effect_dvol_default_parm);
        effect->d_vol = audio_dig_vol_group_hdl_get(sys_digvol_group, "mic_mic");
#else
        effect->d_vol = audio_dig_vol_open((audio_dig_vol_param *)&effect_dvol_default_parm);
        dvol_entry = audio_dig_vol_entry_get(effect->d_vol);
#endif /*SYS_DIGVOL_GROUP_EN*/
    }

    //打开混响变采样
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_SOFT_SRC)) {
        u32 out_sr = audio_output_nor_rate();
        effect->p_echo_src_hdl = open_echo_src(effect->parm.sample_rate, out_sr, ch_num);
#if TCFG_APP_RECORD_EN
        effect->p_echo_src_hdl->entry.prob_handler = prob_handler_to_record;
#endif//TCFG_APP_RECORD_EN
    }

    //混响通路混合linein
    if (effect->parm.effect_config & BIT(MIC_EFFECT_CONFIG_LINEIN)) {
        effect->linein = effect_linein_open();
    }

    u8 output_channels = DAC_OUTPUT_CHANNELS;
    if (output_channels != ch_num) {
        u32 points_num = REVERB_LADC_IRQ_POINTS * 4;
        effect->channel_zoom = channel_switch_open(output_channels == 2 ? AUDIO_CH_LR : (output_channels == 4 ? AUDIO_CH_QUAD : AUDIO_CH_DIFF), output_channels == 4 ? (points_num * 2 + 128) : 1024);
    }
    // dac mix open

    effect->dac = (struct audio_dac_channel *)zalloc(sizeof(struct audio_dac_channel));
    if (effect->dac) {
        audio_dac_new_channel(&dac_hdl, effect->dac);
        struct audio_dac_channel_attr attr;
        audio_dac_channel_get_attr(effect->dac, &attr);
        attr.delay_time = mic_parm->dac_delay;
        attr.write_mode = WRITE_MODE_FORCE;
        audio_dac_channel_set_attr(effect->dac, &attr);
        effect->dac->entry.prob_handler = effect_to_dac_data_pro_handle;
        /* audio_dac_channel_set_pause(effect->dac,1); */
    }

    effect->entry.data_process_len = mic_effect_data_process_len;

#if (RECORDER_MIX_EN)
    ///送录音数据流
    effect->rec_hdl = stream_entry_open(effect, mic_effect_record_stream_callback, 1);
#endif
#if (TCFG_USB_MIC_DATA_FROM_MICEFFECT||TCFG_USB_MIC_DATA_FROM_DAC)
    effect->usbmic_hdl = stream_entry_open(effect, mic_effect_otherout_stream_callback, 1);
#endif

// 数据流串联
    struct audio_stream_entry *entries[15] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &effect->entry;
    if (effect->energy_hdl) {
        entries[entry_cnt++] = audio_energy_detect_entry_get(effect->energy_hdl);
    }
    if (effect->p_noisegate_hdl) {
        entries[entry_cnt++] = &effect->p_noisegate_hdl->entry;
    }
    if (effect->p_howling_hdl) {
        entries[entry_cnt++] = &effect->p_howling_hdl->entry;
    }
    if (effect->eq_drc) {
        entries[entry_cnt++] = &effect->eq_drc->entry;
    }
    if (effect->p_pitch_hdl) {
        entries[entry_cnt++] = &effect->p_pitch_hdl->entry;
    }

    if (effect->p_reverb_hdl) {
        entries[entry_cnt++] = &effect->p_reverb_hdl->entry;
    }
    if (effect->p_echo_hdl) {
        entries[entry_cnt++] = &effect->p_echo_hdl->entry;
    }

    if (effect->d_vol) {
        entries[entry_cnt++] = audio_dig_vol_entry_get(effect->d_vol);
    }

    if (effect->linein) {
        entries[entry_cnt++] = effect_linein_get_stream_entry(effect->linein);
    }

#if (RECORDER_MIX_EN)
    u8 record_entry_cnt = entry_cnt - 1;
#endif

    if (effect->p_echo_src_hdl) {
        entries[entry_cnt++] = &effect->p_echo_src_hdl->entry;
    }

    if (effect->channel_zoom) {
        entries[entry_cnt++] = &effect->channel_zoom->entry;
    }
    if (effect->dac) {
        entries[entry_cnt++] = &effect->dac->entry;
#if (TCFG_USB_MIC_DATA_FROM_DAC)
        if (effect->usbmic_hdl) {
            entries[entry_cnt++] = &effect->usbmic_hdl->entry;
        }
#endif
    }

    effect->stream = audio_stream_open(effect, mic_stream_resume);
    audio_stream_add_list(effect->stream, entries, entry_cnt);

#if (RECORDER_MIX_EN)
    ///数据流分支给录音用
    if (effect->rec_hdl) {
        //从变采样前一节点分流
        audio_stream_add_entry(entries[record_entry_cnt], &effect->rec_hdl->entry);
    }
#endif
#if (TCFG_USB_MIC_DATA_FROM_MICEFFECT)
    ///数据流分支给usbmic用
    if (effect->usbmic_hdl) {
        //从变采样前一节点分流
        audio_stream_add_entry(entries[entry_cnt - 4], &effect->usbmic_hdl->entry);
    }
#endif
    ///mic 数据流初始化
    effect->mic = mic_stream_creat(mic_parm);
    if (effect->mic == NULL) {
        mic_effect_destroy(&effect);
        return false;
    }
    mic_stream_set_output(effect->mic, (void *)effect, mic_effect_effect_run);
    mic_stream_start(effect->mic);
#if (RECORDER_MIX_EN)
    recorder_mix_pcm_stream_open(effect->parm.sample_rate, ch_num);
#endif

    clock_set_cur();
    __this = effect;


    log_info("--------------------------effect start ok\n");
    mem_stats();
    mic_effect_change_mode(0);



    return true;
}

/*----------------------------------------------------------------------------*/
/**@brief    (mic数据流)混响关闭接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_stop(void)
{
    mic_effect_destroy(&__this);
#if (RECORDER_MIX_EN)
    recorder_mix_pcm_stream_close();
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief    (mic数据流)混响暂停接口(整个数据流不运行)
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_pause(u8 mark)
{
    if (__this) {
        __this->pause_mark = mark ? 1 : 0;
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    (mic数据流)混响暂停输出到DAC(数据流后级不写入DAC)
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_dac_pause(u8 mark)
{
    if (__this && __this->dac) {
        audio_dac_channel_set_pause(__this->dac, mark);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    (mic数据流)混响状态获取接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 mic_effect_get_status(void)
{
    return ((__this) ? 1 : 0);
}

/*----------------------------------------------------------------------------*/
/**@brief    数字音量调节接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_set_dvol(u8 vol)
{
    if (__this == NULL) {
        return ;
    }
    audio_dig_vol_set(__this->d_vol, 3, vol);
}
u8 mic_effect_get_dvol(void)
{
    if (__this) {
        return audio_dig_vol_get(__this->d_vol, 1);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    获取mic增益接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 mic_effect_get_micgain(void)
{
    /* if (__this) { */
    /* } */
    //动态调的需实时记录
    return effect_mic_stream_parm_default.mic_gain;
}


/*----------------------------------------------------------------------------*/
/**@brief    reverb 效果声增益调节接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_set_reverb_wet(int wet)
{
    if (__this == NULL || __this->p_reverb_hdl == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);
    __this->fade.wet = wet;
    os_mutex_post(&__this->mutex);
}

int mic_effect_get_reverb_wet(void)
{
    if (__this && __this->p_reverb_hdl) {
        return __this->fade.wet;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    echo 回声延时调节接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_set_echo_delay(u32 delay)
{
    if (__this == NULL || __this->p_echo_hdl == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);
    __this->fade.delay = delay;
    os_mutex_post(&__this->mutex);
}
u32 mic_effect_get_echo_delay(void)
{
    if (__this && __this->p_echo_hdl) {
        return __this->fade.delay;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    echo 回声衰减系数调节接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_set_echo_decay(u32 decay)
{
    if (__this == NULL || __this->p_echo_hdl == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);
    __this->fade.decayval = decay;
    os_mutex_post(&__this->mutex);
}

u32 mic_effect_get_echo_decay(void)
{
    if (__this && __this->p_echo_hdl) {
        return __this->fade.decayval;
    }
    return 0;
}

void mic_effect_set_mic_parm(struct __mic_stream_parm *parm)
{
    g_mic_parm = parm;
}

/*----------------------------------------------------------------------------*/
/**@brief    设置各类音效运行标记
   @param
   @return
   @note 暂不使用
*/
/*----------------------------------------------------------------------------*/
void mic_effect_set_function_mask(u32 mask)
{
    if (__this == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);
    __this->parm.effect_run = mask;
    os_mutex_post(&__this->mutex);
}
/*----------------------------------------------------------------------------*/
/**@brief    获取各类音效运行标记
   @param
   @return
   @note 暂不使用
*/
/*----------------------------------------------------------------------------*/
u32 mic_effect_get_function_mask(void)
{
    if (__this) {
        return __this->parm.effect_run;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    喊mic效果系数计算
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_shout_wheat_cal_coef(int sw)
{
    /* log_info("%s %d\n", __FUNCTION__, __LINE__); */
    if (__this == NULL || __this->filt == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);
    BFILT_API_STRUCT *filt = __this->filt;
    if (__this->parm.sample_rate == 0) {
        __this->parm.sample_rate = MIC_EFFECT_SAMPLERATE;
    }

    if (filt->shout_wheat.center_frequency == 0) {
        memcpy(&filt->shout_wheat, &effect_shout_wheat_default, sizeof(SHOUT_WHEAT_PARM_SET));
    }
    audio_bfilt_update_parm u_parm = {0};
    u_parm.freq = filt->shout_wheat.center_frequency;
    u_parm.bandwidth = filt->shout_wheat.bandwidth;
    u_parm.iir_type = TYPE_BANDPASS;
    u_parm.filt_num = 0;
    u_parm.filt_tar_tab  = get_outval_addr(u_parm.filt_num);
    if (sw) {
        u_parm.gain = filt->shout_wheat.occupy;
        log_i("shout_wheat_cal_coef on\n");
    } else {
        u_parm.gain = 0;
        log_i("shout_wheat_cal_coef off\n");
    }
    audio_bfilt_cal_coeff(filt->hdl, &u_parm);//喊麦滤波器
    os_mutex_post(&__this->mutex);
}
/*----------------------------------------------------------------------------*/
/**@brief    低音系数计算
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_low_sound_cal_coef(int gainN)
{
    /* log_info("%s %d\n", __FUNCTION__, __LINE__); */
    if (__this == NULL || __this->filt == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);
    if (__this->parm.sample_rate == 0) {
        __this->parm.sample_rate = MIC_EFFECT_SAMPLERATE;
    }

    BFILT_API_STRUCT *filt = __this->filt;
    audio_bfilt_update_parm u_parm = {0};

    u_parm.freq = filt->low_sound.cutoff_frequency;
    u_parm.bandwidth = 1024;
    u_parm.iir_type = TYPE_LOWPASS;
    u_parm.filt_num = 1;
    u_parm.filt_tar_tab  = get_outval_addr(u_parm.filt_num);

    gainN = filt->low_sound.lowest_gain + gainN * (filt->low_sound.highest_gain - filt->low_sound.lowest_gain) / 10;

    log_i("low sound gainN %d\n", gainN);
    log_i("lowest_gain %d\n", filt->low_sound.lowest_gain);
    log_i("highest_gain %d\n", filt->low_sound.highest_gain);

    if ((gainN >= filt->low_sound.lowest_gain) && (gainN <= filt->low_sound.highest_gain)) {
        u_parm.gain = gainN;
        audio_bfilt_cal_coeff(filt->hdl, &u_parm);//低音滤波器
    }
    os_mutex_post(&__this->mutex);
}
/*----------------------------------------------------------------------------*/
/**@brief    高音系数计算
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void mic_effect_high_sound_cal_coef(int gainN)
{
    if (__this == NULL || __this->filt == NULL) {
        return ;
    }
    os_mutex_pend(&__this->mutex, 0);

    if (__this->parm.sample_rate == 0) {
        __this->parm.sample_rate = MIC_EFFECT_SAMPLERATE;
    }

    BFILT_API_STRUCT *filt = __this->filt;
    audio_bfilt_update_parm u_parm = {0};
    u_parm.freq = filt->high_sound.cutoff_frequency;
    u_parm.bandwidth = 1024;
    u_parm.iir_type = TYPE_HIGHPASS;
    u_parm.filt_num = 2;
    u_parm.filt_tar_tab  = get_outval_addr(u_parm.filt_num);
    gainN = filt->high_sound.lowest_gain + gainN * (filt->high_sound.highest_gain - filt->high_sound.lowest_gain) / 10;
    log_i("high gainN %d\n", gainN);
    log_i("lowest_gain %d\n", filt->high_sound.lowest_gain);
    log_i("highest_gain %d\n", filt->high_sound.highest_gain);
    if ((gainN >= filt->high_sound.lowest_gain) && (gainN <= filt->high_sound.highest_gain)) {
        u_parm.gain = gainN;
        audio_bfilt_cal_coeff(filt->hdl, &u_parm);//高音滤波器
    }
    os_mutex_post(&__this->mutex);
}


/*----------------------------------------------------------------------------*/
/**@brief    mic_effect_cal_coef 混响喊麦、高低音 调节接口
   @param    filtN:MIC_EQ_MODE_SHOUT_WHEAT 喊麦模式，gainN：喊麦开关，0:关喊麦， 1：开喊麦
   @param    filtN:MIC_EQ_MODE_LOW_SOUND   低音调节  gainN：调节的增益，范围0~10
   @param    filtN:MIC_EQ_MODE_HIGH_SOUND  高音调节  gainN：调节的增益，范围0~10
   @return
   @note     混响喊麦、高低音调节
*/
/*----------------------------------------------------------------------------*/
void mic_effect_cal_coef(u8 type, u32 gainN)
{

    log_i("filN %d, gainN %d\n", type, gainN);
    if (type == MIC_EQ_MODE_SHOUT_WHEAT) {
        mic_effect_shout_wheat_cal_coef(gainN);
    } else if (type == MIC_EQ_MODE_LOW_SOUND) {
        mic_effect_low_sound_cal_coef(gainN);
    } else if (type == MIC_EQ_MODE_HIGH_SOUND) {
        mic_effect_high_sound_cal_coef(gainN);
    }
    mic_eq_drc_update();
}


#if !MIC_EFFECT_EQ_EN
static int outval[3][5]; //开3个2阶滤波器的空间，给硬件eq存系数的
__attribute__((weak))int *get_outval_addr(u8 mode)
{
    //高低音系数表地址
    return outval[mode];
}
#endif

u8 mic_effect_eq_section_num(void)
{
#if TCFG_EQ_ENABLE
    return (EFFECT_EQ_SECTION_MAX + 3);
#else
    return 0;
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief    reverb 混响参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_reverb_parm_fill(REVERBN_PARM_SET *parm, u8 fade, u8 online)
{

    if (__this == NULL || __this->p_reverb_hdl == NULL) {
        return ;
    }
    if (parm == NULL) {
        __this->parm.effect_run &= ~BIT(MIC_EFFECT_CONFIG_REVERB);
        pause_reverb(__this->p_reverb_hdl, 1);
        return ;
    }
    mic_effect_reverb_parm_printf(parm);
    REVERBN_PARM_SET tmp;
    os_mutex_pend(&__this->mutex, 0);
    memcpy(&tmp, parm, sizeof(REVERBN_PARM_SET));
    if (fade) {
        //针对需要fade的参数，读取旧值，通过fade来更新对应的参数
        tmp.wet = __this->p_reverb_hdl->parm.wet;///读取旧值,暂时不更新
        if (online) {
            __this->fade.wet = parm->wet;///设置wet fade 目标值, 通过fade更新
        } else {
            __this->fade.wet = __this->p_reverb_hdl->parm.wet;//值不更新, 通过外部按键更新， 如旋钮
        }
    }
    __this->update_mask |= BIT(MASK_REVERB);
    __this->parm.effect_run |= BIT(MIC_EFFECT_CONFIG_REVERB);
    pause_reverb(__this->p_reverb_hdl, 0);
    update_reverb_parm(__this->p_reverb_hdl, &tmp);

    os_mutex_post(&__this->mutex);
}
/*----------------------------------------------------------------------------*/
/**@brief    echo 混响参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_echo_parm_fill(ECHO_PARM_SET *parm, u8 fade, u8 online)
{

    if (__this == NULL || __this->p_echo_hdl == NULL) {
        return ;
    }
    if (parm == NULL) {
        __this->parm.effect_run &= ~BIT(MIC_EFFECT_CONFIG_ECHO);
        pause_echo(__this->p_echo_hdl, 1);
        return ;
    }
    ECHO_PARM_SET tmp;
    os_mutex_pend(&__this->mutex, 0);
    memcpy(&tmp, parm, sizeof(ECHO_PARM_SET));
    if (fade) {
        //针对需要fade的参数，读取旧值，通过fade来更新对应的参数
        tmp.delay = __this->p_echo_hdl->echo_parm_obj.delay;
        tmp.decayval = __this->p_echo_hdl->echo_parm_obj.decayval;
        if (online) {
            __this->fade.delay = parm->delay;///设置wet fade 目标值, 通过fade更新
            __this->fade.decayval = parm->decayval;///设置wet fade 目标值, 通过fade更新
        } else {
            __this->fade.delay = __this->p_echo_hdl->echo_parm_obj.delay;///值不更新, 通过外部按键更新， 如旋钮
            __this->fade.decayval = __this->p_echo_hdl->echo_parm_obj.decayval;///值不更新, 通过外部按键更新， 如旋钮
        }
    }
    __this->update_mask |= BIT(MASK_ECHO);
    __this->parm.effect_run |= BIT(MIC_EFFECT_CONFIG_ECHO);
    pause_echo(__this->p_echo_hdl, 0);
    update_echo_parm(__this->p_echo_hdl, &tmp);
    os_mutex_post(&__this->mutex);
}
/*----------------------------------------------------------------------------*/
/**@brief    变声参数直接更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void set_pitch_para(u32 shiftv, u32 sr, u8 effect, u32 formant_shift)
{
    if (__this == NULL || __this->p_pitch_hdl == NULL) {
        return ;
    }
    PITCH_SHIFT_PARM p_pitch_parm = {0};//get_pitch_parm();

    p_pitch_parm.sr = sr;
    p_pitch_parm.shiftv = shiftv;
    p_pitch_parm.effect_v = effect;
    p_pitch_parm.formant_shift = formant_shift;

    printf("\n\n\nshiftv[%d],sr[%d],effect[%d],formant_shift[%d] \n\n", p_pitch_parm.shiftv, p_pitch_parm.sr, p_pitch_parm.effect_v, p_pitch_parm.formant_shift);
    update_picth_parm(__this->p_pitch_hdl, &p_pitch_parm);

}

PITCH_SHIFT_PARM picth_mode_table[] = {
    {16000, 56, EFFECT_PITCH_SHIFT, 0}, //哇哇音
    {16000, 136, EFFECT_PITCH_SHIFT, 0}, //女变男
    {16000, 56, EFFECT_VOICECHANGE_KIN0, 150}, //男变女1
    // {16000,56,EFFECT_VOICECHANGE_KIN1,150},	//男变女2
    // {16000,56,EFFECT_VOICECHANGE_KIN2,150},	//男变女3
    {16000, 196, EFFECT_PITCH_SHIFT, 100}, //魔音、怪兽音
    {16000, 100, EFFECT_AUTOTUNE, D_MAJOR} //电音
};

/*----------------------------------------------------------------------------*/
/**@brief    变声参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_pitch_parm_fill(PITCH_PARM_SET2 *parm, u8 fade, u8 online)
{
    if (__this == NULL || __this->p_pitch_hdl == NULL) {
        return ;
    }
    if (parm == NULL) {
        __this->parm.effect_run &= ~BIT(MIC_EFFECT_CONFIG_PITCH);
        pause_pitch(__this->p_pitch_hdl, 1);
        return ;
    }
    mic_effect_pitch2_parm_printf(parm);

    os_mutex_pend(&__this->mutex, 0);
    PITCH_SHIFT_PARM p_parm;
    p_parm.sr = MIC_EFFECT_SAMPLERATE;
    p_parm.effect_v = parm->effect_v;
    p_parm.formant_shift = parm->formant_shift;
    p_parm.shiftv = parm->pitch;

    __this->update_mask |= BIT(MASK_PITCH);
    __this->parm.effect_run |= BIT(MIC_EFFECT_CONFIG_PITCH);
    pause_pitch(__this->p_pitch_hdl, 0);
    update_picth_parm(__this->p_pitch_hdl, &p_parm);

    os_mutex_post(&__this->mutex);
}

/*----------------------------------------------------------------------------*/
/**@brief    噪声抑制参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_noisegate_parm_fill(NOISE_PARM_SET *parm, u8 fade, u8 online)
{
    if (__this == NULL || __this->p_noisegate_hdl == NULL) {
        return ;
    }
    if (parm == NULL) {
        __this->parm.effect_run &= ~BIT(MIC_EFFECT_CONFIG_NOISEGATE);
        pause_noisegate(__this->p_noisegate_hdl, 1);
        return ;
    }
    mic_effect_noisegate_parm_printf(parm);

    os_mutex_pend(&__this->mutex, 0);


    NOISEGATE_PARM noisegate;
    memcpy(&noisegate, &__this->p_noisegate_hdl->parm, sizeof(NOISEGATE_PARM));
    noisegate.attackTime = parm->attacktime;
    noisegate.releaseTime = parm->releasetime;
    noisegate.threshold = parm->threadhold;
    noisegate.low_th_gain = parm->gain;


    __this->update_mask |= BIT(MASK_NOISEGATE);
    __this->parm.effect_run |= BIT(MIC_EFFECT_CONFIG_NOISEGATE);
    pause_noisegate(__this->p_noisegate_hdl, 0);
    update_noisegate(__this->p_noisegate_hdl, &noisegate);
    os_mutex_post(&__this->mutex);
}
/*----------------------------------------------------------------------------*/
/**@brief    喊mic参数更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_shout_wheat_parm_fill(SHOUT_WHEAT_PARM_SET *parm, u8 fade, u8 online)
{
    /* log_info("%s %d\n", __FUNCTION__, __LINE__); */
    if (__this == NULL || __this->filt == NULL || parm == NULL) {
        return ;
    }
    mic_effect_shout_wheat_parm_printf(parm);

    SHOUT_WHEAT_PARM_SET tmp;
    os_mutex_pend(&__this->mutex, 0);
    memcpy(&tmp, parm, sizeof(SHOUT_WHEAT_PARM_SET));
    if (fade) {
        //针对需要fade的参数，读取旧值，通过fade来更新对应的参数
    }
    memcpy(&__this->filt->shout_wheat, &tmp, sizeof(SHOUT_WHEAT_PARM_SET));
    __this->update_mask |= BIT(MASK_SHOUT_WHEAT);
    os_mutex_post(&__this->mutex);
}
void mic_effect_low_sound_parm_fill(LOW_SOUND_PARM_SET *parm, u8 fade, u8 online)
{
    /* log_info("%s %d\n", __FUNCTION__, __LINE__); */
    if (__this == NULL || __this->filt == NULL || parm == NULL) {
        return ;
    }
    mic_effect_low_sound_parm_printf(parm);

    LOW_SOUND_PARM_SET tmp;
    os_mutex_pend(&__this->mutex, 0);
    memcpy(&tmp, parm, sizeof(LOW_SOUND_PARM_SET));
    if (fade) {
        //针对需要fade的参数，读取旧值，通过fade来更新对应的参数
    }
    memcpy(&__this->filt->low_sound, &tmp, sizeof(LOW_SOUND_PARM_SET));
    __this->update_mask |= BIT(MASK_LOW_SOUND);
    os_mutex_post(&__this->mutex);
}
void mic_effect_high_sound_parm_fill(HIGH_SOUND_PARM_SET *parm, u8 fade, u8 online)
{
    if (__this == NULL || __this->filt == NULL || parm == NULL) {
        return ;
    }
    mic_effect_high_sound_parm_printf(parm);

    HIGH_SOUND_PARM_SET tmp;
    os_mutex_pend(&__this->mutex, 0);
    memcpy(&tmp, parm, sizeof(HIGH_SOUND_PARM_SET));
    if (fade) {
        //针对需要fade的参数，读取旧值，通过fade来更新对应的参数
    }
    memcpy(&__this->filt->high_sound, &tmp, sizeof(HIGH_SOUND_PARM_SET));
    __this->update_mask |= BIT(MASK_HIGH_SOUND);
    os_mutex_post(&__this->mutex);
}

/*----------------------------------------------------------------------------*/
/**@brief    mic增益调节
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_effect_mic_gain_parm_fill(EFFECTS_MIC_GAIN_PARM *parm, u8 fade, u8 online)
{
    if (__this == NULL || parm == NULL) {
        return ;
    }
    audio_mic_set_gain(parm->gain);
}
/*----------------------------------------------------------------------------*/
/**@brief    mic效果模式切换（数据流音效组合切换）
   @param
   @return
   @note 使用效果配置文件时生效
*/
/*----------------------------------------------------------------------------*/
void mic_effect_change_mode(u16 mode)
{
    effect_cfg_change_mode(mode);
}
/*----------------------------------------------------------------------------*/
/**@brief    获取mic效果模式（数据流音效组合）
   @param
   @return
   @note 使用效果配置文件时生效
*/
/*----------------------------------------------------------------------------*/
u16 mic_effect_get_cur_mode(void)
{
    return effect_cfg_get_cur_mode();
}




/*----------------------------------------------------------------------------*/
/**@brief    eq接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_eq_drc_update()
{
    local_irq_disable();
    if (__this && __this->eq_drc && __this->eq_drc->eq) {
        __this->eq_drc->eq->updata = 1;
    }
    local_irq_enable();
}


BFILT_API_STRUCT *mic_high_bass_coeff_cal_init(u32 sample_rate)
{
    BFILT_API_STRUCT *filt = zalloc(sizeof(BFILT_API_STRUCT));
    if (!filt) {
        return NULL;
    }
    bfilt_open_parm parm = {0};
    parm.sr = sample_rate;
    filt->hdl = audio_bfilt_open(&parm);

    memcpy(&filt->shout_wheat, &effect_shout_wheat_default, sizeof(SHOUT_WHEAT_PARM_SET));
    memcpy(&filt->low_sound, &effect_low_sound_default, sizeof(LOW_SOUND_PARM_SET));
    memcpy(&filt->high_sound, &effect_high_sound_default, sizeof(HIGH_SOUND_PARM_SET));

    audio_bfilt_update_parm u_parm = {0};
    u_parm.freq = filt->shout_wheat.center_frequency;
    u_parm.bandwidth = filt->shout_wheat.bandwidth;
    u_parm.iir_type = TYPE_BANDPASS;
    u_parm.filt_num = 0;
    u_parm.filt_tar_tab  = get_outval_addr(u_parm.filt_num);
    u_parm.gain = 0;
    audio_bfilt_cal_coeff(filt->hdl, &u_parm);//喊麦滤波器

    u_parm.freq = filt->low_sound.cutoff_frequency;
    u_parm.bandwidth = 1024;
    u_parm.iir_type = TYPE_LOWPASS;
    u_parm.filt_num = 1;
    u_parm.filt_tar_tab  = get_outval_addr(u_parm.filt_num);
    u_parm.gain = 0;
    audio_bfilt_cal_coeff(filt->hdl, &u_parm);//低音滤波器

    u_parm.freq = filt->high_sound.cutoff_frequency;
    u_parm.bandwidth = 1024;
    u_parm.iir_type = TYPE_HIGHPASS;
    u_parm.filt_num = 2;
    u_parm.filt_tar_tab  = get_outval_addr(u_parm.filt_num);
    u_parm.gain = 0;
    audio_bfilt_cal_coeff(filt->hdl, &u_parm);//高音滤波器
    return filt;
}
void mic_high_bass_coeff_cal_uninit(BFILT_API_STRUCT *filt)
{
    local_irq_disable();
    if (filt && filt->hdl) {
        audio_bfilt_close(filt->hdl);
    }
    if (filt) {
        free(filt);
        filt = NULL;
    }
    local_irq_enable();
}

#if !MIC_EFFECT_EQ_EN
__attribute__((weak))int mic_eq_get_filter_info(void *eq, int sr, struct audio_eq_filter_info *info)
{
    log_info("mic_eq_get_filter_info \n");
    info->L_coeff = info->R_coeff = (void *)outval;
    info->L_gain = info->R_gain = 0;
    info->nsection = 3;
    return 0;
}
#endif

void *mic_eq_drc_open(u32 sample_rate, u8 ch_num)
{

#if TCFG_EQ_ENABLE

    log_i("sample_rate %d %d\n", sample_rate, ch_num);
    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

    effect_parm.eq_en = 1;

    if (effect_parm.eq_en) {

        effect_parm.async_en = 0;

        effect_parm.out_32bit = 0;
        effect_parm.online_en = 0;
        effect_parm.mode_en = 0;
    }

    effect_parm.eq_name = mic_eq_mode;

    effect_parm.ch_num = ch_num;
    effect_parm.sr = sample_rate;
    effect_parm.eq_cb = mic_eq_get_filter_info;

    eq_drc = audio_eq_drc_open(&effect_parm);

    clock_add(EQ_CLK);
    return eq_drc;
#else
    return NULL;
#endif//TCFG_EQ_ENABLE

}

void mic_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
    }
#endif
    return;
}




#if TCFG_MIC_DODGE_EN
void mic_e_det_handler(u8 event, u8 ch)
{
    //printf(">>>> ch:%d %s\n", ch, event ? ("MUTE") : ("UNMUTE"));

    struct __mic_effect *effect = (struct __mic_effect *)__this;
#if SYS_DIGVOL_GROUP_EN
    //printf("effect_dvol_default_parm.ch_total %d effect->dodge_en %d\n", effect_dvol_default_parm.ch_total, effect->dodge_en);
    if (ch == effect_dvol_default_parm.ch_total) {
        if (effect->dodge_en) {
            if (effect && effect->d_vol) {
                if (event) { //推出闪避
                    audio_dig_vol_group_dodge(sys_digvol_group, "mic_mic", 100, 100);
                } else { //启动闪避
                    audio_dig_vol_group_dodge(sys_digvol_group, "mic_mic", 100, 0);
                }
            }
        }
    }
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief    打开mic 能量检测
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void *mic_energy_detect_open(u32 sr, u8 ch_num)
{
    audio_energy_detect_param e_det_param = {0};
    e_det_param.mute_energy = dodge_parm.dodge_out_thread;//人声能量小于mute_energy 退出闪避
    e_det_param.unmute_energy = dodge_parm.dodge_in_thread;//人声能量大于 100触发闪避
    e_det_param.mute_time_ms = dodge_parm.dodge_out_time_ms;
    e_det_param.unmute_time_ms = dodge_parm.dodge_in_time_ms;
    e_det_param.count_cycle_ms = 2;
    e_det_param.sample_rate = sr;
    e_det_param.event_handler = mic_e_det_handler;
    e_det_param.ch_total = ch_num;
    e_det_param.dcc = 1;
    void *audio_e_det_hdl = audio_energy_detect_open(&e_det_param);
    return audio_e_det_hdl;
}
/*----------------------------------------------------------------------------*/
/**@brief    关闭mic 能量检测
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_energy_detect_close(void *hdl)
{
    if (hdl) {
        audio_stream_del_entry(audio_energy_detect_entry_get(hdl));
#if SYS_DIGVOL_GROUP_EN
        struct __mic_effect *effect = (struct __mic_effect *)__this;
        if (effect->d_vol) {
            audio_dig_vol_group_dodge(sys_digvol_group, "mic_mic", 100, 100);         // undodge
        }
#endif

        audio_energy_detect_close(hdl);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    能量检测运行过程，是否触发闪避
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mic_dodge_ctr(void)
{
    struct __mic_effect *effect = (struct __mic_effect *)__this;
    if (effect) {
        effect->dodge_en = !effect->dodge_en;
    }
}
u8 mic_dodge_get_status(void)
{
    struct __mic_effect *effect = (struct __mic_effect *)__this;
    if (effect) {
        return effect->dodge_en;
    }
    return 0;
}
#endif


//*********************音效切换例程**********************************//
//参考 音效只保留 echo混响和变声
enum {
    KARAOKE_MIC_OST,//原声,录音棚
    KARAOKE_MIC_KTV,//KTV
    KARAOKE_MIC_SHUSHU,//大叔
    KARAOKE_MIC_GODDESS,//女神
    KARAOKE_MIC_BABY,//娃娃音
    KARAOKE_MIC_MAX,
    /* KARAOKE_ONLY_MIC,//在不是mic模式下初始化mic */
};

//K歌耳机模式切换接口
void mic_mode_switch(u8 eff_mode)
{
#if TCFG_KARAOKE_EARPHONE
    if (!mic_effect_get_status()) {
        mic_effect_start();
        pause_echo(__this->p_echo_hdl, 1);
        return;
    }
    u32 sample_rate = 0;
    switch (eff_mode) {
    case KARAOKE_MIC_OST://原声,录音棚
        /* tone_play_index(IDEX_TONE_MIC_OST, 1); */
        puts("OST\n");
        // 音效直通
        pause_echo(__this->p_echo_hdl, 1);
        pause_pitch(__this->p_pitch_hdl, 1);
        if (__this->p_howling_hdl) {
            pause_howling(__this->p_howling_hdl, 1);
        }
        break;
    case KARAOKE_MIC_KTV://KTV
        puts("KTV\n");
        //变声直通
        pause_echo(__this->p_echo_hdl, 0);
        pause_pitch(__this->p_pitch_hdl, 1);
        break;
    case KARAOKE_MIC_SHUSHU://大叔
        /* tone_play_index(IDEX_TONE_UNCLE, 1); */
        puts("UNCLE\n");
        pause_pitch(__this->p_pitch_hdl, 1);
        set_pitch_para(136, sample_rate, EFFECT_PITCH_SHIFT, 0);
        pause_pitch(__this->p_pitch_hdl, 0);
        break;
    case KARAOKE_MIC_GODDESS://女神
        /* tone_play_index(IDEX_TONE_GODNESS, 1); */
        puts("GODDESS\n");
        pause_pitch(__this->p_pitch_hdl, 1);
        set_pitch_para(66, sample_rate, EFFECT_VOICECHANGE_KIN0, 150);
        pause_pitch(__this->p_pitch_hdl, 0);
        break;
    case KARAOKE_MIC_BABY://娃娃音
        /* tone_play_index(IDEX_TONE_BABY, 1); */
        puts("WAWA\n");
        pause_pitch(__this->p_pitch_hdl, 1);
        set_pitch_para(50, sample_rate, EFFECT_PITCH_SHIFT, 0);
        pause_pitch(__this->p_pitch_hdl, 0);
        break;
    default:
        puts("mic_ERROR\n");
        mic_effect_stop();
        break;
    }
#endif//TCFG_KARAOKE_EARPHONE
}


#endif//TCFG_MIC_EFFECT_ENABLE




