#ifndef __FILE_DELETE_H__
#define __FILE_DELETE_H__

#include "typedef.h"
#include "app_config.h"

void file_delete_init(void (*end_callback)(void));
void file_delete_start(u8 OpCode_SN, u8 *data, u16 len);
void file_delete_end(void);

#endif//__FILE_DELETE_H__

