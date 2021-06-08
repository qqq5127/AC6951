// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _LE_SMARTBOX_ADV_H
#define _LE_SMARTBOX_ADV_H

#include <stdint.h>
#include "bt_common.h"
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_RCSP_DEMO)

u8 get_adv_notify_state(void);
void set_adv_notify_state(u8 notify);
int smartbox_make_set_rsp_data(void);
int smartbox_make_set_adv_data(void);
void bt_ble_init_do(void);
int bt_ble_adv_ioctl(u32 cmd, u32 priv, u8 mode);
void set_connect_flag(u8 value);

#endif
#endif
