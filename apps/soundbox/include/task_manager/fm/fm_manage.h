#ifndef _FM_MANAGE_H
#define _FM_MANAGE_H

#include "printf.h"
#include "cpu.h"
#include "asm/iic_hw.h"
#include "asm/iic_soft.h"
#include "timer.h"
#include "app_config.h"
#include "event.h"
#include "system/includes.h"
#include "typedef.h"


#define FREQ_STEP           (100)//100 步进

#define REAL_FREQ_MIN  		(8750)
#define REAL_FREQ_MAX  		(10800)

#define VIRTUAL_FREQ_STEP   (FREQ_STEP/10)
#define REAL_FREQ(x) 		((REAL_FREQ_MIN-VIRTUAL_FREQ_STEP) + (x)*VIRTUAL_FREQ_STEP)
#define VIRTUAL_FREQ(x)		((x - (REAL_FREQ_MIN-VIRTUAL_FREQ_STEP))/VIRTUAL_FREQ_STEP)
#define MAX_CHANNEL ((REAL_FREQ_MAX - REAL_FREQ_MIN)/VIRTUAL_FREQ_STEP + 1)


enum {

    FM_CUR_FRE = 0,
    FM_FRE_DEC = 1,
    FM_FRE_INC	= 2,
};



typedef struct {
    u8(*init)(void *priv);
    u8(*close)(void *priv);
    u8(*set_fre)(void *priv, u16 fre);
    u8(*mute)(void *priv, u8 flag);
    u8(*read_id)(void *priv);
    u8 logo[20];
} FM_INTERFACE;



struct _fm_dev_info {
    u8 iic_hdl;
    u8 iic_delay;          //这个延时并非影响iic的时钟频率，而是2Byte数据之间的延时
    u8 logo[20];
};

struct _fm_dev_platform_data {
    u8 iic_hdl;
    u8 iic_delay;          //这个延时并非影响iic的时钟频率，而是2Byte数据之间的延时
};


extern FM_INTERFACE fm_dev_begin[];
extern FM_INTERFACE fm_dev_end[];

#define REGISTER_FM(fm) \
	static FM_INTERFACE fm SEC_USED(.fm_dev)

#define list_for_each_fm(c) \
	for (c=fm_dev_begin; c<fm_dev_end; c++)

#define FM_DEV_PLATFORM_DATA_BEGIN(data) \
		static const struct _fm_dev_platform_data data = {

#define FM_DEV_PLATFORM_DATA_END() \
};

extern u8 fm_dev_iic_write(u8 w_chip_id, u8 register_address, u8 *buf, u32 data_len);
extern u8 fm_dev_iic_readn(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len);
extern struct _fm_dev_info t_fm_dev_info;



void fm_manage_check_online();
int fm_manage_init();
int fm_manage_start();
bool fm_manage_set_fre(u16 fre);
u16  fm_manage_get_fre();
void fm_manage_close(void);
void fm_manage_mute(u8 mute);
int fm_dev_init(void *_data);

void fm_mutex_init(void *priv);
void fm_mutex_stop(void *priv);

extern void fm_inside_trim(u32 freq);
extern void save_scan_freq_org(u32 freq);
//内部收音优化使用



#if 0
#define iic_init(iic)                       hw_iic_init(iic)
#define iic_uninit(iic)                     hw_iic_uninit(iic)
#define iic_start(iic)                      hw_iic_start(iic)
#define iic_stop(iic)                       hw_iic_stop(iic)
#define iic_tx_byte(iic, byte)              hw_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               hw_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         hw_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        hw_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    hw_iic_suspend(iic)
#define iic_resume(iic)                     hw_iic_resume(iic)
#else
#define iic_init(iic)                       soft_iic_init(iic)
#define iic_uninit(iic)                     soft_iic_uninit(iic)
#define iic_start(iic)                      soft_iic_start(iic)
#define iic_stop(iic)                       soft_iic_stop(iic)
#define iic_tx_byte(iic, byte)              soft_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               soft_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         soft_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        soft_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    soft_iic_suspend(iic)
#define iic_resume(iic)                     soft_iic_resume(iic)
#endif


#endif
