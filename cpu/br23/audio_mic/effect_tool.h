#ifndef __EFFECT_TOOL_H__
#define __EFFECT_TOOL_H__

#include "generic/typedef.h"
#include "effect_cfg.h"

int effect_tool_open(struct __effect_mode_cfg *parm, struct __tool_callback *cb);
void effect_tool_close(void);

#endif//__EFFECT_TOOL_H__
