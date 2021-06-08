#include "effect_cfg.h"
#include "effect_tool.h"
#include "mic_effect.h"
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_online_cfg.h"
#include "asm/crc16.h"
/* #include "application/audio_eq.h" */
/* #include "reverb/reverb_api.h" */

/* #include "effects_config.h" */
/* #include "effects_default_config.h" */

#define LOG_TAG     "[APP-EFFECTS]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#if (TCFG_MIC_EFFECT_ENABLE)

static const u8 effect_tool_sdk_name[16] 			= "AC695X";
static const u8 effect_tool_ver[4] 			    	= {0, 4, 3, 0};
static const u8 effect_tool_password[16] 			= "000";

typedef struct {
    spinlock_t lock;
    u16 						effect_file_cfg	: 1;
    u16							revert			: 15;
#if TCFG_EQ_ENABLE
    int 						EQ_Coeff_table[(EFFECT_EQ_SECTION_MAX + 3) * 5];
#endif
    u16 						cur_mode_id;
    struct __effect_mode_cfg 	*mode_cfg;
    struct __effect_mode    	*mode;
    struct __tool_callback 		cb;
} EFFECT_CFG;

/*-----------------------------------------------------------*/
static EFFECT_CFG *p_effects_cfg = NULL;
#define __this		p_effects_cfg

extern int eq_seg_design(struct eq_seg_info *seg, int sample_rate, int *coeff);
extern void mic_eq_drc_update();
void cal_center_freq(float *tab, u8 total_section);


static void eq_coeff_set(int sr, u8 ch)
{
    int i;
    if (!sr) {
        sr = 44100;
        log_error("sr is zero");
    }

    struct __effect_mode *cur_mode = &__this->mode[__this->cur_mode_id];
    for (i = 0; i < cur_mode->seg_num; i++) {
        if (cur_mode->eq_design_mask & BIT(i)) {
            cur_mode->eq_design_mask &= ~BIT(i);
#if TCFG_EQ_ENABLE
            eq_seg_design(&cur_mode->parm.eq.val.seg[i], sr, &__this->EQ_Coeff_table[5 * i]);
#if 0
            int *tar_tab = __this->EQ_Coeff_table;
            /* int *tar_tab = EQ_Coeff_table; */
            log_d("i %d, cf0:%d, cf1:%d, cf2:%d, cf3:%d, cf4:%d ", i, tar_tab[5 * i]
                  , tar_tab[5 * i + 1]
                  , tar_tab[5 * i + 2]
                  , tar_tab[5 * i + 3]
                  , tar_tab[5 * i + 4]
                 );
#endif

#endif
        }
    }
}



