#include "effect_reg.h"

#if (TCFG_MIC_EFFECT_ENABLE)
static const u16 echo_cmd_cfg[] = {
    EFFECTS_CMD_ECHO,
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
    EFFECTS_CMD_ECHO,
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
    EFFECTS_CMD_ECHO,
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
    EFFECTS_CMD_ECHO,
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
    EFFECTS_CMD_ECHO,
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

const struct __effect_mode_attr echo_tool_attr[EFFECT_ECHO_MODE_MAX] = {
    [EFFECT_ECHO_MODE_NORMAL] =
    {
        .num = ARRAY_SIZE(echo_cmd_cfg),
        .cmd_tab = (u16 *)echo_cmd_cfg,
        .name = "´¿»ØÉù",
    },
    [EFFECT_ECHO_MODE_BOY_TO_GIRL] =
    {
        .num = ARRAY_SIZE(boy_to_girl_cmd_cfg),
        .cmd_tab = (u16 *)boy_to_girl_cmd_cfg,
        .name = "ÄÐÉù±äÅ®Éù",
    },
    [EFFECT_ECHO_MODE_GIRL_TO_BOY] =
    {
        .num = ARRAY_SIZE(girl_to_boy_cmd_cfg),
        .cmd_tab = (u16 *)girl_to_boy_cmd_cfg,
        .name = "Å®Éù±äÄÐÉù",

    },
    [EFFECT_ECHO_MODE_KIDS] =
    {
        .num = ARRAY_SIZE(kids_cmd_cfg),
        .cmd_tab = (u16 *)kids_cmd_cfg,
        .name = "ÍÞÍÞÒô",
    },
    [EFFECT_ECHO_MODE_MAGIC] =
    {
        .num = ARRAY_SIZE(magic_cmd_cfg),
        .cmd_tab = (u16 *)magic_cmd_cfg,
        .name = "Ä§Òô",
    },
};

#endif//TCFG_MIC_EFFECT_ENABLE




