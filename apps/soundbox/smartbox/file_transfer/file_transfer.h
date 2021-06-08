#ifndef __FILE_TRANSFER_H__
#define __FILE_TRANSFER_H__

#include "typedef.h"
#include "app_config.h"

void file_transfer_init(void (*end_callback)(void));
void file_transfer_download_start(u8 OpCode_SN, u8 *data, u16 len);
void file_transfer_download_end(u8 status, u8 *data, u16 len);
void file_transfer_download_doing(u8 *data, u16 len);
void file_transfer_download_passive_cancel(u8 OpCode_SN, u8 *data, u16 len);
void file_transfer_download_active_cancel(void);
void file_transfer_download_active_cancel_response(u8 status, u8 *data, u16 len);
void file_transfer_file_rename(u8 status, u8 *data, u16 len);
void file_transfer_close(void);

#endif//__FILE_TRANSFER_H__

