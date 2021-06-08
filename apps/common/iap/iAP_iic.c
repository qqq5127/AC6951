#include "apple_dock/iAP_iic.h"
#include "apple_dock/iAP.h"
#include "asm/iic_hw.h"
#include "asm/iic_soft.h"
#include "app_config.h"

#if TCFG_USB_APPLE_DOCK_EN

#if 0
static hw_iic_dev iic = 0;
#define iic_init()                       	hw_iic_init(iic)
#define iic_uninit()                     	hw_iic_uninit(iic)
#define iic_start()                      	hw_iic_start(iic)
#define iic_stop()                       	hw_iic_stop(iic)
#define iic_sendbyte(byte)              	hw_iic_tx_byte(iic, byte)
#define iic_revbyte(ack)               		hw_iic_rx_byte(iic, ack)
#else
static soft_iic_dev iic = 0;
#define iic_init()                       	soft_iic_init(iic)
#define iic_uninit()                     	soft_iic_uninit(iic)
#define iic_start()                      	soft_iic_start(iic)
#define iic_stop()                       	soft_iic_stop(iic)
#define iic_sendbyte(byte)              	soft_iic_tx_byte(iic, byte)
#define iic_revbyte(ack)               		soft_iic_rx_byte(iic, ack)
#endif



#define IIC_START_BIT		BIT(0)
#define IIC_STOP_BIT		BIT(1)
#define IIC_ACK_BIT			BIT(2)

u8 iic_wbyte(u8 st, u8 dat)
{
    bool ack_var = 0;
    if (st & IIC_START_BIT) {
        iic_start();
    }
    ack_var = iic_sendbyte(dat);
    if (st & IIC_STOP_BIT) {
        iic_stop();
    }
    /* printf("a=%d,",ack_var); */
    /* return ack_var ? 1 : 0; */
    return ack_var ? 0 : 1;
}

u8 iic_rbyte(u8 st)
{
    u8 iic_dat = 0;
    if (st & IIC_START_BIT) {
        iic_start();
    }
    /* iic_dat = iic_revbyte(st & IIC_ACK_BIT); */
    iic_dat = iic_revbyte(!(st & IIC_ACK_BIT));
    if (st & IIC_STOP_BIT) {
        iic_stop();
    }
    /* printf("d=0x%x,",iic_dat); */
    return iic_dat;
}

static void iic_start_addr(u8 addr)
{
    u8 cnt = 0xff;
    while (--cnt) {
        if (iic_wbyte(IIC_START_BIT, addr)) {
            delay(200);
            continue;
        } else {
            break;
        }
    }
}


void  iAP_iic_write(u8 iic_addr, u8 iic_dat)
{
    /* iic_write_byte(WRITE_ADDR, iic_addr, iic_dat); */
    iic_start_addr(WRITE_ADDR);      		//Send the I2C write address of the CP
    iic_wbyte(0, iic_addr); //
    iic_wbyte(IIC_STOP_BIT, iic_dat); //
}

void  iAP_iic_writen(u8 iic_addr, u8 *iic_dat, u8 len)
{
    /* iic_write_buf(WRITE_ADDR, iic_addr, iic_dat, len); */
    iic_start_addr(WRITE_ADDR);      		//Send the I2C write address of the CP
    iic_wbyte(0, iic_addr); //

    //deg_string("\niic_dat=");
    for (; len > 1; len--) {
        //iap_put_u8hex(*iic_dat);
        iic_wbyte(0, *iic_dat++); //写数据
    }
    iic_wbyte(IIC_STOP_BIT, *iic_dat); //写数据
}

u8 iAP_iic_read(u8 iic_addr)
{
    /* return iic_read_byte(WRITE_ADDR, READ_ADDR, iic_addr); */
    u8	iic_dat;

    iic_start_addr(WRITE_ADDR);      		//Send the I2C write address of the CP
    iic_wbyte(IIC_STOP_BIT, iic_addr); //
    iic_start_addr(READ_ADDR); 	//Send the I2C read address of the CP
    iic_dat = iic_rbyte(IIC_STOP_BIT | IIC_ACK_BIT); //
    return iic_dat;
}

void iAP_iic_readn(u8 iic_addr, u8 *iic_dat, u8 len)
{
    /* iic_read_buf(WRITE_ADDR, READ_ADDR, iic_addr, iic_dat, len); */
    iic_start_addr(WRITE_ADDR);      		//Send the I2C write address of the CP
    iic_wbyte(IIC_STOP_BIT, iic_addr); //
    iic_start_addr(READ_ADDR); 	//Send the I2C read address of the CP

    /* delay8(20); */

    for (; len > 1; len--) {
        *iic_dat++ = iic_rbyte(0); //
    }
    *iic_dat = iic_rbyte(IIC_STOP_BIT | IIC_ACK_BIT); //

#if (IAP_CRACK_CHIP==IAP_CRACK_CP30)
    if (iic_addr == CHALLENGE_DATA_LENGTH) {
        *iic_dat = 32;
    }
#endif
}


