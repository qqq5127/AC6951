#ifndef __GMA_APP_MAIN_H__
#define __GMA_APP_MAIN_H__
#include "generic/typedef.h"
#include "app_config.h"
#include "system/includes.h"

int gma_app_bt_state_init(void);
int gma_app_bt_state_set_page_scan_enable(void);
int gma_app_bt_state_get_connect_mac_addr(void);
int gma_app_bt_state_cancel_page_scan(void);
int gma_app_bt_state_enter_soft_poweroff(void);
int gma_app_bt_state_tws_init(int paired);
int gma_app_bt_state_tws_connected(int first_pair, u8 *comm_addr);
void gma_app_bredr_handle_reg(void);
int gma_app_sys_event_handler_specific(struct sys_event *event);


#endif//__GMA_APP_MAIN_H__
