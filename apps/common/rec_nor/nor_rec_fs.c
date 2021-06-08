#include "os/os_api.h"
#include "asm/crc16.h"
#include "stdlib.h"
#include "fs/fs.h"
#include "system/includes.h"
#include "nor_interface.h"
#include "nor_rec_fs.h"

#define RECFILEHEAD  SDFILE_FILE_HEAD

#define NO_DEBUG_EN
#ifdef NO_DEBUG_EN
#undef r_printf
#undef y_printf
#define r_printf(...)
#define y_printf(...)
#endif

OS_MUTEX recfs_mutex;             ///<互斥信号量
void recfs_mutex_init(void)       ///<信号量初始化
{
    static u8 init = 0;
    if (init) {
        return;
    }
    init = 1;
    os_mutex_create(&recfs_mutex);
}
void recfs_mutex_enter(void)      ///<申请信号量
{
    recfs_mutex_init();
    os_mutex_pend(&recfs_mutex, 0);
}
void recfs_mutex_exit(void)       ///<释放信号量
{
    os_mutex_post(&recfs_mutex);
}

/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理读函数
   @param   addr:读取的地址，字节单位。
   @param   len:读取的长度，字节单位。
   @param   buf:读取的目标BUFF。
   @return  u16:读取的长度
   @author  liujie
   @note    u8 recpfs_read(u32 addr,u8 *buf,u16 len)
*/
/*----------------------------------------------------------------------------*/
u16 recpfs_read(NOR_RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len)
{
    putchar('r');
    recfs_mutex_enter();
    pfs->read(pfs->device, buf, addr, len);
    recfs_mutex_exit();
    return len;
}

/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理写函数
   @param   addr:要写入设备的地址，字节单位。
   @param   len:需要写入的长度，字节单位。
   @param   buf:数据来源BUFF。
   @return  u16:写入的长度
   @author  liujie
   @note    u16 recpfs_write(u32 addr,u8 *buf,u16 len)
*/
/*----------------------------------------------------------------------------*/
u16 recpfs_write(NOR_RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len)
{
    putchar('w');
    recfs_mutex_enter();
    pfs->write(pfs->device, buf, addr, len);
    recfs_mutex_exit();
    return len;
}


/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理写函数(对齐)
   @param   pfs:文件系统句柄
   @param   addr:要写入设备的地址，字节单位。
   @param   len:需要写入的长度，字节单位。
   @param   buf:数据来源BUFF。
   @return  u16:写入的长度
   @author  liujie
   @note    u16 recpfs_write_align(NOR_RECFILESYSTEM *pfs, u32 addr,u8 *buf,u16 len)
*/
/*----------------------------------------------------------------------------*/
u16 recpfs_write_align(NOR_RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len)
{
    u32 align_addr = addr % FLASH_PAGE_SIZE;
    u16 rlen = FLASH_PAGE_SIZE - align_addr;
    u16 total_len = len;
    u8 *tmpbuf = pfs->buf;

    if (len == 90) {
        align_addr = 0;
    }

    addr -= align_addr;

    if (align_addr) {
        memset(tmpbuf, 0xff, FLASH_PAGE_SIZE);
        if (rlen > len) {
            rlen = len;
        }
        memcpy(tmpbuf + align_addr, buf, rlen);
        putchar('C');
        recpfs_write(pfs, addr, tmpbuf, FLASH_PAGE_SIZE);
        memset(tmpbuf, 0, FLASH_PAGE_SIZE);
        len -= rlen;
        buf += rlen;
        addr += FLASH_PAGE_SIZE;
    }

    while (len >= FLASH_PAGE_SIZE) {
        putchar('A');
        memset(tmpbuf, 0xff, FLASH_PAGE_SIZE);
        memcpy(tmpbuf, buf, FLASH_PAGE_SIZE);
        recpfs_write(pfs, addr, tmpbuf, FLASH_PAGE_SIZE);
        len -= FLASH_PAGE_SIZE;
        buf += FLASH_PAGE_SIZE;
        addr += FLASH_PAGE_SIZE;
    }

    if (len) {
        putchar('B');
        memset(tmpbuf, 0xff, FLASH_PAGE_SIZE);
        memcpy(tmpbuf, buf, len);
        recpfs_write(pfs, addr, tmpbuf, len);
    }
    return total_len;
}

