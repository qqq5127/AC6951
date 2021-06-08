#ifndef _IAP_IIC_H_
#define _IAP_IIC_H_

#include "generic/typedef.h"

#define IAP_CRACK_CP20		0
#define IAP_CRACK_CP30		1

#define IAP_CRACK_CHIP		IAP_CRACK_CP20	//	IAP_CRACK_CP30

///解密芯片地址
#if (IAP_CRACK_CHIP==IAP_CRACK_CP30)
#define WRITE_ADDR  	 0x20
#define	READ_ADDR   	 0x21
#else
#define WRITE_ADDR  	 0x22
#define	READ_ADDR   	 0x23
#endif
//Authentication Coprocessor 2.0C register map
#define ADDR_DEVICE_VERSION							0x00 //read only
#define ADDR_FIRMWARE_VERSION							0x01 //read only
#define ADDR_AUTHENTICATION_PROTOCOL_MAJOR_VERSION	0x02 //read only
#define ADDR_AUTHENTICATION_PROTOCOL_MINOR_VERSION	0x03 //read only
#define ADDR_DEVICE_ID								0x04 //read only


//MFI CHIP ONLINE STATUS
enum {
    MFI_CHIP_OFFLINE = 0,
    MFI_CHIP_ONLINE,
    MFI_DV_ERROR,
    MFI_FV_ERROR,
    MFI_AP_MAJOR_V_ERROR,
    MFI_AP_MINOR_V_ERROR,
    MFI_DEVICE_ID_ERROR,
};

///inside call
///outside call
void iAP_iic_write(u8 iic_addr, u8 iic_dat);
void iAP_iic_writen(u8 iic_addr, u8 *iic_dat, u8 len);
u8   iAP_iic_read(u8 iic_addr);
void iAP_iic_readn(u8 iic_addr, u8 *iic_dat, u8 len);
u8 apple_mfi_chip_online_check_api(void);
#define iAP_iic_errcode_check()		iAP_iic_read(0x05)


#endif	/*	_IAP_IIC_H_  */
