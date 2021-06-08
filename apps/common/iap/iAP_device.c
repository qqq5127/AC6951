/*************************************************************/
/** @file:		iAP.c
    @brief:		苹果iAP 协议
    @details:
    @author:	Bingquan Cai
    @date: 		2013-10-11,09:34
    @note:
*/
/*************************************************************/
#include "apple_dock/iAP.h"
#include "apple_dock/iAP2.h"
#include "apple_dock/iAP_device.h"
#include "apple_dock/iAP_iic.h"
/* #include "usb_global_var.h" */
/* #include "usb_driver.h" */
/* #include "usb_slave_scsi.h" */
#include "app_config.h"
#include "init.h"

#if TCFG_USB_APPLE_DOCK_EN

static void *iap_usb_hdl = NULL;

#if USB_MALLOC_ENABLE
#else
static u8 USB_IAP_SEND_BUF[0x400] SEC(.mass_storage) __attribute__((aligned(4)));
static u8 USB_IAP_RECV_BUF[0x200] SEC(.mass_storage) __attribute__((aligned(4)));
#endif

extern u8 MCU_SRAMToUSB_app(void *hdl, u8 *pBuf, u16 uCount);
extern u8 MCU_USBToSRAM_app(void *hdl, u8 *pBuf, u16 uCount);


static void iAP_ZLP(void)
{
    delay(50000);
}

static u8 iAP_bulk_in_process(void)
{
    //iAP_printf_buf(iAP_var_pTxBuf, iAP_var_wLength);
    return MCU_SRAMToUSB_app(iap_usb_hdl, iAP_var_pTxBuf, iAP_var_wLength);
}

static u8 iAP_bulk_out_process(void)
{
    u8 len;

    len = MCU_USBToSRAM_app(iap_usb_hdl, iAP_var_pRxBuf, 0x200);
    iAP_var_wLength = len;
    /* iAP_printf_buf(iAP_var_pRxBuf, len); */
    return len;
}

u8 iAP_transport(u8 type)
{
    u8 res = TRUE;
    if (type & BIT(IAP_TRANSPORT_ZLP)) {
        iAP_ZLP();
    }
    if (type & BIT(IAP_TRANSPORT_IN)) {
        res = iAP_bulk_in_process();
    }
    if (type & BIT(IAP_TRANSPORT_OUT)) {
        u8 len = iAP_bulk_out_process();
        if (type & BIT(IAP_TRANSPORT_OUT_LEN)) {
            return len;
        }
    }
    return res;
}


u8 apple_mfi_link(void *hdl)
{
    u8 res;

    if (MFI_CHIP_OFFLINE == chip_online_status) {
        iAP_deg_str("mfi chip offline\n");
        return NO_MFI_CHIP;
    }
    if (MFI_PROCESS_READY != mfi_pass_status) {
        return mfi_pass_status;
    }

    iap_usb_hdl = hdl;

    //Reset session Tx & Rx buffer
#if USB_MALLOC_ENABLE
    if (iAP_var_pTxBuf == NULL) {
        iAP_var_pTxBuf = malloc(0x400);
    }
    if (iAP_var_pRxBuf == NULL) {
        iAP_var_pRxBuf = malloc(0x200);
    }
#else
    iAP_var_pTxBuf = USB_IAP_SEND_BUF;
    iAP_var_pRxBuf = USB_IAP_RECV_BUF;
#endif

    memset(iAP_var_pTxBuf, 0, 0x400);
    memset(iAP_var_pRxBuf, 0, 0x200);

    /*Detect iAP support protocol type*/
    if (IAP2_VERSIONS == iAP_support()) {
        iAP_deg_str("iAP2 support!\n");
        /* iAP_deg_str("iap2"); */
        /* os_time_dly(2); */
        res = iAP2_link() + IAP2_LINK_ERR;
        if (IAP2_LINK_SUCC == res) {
            mfi_pass_status = MFI_PROCESS_PASS;
        }
        return res;
    }

    return IAP2_LINK_ERR;
}

void apple_mfi_unlink(void)
{
#if USB_MALLOC_ENABLE
    local_irq_disable();
    if (iAP_var_pTxBuf) {
        free(iAP_var_pTxBuf);
        iAP_var_pTxBuf = NULL;
    }
    if (iAP_var_pRxBuf) {
        free(iAP_var_pRxBuf);
        iAP_var_pRxBuf = NULL;
    }
    local_irq_enable();
#endif
}

static int apple_dock_init(void)
{
    apple_link_init();
    apple_mfi_chip_online_check_api();
    return 0;
}
early_initcall(apple_dock_init);

#endif /* TCFG_USB_APPLE_DOCK_EN */

