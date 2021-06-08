#include "nandflash.h"
#include "timer.h"
#include "app_config.h"
#include "asm/clock.h"

#if TCFG_NANDFLASH_DEV_ENABLE

#undef LOG_TAG_CONST
#define LOG_TAG     "[FLASH]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#define NAND_FLASH_TIMEOUT			500
#define NAND_PAGE_SIZE 					2048
#define MAX_NANDFLASH_PART_NUM      4
#define NAND_BLOCK_SIZE    			0x20000

struct nandflash_partition {
    const char *name;
    u32 start_addr;
    u32 size;
    struct device device;
};

static struct nandflash_partition nor_part[MAX_NANDFLASH_PART_NUM];

struct nandflash_info {
    u32 flash_id;
    int spi_num;
    int spi_err;
    u8 spi_cs_io;
    u8 spi_r_width;
    u8 part_num;
    u8 open_cnt;
    struct nandflash_partition *const part_list;
    OS_MUTEX mutex;
    u32 max_end_addr;
};

static struct nandflash_info _nandflash = {
    .spi_num = (int) - 1,
    .part_list = nor_part,
};

int nand_flash_read(u32 addr, u8 *buf, u32 len);
int nand_flash_erase(u32 addr);


#define spi_cs_init() \
    do { \
        gpio_set_die(_nandflash.spi_cs_io, 1); \
        gpio_set_direction(_nandflash.spi_cs_io, 0); \
        gpio_write(_nandflash.spi_cs_io, 1); \
    } while (0)

#define spi_cs_uninit() \
    do { \
        gpio_set_die(_nandflash.spi_cs_io, 0); \
        gpio_set_direction(_nandflash.spi_cs_io, 1); \
        gpio_set_pull_up(_nandflash.spi_cs_io, 0); \
        gpio_set_pull_down(_nandflash.spi_cs_io, 0); \
    } while (0)
#define spi_cs_h()                  gpio_write(_nandflash.spi_cs_io, 1)
#define spi_cs_l()                  gpio_write(_nandflash.spi_cs_io, 0)
#define spi_read_byte()             spi_recv_byte(_nandflash.spi_num, &_nandflash.spi_err)
#define spi_write_byte(x)           spi_send_byte(_nandflash.spi_num, x)
#define spi_dma_read(x, y)          spi_dma_recv(_nandflash.spi_num, x, y)
#define spi_dma_write(x, y)         spi_dma_send(_nandflash.spi_num, x, y)
#define spi_set_width(x)            spi_set_bit_mode(_nandflash.spi_num, x)

static struct nandflash_partition *nandflash_find_part(const char *name)
{
    struct nandflash_partition *part = NULL;
    u32 idx;
    for (idx = 0; idx < MAX_NANDFLASH_PART_NUM; idx++) {
        part = &_nandflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if (!strcmp(part->name, name)) {
            return part;
        }
    }
    return NULL;
}

static struct nandflash_partition *nandflash_new_part(const char *name, u32 addr, u32 size)
{
    struct nandflash_partition *part;
    u32 idx;
    for (idx = 0; idx < MAX_NANDFLASH_PART_NUM; idx++) {
        part = &_nandflash.part_list[idx];
        if (part->name == NULL) {
            break;
        }
    }
    if (part->name != NULL) {
        log_error("create nandflash part fail\n");
        return NULL;
    }
    memset(part, 0, sizeof(*part));
    part->name = name;
    part->start_addr = addr;
    part->size = size;
    if (part->start_addr + part->size > _nandflash.max_end_addr) {
        _nandflash.max_end_addr = part->start_addr + part->size;
    }
    _nandflash.part_num++;
    return part;
}

static void nandflash_delete_part(const char *name)
{
    struct nandflash_partition *part;
    u32 idx;
    for (idx = 0; idx < MAX_NANDFLASH_PART_NUM; idx++) {
        part = &_nandflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if (!strcmp(part->name, name)) {
            part->name = NULL;
            _nandflash.part_num--;
        }
    }
}

