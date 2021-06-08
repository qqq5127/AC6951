#ifndef __SMARTBOX_ADV_BLUETOOTH__
#define __SMARTBOX_ADV_BLUETOOTH__
#include "typedef.h"
#include "system/event.h"


#define		SDK_TYPE_AC690X				0x0
#define		SDK_TYPE_AC692X				0x1
#define 	SDK_TYPE_AC693X				0x2
#define 	SDK_TYPE_AC695X 			0x3
#define		SDK_TYPE_AC697X 			0x4
#define		SDK_TYPE_AC696X 			0x5
#define		SDK_TYPE_AC696X_TWS			0x6
#define 	SDK_TYPE_AC695X_SOUNDCARD	0x7

#if   (defined CONFIG_CPU_BR21)
#define		RCSP_SDK_TYPE		SDK_TYPE_AC692X
#elif (defined CONFIG_CPU_BR22)
#define		RCSP_SDK_TYPE		SDK_TYPE_AC693X
#elif (defined CONFIG_CPU_BR23 && SOUNDCARD_ENABLE)
#define		RCSP_SDK_TYPE		SDK_TYPE_AC695X_SOUNDCARD
#elif (defined CONFIG_CPU_BR23)
#define		RCSP_SDK_TYPE		SDK_TYPE_AC696X//SDK_TYPE_AC695X
#elif (defined CONFIG_CPU_BR25)
#define		RCSP_SDK_TYPE		SDK_TYPE_AC696X
#elif (defined CONFIG_CPU_BR30)
#define		RCSP_SDK_TYPE		SDK_TYPE_AC697X
#else
#define		RCSP_SDK_TYPE		SDK_TYPE_AC693X
#endif

// enum RCSP_ADV_MSG_T {
//     MSG_JL_ADV_SETTING_SYNC,
//     MSG_JL_ADV_SETTING_UPDATE,
// };
//
// bool smartbox_adv_msg_post(int msg, int argc, ...);
int JL_smartbox_adv_event_handler(struct rcsp_event *rcsp);
int JL_smartbox_adv_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len);

#endif
