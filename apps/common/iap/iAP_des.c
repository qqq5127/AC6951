
#include "apple_dock/iAP.h"
#include "apple_dock/iAP2.h"
#include "apple_dock/iAP_device.h"
/* #include "apple_dock/descriptor_iap.h" */

#include "app_config.h"

#if TCFG_USB_APPLE_DOCK_EN

#define ID_INFORMATION			0x1D01L

const u8 iAP2_message_name[] = {
    GET_U16L(SW16(START_OF_MESSAGE)), GET_U16H(SW16(START_OF_MESSAGE)),
    0x00, 0x00,
    GET_U16L(SW16(ID_INFORMATION)), GET_U16H(SW16(ID_INFORMATION)),

    //Name--can alter
    0x00, 4 + IAP2_ACCESSORY_NAME_LEN, 0x00, 0x00, IAP2_ACCESSORY_NAME,
    //ModelIdentifier(模式描述符)--can alter
    0x00, 4 + IAP2_ACCESSORY_MODEL_IDENTIFIER_LEN, 0x00, 0x01, IAP2_ACCESSORY_MODEL_IDENTIFIER,
    //Manufacturer(制造商)--can alter
    0x00, 4 + IAP2_ACCESSORY_MANUFACTURER_LEN, 0x00, 0x02, IAP2_ACCESSORY_MANUFACTURER,
    //SerialNumber(序列号)--can alter
    0x00, 4 + IAP2_ACCESSORY_SERIALNUMBER_LEN, 0x00, 0x03, IAP2_ACCESSORY_SERIALNUMBER,
    //FirmwareVersion(固件版本)--can alter
    0x00, 4 + IAP2_ACCESSORY_FIRMWARE_VER_LEN, 0x00, 0x04, IAP2_ACCESSORY_FIRMWARE_VER,
    //HardwareVersion(硬件版本)--can alter
    0x00, 4 + IAP2_ACCESSORY_HARDWARE_VER_LEN, 0x00, 0x05, IAP2_ACCESSORY_HARDWARE_VER,
};

const u32 iAP2_str_tab[] = {
    (u32)iAP2_message_name, sizeof(iAP2_message_name),
};

#if 0
const IAP_USB_DEVICE_STRING IAP_USB_string_set[] = {
    {(u8 *)IAP_sDeviceDescriptor, 	sizeof(IAP_sDeviceDescriptor)},
    {(u8 *)IAP_sConfigDescriptor, 	sizeof(IAP_sConfigDescriptor)},
    {(u8 *)IAP_iManufacturer, 	sizeof(IAP_iManufacturer)},
    {(u8 *)IAP_iProduct, 		sizeof(IAP_iProduct)},
    {(u8 *)IAP_iSerialNumber, 	sizeof(IAP_iSerialNumber)},
};
#endif

#endif /* TCFG_USB_APPLE_DOCK_EN */


