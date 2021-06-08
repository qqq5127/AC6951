#ifndef __QN8007_H__
#define __QN8007_H__

#include "typedef.h"

/***********************************************************/
/*power = (0.37*PAC_TARGET+68)dB,PAC_TARGET range is 31-131*/
#define QN8007_TX_POWER_MIN			31
#define QN8007_TX_POWER_MAX			131
/***********************************************************/

/************QN8007 Address****************/
#define QN8007_ADDR_WRITE  0x56
#define QN8007_ADDR_READ   0x57
/******************************************/


/********QN8007 User Control Registers********/
#define QN8007_SYSTEM1    0x00
#define QN8007_SYSTEM2    0x01
#define QN8007_DEV_ADD    0x02
#define QN8007_ANACTL1    0x03
#define QN8007_REG_VGA    0x04
#define QN8007_CID1       0x05
#define QN8007_CID2       0x06
#define QN8007_I2S        0x07
#define QN8007_CH         0x08
#define QN8007_CH_START   0x09
#define QN8007_CH_STOP    0x0A
#define QN8007_CH_STEP    0x0B
#define QN8007_PAC_TARGET 0x0C
#define QN8007_TXAGC_GAIN 0x0D
#define QN8007_TX_FDEV    0x0E
#define QN8007_GAIN_TXPLT 0x0F
#define QN8007_RDSD0      0X10
#define QN8007_RDSD1      0x11
#define QN8007_RDSD2      0x12
#define QN8007_RDSD4      0x14
#define QN8007_RDSD5      0x15
#define QN8007_RDSD6      0x16
#define QN8007_RDSD7      0x17
#define QN8007_RDSFDEV    0x18
#define QN8007_CCA        0x19
#define QN8007_STATUS1    0x1A
#define QN8007_STATUS2    0x1B
#define QN8007_REG_XLT3   0x49
#define QN8007_PAC_CAL    0x59
#define QN8007_PAG_CAL    0x5A
/*********************************************/
// SYSTEM1
#define QN8007_RXREQ      0x80
#define QN8007_TXREQ      0x40
#define QN8007_CHSC       0x20
#define QN8007_STNBY      0x10
#define QN8007_RXI2S      0x08
#define QN8007_TXI2S      0x04
#define QN8007_RDSEN      0x02
#define QN8007_CCA_CH_DIS 0x01
// SYSTEM2
#define QN8007_SWRST      0x80
#define QN8007_RECAL      0x40
#define QN8007_FORCE_MO   0x20
#define QN8007_ST_MO_TX   0x10
#define QN8007_PETC       0x08
#define QN8007_RDSTXRDY   0x04
#define QN8007_TMOUT1     0x02
#define QN8007_TMOUT0     0x01
// channel
#define QN8007_CH_AUTO		0
#define QN8007_CH_USER	    1

extern void qn8007_init(u16 fre);
extern void qn8007_transmit_start(void);
extern void qn8007_transmit_stop(void);
extern void qn8007_set_freq(u16 freq);
extern void qn8007_set_power(u8 power, u16 freq);
extern u16 qn8007_channel_scan(u16 ch_start, u16 ch_stop);

#endif