static int nandflash_verify_part(struct nandflash_partition *p)
{
    struct nandflash_partition *part = NULL;
    u32 idx;
    for (idx = 0; idx < MAX_NANDFLASH_PART_NUM; idx++) {
        part = &_nandflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if ((p->start_addr >= part->start_addr) && (p->start_addr < part->start_addr + part->size)) {
            if (strcmp(p->name, part->name) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

u16 nand_flash_read_id()
{
    u16 id;
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(0xff); //命令头
    spi_cs_h();//SPI_CS(1);
    os_time_dly(1);
    spi_cs_l();//SPI_CS(0)
    spi_write_byte(GD_READ_ID);
    spi_write_byte(0x0);
    id = spi_read_byte();
    id <<= 8;
    id |= spi_read_byte();
    spi_cs_h();//SPI_CS(1);yy
    return id;
}

static void nand_write_enable()
{
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_WRITE_ENABLE);
    spi_cs_h();//SPI_CS(1);
}

static void nand_write_disable()
{
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_WRITE_DISABLE);
    spi_cs_h();//SPI_CS(1);
}

static void nand_set_features(u8 cmd, u8 dat)
{
    nand_write_enable();
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_SET_FEATURES); //命令头
    spi_write_byte(cmd);             //发送protection寄存器地址
    spi_write_byte(dat);             //需要设置的数据
    spi_cs_h();;//SPI_CS(1);
    nand_write_disable();
}

static u8 nand_get_features(u8 cmd)
{
    spi_cs_l();
    spi_write_byte(GD_GET_FEATURES); //命令头
    spi_write_byte(cmd);             //发送protection寄存器地址
    cmd = spi_read_byte();
    spi_cs_h();
    return cmd;
}

static u8 nand_flash_wait_ok(u32 timeout)
{
    u8 sta;
    do {
        sta = nand_get_features(GD_GET_STATUS);
        if ((sta & NAND_STATUS_OIP) == 0) {
            break;
        }
#if 0
        if (sta & NAND_STATUS_OIP) { //ing
            continue;
        }
        if (sta & NAND_STATUS_E_FAIL) { //erase fail
            break;
        }
        if ((sta & NAND_STATUS_ECCS) == 0x20) { //ecc error
            break;
        }
#endif
    } while (timeout--);
    if (timeout == 0) {
        puts("nand_flash_wait_ok timeout!\r\n");
        sta = -1;
    }
    return 0;
}


void nand_flash_reset(void)
{
    spi_cs_l();
    spi_write_byte(0xff); //命令头
    spi_cs_h();
}

static u16 block_page_get(u32 addr, u32 *cache_addr)
{
    u32  page = 0;
    u32 block = 0, bp_mix = 0;

    //<地址超1页范围
    if (addr >= NAND_PAGE_SIZE) {
        if (addr >= NAND_BLOCK_SIZE) {
            while (addr >= NAND_BLOCK_SIZE) {
                block++;
                addr -= NAND_BLOCK_SIZE;
            }
            goto _page_count;
        } else {
            goto _page_count;
        }
    }
    //<地址不超1页范围
    else {
        goto _end_count;
    }

_page_count:
    while (addr >= NAND_PAGE_SIZE) {
        page++;
        addr -= NAND_PAGE_SIZE;
    }

_end_count:
    *cache_addr = addr;
    bp_mix = (block << 6) | page; //16位构成里高8位为block号码，低8位里高两位依然为block，低6位为page号码
    //printf_u16(g_real_page);
    //printf_u16(bp_mix);
    //printf_u16(block);
    //printf_u16(page);
    return bp_mix;
}

#define block_change(x) (x)

int nand_flash_erase(u32 address)
{
    u32 bp_mix = 0;
    u32 cache_addr;
//    puts("erase\n");
    bp_mix = block_page_get(address, &cache_addr);
    nand_write_enable();
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_BLOCK_ERASE);
    spi_write_byte(0xff); //8位dummy bits(值任意)
    spi_write_byte(bp_mix >> 8);
    spi_write_byte(bp_mix);
    spi_cs_h();//SPI_CS(1);
    nand_write_disable();
    nand_flash_wait_ok(NAND_FLASH_TIMEOUT);
    return nand_get_features(GD_GET_STATUS);
}

static void nand_flash_erase_all()
{
    nand_set_features(0xA0, 0x00);
    for (int i = 0; i <= 1024; i++) {
        nand_flash_erase(128000 * i);
    }
    r_printf("nandflash erase all!!!");
}

