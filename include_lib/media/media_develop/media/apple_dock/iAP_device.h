/*************************************************************/
/** @file:		usb_device.h
    @brief:		USB 从机驱动重写，加入iAP协议
    @details:
    @author:	Bingquan Cai
    @date: 		2013-10-11,09:34
    @note:
*/
/*************************************************************/
#ifndef _IAP_DEVICE_H_
#define _IAP_DEVICE_H_

#include "generic/typedef.h"

enum {
    IAP_DEVICE_DESCRIPTOR = 0x0,
    IAP_CONFIG_DESCRIPTOR,
    IAP_IMANU,
    IAP_IPRODUCT,
    IAP_ISERIAL,
};
typedef struct _IAP_USB_DEVICE_STRING {
    u8 *pString;
    u8  bLen;
} IAP_USB_DEVICE_STRING;
extern const IAP_USB_DEVICE_STRING IAP_USB_string_set[];

extern const u8 MFi_iManufacturer[20];
extern const u8 MFi_iProduct[26];
extern const u8 MFi_iSerialNumber[28];
extern const u8 MFi_Configuration_Descriptor[140];

#define IAP_TRANSPORT_ZLP			0
#define IAP_TRANSPORT_IN			1
#define IAP_TRANSPORT_OUT			2
#define IAP_TRANSPORT_OUT_LEN		7

// void iAP_ZLP(void);
// bool iAP_bulk_in_process(void);
// u8   iAP_bulk_out_process(void);
extern u8   iAP_link_checksum(u8 *pkt, u16 len);
extern u8   iAP_transport(u8 type);
extern u16  iAP_iic_crack(u8 *TxBuf, u8 *RxBuf, u8 len);
extern u16  iAP_iic_X509(u8 *RxBuf, u16 max_len);

///outside call


#endif	/*	_IAP_H_	*/

