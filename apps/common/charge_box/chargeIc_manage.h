#ifndef _CHARGEIC_MANAGE_H
#define _CHARGEIC_MANAGE_H

#include "printf.h"
#include "cpu.h"
#include "timer.h"
#include "app_config.h"
#include "event.h"
#include "system/includes.h"

void chargeIc_init(void);
void chargeIc_boost_ctrl(u8 en);
void chargeIc_pwr_ctrl(u8 en);
void chargebox_charge_start(void);
void chargebox_charge_close(void);
u8 chargebox_get_charge_en(void);

#endif


