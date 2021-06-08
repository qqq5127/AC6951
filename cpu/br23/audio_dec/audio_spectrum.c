#include "audio_spectrum.h"


#if AUDIO_SPECTRUM_CONFIG
/*----------------------------------------------------------------------------*/
/**@brief   频响输出例子
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void spectrum_get_demo(void *p)
{
    spectrum_fft_hdl *hdl = p;
    if (hdl) {
        u8 db_num = audio_spectrum_fft_get_num(hdl);//获取频谱个数
        short *db_data = audio_spectrum_fft_get_val(hdl);//获取存储频谱值得地址
        if (!db_data) {
            return;
        }
        for (int i = 0; i < db_num; i++) {
            //输出db_num个 db值
            printf("db_data db[%d] %d\n", i, db_data[i]);
        }
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   打开频响统计
   @param   sr:采样率
   @return  hdl:句柄
   @note
*/
/*----------------------------------------------------------------------------*/
spectrum_fft_hdl *spectrum_open_demo(u32 sr, u8 channel)
{
    spectrum_fft_hdl *hdl = NULL;
    spectrum_fft_open_parm parm = {0};
    parm.sr = sr;
    parm.channel = channel;
    parm.attackFactor = 0.9;
    parm.releaseFactor = 0.9;
    parm.mode = 2;
    hdl = audio_spectrum_fft_open(&parm);
    /* int ret = sys_timer_add(hdl, spectrum_get_demo, 500);//频谱值获取测试 */
    clock_add(SPECTRUM_CLK);
    return hdl;
}

/*----------------------------------------------------------------------------*/
/**@brief   关闭频响统计
   @param  hdl:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void spectrum_close_demo(spectrum_fft_hdl *hdl)
{
    audio_spectrum_fft_close(hdl);
    clock_remove(SPECTRUM_CLK);
}


spectrum_fft_hdl *spec_hdl;
/*----------------------------------------------------------------------------*/
/**@brief  切换频响计算
   @param  en:0 不做频响计算， 1 使能频响计算（通话模式，需关闭频响计算）
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void spectrum_switch_demo(u8 en)
{
    if (spec_hdl) {
        audio_spectrum_fft_switch(spec_hdl, en);
    }
}


#else
void spectrum_switch_demo(u8 en)
{
}
#endif
