#ifndef __MUSIC_NEW_H__
#define __MUSIC_NEW_H__

#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "media/audio_decoder.h"
#include "music_player.h"
#include "app_config.h"

#if (TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0)
#define MUSIC_DEV_ONLINE_START_AFTER_MOUNT_EN			0//如果是u盘和SD卡复用， 这里必须为0， 保证usb枚举的时候解码是停止的
#else
#define MUSIC_DEV_ONLINE_START_AFTER_MOUNT_EN			1
#endif

enum {
    MUSIC_TASK_START_BY_NORMAL = 0x0,
    MUSIC_TASK_START_BY_BREAKPOINT,
    MUSIC_TASK_START_BY_SCLUST,
    MUSIC_TASK_START_BY_NUMBER,
    MUSIC_TASK_START_BY_PATH,
};

void music_task_dev_online_start(void);
void music_task_set_parm(u8 type, int val);
void music_player_err_deal(int err);

#endif//__MUSIC_NEW_H__
