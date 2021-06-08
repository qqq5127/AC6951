#ifndef _QN8027_H_
#define _QN8027_H_

/******************************************************/
/*Power = (0.62*PA_TRRGT+71)dB,PA_TRRGT range is 20-75*/
#define QN8027_TX_POWER_MIN			20
#define QN8027_TX_POWER_MAX			75
/******************************************************/

/************QN8027 Address****************/
#define QN8027_ADDR_WRITE  0x58
#define QN8027_ADDR_READ   0x59
/******************************************/

/********QN8027 User Control Registers********/
#define QN8027_SYSTEM  	0x00
#define QN8027_CH1     	0x01
#define QN8027_GPLT    	0x02
#define QN8027_REG_XTL 	0x03
#define QN8027_REG_VGA 	0x04
#define QN8027_CID1    	0x05
#define QN8027_CID2    	0x06
#define QN8027_STATUS  	0x07
#define QN8027_RDSD0   	0x08
#define QN8027_RDSD1   	0x09
#define QN8027_RDSD2   	0x0A
#define QN8027_RDSD3   	0x0B
#define QN8027_RDSD4   	0x0C
#define QN8027_RDSD5   	0x0D
#define QN8027_RDSD6   	0x0E
#define QN8027_RDSD7   	0x0F
#define QN8027_PAC     	0x10
#define QN8027_FDEV    	0x11
#define QN8027_RDS     	0x12
/*********************************************/

void qn8027_init(u16 fre);
void qn8027_set_freq(u16 fre);
void qn8027_set_power(u8 power, u16 freq);
void qn8027_transmit_start(void);
void qn8027_transmit_stop(void);
void qn8027_mute(u8 mute);

#endif


