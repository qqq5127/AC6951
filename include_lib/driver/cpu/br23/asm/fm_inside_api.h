#ifndef __FM_INSIDE_API_H_
#define __FM_INSIDE_API_H_
#include "typedef.h"

enum {
    SET_FM_INSIDE_SCAN_ARG1,
    SET_FM_INSIDE_SCAN_ARG2,
    SET_FM_INSIDE_PRINTF,
    SET_FM_INSIDE_SYS_CLK,
    SET_FM_INSIDE_AGC_PARAM,
    SET_FM_INSIDE_COUNTRY_PARAM,
};
void fm_inside_io_ctrl(int ctrl, ...);

void fm_inside_default_config(void);//默认配置参数
void fm_inside_mem_init(void);
void fm_inside_on(void);//内置收音初始化
void fm_inside_off(void);//关闭内置收音
u8 fm_inside_freq_set(u32 freq);//设置频点 87.5M 填 87500
void fm_inside_int_set(u8 mute);//中断开关
u16 fm_inside_id_read(void);//获取内置收音的ID
s8 fm_inside_rssi_read(void); //获取电台的功率 -96 ~ 0 dB
u8 fm_inside_st_read(void);//获取电台的声道 1：立体声;  0: 单声道;
s8 fm_inside_cnr_read(void);//获取当前频点的载噪比  dB
void fm_inside_set_stereo(u8 set);  //set[0,128], 0 mono, 128 stereo。0~128对应0~30dB分离度
void fm_inside_set_abw(u8 set);  //audio bandwidth set //set[0,127] <=>2k~16k
void fm_inside_deempasis_set(u8 set);// set[0/1], 0:50us, 1:75us
#endif