#if MIC_EFFECT_EQ_EN
int *get_outval_addr(u8 mode)
{
    return &__this->EQ_Coeff_table[EFFECT_EQ_SECTION_MAX * 5 + mode * 5];
}
int mic_eq_get_filter_info(void *eq, int sr, struct audio_eq_filter_info *info)
{
    log_info("mic_eq_get_filter_info \n");
    int *coeff = NULL;
    ASSERT(__this);
    struct __effect_mode *cur_mode = &__this->mode[__this->cur_mode_id];
    if ((sr != __this->mode_cfg->sample_rate) || (cur_mode->parm.eq.update)) {
        spin_lock(&__this->lock);
        if (sr != __this->mode_cfg->sample_rate) {
            cur_mode->eq_design_mask = (u32) - 1;
        }
        eq_coeff_set(sr, 0);//算前五段eq
        __this->mode_cfg->sample_rate = sr;
        cur_mode->parm.eq.update = 0;

        spin_unlock(&__this->lock);
    }

    /* for (int i = 0; i < mic_effect_eq_section_num(); i++) { */
    /* int *tar_tab = __this->EQ_Coeff_table; */
    /* log_d("i %d, cf0:%d, cf1:%d, cf2:%d, cf3:%d, cf4:%d ", i, tar_tab[5 * i] */
    /* , tar_tab[5 * i + 1] */
    /* , tar_tab[5 * i + 2] */
    /* , tar_tab[5 * i + 3] */
    /* , tar_tab[5 * i + 4] */
    /* ); */
    /* } */

    coeff = __this->EQ_Coeff_table;
    info->L_coeff = info->R_coeff = (void *)coeff;
    info->L_gain = info->R_gain = cur_mode->global_gain;
    info->nsection = mic_effect_eq_section_num();
    return 0;
}
/*
struct eq_seg_info {
    u16 index;
    u16 iir_type; ///<EQ_IIR_TYPE
    int freq;     //
    int gain;    //gain*(1<<20)  float gain(-12~12)
    int q;       //q*(1<<24)    float q(0.3~30)
};
*/
/*----------------------------------------------------------------------------*/
/**@brief    mic eq设置接口
   @param   详见struct eq_seg_info结构体
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

int mic_eq_set_info(struct eq_seg_info *info)
{
    struct __effect_mode *cur_mode = &__this->mode[__this->cur_mode_id];
    if (!cur_mode) {
        return -1;
    }

    if (info) {
        u16 index = info->index;
        if (index > EFFECT_EQ_SECTION_MAX) {
            log_e("index err %d\n", index);
            return -1;
        }
        memcpy(&cur_mode->parm.eq.val.seg[index], info, sizeof(struct eq_seg_info));
        cur_mode->eq_design_mask |= BIT(index);
        cur_mode->parm.eq.update = 1;
        mic_eq_drc_update();
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    mic eq信息获取接口
   @param   index:获取第几段eq
   @return  返回struct eq_seg_info结构体
   @note
*/
/*----------------------------------------------------------------------------*/
struct eq_seg_info *mic_eq_get_info(u16 index)
{
    struct __effect_mode *cur_mode = &__this->mode[__this->cur_mode_id];
    if (!cur_mode) {
        return NULL;
    }
    if (index > EFFECT_EQ_SECTION_MAX) {
        log_e("index err %d\n", index);
        return NULL;
    }
    return (struct eq_seg_info *)&cur_mode->parm.eq.val.seg[index];
}


#endif