/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理擦除函数
   @param   pfs:要写入设备的地址，字节单位。
   @param   start_sector:需要擦除的第一个扇区。
   @param   end_sector:需要擦除的最后一个扇区。
   @return  u16:擦除的扇区数目
   @author  liujie
   @note    u16 recpfs_erase_sector(NOR_RECFILESYSTEM *pfs,u16 start_sector,u16 end_sector)
*/
/*----------------------------------------------------------------------------*/
u16 recpfs_erase_sector(NOR_RECFILESYSTEM *pfs, u16 start_sector, u16 end_sector)
{
    if ((start_sector < pfs->first_sector) || (start_sector > pfs->last_sector)) {
        return 0 ;
    }
    if ((end_sector < pfs->first_sector) || (end_sector > pfs->last_sector)) {
        return 0 ;
    }
    u16 sector_cnt = 0;
    /* r_printf(">>>[test]:start_sector = %d, end_sector =%d\n", start_sector , end_sector); */
    while (1) {
        recfs_mutex_enter();
        pfs->eraser(pfs->device, ((u32)start_sector << pfs->sector_size));
        recfs_mutex_exit();
        sector_cnt++;
        if (start_sector == end_sector) {
            break;
        }
        start_sector++;
        if (start_sector > pfs->last_sector) {
            start_sector = pfs->first_sector;
        }
    }
    return sector_cnt;
}