static void nand_page_read_to_cache(u32 block_page_addr) //PageRead
{
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_PAGE_READ_CACHE);         //PageRead to cache
    spi_write_byte(0xff);  //send the Page/Block Add
    spi_write_byte((u8)((block_page_addr) >> 8));
    spi_write_byte((u8)block_page_addr);
    spi_cs_h();//SPI_CS(1);
    nand_flash_wait_ok(NAND_FLASH_TIMEOUT);
}

static void nand_read_from_cache(u8 *buf, u32 cache_addr, u32 len) //ReadCache
{
    /* cache_addr |= 0x4000; */
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_READ_FROM_CACHE);     //PageRead to cache
    spi_write_byte((u8)((cache_addr) >> 8));
    spi_write_byte((u8)cache_addr);
    spi_write_byte(0xFF);  //send 1byte dummy clk
    spi_dma_read(buf, len);
    spi_cs_h();//SPI_CS(1);
}

#if 1
static void nand_program_load(u8 *buf, u32 cache_addr, u32 len)
{
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_PROGRAM_LOAD);         //PageLoad to cache ,change the data
    spi_write_byte((u8)((cache_addr) >> 8));
    spi_write_byte((u8)cache_addr);
    spi_dma_write(buf, len); //将数据放到总线上
    spi_cs_h();//SPI_CS(1);
}

static int nand_program_excute(u32 block_page_addr)
{
    int reg;
    nand_write_enable();
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_PROGRAM_EXECUTE);
    spi_write_byte(0xff);  //send the Page/Block Add
    spi_write_byte((u8)((block_page_addr) >> 8));
    spi_write_byte((u8)block_page_addr);
    spi_cs_h();//SPI_CS(1);
    nand_write_disable();
    reg = nand_flash_wait_ok(NAND_FLASH_TIMEOUT);
    return reg;
}


static int nand_write(u32 addr, u16 len, u8 *buf)
{
    int reg;
    u32 bp_mix = 0;
    u32 cache_addr;

    bp_mix = block_page_get(addr, &cache_addr);
    bp_mix = block_change(bp_mix);
    //printf_u16(cache_addr);
    nand_program_load(buf, cache_addr, len);
    reg = nand_program_excute(bp_mix);
    return reg;
}/**/


static void nand_program_load_random_data(u8 *buf, u16 cache_addr, u16 len)
{
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_PROGRAM_LOAD_RANDOM_DATA);
    spi_write_byte((u8)((cache_addr) >> 8));
    spi_write_byte((u8)cache_addr);
    spi_dma_write(buf, len); //将数据放到总线上
    spi_cs_h();//SPI_CS(1);
    nand_flash_wait_ok(NAND_FLASH_TIMEOUT);
}

int nand_internal_data_move(u32 addr, u16 len, u8 *buf)
{
    u32 bp_mix = 0;
    u32 cache_addr;

    bp_mix = block_page_get(addr, &cache_addr);
    bp_mix = block_change(bp_mix);

    //<1、PAGE READ to cache
    nand_page_read_to_cache(bp_mix);
    //<2、PROGRAM LOAD RANDOM DATA
    nand_program_load_random_data(buf, cache_addr, len);
    //<3、WRITE ENABLE
    //<4、PROGRAM EXECUTE
    nand_program_excute(bp_mix);
    //<5、GET FEATURE
    return nand_get_features(GD_GET_STATUS);
}

int nand_flash_write(u32 offset, u8 *buf,  u16 len)
{
    /* printf("%s() %x l: %x @:%x \n",__func__,buf,len,offset); */
    int reg;
    int _len = len;
    u8 *_buf = (u8 *)buf;
    os_mutex_pend(&_nandflash.mutex, 0);
    u32 first_page_len = NAND_PAGE_SIZE - (offset % NAND_PAGE_SIZE);
    /* printf("first_page_len %x %x \n", first_page_len, offset % NAND_PAGE_SIZE); */
    first_page_len = _len > first_page_len ? first_page_len : _len;
    reg = nand_write(offset, first_page_len, _buf);
    if (reg) {
        goto __exit;
    }
    _len -= first_page_len;
    _buf += first_page_len;
    offset += first_page_len;
    while (_len) {
        u32 cnt = _len > NAND_PAGE_SIZE ? NAND_PAGE_SIZE : _len;
        reg = nand_write(offset, cnt, _buf);
        if (reg) {
            goto __exit;
        }
        _len -= cnt;
        offset += cnt;
        _buf += cnt;
    }
__exit:
    os_mutex_post(&_nandflash.mutex);
    return 0;
}