////根据命令设置参数
static int effect_cfg_update_parm(void *priv, u16 mode_index, u16 cmd_index, u8 *data, u32 len, u8 online)
{
    __this = (EFFECT_CFG *)priv;
    struct __effect_mode *cur_mode = &__this->mode[mode_index];

    log_i("mode = %d,cmd = %x\n", mode_index, cmd_index);
    switch (cmd_index) {
    case EFFECTS_CMD_REVERB:
        memcpy(&cur_mode->parm.reverb.val, data, len);
        cur_mode->parm.reverb.flag = 1;
        cur_mode->parm.reverb.update = 1;
        mic_effect_reverb_parm_fill(&cur_mode->parm.reverb.val, 1, online);
        break;
    case EFFECTS_CMD_PITCH2:
        memcpy(&cur_mode->parm.pitch.val, data, len);
        cur_mode->parm.pitch.flag = 1;
        cur_mode->parm.pitch.update = 1;
        mic_effect_pitch_parm_fill(&cur_mode->parm.pitch.val, 1, online);
        break;
    case EFFECTS_CMD_ECHO:
        memcpy(&cur_mode->parm.ehco.val, data, len);
        cur_mode->parm.ehco.flag = 1;
        cur_mode->parm.ehco.update = 1;
        mic_effect_echo_parm_fill(&cur_mode->parm.ehco.val, 1, online);
        break;
    case EFFECTS_CMD_NOISE:
        memcpy(&cur_mode->parm.noise.val, data, len);
        cur_mode->parm.noise.flag = 1;
        cur_mode->parm.noise.update = 1;
        mic_effect_noisegate_parm_fill(&cur_mode->parm.noise.val, 1, online);
        break;
    case EFFECTS_CMD_SHOUT_WHEAT:
        memcpy(&cur_mode->parm.shout_wheat.val, data, len);
        cur_mode->parm.shout_wheat.flag = 1;
        cur_mode->parm.shout_wheat.update = 1;
        mic_effect_shout_wheat_parm_fill(&cur_mode->parm.shout_wheat.val, 1, online);
        break;
    case EFFECTS_CMD_LOW_SOUND:
        memcpy(&cur_mode->parm.low_sound.val, data, len);
        cur_mode->parm.low_sound.flag = 1;
        cur_mode->parm.low_sound.update = 1;
        mic_effect_low_sound_parm_fill(&cur_mode->parm.low_sound.val, 1, online);
        break;
    case EFFECTS_CMD_HIGH_SOUND:
        memcpy(&cur_mode->parm.high_sound.val, data, len);
        cur_mode->parm.high_sound.flag = 1;
        cur_mode->parm.high_sound.update = 1;
        mic_effect_high_sound_parm_fill(&cur_mode->parm.high_sound.val, 1, online);
        break;
    case EFFECTS_EQ_ONLINE_CMD_PARAMETER_SEG:
#if TCFG_EQ_ENABLE
        if (online) {
            struct eq_seg_info seg;
            memcpy(&seg, data, sizeof(struct eq_seg_info));
            if (seg.index == (u16) - 1) {
                memcpy(&cur_mode->global_gain, &seg.iir_type, sizeof(float));
            }
            /* log_i("cur_mode->global_gain %x\n", *(int *)&cur_mode->global_gain); */
            if (seg.index >= EFFECT_EQ_SECTION_MAX) {
                if (seg.index != (u16) - 1) {
                    log_e("eq seg inex err!!\n");
                } else {
                    cur_mode->parm.eq.update = 1;
                    mic_eq_drc_update();
                }
                break;
            }
            memcpy(&cur_mode->parm.eq.val.seg[seg.index], &seg, sizeof(struct eq_seg_info));
            cur_mode->eq_design_mask |= BIT(seg.index);
            cur_mode->parm.eq.update = 1;
            mic_eq_drc_update();
            log_info("idx:%d, iir:%d, frq:%d, gain:%d, q:%d \n", seg.index, seg.iir_type, seg.freq, 10 * seg.gain / (1 << 20), 10 * seg.q / (1 << 24));

        } else {
            memcpy(&cur_mode->parm.eq.val, data, sizeof(EFFECTS_EQ_PARM_SET));
            cur_mode->parm.eq.flag = 1;
            cur_mode->global_gain = cur_mode->parm.eq.val.global_gain;
            cur_mode->seg_num = cur_mode->parm.eq.val.seg_num;
            cur_mode->eq_design_mask = (u32) - 1;
            eq_coeff_set(__this->mode_cfg->sample_rate, 0);
            cur_mode->parm.eq.update = 1;
        }
#endif
        break;
    case EFFECTS_CMD_MIC_ANALOG_GAIN:
        memcpy(&cur_mode->parm.mic_gain.val, data, len);
        cur_mode->parm.mic_gain.flag = 1;
        cur_mode->parm.mic_gain.update = 1;
        mic_effect_mic_gain_parm_fill(&cur_mode->parm.mic_gain.val, 1, online);
        break;
    default:
        return -1;
        break;
    }
    return 0;
}
////更新模式参数
static void effect_cfg_fill_mode_parm(u16 mode, u8 online)
{
    if (__this == NULL) {
        return ;
    }
    if (mode >= __this->mode_cfg->mode_max) {
        log_e("effect mode overlimit, %d, mode max = %d\n", mode, __this->mode_cfg->mode_max);
        return ;
    }
    log_i("mic_effect_change_mode %d\n", mode);
    __this->cur_mode_id = mode;
    struct __effect_mode *cur_mode = &__this->mode[__this->cur_mode_id];
    ///根据模式参数配置来更新参数
    //<更新reverb参数
    if (cur_mode->parm.reverb.flag) {
        mic_effect_reverb_parm_fill(&cur_mode->parm.reverb.val, 1, online);
    } else {
        mic_effect_reverb_parm_fill(NULL, 1, online);
    }

    //<更新pitch参数
    if (cur_mode->parm.pitch.flag) {
        mic_effect_pitch_parm_fill(&cur_mode->parm.pitch.val, 1, online);
    } else {
        mic_effect_pitch_parm_fill(NULL, 1, online);
    }

    //<更新echo参数
    if (cur_mode->parm.ehco.flag) {
        mic_effect_echo_parm_fill(&cur_mode->parm.ehco.val, 1, online);
    } else {
        mic_effect_echo_parm_fill(NULL, 1, online);
    }

    //<更新噪声门限参数
    if (cur_mode->parm.noise.flag) {
        mic_effect_noisegate_parm_fill(&cur_mode->parm.noise.val, 1, online);
    } else {
        mic_effect_noisegate_parm_fill(NULL, 1, online);
    }

    //<更新喊麦参数
    if (cur_mode->parm.shout_wheat.flag) {
        mic_effect_shout_wheat_parm_fill(&cur_mode->parm.shout_wheat.val, 1, online);
    } else {
        mic_effect_shout_wheat_parm_fill(NULL, 1, online);
    }

    //<更新低音参数
    if (cur_mode->parm.low_sound.flag) {
        mic_effect_low_sound_parm_fill(&cur_mode->parm.low_sound.val, 1, online);
    } else {
        mic_effect_low_sound_parm_fill(NULL, 1, online);
    }

    //<更新高音参数
    if (cur_mode->parm.high_sound.flag) {
        mic_effect_high_sound_parm_fill(&cur_mode->parm.high_sound.val, 1, online);
    } else {
        mic_effect_high_sound_parm_fill(NULL, 1, online);
    }


    //<更新eq参数
    if (cur_mode->parm.eq.flag) {

    } else {

    }

    //<更新mic模拟增益参数
    if (cur_mode->parm.mic_gain.flag) {
        mic_effect_mic_gain_parm_fill(&cur_mode->parm.mic_gain.val, 1, online);
    } else {
        mic_effect_mic_gain_parm_fill(NULL, 1, online);
    }
}



