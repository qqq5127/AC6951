#ifndef _CHGBOX_WIRELESS_H_
#define _CHGBOX_WIRELESS_H_

#include "device/wireless_charge.h"

//api函数声明
u16 get_wireless_power(void);
void wireless_api_close(void);
void wireless_api_open(void);
void wireless_init_api(void);
void wireless_io_wakeup_deal(void);
void wireless_data_over_run(void);

#endif
