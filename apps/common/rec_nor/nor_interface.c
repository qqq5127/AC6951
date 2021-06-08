#include "nor_interface.h"
#include "nor_rec_fs.h"

#define NO_DEBUG_EN

#ifdef NO_DEBUG_EN
#undef r_printf
#undef y_printf
#define r_printf(...)
#define y_printf(...)
#endif

#define NOR_FS_SECTOR_S     (4*1024L)
#define NOR_FS_S_SECTOR     (1)
#define NOR_FS_E_SECTOR     (NOR_FS_SECTOR_S-1)

#define RECFILEHEAD  SDFILE_FILE_HEAD

struct nor_fs_rec {
    NOR_RECFILESYSTEM recfs;
    NOR_REC_FILE recfile;
    u32 nor_fs_index;
    int first_flag;
    int is_read_only;
};

static int rec_norflash_spirec_read(void *device, u8 *buf, u32 addr, u32 len)
{
    return dev_bulk_read(device, buf, addr, len);
}

static int rec_norflash_spirec_write(void *device, u8 *buf, u32 addr, u32 len)
{
    return dev_bulk_write(device, buf, addr, len);
}

static void rec_norflash_spirec_eraser(void *device, u32 addr)
{
    dev_ioctl(device, IOCTL_ERASE_SECTOR, addr);
}

static int rec_norflash_get_capacity(void *device)
{
    int capacity;
    dev_ioctl(device, IOCTL_GET_CAPACITY, (u32)&capacity);
    return capacity;
}

static int rec_norflash_get_part_info(void *device, u32 *s_addr, u32 *size)
{
    u32 buf[2] = {0};
    buf[0] = (u32)s_addr;
    buf[1] = (u32)size;
    int reg = dev_ioctl(device, IOCTL_GET_PART_INFO, (u32)buf);
    if (reg) {
        log_e("get part info err!!!!!!!!");
    }
    return reg;
}

static int get_ext_name(char *path, char *ext)
{
    int len = strlen(path);
    char *name = (char *)path;
    while (len--) {
        if (*name++ == '.') {
            break;
        }
    }
    if (len <= 0) {
        name = path + (strlen(path) - 3);
    }
    memcpy(ext, name, 3);
    ext[3] = 0;
    return 1;
}

static void add_path_index(const char *path, u32 index)
{
    int last_num = -1;
    char *file_num_str;

    last_num = index ;
    file_num_str = strrchr(path, '.') - 4;
    if (last_num > 9999) {
        last_num = 0;
    }
    file_num_str[0] = last_num / 1000 + '0';
    file_num_str[1] = last_num % 1000 / 100 + '0';
    file_num_str[2] = last_num % 100 / 10 + '0';
    file_num_str[3] = last_num % 10 + '0';
}

static u32 recfs_init(struct __NOR_RECFILESYSTEM *recfs)
{
    recfs->eraser = rec_norflash_spirec_eraser;
    recfs->read   = rec_norflash_spirec_read;
    recfs->write  = rec_norflash_spirec_write;

    return true;
}

static int nor_fs_mount(struct imount *mt, int cache_num)
{
    int part_capacity = 0;
    y_printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);
    struct nor_fs_rec *rec_fs_file = zalloc(sizeof(struct nor_fs_rec));
    struct vfs_partition *app_part = &mt->part;
    app_part->private_data = rec_fs_file;
    rec_fs_file->recfs.device = mt->dev.fd;
    rec_fs_file->is_read_only = 0;

    part_capacity = rec_norflash_get_capacity(rec_fs_file->recfs.device);
    r_printf(">>>[test]:capacity = 0x%x\n", part_capacity);

    recfs_init(&rec_fs_file->recfs);
    nor_pfs_init(&rec_fs_file->recfs, NOR_FS_S_SECTOR, part_capacity / NOR_FS_E_SECTOR, 12);
    rec_pfs_scan(&rec_fs_file->recfs);
    rec_fs_file->nor_fs_index = rec_fs_file->recfs.index.index;

    app_part->offset = 0;
    app_part->dir[0] = 'C';
    app_part->dir[1] = '\0';

    return 0;
}

static int nor_fs_open(FILE *_file, const char *path, const char *mode)
{
    int index;
    int reg;
    y_printf("\n>>>[test]:nor_fs_open\n");
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;

    if (mode[0] == 'w') {
        y_printf(">>>[test]:goto create\n");
        char ext[4] = {0};
        char filename[16] = {0};
        get_ext_name((char *)path, (char *)ext);
        if (memcmp(ext, "WAV", 3) == 0 || memcmp(ext, "wav", 3) == 0) {
            private_data->first_flag = 1;
        } else {
            private_data->first_flag = 0;
        }

        index = create_nor_recfile(&private_data->recfs, &private_data->recfile);
        private_data->nor_fs_index = index;
        r_printf(">>>[test]: create_recfile index= %d\n", index);
        add_path_index(path, index);
        printf(">>>[test]: create_recfile path= %s\n", path);
        return 0;
    } else if (mode[0] == 'r') {
        y_printf(">>>[test]: nor_fs_open private_data->index= %d\n", private_data->nor_fs_index);
        if (private_data->nor_fs_index) {
            index = private_data->nor_fs_index;
        } else {
            log_e("error!!!!!!!!!");
            index = 0;
            private_data->nor_fs_index = 0;
            return 1;
        }
        add_path_index(path, index);
        printf(">>>[test]: nor_fs_open read path= %s\n", path);
        reg = open_nor_recfile(index, &private_data->recfs, &private_data->recfile);
        private_data->is_read_only = 1;
        return 0;
    }
    return 0;
}

