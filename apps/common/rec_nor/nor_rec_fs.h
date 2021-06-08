#ifndef _NOR_REC_FS_H_
#define _NOR_REC_FS_H_

#define REC_FILE_END 0xFE

enum {
    NOR_FS_SEEK_SET = 0,
    NOR_FS_SEEK_CUR = 0x01
};

u16 recpfs_read(NOR_RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len);
u16 recpfs_write(NOR_RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len);
u16 recpfs_write_align(NOR_RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len);
u16 recpfs_erase_sector(NOR_RECFILESYSTEM *pfs, u16 start_sector, u16 end_sector);
u8 recfile_seek(NOR_REC_FILE *pfile, u8 type, int offsize);
u16 recfile_read(NOR_REC_FILE *pfile, u8 *buff, u16 btr);
u16 recfile_write(NOR_REC_FILE *pfile, u8 *buff, u16 btw);
void recfile_idx_clear(NOR_RECFILESYSTEM *pfs);
void recpfs_clear(NOR_REC_FILE *pfile);
u32 rec_pfs_scan(NOR_RECFILESYSTEM *pfs);
u32 create_nor_recfile(NOR_RECFILESYSTEM *pfs, NOR_REC_FILE *pfile);
u32 close_nor_recfile(NOR_REC_FILE *pfile);
u32 recfile_index_scan(u32 index, NOR_REC_FILE *pfile);
u32 open_nor_recfile(u32 index, NOR_RECFILESYSTEM *pfs, NOR_REC_FILE *pfile);
void nor_pfs_init(NOR_RECFILESYSTEM *pfs, u16 sector_start, u16 sector_end, u8 sector_size);





#endif
