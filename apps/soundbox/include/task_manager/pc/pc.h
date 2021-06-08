#ifndef _PC_H_
#define _PC_H_

#include "system/event.h"
#include "music/music_player.h"




int pc_device_event_handler(struct sys_event *event);

void app_pc_tone_play_start(u8 mix);


#endif
