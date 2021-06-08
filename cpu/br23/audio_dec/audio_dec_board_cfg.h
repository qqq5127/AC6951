#ifndef _AUDIO_DEC_BOARD_CONFIG_H_
#define _AUDIO_DEC_BOARD_CONFIG_H_

#include "system/includes.h"
#include "app_config.h"

struct dec_board_param
{
	const char *name;
	u8	d_vol_en;
	u16 d_vol_fade_step;
	u8	(*d_vol_max)(void);		
	u8	(*d_vol_cur)(void);		
	u8  voice_cut_en;//人声消除 使能
	///other dec param...
};

#define REGISTER_DEC_BOARD_PARAM(p) \
    struct dec_board_param p sec(.dec_board_param_mem)

extern struct dec_board_param dec_board_param_mem_begin[];
extern struct dec_board_param dec_board_param_mem_end[];

#define list_for_each_dec_board_param(p) \
    for (p = dec_board_param_mem_begin; p < dec_board_param_mem_end; p++)

#endif//_AUDIO_DEC_BOARD_CONFIG_H_

