#ifndef _FM_EMITTTER_MANAGE_H
#define _FM_EMITTTER_MANAGE_H

#include "printf.h"
#include "cpu.h"
#include "asm/iic_hw.h"
#include "asm/iic_soft.h"
#include "timer.h"
#include "app_config.h"
#include "event.h"
#include "system/includes.h"

#define FM_EMITTER_MAX_VOL     (30)

typedef struct {
    u8 name[20];
    u8(*init)(u16 fre);
    u8(*start)(void);
    u8(*stop)(void);
    u8(*set_fre)(u16 fre);
    u8(*set_power)(u8 power, u16 fre);
    void (*data_cb)(void *cb);
} FM_EMITTER_INTERFACE;

extern FM_EMITTER_INTERFACE fm_emitter_dev_begin[];
extern FM_EMITTER_INTERFACE fm_emitter_dev_end[];

#define REGISTER_FM_EMITTER(fm) \
	static FM_EMITTER_INTERFACE fm SEC_USED(.fm_emitter_dev)

#define list_for_each_fm_emitter(c) \
	for (c=fm_emitter_dev_begin; c<fm_emitter_dev_end; c++)

void fm_emitter_manage_init(u16 fre, void (*fmtx_isr_cb)(s16 *, u32));
void fm_emitter_manage_start(void);
void fm_emitter_manage_stop(void);
void fm_emitter_manage_set_fre(u16 fre);
void fm_emitter_manage_set_power(u8 power, u16 fre);

u16 fm_emitter_manage_get_fre();

#endif