#endif

static void nand_read(u32 addr, u32 len, u8 *buf)
{
    u32 bp_mix = 0;
    u32 cache_addr;
    bp_mix = block_page_get(addr, &cache_addr);
    bp_mix = block_change(bp_mix);
    nand_page_read_to_cache(bp_mix);
    nand_read_from_cache(buf, cache_addr, len);
}


int nand_flash_read(u32 offset, u8 *buf,  u32 len)
{
    /* printf("%s() %x l: %x @:%x \n",__func__,buf,len,offset); */
    int reg = 0;
    int _len = len;
    u8 *_buf = (u8 *)buf;
    os_mutex_pend(&_nandflash.mutex, 0);
    u32 first_page_len = NAND_PAGE_SIZE - (offset % NAND_PAGE_SIZE);
    /* printf("first_page_len %x %x \n", first_page_len, offset % NAND_PAGE_SIZE); */
    first_page_len = _len > first_page_len ? first_page_len : _len;
    nand_read(offset, first_page_len, _buf);
    _len -= first_page_len;
    _buf += first_page_len;
    offset += first_page_len;
    while (_len) {
        u32 cnt = _len > NAND_PAGE_SIZE ? NAND_PAGE_SIZE : _len;
        nand_read(offset, cnt, _buf);
        _len -= cnt;
        offset += cnt;
        _buf += cnt;
    }
    os_mutex_post(&_nandflash.mutex);
    return reg;
}


int _nandflash_init(const char *name, struct nandflash_dev_platform_data *pdata)
{
    log_info("nandflash_init ! %x %x", pdata->spi_cs_port, pdata->spi_read_width);
    if (_nandflash.spi_num == (int) - 1) {
        _nandflash.spi_num = pdata->spi_hw_num;
        _nandflash.spi_cs_io = pdata->spi_cs_port;
        _nandflash.spi_r_width = pdata->spi_read_width;
        _nandflash.flash_id = 0;
        os_mutex_create(&_nandflash.mutex);
        _nandflash.max_end_addr = 0;
        _nandflash.part_num = 0;
    }
    ASSERT(_nandflash.spi_num == pdata->spi_hw_num);
    ASSERT(_nandflash.spi_cs_io == pdata->spi_cs_port);
    ASSERT(_nandflash.spi_r_width == pdata->spi_read_width);
    struct nandflash_partition *part;
    part = nandflash_find_part(name);
    if (!part) {
        part = nandflash_new_part(name, pdata->start_addr, pdata->size);
        ASSERT(part, "not enough nandflash partition memory in array\n");
        ASSERT(nandflash_verify_part(part) == 0, "nandflash partition %s overlaps\n", name);
        log_info("nandflash new partition %s\n", part->name);
    } else {
        ASSERT(0, "nandflash partition name already exists\n");
    }
    return 0;
}

static void clock_critical_enter()
{

}
static void clock_critical_exit()
{
    if (!(_nandflash.flash_id == 0 || _nandflash.flash_id == 0xffff)) {
        spi_set_baud(_nandflash.spi_num, spi_get_baud(_nandflash.spi_num));
    }
}
CLOCK_CRITICAL_HANDLE_REG(spi_nandflash, clock_critical_enter, clock_critical_exit);

int _nandflash_open(void *arg)
{
    int reg = 0;
    os_mutex_pend(&_nandflash.mutex, 0);
    log_info("nandflash open\n");
    if (!_nandflash.open_cnt) {
        spi_cs_init();
        spi_open(_nandflash.spi_num);
        _nandflash.flash_id = nand_flash_read_id();
        log_info("nandflash_read_id: 0x%x\n", _nandflash.flash_id);
        if ((_nandflash.flash_id == 0) || (_nandflash.flash_id == 0xffff)) {
            log_error("read nandflash id error !\n");
            reg = -ENODEV;
            goto __exit;
        }
        log_info("nandflash open success !\n");
    }
    if (_nandflash.flash_id == 0 || _nandflash.flash_id == 0xffff)  {
        log_error("re-open nandflash id error !\n");
        reg = -EFAULT;
        goto __exit;
    }
    _nandflash.open_cnt++;
    nand_flash_erase_all();
__exit:
    os_mutex_post(&_nandflash.mutex);
    return reg;
}

