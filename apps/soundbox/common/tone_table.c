#include "app_config.h"
#include "audio_config.h"
#include "tone_player.h"

const char *tone_table[] = {
    [IDEX_TONE_NUM_0] 			= TONE_RES_ROOT_PATH"tone/0.*",
    [IDEX_TONE_NUM_1] 			= TONE_RES_ROOT_PATH"tone/1.*",
    [IDEX_TONE_NUM_2] 			= TONE_RES_ROOT_PATH"tone/2.*",
    [IDEX_TONE_NUM_3] 			= TONE_RES_ROOT_PATH"tone/3.*",
    [IDEX_TONE_NUM_4] 			= TONE_RES_ROOT_PATH"tone/4.*",
    [IDEX_TONE_NUM_5] 			= TONE_RES_ROOT_PATH"tone/5.*",
    [IDEX_TONE_NUM_6] 			= TONE_RES_ROOT_PATH"tone/6.*",
    [IDEX_TONE_NUM_7] 			= TONE_RES_ROOT_PATH"tone/7.*",
    [IDEX_TONE_NUM_8] 			= TONE_RES_ROOT_PATH"tone/8.*",
    [IDEX_TONE_NUM_9] 			= TONE_RES_ROOT_PATH"tone/9.*",
    [IDEX_TONE_BT_MODE] 		= TONE_RES_ROOT_PATH"tone/bt.*",
    [IDEX_TONE_BT_CONN] 		= TONE_RES_ROOT_PATH"tone/bt_conn.*",
    [IDEX_TONE_BT_DISCONN] 		= TONE_RES_ROOT_PATH"tone/bt_dconn.*",
    [IDEX_TONE_TWS_CONN] 		= TONE_RES_ROOT_PATH"tone/tws_conn.*",
    [IDEX_TONE_TWS_DISCONN]		= TONE_RES_ROOT_PATH"tone/tws_dconn.*",
    [IDEX_TONE_LOW_POWER] 		= TONE_RES_ROOT_PATH"tone/low_power.*",
    [IDEX_TONE_POWER_OFF] 		= TONE_RES_ROOT_PATH"tone/power_off.*",
    [IDEX_TONE_POWER_ON] 		= TONE_RES_ROOT_PATH"tone/power_on.*",
    [IDEX_TONE_RING] 			= TONE_RES_ROOT_PATH"tone/ring.*",
    [IDEX_TONE_MAX_VOL] 		= TONE_NORMAL,
    [IDEX_TONE_NORMAL] 			= TONE_NORMAL,
    [IDEX_TONE_MUSIC] 			= TONE_RES_ROOT_PATH"tone/music.*",
    [IDEX_TONE_LINEIN] 			= TONE_RES_ROOT_PATH"tone/linein.*",
    [IDEX_TONE_FM] 				= TONE_RES_ROOT_PATH"tone/fm.*",
    [IDEX_TONE_PC] 				= TONE_RES_ROOT_PATH"tone/pc.*",
    [IDEX_TONE_RTC] 			= TONE_RES_ROOT_PATH"tone/rtc.*",
    [IDEX_TONE_RECORD] 			= TONE_RES_ROOT_PATH"tone/record.*",
    [IDEX_TONE_UDISK] 			= TONE_RES_ROOT_PATH"tone/udisk.*",
    [IDEX_TONE_SD] 				= TONE_RES_ROOT_PATH"tone/sd.*",
    [IDEX_TONE_SPEECH_START]	= TONE_NORMAL,//TONE_RES_ROOT_PATH"tone/speech.*",

};

