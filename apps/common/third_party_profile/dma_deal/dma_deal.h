

#ifndef _DUEROS_DEAL_H
#define _DUEROS_DEAL_H

#include<string.h>
#include <stdint.h>
#include "btstack/third_party/baidu/dueros_dma_pb.h"
#include "btstack/third_party/baidu/dueros_platform.h"
//#include "le_common.h"

#define SYS_BT_AI_EVENT_TYPE_STATUS (('B' << 24) | ('A' << 16) | ('I' << 8) | '\0')

void smart_dueros_dma_init(void (*)(void), void (*)(void));
void smart_dueros_dma_close(void);
int dueros_process_cmd_from_phone(unsigned char *buf, int len);
void set_spp_connect_flg(u8 flg);
bool dma_msg_deal(u32 param);
void dueros_send_ver(void);
void dueros_event_post(u32 type, u8 event);
int dueros_stop_speech(void);
void set_mic_busy_timeout(u8 *data, u8 len);
void tws_ble_dueros_handle_create();

void dma_spp_init(void);

bool dueros_lock_connection_chl(u8 chl);
bool dueros_unlock_connection_chl(u8 chl);


#endif
