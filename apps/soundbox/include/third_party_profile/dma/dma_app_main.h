#ifndef __DMA_APP_MAIN_H__
#define __DMA_APP_MAIN_H__
#include "generic/typedef.h"
#include "app_config.h"
#include "system/includes.h"

int dma_app_bt_state_init(void);
void dma_app_bredr_handle_reg(void);
int dma_app_sys_event_handler_specific(struct sys_event *event);


#endif//__DMA_APP_MAIN_H__

