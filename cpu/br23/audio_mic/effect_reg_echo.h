#ifndef __EFFECT_REG_ECHO_H__
#define __EFFECT_REG_ECHO_H__

#include "effect_cfg.h"

enum {
    EFFECT_ECHO_MODE_NORMAL = 0x0,
    EFFECT_ECHO_MODE_BOY_TO_GIRL,
    EFFECT_ECHO_MODE_GIRL_TO_BOY,
    EFFECT_ECHO_MODE_KIDS,
    EFFECT_ECHO_MODE_MAGIC,
    EFFECT_ECHO_MODE_MAX,
};

extern const struct __effect_mode_attr echo_tool_attr[EFFECT_ECHO_MODE_MAX];

#endif//__EFFECT_REG_ECHO_H__
