
#ifndef __EFFECT_LINEIN_H__
#define __EFFECT_LINEIN_H__

#include "system/includes.h"
#include "app_config.h"
#include "media/includes.h"
#include "application/audio_vocal_tract_synthesis.h"

struct __effect_linein;

struct __effect_linein *effect_linein_open(void);
void effect_linein_close(struct __effect_linein **hdl);
struct audio_stream_entry *effect_linein_get_stream_entry(struct __effect_linein *hdl);

#endif//__EFFECT_LINEIN_H__