//Authentication Coprocessor 2.0C register map
#define VALUE_DEVICE_VERSION							0x05
#define VALUE_FIRMWARE_VERSION							0x01
#define VALUE_AUTHENTICATION_PROTOCOL_MAJOR_VERSION		0x02
#define VALUE_AUTHENTICATION_PROTOCOL_MINOR_VERSION		0x00
#define VALUE_DEVICE_IDL								0x0000
#define VALUE_DEVICE_IDH								0x0002

#define DEVICE_ID_SIZE		4

u8 iap_iic_id[4];
const u8 iap_iic_chip_id[4] = {
#if (IAP_CRACK_CHIP==IAP_CRACK_CP30)
    0x00, 0x00, 0x03, 0x00
#else
    0x00, 0x00, 0x02, 0x00
#endif
};

u8 apple_mfi_chip_online_check_api(void)
{
    iic_init();
    os_time_dly(2);

    chip_online_status = MFI_CHIP_OFFLINE;

#if 0
    if (VALUE_DEVICE_VERSION != iAP_iic_read(ADDR_DEVICE_VERSION)) {
        return MFI_DV_ERROR;
    }
    if (VALUE_FIRMWARE_VERSION != iAP_iic_read(ADDR_FIRMWARE_VERSION)) {
        return MFI_FV_ERROR;
    }
    if (VALUE_AUTHENTICATION_PROTOCOL_MAJOR_VERSION != iAP_iic_read(ADDR_AUTHENTICATION_PROTOCOL_MAJOR_VERSION)) {
        return MFI_AP_MAJOR_V_ERROR;
    }
    if (VALUE_AUTHENTICATION_PROTOCOL_MINOR_VERSION != iAP_iic_read(ADDR_AUTHENTICATION_PROTOCOL_MINOR_VERSION)) {
        return MFI_AP_MINOR_V_ERROR;
    }
#endif
#if 0
    u16 id_l, id_h;
    my_memset(iap_iic_id, 0, sizeof(iap_iic_id));
    iAP_iic_readn(ADDR_DEVICE_ID, (u8 *)(iap_iic_id), DEVICE_ID_SIZE);
    /* id = SW32(id); */

    id_l = iap_iic_id[1];
    id_l = (id_l << 8) | iap_iic_id[0];
    id_h = iap_iic_id[3];
    id_h = (id_h << 8) | iap_iic_id[2];

    iAP_put_u16hex(id_l);
    iAP_put_u16hex(id_h);

    if ((VALUE_DEVICE_IDL != id_l) || (VALUE_DEVICE_IDH != id_h)) {
        /* iAP_deg_puts("-err-"); */
        return MFI_DEVICE_ID_ERROR;
    } else {
        /* iAP_deg_puts("-ok-"); */
        /* iAP_printf_buf((u8 *)id, 4); */
    }
#else
__gain:
    memset(iap_iic_id, 0, sizeof(iap_iic_id));
    iAP_iic_readn(ADDR_DEVICE_ID, (u8 *)(iap_iic_id), DEVICE_ID_SIZE);
    /* id = SW32(id); */

    printf("-iap id \n");
    put_buf(iap_iic_id, sizeof(iap_iic_id));
    put_buf((u8 *)iap_iic_chip_id, sizeof(iap_iic_chip_id));
    printf("cmp=0x%x \n", memcmp(iap_iic_id, (u8 *)iap_iic_chip_id, 4));
    /* if ((iap_iic_id[0] != 0x00) */
    /* || (iap_iic_id[1] != 0x00) */
    /* || (iap_iic_id[2] != 0x02) */
    /* || (iap_iic_id[3] != 0x00) */
    /* ) { */
    if (memcmp(iap_iic_id, (u8 *)iap_iic_chip_id, 4)) {
        /* iAP_deg_puts("-err-"); */
        printf("--err \n");
        os_time_dly(1);
        /* goto __gain; */
        return MFI_DEVICE_ID_ERROR;
    } else {
        printf("--ok \n");
        /* iAP_deg_puts("-ok-"); */
        /* iAP_printf_buf((u8 *)id, 4); */
    }
#endif

    chip_online_status = MFI_CHIP_ONLINE;

    return MFI_CHIP_ONLINE;
}

#endif /* TCFG_USB_APPLE_DOCK_EN */

