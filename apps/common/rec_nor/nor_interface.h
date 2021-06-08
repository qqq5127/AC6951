#ifndef _NOR_INTERFACE_H_
#define _NOR_INTERFACE_H_

#include "typedef.h"
#include "system/includes.h"
#define NORFS_DATA_LEN  16
#define FLASH_PAGE_SIZE 256

//文件索引
typedef struct __NOR_RECF_INDEX_INFO {
    u16 index;  //文件索引号
    u16 sector; //文件所在扇区
} NOR_RECF_INDEX_INFO;

//文件系统句柄
typedef struct __NOR_RECFILESYSTEM {
    NOR_RECF_INDEX_INFO index;
    u8 buf[FLASH_PAGE_SIZE];
    u16 total_file;
    u16 first_sector;
    u16 last_sector;
    u8 sector_size;
    void *device;
    void (*eraser)(void *device, u32 address);
    s32(*read)(void *device, u8 *buf, u32 addr, u32 len);
    s32(*write)(void *device, u8 *buf, u32 addr, u32 len);
} NOR_RECFILESYSTEM, *NOR_PRECFILESYSTEM ;

//文件句柄
typedef struct __NOR_REC_FILE {
    NOR_RECF_INDEX_INFO index;
    NOR_RECFILESYSTEM *pfs;
    u32 addr;
    char priv_data[NORFS_DATA_LEN];
    u32 len;
    u32 w_len;
    u32 rw_p;
    u16 sr;
} NOR_REC_FILE;

int get_rec_capacity(FILE *_file);
void clear_norfs_allfile(struct imount *mt);
void nor_get_data_info(FILE *_file, u32 *s_addr, u32 *part_size);

#endif