int _nandflash_close(void)
{
    os_mutex_pend(&_nandflash.mutex, 0);
    log_info("nandflash close\n");
    if (_nandflash.open_cnt) {
        _nandflash.open_cnt--;
    }
    if (!_nandflash.open_cnt) {
        spi_close(_nandflash.spi_num);
        spi_cs_uninit();

        log_info("nandflash close done\n");
    }
    os_mutex_post(&_nandflash.mutex);
    return 0;
}


int _nandflash_ioctl(u32 cmd, u32 arg, u32 unit, void *_part)
{
    int reg = 0;
    struct nandflash_partition *part = _part;
    os_mutex_pend(&_nandflash.mutex, 0);
    switch (cmd) {
    case IOCTL_GET_STATUS:
        *(u32 *)arg = 1;
        break;
    case IOCTL_GET_ID:
        *((u32 *)arg) = _nandflash.flash_id;
        break;
    case IOCTL_GET_BLOCK_SIZE:
        *(u32 *)arg = 512;
        break;
    case IOCTL_ERASE_BLOCK:
        reg = nand_flash_erase(arg);
        break;
    case IOCTL_GET_CAPACITY:
        *(u32 *)arg = 128 * 1024 / unit;
        break;
    case IOCTL_FLUSH:
        break;
    case IOCTL_CMD_RESUME:
        break;
    case IOCTL_CMD_SUSPEND:
        break;
    default:
        reg = -EINVAL;
        break;
    }
__exit:
    os_mutex_post(&_nandflash.mutex);
    return reg;
}


/*************************************************************************************
 *                                  挂钩 device_api
 ************************************************************************************/

static int nandflash_dev_init(const struct dev_node *node, void *arg)
{
    struct nandflash_dev_platform_data *pdata = arg;
    return _nandflash_init(node->name, pdata);
}

static int nandflash_dev_open(const char *name, struct device **device, void *arg)
{
    struct nandflash_partition *part;
    part = nandflash_find_part(name);
    if (!part) {
        log_error("no nandflash partition is found\n");
        return -ENODEV;
    }
    *device = &part->device;
    (*device)->private_data = part;
    if (atomic_read(&part->device.ref)) {
        return 0;
    }
    return _nandflash_open(arg);
}
static int nandflash_dev_close(struct device *device)
{
    return _nandflash_close();
}
static int nandflash_dev_read(struct device *device, void *buf, u32 len, u32 offset)
{
    int reg;
    offset = offset * 512;
    len = len * 512;
    /* r_printf("flash read sector = %d, num = %d\n", offset, len); */
    struct nandflash_partition *part;
    part = (struct nandflash_partition *)device->private_data;
    if (!part) {
        log_error("nandflash partition invalid\n");
        return -EFAULT;
    }
    offset += part->start_addr;
    reg = nand_flash_read(offset, buf, len);
    if (reg) {
        r_printf(">>>[r error]:\n");
        len = 0;
    }
    len = len / 512;
    return len;
}
static int nandflash_dev_write(struct device *device, void *buf, u32 len, u32 offset)
{
    /* r_printf("flash write sector = %d, num = %d\n", offset, len); */
    int reg = 0;
    offset = offset * 512;
    len = len * 512;
    struct nandflash_partition *part = device->private_data;
    if (!part) {
        log_error("nandflash partition invalid\n");
        return -EFAULT;
    }
    offset += part->start_addr;
    reg = nand_flash_write(offset, buf, len);
    if (reg) {
        r_printf(">>>[w error]:\n");
        len = 0;
    }
    len = len / 512;
    return len;
}
static bool nandflash_dev_online(const struct dev_node *node)
{
    return 1;
}
static int nandflash_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    struct nandflash_partition *part = device->private_data;
    if (!part) {
        log_error("nandflash partition invalid\n");
        return -EFAULT;
    }
    return _nandflash_ioctl(cmd, arg, 512,  part);
}
const struct device_operations nandflash_dev_ops = {
    .init   = nandflash_dev_init,
    .online = nandflash_dev_online,
    .open   = nandflash_dev_open,
    .read   = nandflash_dev_read,
    .write  = nandflash_dev_write,
    .ioctl  = nandflash_dev_ioctl,
    .close  = nandflash_dev_close,
};
#endif
