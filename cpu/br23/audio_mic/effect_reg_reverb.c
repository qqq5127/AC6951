#include "effect_reg_reverb.h"

#if (TCFG_MIC_EFFECT_ENABLE)
static const u16 reverb_cmd_cfg[] = {
    EFFECTS_CMD_REVERB,
    EFFECTS_CMD_NOISE,
#if TCFG_EQ_ENABLE
    EFFECTS_CMD_HIGH_SOUND,
    EFFECTS_CMD_LOW_SOUND,
#if MIC_EFFECT_EQ_EN
    EFFECTS_EQ_ONLINE_CMD_PARAMETER_SEG,
#endif
#endif
    EFFECTS_CMD_MIC_ANALOG_GAIN,
};

static const u16 electric_cmd_cfg[] = {
    EFFECTS_CMD_REVERB,
    EFFECTS_CMD_NOISE,
#if TCFG_EQ_ENABLE
    EFFECTS_CMD_HIGH_SOUND,
    EFFECTS_CMD_LOW_SOUND,
#if MIC_EFFECT_EQ_EN
    EFFECTS_EQ_ONLINE_CMD_PARAMETER_SEG,
#endif
#endif
    EFFECTS_CMD_MIC_ANALOG_GAIN,
};

static const u16 boy_to_girl_cmd_cfg[] = {
    EFFECTS_CMD_REVERB,
    EFFECTS_CMD_PITCH2,
    EFFECTS_CMD_NOISE,
#if TCFG_EQ_ENABLE
    EFFECTS_CMD_HIGH_SOUND,
    EFFECTS_CMD_LOW_SOUND,
#if MIC_EFFECT_EQ_EN
    EFFECTS_EQ_ONLINE_CMD_PARAMETER_SEG,
#endif
#endif
    EFFECTS_CMD_MIC_ANALOG_GAIN
};

static const u16 girl_to_boy_cmd_cfg[] = {
    EFFECTS_CMD_REVERB,
    EFFECTS_CMD_PITCH2,
    EFFECTS_CMD_NOISE,
#if TCFG_EQ_ENABLE
    EFFECTS_CMD_HIGH_SOUND,
    EFFECTS_CMD_LOW_SOUND,
#if MIC_EFFECT_EQ_EN
    EFFECTS_EQ_ONLINE_CMD_PARAMETER_SEG,
#endif
#endif
    EFFECTS_CMD_MIC_ANALOG_GAIN
};

static const u16 kids_cmd_cfg[] = {
    EFFECTS_CMD_REVERB,
    EFFECTS_CMD_PITCH2,
    EFFECTS_CMD_NOISE,
#if TCFG_EQ_ENABLE
    EFFECTS_CMD_HIGH_SOUND,
    EFFECTS_CMD_LOW_SOUND,
#if MIC_EFFECT_EQ_EN
    EFFECTS_EQ_ONLINE_CMD_PARAMETER_SEG,
#endif
#endif
    EFFECTS_CMD_MIC_ANALOG_GAIN
};
static const u16 magic_cmd_cfg[] = {
    EFFECTS_CMD_REVERB,
    EFFECTS_CMD_PITCH2,
    EFFECTS_CMD_NOISE,
#if TCFG_EQ_ENABLE
    EFFECTS_CMD_HIGH_SOUND,
    EFFECTS_CMD_LOW_SOUND,
#if MIC_EFFECT_EQ_EN
    EFFECTS_EQ_ONLINE_CMD_PARAMETER_SEG,
#endif
#endif
    EFFECTS_CMD_MIC_ANALOG_GAIN
};
static const u16 boom_cmd_cfg[] = {
    EFFECTS_CMD_REVERB,
    EFFECTS_CMD_NOISE,
#if TCFG_EQ_ENABLE
    EFFECTS_CMD_HIGH_SOUND,
    EFFECTS_CMD_LOW_SOUND,
#if MIC_EFFECT_EQ_EN
    EFFECTS_EQ_ONLINE_CMD_PARAMETER_SEG,
#endif
#endif
    EFFECTS_CMD_MIC_ANALOG_GAIN
};
static const u16 shouting_wheat_cmd_cfg[] = {
    EFFECTS_CMD_REVERB,
    EFFECTS_CMD_NOISE,
#if TCFG_EQ_ENABLE
    EFFECTS_CMD_HIGH_SOUND,
    EFFECTS_CMD_LOW_SOUND,
    EFFECTS_CMD_SHOUT_WHEAT,
#if MIC_EFFECT_EQ_EN
    EFFECTS_EQ_ONLINE_CMD_PARAMETER_SEG,
#endif
#endif
    EFFECTS_CMD_MIC_ANALOG_GAIN
};

const struct __effect_mode_attr reverb_tool_attr[EFFECT_REVERB_MODE_MAX] = {
    [EFFECT_REVERB_MODE_NORMAL] =
    {
        .num = ARRAY_SIZE(reverb_cmd_cfg),
        .cmd_tab = (u16 *)reverb_cmd_cfg,
        .name = "纯混响",
    },
    [EFFECT_REVERB_MODE_ELECTRIC] =
    {
        .num = ARRAY_SIZE(electric_cmd_cfg),
        .cmd_tab = (u16 *)electric_cmd_cfg,
        .name = "电音",

    },
    [EFFECT_REVERB_MODE_BOY_TO_GIRL] =
    {
        .num = ARRAY_SIZE(boy_to_girl_cmd_cfg),
        .cmd_tab = (u16 *)boy_to_girl_cmd_cfg,
        .name = "男声变女声",

    },
    [EFFECT_REVERB_MODE_GIRL_TO_BOY] =
    {
        .num = ARRAY_SIZE(girl_to_boy_cmd_cfg),
        .cmd_tab = (u16 *)girl_to_boy_cmd_cfg,
        .name = "女声变男声",

    },
    [EFFECT_REVERB_MODE_KIDS] =
    {
        .num = ARRAY_SIZE(kids_cmd_cfg),
        .cmd_tab = (u16 *)kids_cmd_cfg,
        .name = "娃娃音",
    },
    [EFFECT_REVERB_MODE_MAGIC] =
    {
        .num = ARRAY_SIZE(magic_cmd_cfg),
        .cmd_tab = (u16 *)magic_cmd_cfg,
        .name = "魔音",
    },
    [EFFECT_REVERB_MODE_BOOM] =
    {
        .num = ARRAY_SIZE(boom_cmd_cfg),
        .cmd_tab = (u16 *)boom_cmd_cfg,
        .name = "爆音",
    },
    [EFFECT_REVERB_MODE_SHOUTING_WHEAT] =
    {
        .num = ARRAY_SIZE(shouting_wheat_cmd_cfg),
        .cmd_tab = (u16 *)shouting_wheat_cmd_cfg,
        .name = "喊麦",
    },
};


#endif//TCFG_MIC_EFFECT_ENABLE
