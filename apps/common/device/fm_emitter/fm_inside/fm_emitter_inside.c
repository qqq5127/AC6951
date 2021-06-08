#include "app_config.h"
#include "system/includes.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "fm_emitter_inside.h"

#if(TCFG_FM_EMITTER_INSIDE_ENABLE == ENABLE)

#define FM_EMITTER_INSIDE_USE_STEREO		0	// 1:fm 发射使用立体声  0:非立体声


//*****************************************************************
//695 1:pb11 超强驱动天线  0:pb10 普通天线
//696 1:pb3 超强驱动天线   0:pb1 普通天线,使用pb1需要需要改变唤醒IO
//*****************************************************************
#define FM_EMITTER_INSIDE_USE_CH    		1

static void fm_emitter_inside_init(u16 fre)
{
    printf("fm emitter inside init \n");
    extern void fm_emitter_init(void);
    extern void fm_emitter_set_freq(u16 fre);
    extern void fmtx_stereo_onoff(u8 onoff);
    extern void fm_emitter_set_ch(u8 ch);
    extern void fm_emitter_set_power(u8 level);
#if FM_EMITTER_INSIDE_USE_STEREO
    fmtx_stereo_onoff(1);
#endif
#if FM_EMITTER_INSIDE_USE_CH
    fm_emitter_set_ch(1);
    fm_emitter_init();
    fm_emitter_set_power(3);//功率等级0~3，最高发射功率为3级，在初始化之后设置,只用于强驱
#else
    fm_emitter_init();
#endif

    if (fre) {
        fm_emitter_set_freq(fre);
    }

}

static void fm_emitter_inside_start(void)
{
    extern void fmtx_on();
    fmtx_on();
}
static void fm_emitter_inside_stop(void)
{
    extern void fmtx_off();
    fmtx_off();
}

static void fm_emitter_inside_set_fre(u16 fre)
{
    extern void fm_emitter_set_freq(u16 fre);
    fm_emitter_set_freq(fre);
}

//**************************************************
//功率调节只适用于超强驱动,使用普通IO发射不调用这个函数
//等级0-3,3为最高输出功率，默认使用等级3最高功率
//功率设置放在初始化之后
//该功率调节为全频点功率调节，所以参数 fre 无效，填0即可
//**************************************************
static void fm_emitter_inside_set_power(u8 level, u16 fre)
{
    extern void fm_emitter_set_power(u8 level);
    fm_emitter_set_power(level);
}

static void fm_emitter_inside_set_data_cb(void *cb)
{
    extern void fm_emitter_set_data_cb(void *cb);
    fm_emitter_set_data_cb(cb);
}

REGISTER_FM_EMITTER(fm_emitter_inside) = {
    .name      = "fm_emitter_inside",
    .init      = fm_emitter_inside_init,
    .start     = fm_emitter_inside_start,
    .stop      = fm_emitter_inside_stop,
    .set_fre   = fm_emitter_inside_set_fre,
    .set_power = fm_emitter_inside_set_power,
    .data_cb   = fm_emitter_inside_set_data_cb,
};

#endif