static s32 effect_cfg_get_file_info(void)
{
    unsigned short mode_seq;
    unsigned short mode_len;
    u16 cur_mode_id = 0;
    u8 mode_cnt = 0;
    unsigned short magic;
    int ret = 0;
    int read_size;
    FILE *file = NULL;
    u8 *file_data = NULL;
    u8 *head_buf = NULL;
    if (__this == NULL) {
        return  -EINVAL;
    }
    file = fopen(__this->mode_cfg->file_path, "r");
    if (file == NULL) {
        log_error("effects file open err\n");
        return  -ENOENT;
    }
    log_info("effects file open ok \n");

    // effects ver
    u8 fmt = 0;
    if (1 != fread(file, &fmt, 1)) {
        ret = -EIO;
        goto err_exit;
    }
    u8 ver[4] = {0};
    if (4 != fread(file, ver, 4)) {
        ret = -EIO;
        goto err_exit;
    }
    if (memcmp(ver, effect_tool_ver, sizeof(effect_tool_ver))) {
        log_info("effects ver err \n");
        log_info_hexdump(ver, 4);
        fseek(file, 0, SEEK_SET);
    }

__next_mode:
    if (sizeof(unsigned short) != fread(file, &mode_seq, sizeof(unsigned short))) {
        ret = 0;
        goto err_exit;
    }
    if (sizeof(unsigned short) != fread(file, &mode_len, sizeof(unsigned short))) {
        ret = 0;
        goto err_exit;
    }
    int mode_start = fpos(file);
    cur_mode_id = MODE_SEQ_TO_INDEX(mode_seq);
    if (cur_mode_id >= __this->mode_cfg->mode_max) {
        if (cur_mode_id != 0xdfff) {
            log_info("effect mode index err!!!\n");
            ret = -EINVAL;
        } else {
            ret = 0;
        }
        goto err_exit;
    }
    log_info("effect mode index = %d\n", cur_mode_id);
    while (1) {
        //read crc
        if (sizeof(unsigned short) != fread(file, &magic, sizeof(unsigned short))) {
            ret = 0;
            break;
        }

        int pos = fpos(file);
        //read id len
        EFFECTS_FILE_HEAD *effect_file_h;
        int head_size = sizeof(EFFECTS_FILE_HEAD);
        head_buf = malloc(head_size);
        if (sizeof(EFFECTS_FILE_HEAD) != fread(file, head_buf, sizeof(EFFECTS_FILE_HEAD))) {
            ret = -EIO;
            break;
        }
        effect_file_h = (EFFECTS_FILE_HEAD *)head_buf;
        if ((effect_file_h->id >= EFFECTS_CMD_REVERB) && (effect_file_h->id < EFFECTS_CMD_MAX)) {
            //reread id len  and read data
            fseek(file, pos, SEEK_SET);
            int data_size = effect_file_h->len + sizeof(EFFECTS_FILE_HEAD);
            file_data = malloc(data_size);
            if (file_data == NULL) {
                ret = -ENOMEM;
                break;
            }
            if (data_size != fread(file, file_data, data_size)) {
                ret = -EIO;
                break;
            }
            //compare crc
            if (magic == CRC16(file_data, data_size)) {
                u8 *parm = file_data + head_size;
                int parm_size = data_size - head_size;
                spin_lock(&__this->lock);
                log_info("effect_file_h->id %x, %d\n", effect_file_h->id, cur_mode_id);
                effect_cfg_update_parm(__this, cur_mode_id, effect_file_h->id, parm, parm_size, 0);
                spin_unlock(&__this->lock);
            } else {
                log_error("effects_cfg_info crc err\n");
                ret = -ENOEXEC;
            }
            free(head_buf);
            head_buf = NULL;
            free(file_data);
            file_data = NULL;
        }
        int mode_end = fpos(file);
        if ((mode_end - mode_start) == mode_len) {
            goto __next_mode;
        }
    }
err_exit:
    if (head_buf) {
        free(head_buf);
        head_buf = NULL;
    }
    if (file_data) {
        free(file_data);
        file_data = NULL;
    }
    fclose(file);
    if (ret == 0) {
        log_info("effects cfg_info ok \n");
    }
    return ret;
}


