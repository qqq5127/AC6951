#include "app_config.h"
#include "system/includes.h"
#include "fm/fm_manage.h"
#include "rda5807/RDA5807.h"
#include "bk1080/BK1080.h"
#include "qn8035/QN8035.h"
#include "fm_inside/fm_inside.h"
#include "asm/audio_linein.h"
#include "audio_config.h"
#include "audio_dec/audio_dec_fm.h"
#if TCFG_APP_FM_EN


#define LOG_TAG_CONST       APP_FM
#define LOG_TAG             "[APP_FM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static FM_INTERFACE *t_fm_hdl = NULL;
static FM_INTERFACE *fm_hdl = NULL;
static struct _fm_dev_info *fm_dev_info;
static struct _fm_dev_platform_data *fm_dev_platform_data;
int linein_dec_open(u8 source, u32 sample_rate);
void linein_dec_close(void);

int fm_dev_init(void *_data)
{
    fm_dev_platform_data = (struct _fm_dev_platform_data *)_data;
    return 0;

    fm_dev_info = (struct _fm_dev_info *)malloc(sizeof(struct _fm_dev_info));
    if (fm_dev_info == NULL) {
        printf("fm_dev_init info malloc err!");
        return -1;
    }

    fm_dev_platform_data = (struct _fm_dev_platform_data *)_data;
    memset(fm_dev_info, 0x00, sizeof(struct _fm_dev_info));
    fm_dev_info->iic_hdl = fm_dev_platform_data->iic_hdl;
    fm_dev_info->iic_delay = fm_dev_platform_data->iic_delay;

    /* iic_init(fm_dev_info->iic_hdl);

    extern void bk1080_test(void);
    bk1080_test(); */

    fm_manage_check_online();
    if (!fm_hdl) {
        /* printf("fm_manage could not find dev\n"); */
        return -1;
    } else {
        extern void wdt_clear();
        wdt_clear();
        memcpy(fm_dev_info->logo, fm_hdl->logo, strlen(fm_hdl->logo));
        /* printf("fm_manage found dev %s\n", fm_dev_info->logo); */

        /* fm_manage_init(); */
        /* fm_manage_set_fre(875); */
        /* fm_manage_mute(0); */
        /* u16 iii = 875; */
        /* while(!fm_manage_set_fre(iii) && (iii < 1080)) { */
        /* printf("fm fre no %d\n", iii); */
        /* iii++; */
        /* os_time_dly(1); */
        /* } */
        /* printf("fm fre %d\n", iii); */
        /* while(1) { */
        /* os_time_dly(1); */
        /* } */
    }

    return 0;
}

u8 fm_dev_iic_write(u8 w_chip_id, u8 register_address, u8 *buf, u32 data_len)
{
    u8 ret = 1;
    iic_start(fm_dev_info->iic_hdl);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, w_chip_id)) {
        ret = 0;
        log_e("\n fm iic wr err 0\n");
        goto __gcend;
    }

    delay(fm_dev_info->iic_delay);

    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, register_address)) {
        ret = 0;
        log_e("\n fm iic wr err 1\n");
        goto __gcend;
    }
    u8 *pbuf = buf;

    while (data_len--) {
        delay(fm_dev_info->iic_delay);

        if (0 == iic_tx_byte(fm_dev_info->iic_hdl, *pbuf++)) {
            ret = 0;
            log_e("\n fm iic wr err 2\n");
            goto __gcend;
        }
    }

__gcend:
    iic_stop(fm_dev_info->iic_hdl);

    return ret;
}

u8 fm_dev_iic_readn(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len)
{
    u8 read_len = 0;

    iic_start(fm_dev_info->iic_hdl);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, (r_chip_id & 0x01) ? (r_chip_id - 1) : (r_chip_id))) {
        log_e("\n fm iic rd err 0\n");
        read_len = 0;
        goto __gdend;
    }


    delay(fm_dev_info->iic_delay);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, register_address)) {
        log_e("\n fm iic rd err 1\n");
        read_len = 0;
        goto __gdend;
    }

    delay(fm_dev_info->iic_delay);
    iic_start(fm_dev_info->iic_hdl);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, r_chip_id)) {
        log_e("\n fm iic rd err 2\n");
        read_len = 0;
        goto __gdend;
    }

    delay(fm_dev_info->iic_delay);

    for (; data_len > 1; data_len--) {
        *buf++ = iic_rx_byte(fm_dev_info->iic_hdl, 1);
        read_len ++;
    }

    *buf = iic_rx_byte(fm_dev_info->iic_hdl, 0);

__gdend:

    iic_stop(fm_dev_info->iic_hdl);
    delay(fm_dev_info->iic_delay);

    return read_len;
}


