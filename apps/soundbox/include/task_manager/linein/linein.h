#ifndef _LINEIN_H_
#define _LINEIN_H_

#include "system/event.h"

int linein_device_event_handler(struct sys_event *event);
void app_linein_tone_play_start(u8 mix);

int  linein_start(void);
void linein_stop(void);
void linein_key_vol_up();
void linein_key_vol_down();
u8   linein_get_status(void);
int  linein_volume_pp(void);






#endif

