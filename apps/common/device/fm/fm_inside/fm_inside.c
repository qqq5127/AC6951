#include "app_config.h"
#include "system/includes.h"
#include "fm/fm_manage.h"
#include "asm/fm_inside_api.h"
#include "asm/dac.h"
#include "audio_config.h"
#include "media/audio_base.h"
#include "overlay_code.h"
#include "clock_cfg.h"
#include "fm_inside.h"


#if TCFG_APP_FM_EN
#if(TCFG_FM_INSIDE_ENABLE == ENABLE)


int fm_dec_open(u8 source, u32 sample_rate);
void fm_dec_close(void);
void fm_dig_mute(u8 mute);

///--------------------------FM_INSIDE_API------------------------
void fm_inside_init(void *priv)
{
    puts("fm_insice_init\n");

    overlay_load_code(OVERLAY_FM);

    clock_add_set(FM_INSIDE_CLK);

    /* fm_dec_open(0xff, 37500); */

    /* fm_inside_default_config(); */

    /* #if CONFIG_CLOCK_BY_TYPE
    	u32 fm_clk = matching_code_type(AUDIO_ANOTHER_CLK_TYPE_FM, 1);
        fm_inside_io_ctrl(SET_FM_INSIDE_SYS_CLK, fm_clk / 1000000L);
    #else
        fm_inside_io_ctrl(SET_FM_INSIDE_SYS_CLK, FM_CLOCK / 1000000L);
    #endif */

    fm_inside_mem_init();

    fm_inside_io_ctrl(SET_FM_INSIDE_AGC_PARAM, FMSCAN_AGC, FMSCAN_ADD_DIFFER, FMSCAN_P_DIFFER, FMSCAN_N_DIFFER);
    fm_inside_io_ctrl(SET_FM_INSIDE_COUNTRY_PARAM, REAL_FREQ_MIN, REAL_FREQ_MAX, FREQ_STEP);
    fm_inside_io_ctrl(SET_FM_INSIDE_SCAN_ARG1, FMSCAN_SEEK_CNT_MIN, FMSCAN_SEEK_CNT_MAX, FMSCAN_CNR, FM_IF);
    fm_inside_io_ctrl(SET_FM_INSIDE_SCAN_ARG2, FMSCAN_960_CNR, FMSCAN_1080_CNR);

    fm_inside_on();  //fm analog init
    fm_inside_set_stereo(0);  //set[0,128], 0 mono, 1~128 stereo.
}

bool fm_inside_set_fre(void *priv, u16 fre)
{
    u8 ret;
    u32 freq ;
    //fm_printf("[%d]", fre);
    freq = fre * 10;
    ret =  fm_inside_freq_set(freq);
    return ret;
}

bool fm_inside_read_id(void *priv)
{
    return (bool)fm_inside_id_read();
}

void fm_inside_powerdown(void *priv)
{
    fm_inside_off();
    /* dac_channel_off(FM_INSI_CHANNEL, FADE_ON); */
    fm_dec_close();
    clock_remove_set(FM_INSIDE_CLK);
}

void fm_inside_mute(void *priv, u8 flag)
{
    if (!flag) {
        /* fm_dec_open(0xff, 37500); */
    }

    if (flag) {
        fm_dig_mute(flag);
    }

    //fm_inside_int_set(!flag);

    if (!flag) {
        /* os_time_dly(3); */
        fm_dig_mute(flag);
    }

    if (flag) {
        /* fm_dec_close(); */
    }
}


REGISTER_FM(fm_inside) = {
    .logo    = "fm_inside",
    .init    = fm_inside_init,
    .close   = fm_inside_powerdown,
    .set_fre = fm_inside_set_fre,
    .mute    = fm_inside_mute,
    .read_id = fm_inside_read_id,
};


#endif // FM_INSIDE
#endif

