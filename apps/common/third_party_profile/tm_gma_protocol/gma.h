
#ifndef __GMA_H__
#define __GMA_H__

#include "typedef.h"

typedef enum {
    START_MIC = 0,
    STOP_MIC,
} mic_status_e;

#define  VOICE_SEND_MAX     150
#define  VOICE_MIC_NUM      1
#define  MIC_NUM            1
#define  REF_NUM            0


extern bool tm_gma_msg_deal(u32 param);
void gma_event_post(u32 type, u8 event);

int gma_mic_status_report(mic_status_e status);
int gma_recv_proc(uint8_t *buf, uint16_t len);
void gma_init(int (*should_send)(uint16_t len), int(*send_data)(uint8_t *buf, uint16_t len), int(*send_audio_data)(uint8_t *buf, uint16_t len));
void gma_exit(void);
int gma_adpcm_voice_mic_send(short *voice_buf, uint16_t voice_len);
int gma_opus_voice_mic_send(uint8_t *voice_buf, uint16_t voice_len);
bool gma_connect_success(void);
bool gma_ali_para_check(void);
void gma_send_secret_to_sibling(void);
void gma_sync_remote_addr(u8 *buf);
void gma_kick_license_to_sibling(void);
void gma_send_secret_sync(void);
void gma_slave_sync_remote_addr(u8 *buf);
void ble_mac_reset(void);
void gma_adv_restore(void);
int gma_reply_ota_crc_result(int crc_res);
int gma_hw_start_speech(void);
int gma_hw_stop_speech(void);
void tm_send_process_resume(void);
void tm_cmd_analysis_resume(void);


void gma_spp_disconnect(void);
void gma_spp_init(void);

//ota interface
int tws_ota_get_data_from_sibling(u8 opcode, u8 *data, u8 len);
u8 tws_ota_control(int type, ...);
void tws_ota_app_event_deal(u8 evevt);
#endif /* __GMA_H__ */


