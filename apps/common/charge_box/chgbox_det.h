#ifndef _CHGBOX_DET_H_
#define _CHGBOX_DET_H_

#include "typedef.h"

u16 get_vbat_voltage(void);
u16 get_ear_current(void);
void chargebox_det_init(void);
void usb_wakeup_deal(void);
void usb_wakeup_deal(void);
void hall_wakeup_deal(void);
void ldo_wakeup_deal(void *priv);
void usb_charge_full_wakeup_deal(void);
void ear_current_detect_enable(u8 en);

#endif
