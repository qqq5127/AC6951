
#ifndef __DEV_FORMAT_H__
#define __DEV_FORMAT_H__

#include "typedef.h"
#include "app_config.h"

void dev_format_init(void (*end_callback)(void));
void dev_format_start(u8 OpCode_SN, u8 *data, u16 len);

#endif//__DEV_FORMAT_H__

