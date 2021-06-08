#ifndef __GENERAL_PLAYER_H__
#define __GENERAL_PLAYER_H__

#include "system/app_core.h"
#include "system/includes.h"
#include "system/event.h"
#include "dev_manager.h"

///解码错误码表
enum {
    GENERAL_PLAYER_ERR_NULL = 0x0,		///没有错误， 不需要做任何
    GENERAL_PLAYER_SUCC	  = 0x1,		///播放成功， 可以做相关显示、断点记忆等处理
    GENERAL_PLAYER_ERR_PARM,	    	///参数错误
    GENERAL_PLAYER_ERR_DECODE_FAIL,		///解码器启动失败错误
    GENERAL_PLAYER_ERR_NO_RAM,			///没有ram空间错误
    GENERAL_PLAYER_ERR_DEV_NOFOUND,		///没有找到指定设备
    GENERAL_PLAYER_ERR_DEV_OFFLINE,		///设备不在线错误
    GENERAL_PLAYER_ERR_FSCAN,			///设备扫描失败
    GENERAL_PLAYER_ERR_FILE_NOFOUND,	///没有找到文件
};

int general_player_init(struct __scan_callback *scan_cb);
void general_player_stop(u8 fsn_release);
int general_play_by_sculst(char *logo, u32 sclust);
void general_player_exit();
void general_player_test();
int general_player_scandisk_break(void);
#endif