static void effect_tool_change_mode(u16 mode)
{
    log_i("effect_tool_change_mode = %d\n", mode);
    effect_cfg_fill_mode_parm(mode, 1);
}

static void effect_eq_default_init()
{
#if TCFG_EQ_ENABLE
    struct eq_seg_info eq_tab_normal[] = {
        {0, EQ_IIR_TYPE_BAND_PASS, 31,    0 << 20, (int)(0.7f * (1 << 24))}
    };

    for (int i = 0; i < __this->mode_cfg->mode_max; i++) {
        struct __effect_mode *cur_mode = &__this->mode[i];
        cur_mode->seg_num = EFFECT_EQ_SECTION_MAX;
        cur_mode->global_gain = 0;
        float tab[EFFECT_EQ_SECTION_MAX];
        cal_center_freq(tab, cur_mode->seg_num);//自定义系数表的中心频率重计算
        for (int j = 0; j < cur_mode->seg_num; j++) {
            eq_tab_normal[0].freq = (int)tab[j];
            memcpy(&cur_mode->parm.eq.val.seg[j],
                   &eq_tab_normal[0],
                   sizeof(struct eq_seg_info));
        }
        cur_mode->eq_design_mask = (u32) - 1;
    }
#endif
}
bool effect_cfg_open(struct __effect_mode_cfg *parm)
{
    if (__this) {
        return true;
    }
    if (parm == NULL) {
        return false;
    }
    if (parm->attr == NULL) {
        return false;
    }
    if (parm->mode_max == 0) {
        return false;
    }

    __this = zalloc(sizeof(EFFECT_CFG));
    __this->mode = (struct __effect_mode *)zalloc(parm->mode_max * sizeof(struct __effect_mode));
    __this->mode_cfg = parm;
    effect_eq_default_init();

    spin_lock_init(&__this->lock);
    __this->effect_file_cfg = 1;
    if (effect_cfg_get_file_info()) { //获取EFFECTS文件失败
        __this->effect_file_cfg = 0;
        effect_eq_default_init();
    }

    __this->cb.priv = __this;
    __this->cb.cmd_func = effect_cfg_update_parm;
    __this->cb.change_mode = effect_tool_change_mode;
#if TCFG_MIC_EFFECT_ONLINE_ENABLE
    effect_tool_open((struct __effect_mode_cfg *)parm, &__this->cb);
#endif
    return true;
}

void effect_cfg_close(void)
{
    if (!__this) {
        return ;
    }
    effect_tool_close();
    void *ptr = __this;
    __this = NULL;
    free(ptr);
}

int effect_cfg_init(void)
{
    effect_cfg_open(&effect_tool_parm);
    return 0;
}
__initcall(effect_cfg_init);


void effect_cfg_change_mode(u16 mode)
{
    if (__this == NULL) {
        return ;
    }
    if (__this->effect_file_cfg == 0) {
        log_e("no effects_cfg file, change mode fail!\n");
        return ;
    }
    effect_cfg_fill_mode_parm(mode, 0);
}

u16 effect_cfg_get_cur_mode(void)
{
    return (__this ? __this->cur_mode_id : 0);
}

#endif//TCFG_MIC_EFFECT_ENABLE

