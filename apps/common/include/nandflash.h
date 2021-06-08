#ifndef _NANDFLASH_H
#define _NANDFLASH_H

#include "device/device.h"
#include "ioctl_cmds.h"
#include "asm/spi.h"
#include "printf.h"
#include "gpio.h"
#include "device_drive.h"
#include "malloc.h"


#define GD_WRITE_ENABLE	        0x06
#define GD_WRITE_DISABLE	    0x04
#define GD_BLOCK_ERASE		    0xD8
#define GD_READ_ID		        0x9F
/*for features*/
#define GD_GET_FEATURES         0x0F
#define GD_SET_FEATURES         0x1F
//<features addr
#define GD_FEATURES_PROTECT     0xA0
#define GD_FEATURES             0xB0
#define GD_GET_STATUS           0xC0
/*for write*/
#define GD_PROGRAM_LOAD	        0x02
#define GD_PROGRAM_EXECUTE      0x10
#define GD_PROGRAM_LOAD_RANDOM_DATA     0x84
//for write x4
#define GD_PROGRAM_LOAD_X4	        0x32
/*for read*/
#define GD_PAGE_READ_CACHE		0x13
#define GD_READ_FROM_CACHE		0x03
//for read x2
#define GD_READ_FROM_CACHE_X2		0x3B
//for read x4
#define GD_READ_FROM_CACHE_X4		0x6B
#define GD_PROGRAM_LOAD_RANDOM_DATA_X4  0x32

#define NAND_OTP_PRT    BIT(7)
#define NAND_OTP_EN     BIT(6)
#define NAND_ECC_EN     BIT(4)
#define NAND_X4_EN      BIT(0)

#define NAND_STATUS_OIP      BIT(0)
#define NAND_STATUS_WEL      BIT(1)
#define NAND_STATUS_E_FAIL      BIT(2)
#define NAND_STATUS_P_FAIL      BIT(3)
#define NAND_STATUS_ECCS        (0x30)

struct nandflash_dev_platform_data {
    int spi_hw_num;         //只支持SPI1或SPI2
    u32 spi_cs_port;        //cs的引脚
    u32 spi_read_width;     //flash读数据的线宽
    const struct spi_platform_data *spi_pdata;
    u32 start_addr;         //分区起始地址
    u32 size;               //分区大小，若只有1个分区，则这个参数可以忽略
};

#define NANDFLASH_DEV_PLATFORM_DATA_BEGIN(data) \
	const struct nandflash_dev_platform_data data = {

#define NANDFLASH_DEV_PLATFORM_DATA_END()  \
};


extern const struct device_operations nandflash_dev_ops;

#endif