/*----------------------------------------------------------------------------*/
/**@brief   录音文件seek函数
   @param   pfile:文件句柄
   @param   type:seek的格式
   @param   offsize:seek的偏移量。
   @return  u8:返回值
   @author  liujie
   @note    u8 recfile_seek (REC_FILE *pfile, u8 type, u32 offsize)
*/
/*----------------------------------------------------------------------------*/
u8 recfile_seek(NOR_REC_FILE *pfile, u8 type, int offsize)
{
    u32 file_len;
    if (NULL == pfile) {
        return 0;
    }
    /* y_printf(">>>[test]:pfile->len = %d,type = %d, offsize = %d\n",pfile->len, type, offsize); */
    file_len = pfile->len;//(u32)pfile->index.sector << pfile->pfs->sector_size;
    if (NOR_FS_SEEK_SET == type) {
        if (abs(offsize) >= file_len) {
            return REC_FILE_END;
        }
        pfile->rw_p = offsize + sizeof(RECFILEHEAD);
    } else {
        offsize += pfile->rw_p;
        if (offsize >= file_len) {
            return REC_FILE_END;
        }
        pfile->rw_p += offsize;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief   录音文件读函数
   @param   pfile:文件句柄
   @param   buff:数据BUFF
   @param   btr:需要读取的数据长度，字节单位
   @return  u16:读取回来的长度
   @author  liujie
   @note    u16 recfile_read(REC_FILE _xdata *pfile ,u8 _xdata *buff , u16 btr)
*/
/*----------------------------------------------------------------------------*/
u16 recfile_read(NOR_REC_FILE *pfile, u8 *buff, u16 btr)
{
    u32 file_len, addr;
    u16 read_len = btr;
    u16 len, len0;
    u32 max_addr, min_addr;
    max_addr = ((u32)(pfile->pfs->last_sector + 1) << pfile->pfs->sector_size) ;
    min_addr = ((u32)(pfile->pfs->first_sector) << pfile->pfs->sector_size) ;
    file_len = (u32)pfile->len;
    if ((u32)(btr + pfile->rw_p) >= file_len) {
        if (pfile->rw_p >= file_len) {
            return 0;
        }
        read_len = file_len - pfile->rw_p;
    }
    addr = pfile->rw_p + ((u32)pfile->index.sector << pfile->pfs->sector_size);
    len0 = read_len;
    /* y_printf(">>>[test]:r_addr = %d,max_addr =%d, min_addr=%d, pfile->rw_p =%d\n", addr, max_addr, min_addr, pfile->rw_p); */
    //y_printf(">>>[test]:r_len0 = %d,btr = %d, file_len = %d\n",len0, btr, file_len);
    while (len0) {
        if (addr >= max_addr) {
            addr = addr - max_addr + min_addr;
        }
        if ((addr + len0) > max_addr) {
            len = max_addr - addr;
        } else {
            len = len0;
        }
        /* y_printf(">>>[test]2222r_addr = %d,max_addr =%d, min_addr=%d, pfile->rw_p =%d\n",addr, max_addr, min_addr, pfile->rw_p); */
        recpfs_read(pfile->pfs, addr, buff, len);
        pfile->rw_p += len;
        len0 -= len;
        buff += len;
        addr += len;
    }
    return read_len;
}

/*----------------------------------------------------------------------------*/
/**@brief   录音文件写函数
   @param   pfile:文件句柄
   @param   buff:数据BUFF
   @param   btr:需要写入的数据长度，字节单位
   @return  u16:写入的长度
   @author  liujie
   @note    u16 recf_write(NOR_REC_FILE *pfile,u8 *buff,u16 btw)
*/
/*----------------------------------------------------------------------------*/
u16 recfile_write(NOR_REC_FILE *pfile, u8 *buff, u16 btw)
{
    u32 addr;
    u32 max_addr, min_addr;
    u16 sector;
    u16 len0 = 0;
    u16 len = 0;;
    max_addr = ((u32)(pfile->pfs->last_sector + 1) << pfile->pfs->sector_size) ;
    min_addr = ((u32)(pfile->pfs->first_sector) << pfile->pfs->sector_size)  ;
    addr = pfile->rw_p + ((u32)pfile->index.sector << pfile->pfs->sector_size);
    /* y_printf(">>>[test]:addr = %d,max_addr =%d, min_addr=%d, pfile->rw_p =%d\n",addr, max_addr, min_addr, pfile->rw_p); */
    /*printf(">>>[test]:addr =0x%x,max_addr =0x%x, min_addr=0x%x,offset_addr=0x%x, pfile->rw_p =0x%x\n",addr, max_addr, min_addr, offset_addr,pfile->rw_p);*/
    while (1) {
        if ((pfile->rw_p + btw) > pfile->len) {
            //获取更多的空间；
            sector = (pfile->len + (1L << pfile->pfs->sector_size) - 1) >> pfile->pfs->sector_size;
            sector += pfile->index.sector;
            if (sector > pfile->pfs->last_sector) {
                sector = sector - pfile->pfs->last_sector + pfile->pfs->first_sector - 1;
            }
            if (pfile->index.sector != sector) {
                /* r_printf(">>>[test]:goto erase\n"); */
                recpfs_erase_sector(pfile->pfs, sector, sector);
                /* r_printf(">>>[test]:exit erase\n"); */
                pfile->len = pfile->len + (1L << pfile->pfs->sector_size);
            } else {
                /* y_printf(">>>[test]:not erase, sector = %d\n", sector); */
                break;
            }

        } else {
            break;
        }
    }
    if ((pfile->rw_p + btw) > pfile->len) {
        len = pfile->len - pfile->rw_p ;
        y_printf(">>>[test]:len = %d, pfile->len = %d,pfile->rw_p =%d \n", len, pfile->len, pfile->rw_p);
    } else {
        len = btw;
    }
    len0 = len;

    while (len0) {
        if (addr >= max_addr) {
            addr = addr - max_addr + min_addr;
            /* r_printf(">>>[test]:write the end, goto first addr write\n"); */
        }

        if ((addr + len0) > max_addr) {
            btw = max_addr - addr;
        } else {
            btw = len0;
        }
        /* y_printf(">>>[test]:btw = %d\n", btw); */
        recpfs_write_align(pfile->pfs, addr, buff, btw);
        /* y_printf(">>>[test]:len0 = %d\n",len0); */
        addr += btw;
        buff += btw;
        len0 -= btw;
    }
    pfile->rw_p += len;
    if (pfile->rw_p > pfile->w_len) {
        pfile->w_len = pfile->rw_p;
    }
    return len;
}

/*----------------------------------------------------------------------------*/
/**@brief   清除索引
   @param   pfs:文件系统句柄
   @return  void
   @author  liujie
   @note    void recfile_idx_clear(NOR_RECFILESYSTEM *pfs)
*/
/*----------------------------------------------------------------------------*/
void recfile_idx_clear(NOR_RECFILESYSTEM *pfs)
{
    RECFILEHEAD file_h;
    u32 addr;
    u16 curr_sector;

    curr_sector = pfs->first_sector;
    memset(&file_h, 0, sizeof(RECFILEHEAD));

    while (curr_sector <= pfs->last_sector) {
        addr = (u32)curr_sector << pfs->sector_size;
        recpfs_write_align(pfs, addr, (u8 *)(&file_h), sizeof(RECFILEHEAD));
        curr_sector++;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   delete a rec_file
   @param   pfs:文件系统句柄
   @return  void
   @author  phew
*/
/*----------------------------------------------------------------------------*/
void recpfs_clear(NOR_REC_FILE *pfile)
{
    RECFILEHEAD file_h;
    u32 addr;
    u16 curr_sector;

    curr_sector = pfile->index.sector;
    memset(&file_h, 0, sizeof(RECFILEHEAD));
    addr = (u32)curr_sector << pfile->pfs->sector_size;
    recpfs_write_align(pfile->pfs, addr, (u8 *)(&file_h), sizeof(RECFILEHEAD));

}

/*----------------------------------------------------------------------------*/
/**@brief   扫描文件系统
   @param   pfs:文件系统句柄
   @return  u32：总文件数
   @author  liujie
   @note    u32 recfs_scan(NOR_RECFILESYSTEM *pfs)
*/
/*----------------------------------------------------------------------------*/
/*扫描出最少的文件*/
u32 rec_pfs_scan(NOR_RECFILESYSTEM *pfs)
{
    RECFILEHEAD file_h __attribute__((aligned(4)));
    NOR_RECF_INDEX_INFO max_index;
    u32 addr;
    u32 total_len;
    u16 crc;
    u16 file_cnt = 0;
    u16 curr_sector = pfs->first_sector;
    pfs->total_file = 0;
    max_index.index = 0;
    max_index.sector = pfs->first_sector;
    total_len = (u32)((u32)pfs->last_sector + 1 - (u32)pfs->first_sector) << pfs->sector_size;
    while (1) {
        addr = (u32)curr_sector << pfs->sector_size;
        /* r_printf(">>>[test]:addr=0x%x\n", addr); */
        recpfs_read(pfs, addr, (u8 *)(&file_h), sizeof(RECFILEHEAD));
        crc = CRC16(&file_h.data_crc, sizeof(RECFILEHEAD) - 2);
        /* y_printf(">>>[test]:crc = 0x%x, file_h.len = %d, file_h.index = %d\n", crc, file_h.len, file_h.index); */
        if ((0 != crc) && (crc == file_h.head_crc) && (file_h.len <= total_len)) {
            if (file_h.index >= max_index.index) {
                max_index.index = file_h.index;
                max_index.sector = curr_sector;
            }
            r_printf(">>>[test]:addr=%d\n", addr);
            file_cnt++;
            if (0 != file_h.len) {
                curr_sector += file_h.len >> pfs->sector_size;
                if (file_h.len % (1L << pfs->sector_size)) {
                    curr_sector++;
                }
            } else {
                curr_sector++;
            }

        } else {
            curr_sector++;
        }
        if (curr_sector > pfs->last_sector) {
            //curr_sector = pfs->first_sector;
            break;
        }
    }

    if (max_index.index < 0) {
        //清空整个MEM()
        y_printf(">>>[test]:goto clear  index.sector\n");
        recfile_idx_clear(pfs);
        pfs->total_file = 0;
        pfs->index.index = 0;
        pfs->index.sector = pfs->first_sector;
    } else {
        pfs->index.index = max_index.index;
        pfs->index.sector = max_index.sector;
        /* y_printf(">>>[test]:recfs_scan   pfs->index.sector = %d, pfs->index.index = %d\n", pfs->index.sector, pfs->index.index); */
        pfs->total_file = file_cnt;
        /* y_printf(">>>[test]:recfs_scan   file_cnt = %d\n", file_cnt); */
    }

    return file_cnt;
}
/*----------------------------------------------------------------------------*/
/**@brief   检查指定扇区是否存爱文件
   @param   pfs:文件系统的句柄
   @param   pfile_h:文件头指针
   @param   sector:需要分析的扇区
   @return  u16:下一个有效扇区
   @author  liujie
   @note    u16 check_head(NOR_RECFILESYSTEM *pfs,RECFILEHEAD *pfile_h,u16 sector)
*/
/*----------------------------------------------------------------------------*/
u16 nor_check_head(NOR_RECFILESYSTEM *pfs, RECFILEHEAD *pfile_h, u16 sector)
{
    u32 addr;
    u32 total_len;
    addr = sector;
    addr = addr << pfs->sector_size;

    total_len = (u32)((u32)pfs->last_sector + 1 - (u32)pfs->first_sector) << pfs->sector_size;
    y_printf(">>>[test]:  check head  addr = %d\n", addr);
    recpfs_read(pfs, addr, (u8 *)pfile_h, sizeof(RECFILEHEAD));
    if ((CRC16(&pfile_h->data_crc, sizeof(RECFILEHEAD) - 2) == pfile_h->head_crc)
        && (pfile_h->len <= total_len)
       ) {
        r_printf(">>>[test]:AAAAAAAAAAA\n");
        sector = sector + (pfile_h->len >> pfs->sector_size);
        r_printf(">>>[test]:sector = %d\n", sector);
        if (pfile_h->len % (1L << pfs->sector_size)) {
            r_printf(">>>[test]:BBBBBBBBBBB\n");
            sector++;
        } else {
        }
    } else {
    }
    return sector;
}



/*----------------------------------------------------------------------------*/
/**@brief   建立一个文件
   @param   pfs:文件系统的句柄
   @param   pfile:文件的句柄
   @return  u32:建立文件索引
   @author  liujie
   @note    u32 create_nor_recfile(NOR_RECFILESYSTEM *pfs,NOR_REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 create_nor_recfile(NOR_RECFILESYSTEM *pfs, NOR_REC_FILE *pfile)
{
    RECFILEHEAD file_h;
    u16 sector, sec0;
    u16 sec_len;

    memset((u8 *)pfile, 0, sizeof(NOR_REC_FILE));
    pfile->pfs = pfs;
    sector = pfs->index.sector;
    /* y_printf(">>>[test]:create   pfs->index.sector = %d\n", pfs->index.sector); */
    sec0 = nor_check_head(pfs, &file_h, sector);
    if (sector != sec0) {
        if (sec0 > pfs->last_sector) {
            sec0 = sec0 - pfs->last_sector + pfs->first_sector - 1;
        }
        sector = sec0;
    }
    pfile->index.index = pfs->index.index + 1;
    pfile->index.sector = sec0;
    sec_len = recpfs_erase_sector(pfs, sector, sec0);
    /* y_printf(">>>[test]:sec_len = %d\n",sec_len); */
    pfile->len = (u32)sec_len << pfs->sector_size;
    pfile->w_len = sizeof(RECFILEHEAD);
    pfile->rw_p = sizeof(RECFILEHEAD);
    pfile->addr = pfile->rw_p + (pfile->index.sector << pfile->pfs->sector_size) ;
    /* y_printf(">>>[test]:rw_p = %d\n", pfile->rw_p); */
    return pfile->index.index;
}


/*----------------------------------------------------------------------------*/
/**@brief   关闭一个文件
   @param   pfile:文件的句柄
   @return  u32:建立文件索引
   @author  liujie
   @note    u32 close_nor_recfile(NOR_REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 close_nor_recfile(NOR_REC_FILE *pfile)
{
    RECFILEHEAD file_h;

    memset(&file_h, 0xff, sizeof(RECFILEHEAD));
    pfile->w_len = pfile->rw_p;
    pfile->len = pfile->w_len;
    pfile->rw_p = 0;//sizeof(RECFILEHEAD);
    file_h.len = pfile->w_len;
    file_h.index = pfile->index.index;
    file_h.addr = pfile->addr;
    memcpy(file_h.name, pfile->priv_data, NORFS_DATA_LEN);
    r_printf(">>>[test]:file_h.priv_data = %s, len = %d\n", file_h.name, NORFS_DATA_LEN);

    file_h.head_crc = CRC16(&file_h.data_crc, sizeof(RECFILEHEAD) - 2);
    r_printf(">>>[test]:pfile->index.sector = %d\n", pfile->index.sector);

    recfile_write(pfile, (u8 *)&file_h, sizeof(RECFILEHEAD));
    pfile->pfs->index.index = pfile->index.index;
    pfile->pfs->index.sector = pfile->index.sector;

    /* r_printf(">>>[test]:file_h.len = %d\n", file_h.len); */
    return file_h.len;
}

void recfile_save_sr(NOR_REC_FILE *pfile, u16 sr)
{
    if (pfile) {
        pfile->sr = sr;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   按照索引号，搜索一个文件
   @param   index:索引号
   @param   pfile:文件的句柄
   @return  u32:找到文件的索引
   @author  liujie
   @note    u32 recfile_index_scan(u32 index,NOR_REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 recfile_index_scan(u32 index, NOR_REC_FILE *pfile)
{
    RECFILEHEAD file_h;
    NOR_RECFILESYSTEM *pfs;
    u32 addr;
    u32 total_len;
    u16 curr_sector, crc;

    pfs = pfile->pfs;
    curr_sector = pfs->index.sector;
    total_len = (u32)((u32)pfs->last_sector + 1 - (u32)pfs->first_sector) << pfs->sector_size;
    /* r_printf(">>>[test]:pfile->pfs->index.sector = %d, total_len = %d\n", pfile->pfs->index.sector, total_len); */
    while (1) {
        addr = (u32)curr_sector << pfs->sector_size;
        /* r_printf(">>>[test]:offset_addr = 0x%x, addr = 0x%x\n", offset_addr, addr); */
        /* } */
        recpfs_read(pfs, addr, (u8 *)(&file_h), sizeof(RECFILEHEAD));
        crc = CRC16(&file_h.data_crc, sizeof(RECFILEHEAD) - 2);
        /* y_printf(">>>[test]:crc = 0x%x, file_h.len = %d, file_h.index = %d\n", crc, file_h.len, file_h.index); */
        if ((0 != crc) && (crc == file_h.head_crc) && (file_h.len <= total_len)) {
            if (file_h.index == index) {
                printf(">>>[test]:go in AAAAAA\n");
                pfile->index.index = index;
                pfile->index.sector = curr_sector;
                pfile->len = file_h.len;
                pfile->w_len = file_h.len;
                pfile->rw_p = sizeof(RECFILEHEAD);
                pfile->addr = file_h.addr;
                memcpy(pfile->priv_data, file_h.name, NORFS_DATA_LEN);
                return index;
            } else {
                if (0 == file_h.len) {
                    curr_sector++;
                }
                curr_sector = curr_sector + (file_h.len >> pfs->sector_size);
                if (file_h.len % (1L << pfs->sector_size)) {
                    curr_sector++;
                }
            }
        } else {
            curr_sector++;
        }

        if (curr_sector > pfs->last_sector) {
            curr_sector = pfs->first_sector;
        }
        /* r_printf(">>>[test]:pfile->index.sector = %d, curr_sector = %d\n", pfile->index.sector, curr_sector); */
        if (pfs->index.sector == curr_sector) {
            break;
        }
    }
    return index - 1;
}

/*----------------------------------------------------------------------------*/
/**@brief   按照索引号打开一个文件
   @param   index:索引号
   @param   pfs:文件系统的句柄
   @param   pfile:文件的句柄
   @return  u32:找到文件的索引
   @author  liujie
   @note    u32 open_nor_recfile(u32 index,NOR_RECFILESYSTEM *pfs,NOR_REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 open_nor_recfile(u32 index, NOR_RECFILESYSTEM *pfs, NOR_REC_FILE *pfile)
{
    memset((u8 *)pfile, 0, sizeof(NOR_REC_FILE));
    pfile->pfs = pfs;
    pfile->rw_p = sizeof(RECFILEHEAD);
    return recfile_index_scan(index, pfile);
}


/*----------------------------------------------------------------------------*/
/**@brief   初始化文件系统信息
   @param   pfs:文件系统的句柄
   @param   sector_start:起始扇区
   @param   sector_end:结束扇区
   @param   sector_size:扇区大小（1L<<sector_size）
   @return  u32:找到文件的索引
   @author  liujie
   @note    void init_nor_fs(NOR_RECFILESYSTEM *pfs,u16 sector_start,u16 sector_end,u8 sector_size)
*/
/*----------------------------------------------------------------------------*/
void nor_pfs_init(NOR_RECFILESYSTEM *pfs, u16 sector_start, u16 sector_end, u8 sector_size)
{
    pfs->first_sector = sector_start;
    pfs->last_sector = sector_end;
    pfs->sector_size = sector_size;
}

