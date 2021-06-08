#include "product_info_api.h"
#include "syscfg_id.h"
#include "vm.h"
#include "app_config.h"
#include "typedef.h"
#include "system/includes.h"

#define PRODUCT_INFO_API_EN			0  //默认不开启

#if (PRODUCT_INFO_API_EN)
struct __product_info_head {
    s16 crc;
    u16 len;
    const u8 data[];
} __attribute__((packed));

typedef enum _FLASH_ERASER {
    CHIP_ERASER = 0,
    BLOCK_ERASER,
    SECTOR_ERASER,
    PAGE_ERASER,
} FLASH_ERASER;

//*----------------------------------------------------------------------------*/
/**@brief    获取产品信息地址偏移
   @param
   @return	 返回产品信息地址偏移
   @note	 产品信息是在芯片烧写的时候， 使用1拖2或者1拖8进行烧录的
*/
/*----------------------------------------------------------------------------*/
static u32 product_info_get_ptr(void)
{
    u32 flash_capacity = sdfile_get_disk_capacity();
    u32 addr = flash_capacity - 256 + 80;
    printf("flash capacity:%x \n", flash_capacity);
    return sdfile_flash_addr2cpu_addr(addr);
}

//*----------------------------------------------------------------------------*/
/**@brief    产品信息检查
   @param    para:产品信息位置偏移
   @return
   @note	 产品信息是在芯片烧写的时候， 使用1拖2或者1拖8进行烧录的
*/
/*----------------------------------------------------------------------------*/
static int product_info_check(const u8 *para)
{
    struct __product_info_head *head;

    //fill head
    head = (struct __product_info_head *)para;
    if (head->len >= 0xff) {
        printf("product info len err!!\n");
        return -1;
    }

    ///crc check
    u8 *crc_data = (u8 *)(para + sizeof(((struct __product_info_head *)0)->crc));
    u32 crc_len = sizeof(struct __product_info_head)
                  - sizeof(((struct __product_info_head *)0)->crc)/*head crc*/
                  + (head->len)/*content crc,include end character '\0'*/;
    s16 crc_sum = 0;

    crc_sum = CRC16(crc_data, crc_len);

    if (crc_sum != head->crc) {
        ("product info crc error !!! %x %x \n", (u32)crc_sum, (u32)head->crc);
        return -1;
    }

    return 0;
}

//*----------------------------------------------------------------------------*/
/**@brief    获取产品信息接口
   @param    buf:外部传进来的缓存， len:缓存长度
   @return
   @note	 产品信息是在芯片烧写的时候， 使用1拖2或者1拖8进行烧录的
*/
/*----------------------------------------------------------------------------*/
int product_info_prase(u8 *buf, u16 len)
{
    //产品信息内容建议不要超过128
    u8 *data_ptr = (u8 *)product_info_get_ptr();
    int ret = product_info_check((const u8 *)data_ptr);
    if (ret) {
        return -1;
    }

    struct __product_info_head *head = (struct __product_info_head *)data_ptr;
    if (len < head->len) {
        printf("prase buf not enough\n");
        return -1;
    }

    memcpy(buf, data_ptr + sizeof(struct __product_info_head), head->len);
    return 0;
}

//测试代码
#if 0
void product_info_debug(void)
{
    printf("product_info_debug \n");
    u8 tst_data[128] = {0};
    product_info_prase(tst_data, sizeof(tst_data));
    printf("tst_data = %s\n", tst_data);
    put_buf(tst_data, 256);
    //针对获出来的数据结合方案需求进行解析
}
#endif

#endif//PRODUCT_INFO_API_EN