void fm_manage_check_online(void)
{
    /* printf("fm check online dev \n"); */
    /* printf("addr %x %x\n", fm_dev_begin, fm_dev_end); */
    ////先找外挂fm
    list_for_each_fm(t_fm_hdl) {
        /* printf("find %x %x %s\n", t_fm_hdl, t_fm_hdl->read_id((void *)fm_dev_info), t_fm_hdl->logo); */
        if (t_fm_hdl->read_id((void *)fm_dev_info) &&
            (memcmp(t_fm_hdl->logo, "fm_inside", strlen(t_fm_hdl->logo)))) {
            fm_hdl = t_fm_hdl;
            printf("fm find dev %s \n", t_fm_hdl->logo);
            return;
        }
    }
    /* printf("no ex fm dev !!!\n"); */

    list_for_each_fm(t_fm_hdl) {
        /* printf("# find %x %x %s\n", t_fm_hdl, t_fm_hdl->read_id((void *)fm_dev_info), t_fm_hdl->logo); */
        if (t_fm_hdl->read_id((void *)fm_dev_info) &&
            (!memcmp(t_fm_hdl->logo, "fm_inside", strlen(t_fm_hdl->logo)))) {
            fm_hdl = t_fm_hdl;
            printf("fm find dev %s \n", t_fm_hdl->logo);
            return;
        }
    }
}
/* early_initcall(fm_manage_check_online); */

int fm_manage_init()
{
    fm_dev_info = (struct _fm_dev_info *)malloc(sizeof(struct _fm_dev_info));
    if (fm_dev_info == NULL) {
        printf("fm_dev_init info malloc err!");
        return -1;
    }

    memset(fm_dev_info, 0x00, sizeof(struct _fm_dev_info));
    fm_dev_info->iic_hdl = fm_dev_platform_data->iic_hdl;
    fm_dev_info->iic_delay = fm_dev_platform_data->iic_delay;

    fm_manage_check_online();
    if (!fm_hdl) {
        printf("fm_manage could not find dev\n");
        return -1;
    } else {
        memcpy(fm_dev_info->logo, fm_hdl->logo, strlen(fm_hdl->logo));
        printf("fm_manage found dev %s\n", fm_dev_info->logo);
    }

    if (fm_hdl) {
        fm_hdl->init((void *)fm_dev_info);
    }

    return 0;
}



int fm_manage_start()
{

    if (!fm_hdl || !fm_dev_info) {
        printf("fm_manage could not find dev\n");
        return -1;
    }
    if (memcmp(fm_dev_info->logo, "fm_inside", strlen(fm_dev_info->logo))) {//不开启内部收音开启linein
#if (TCFG_FM_INPUT_WAY != LINEIN_INPUT_WAY_ADC)
        if (!app_audio_get_volume(APP_AUDIO_STATE_MUSIC)) {
            audio_linein_mute(1);    //模拟输出时候，dac为0也有数据
        }
#endif
#if (TCFG_FM_INPUT_WAY == LINEIN_INPUT_WAY_ADC)
        linein_dec_open(TCFG_FMIN_LR_CH, 44100);
#else
        if (TCFG_FMIN_LR_CH & (BIT(0) | BIT(1))) {
            audio_linein0_open(TCFG_FMIN_LR_CH, 1);
        } else if (TCFG_FMIN_LR_CH & (BIT(2) | BIT(3))) {
            audio_linein1_open(TCFG_FMIN_LR_CH, 1);
        } else if (TCFG_FMIN_LR_CH & (BIT(4) | BIT(5))) {
            audio_linein2_open(TCFG_FMIN_LR_CH, 1);
        }
        audio_linein_gain(1);   // high gain
#endif
#if (TCFG_FM_INPUT_WAY != LINEIN_INPUT_WAY_ADC)
        if (app_audio_get_volume(APP_AUDIO_STATE_MUSIC)) {
            audio_linein_mute(0);
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_audio_get_volume(APP_AUDIO_STATE_MUSIC), 1);//防止无法调整
        }
        //模拟输出时候，dac为0也有数据
#endif

    } else {
        fm_dec_open(0xff, 37500);//打开内置收音解码通道
    }
    return 0;
}

static u16 cur_fmrx_freq = 0;
bool fm_manage_set_fre(u16 fre)
{
    if (fm_hdl) {
        cur_fmrx_freq = fre;
        return fm_hdl->set_fre((void *)fm_dev_info, fre);
    }
    return 0;
}

u16 fm_manage_get_fre()
{
    if (fm_hdl) {
        return  cur_fmrx_freq;
    }
    return 0;
}



void fm_manage_close(void)
{
    if (fm_dev_info != NULL) {
        free(fm_dev_info);
        fm_dev_info = NULL;
    }

    if (fm_hdl) {
        fm_hdl->close((void *)fm_dev_info);
        fm_hdl = NULL;
    }

    if (t_fm_hdl && memcmp(t_fm_hdl->logo, "fm_inside", strlen(t_fm_hdl->logo))) { //不使用内部收音开启linein
#if (TCFG_FM_INPUT_WAY == LINEIN_INPUT_WAY_ADC)
        linein_dec_close();
#else
        if (TCFG_FMIN_LR_CH & (BIT(0) | BIT(1))) {
            audio_linein0_close(TCFG_FMIN_LR_CH, 0);
        } else if (TCFG_FMIN_LR_CH & (BIT(2) | BIT(3))) {
            audio_linein1_close(TCFG_FMIN_LR_CH, 0);
        } else if (TCFG_FMIN_LR_CH & (BIT(4) | BIT(5))) {
            audio_linein2_close(TCFG_FMIN_LR_CH, 0);
        }
#endif

    }
}


void fm_manage_mute(u8 mute)
{
    if (fm_hdl) {
        fm_hdl->mute((void *)fm_dev_info, mute);
    }
}

#endif