static int nor_fs_write(FILE *_file, void *buff, u32 len)
{
    u16 w_len;
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    if (private_data->first_flag == 1) {
        memset(buff, 0xff, 90);
        private_data->first_flag = 0;
    }
    w_len = recfile_write(&private_data->recfile, buff, len);
    return w_len;
}

static int nor_fs_read(FILE *_file, void *buff, u32 len)
{
    u16 r_len;
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    y_printf(">>>[test]:goto nor_fs read\n");
    if (((u32)buff % 4) != 0) {
        void *tmp_buf = malloc(512);
        r_len = recfile_read(&private_data->recfile, tmp_buf, len);
        memcpy(buff, tmp_buf, len);
        free(tmp_buf);
        return r_len;
    }
    r_len = recfile_read(&private_data->recfile, buff, len);
    return r_len;
}

static int nor_fs_close(FILE *_file)
{
    int reg;
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    y_printf(">>>[test]:goto nor_fs close\n");
    if (private_data->is_read_only) {
        private_data->recfile.pfs->index.index = private_data->recfile.index.index;
        private_data->recfile.pfs->index.sector = private_data->recfile.index.sector;
        private_data->is_read_only = 0;
        return private_data->recfile.w_len;
    }
    reg = close_nor_recfile(&private_data->recfile);
    return reg;
}

static int nor_fs_seek(FILE *_file, int offsize, int type)
{
    int reg;
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    y_printf(">>>[test]:goto nor_fs seek\n");
    reg = recfile_seek(&private_data->recfile, (u8) type, offsize);
    return reg;
}

static int nor_fs_flen(FILE *_file)
{
    RECFILEHEAD file_h;
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    y_printf(">>>[test]:goto nor_fs_flen\n");
    r_printf(">>>[test]:pfile->len = %d\n", private_data->recfile.len);
    return private_data->recfile.len;
}

static int nor_fs_fpos(FILE *_file)
{
    RECFILEHEAD file_h;
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    r_printf(">>>[test]:pfile->fpos(rw_p) = %d\n", private_data->recfile.rw_p);
    return private_data->recfile.rw_p;
}

static int nor_fs_unmount(struct imount *mt)
{
    int reg;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    y_printf(">>>[test]:goto nor_fs unmount\n");
    free(private_data);
    return 0;
}

int get_rec_capacity(FILE *_file)
{
    int part_capacity = 0;
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    part_capacity = rec_norflash_get_capacity(private_data->recfs.device);
    return part_capacity;
}

static int nor_fs_fget_attrs(FILE *_file, struct vfs_attr *attr)
{
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    attr->fsize = private_data->recfile.len;
    attr->sclust = private_data->recfile.addr;

    return 0;
}


static int nor_fs_delete(FILE *_file)
{
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    recpfs_clear(&private_data->recfile);
    return 0;
}


REGISTER_VFS_OPERATIONS(nor_rec_fs_vfs_ops) = {
    .fs_type         = "rec_fs",
    .mount           = nor_fs_mount,
    .unmount	     = nor_fs_unmount,
    .format          = NULL,
    .fopen	         = nor_fs_open,
    .fwrite          = nor_fs_write,
    .fread        	 = nor_fs_read,
    .fseek 		     = nor_fs_seek,
    .flen 		     = nor_fs_flen,
    .fpos 		     = nor_fs_fpos,
    .fclose          = nor_fs_close,
    .fscan  	     = NULL,
    .fscan_release 	 = NULL,
    .fsel	         = nor_fs_delete,
    .fdelete	     = NULL,
    .fget_name       = NULL,
    .frename         = NULL,
    .fget_free_space = NULL,
    .fget_attr       = NULL,
    .fset_attr       = NULL,
    .fget_attrs      = nor_fs_fget_attrs,
    .fset_vol        = NULL,
    .fmove           = NULL,
    .ioctl           = NULL,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void clear_norfs_allfile(struct imount *mt)
{
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    recfile_idx_clear(&private_data->recfs);
    private_data->recfs.total_file = 0;
    private_data->recfs.index.index = 0;
    private_data->recfs.index.sector = private_data->recfs.first_sector;
}

void nor_get_data_info(FILE *_file, u32 *s_addr, u32 *part_size)
{
    struct imount *mt = _file->mt;
    struct nor_fs_rec *private_data = (struct nor_fs_rec *)mt->part.private_data;
    u32 start_addr, size;
    rec_norflash_get_part_info(private_data->recfs.device, &start_addr, &size);
    *s_addr = start_addr + private_data->recfile.addr;
    *part_size = size;
}


