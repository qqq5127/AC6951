
#ifndef __SMARTBOX_FUNCTION_H__
#define __SMARTBOX_FUNCTION_H__

#include "typedef.h"
#include "app_config.h"

enum {
    COMMON_FUNCTION_ATTR_TYPE_BATTERY 					= 0,
    COMMON_FUNCTION_ATTR_TYPE_VOL 						= 1,
    COMMON_FUNCTION_ATTR_TYPE_DEV_INFO 					= 2,
    COMMON_FUNCTION_ATTR_TYPE_ERROR_STATS				= 3,
    COMMON_FUNCTION_ATTR_TYPE_EQ_INFO					= 4,
    COMMON_FUNCTION_ATTR_TYPE_BS_FILE_TYPE				= 5,
    COMMON_FUNCTION_ATTR_TYPE_FUNCTION_MODE				= 6,
    COMMON_FUNCTION_ATTR_TYPE_COLOR_LED_SETTING_INFO  	= 7,
    COMMON_FUNCTION_ATTR_TYPE_FMTX_FREQ					= 8,
    COMMON_FUNCTION_ATTR_TYPE_BT_EMITTER_SW				= 9,
    COMMON_FUNCTION_ATTR_TYPE_BT_EMITTER_CONNECT_STATES	= 10,
    COMMON_FUNCTION_ATTR_TYPE_HIGH_LOW_SET				= 11,
    COMMON_FUNCTION_ATTR_TYPE_PRE_FETCH_ALL_EQ_INFO  	= 12,
    COMMON_FUNCTION_ATTR_TYPE_PHONE_SCO_STATE_INFO  	= 15,
    COMMON_FUNCTION_ATTR_TYPE_MISC_SETTING_INFO  		= 16,
    COMMON_FUNCTION_ATTR_TYPE_PRE_FETCH_KARAOKE_EQ_INFO	= 17,
    COMMON_FUNCTION_ATTR_TYPE_KARAOKE_EQ_SETTING_INFO	= 18,
    COMMON_FUNCTION_ATTR_TYPE_KARAOKE_SETTING_INFO		= 19,
    COMMON_FUNCTION_ATTR_TYPE_MAX,
};

bool smartbox_function_cmd_set(void *priv, u8 function, u8 *data, u16 len);
bool smartbox_function_set(void *priv, u8 function, u8 *data, u16 len);
u32 smartbox_function_get(void *priv, u8 function, u8 *data, u16 len, u8 *buf, u16 buf_size);
void smartbox_function_update(u8 function, u32 mask);
void smartbox_update_bt_emitter_connect_state(u8 state, u8 *addr);


#if (SMART_BOX_EN)
#define SMARTBOX_UPDATE					smartbox_function_update
#else
#define SMARTBOX_UPDATE(...)
#endif//SMART_BOX_EN

#endif//__SMARTBOX_FUNCTION_H__

