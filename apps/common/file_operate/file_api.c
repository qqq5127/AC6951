
#if 0
#include "app_config.h"
#include "system/includes.h"
#include "system/os/os_cpu.h"
#include "file_operate/file_bs_deal.h"

// folder : /xxxxxx
// filename : xxxx0000.yyy
FILE *file_api_auto_create_file(const char *path, const char *folder, const char *filename, u32 *file_index)
{
    int ret;
    u32 last_num = (u32) - 1;
    u32 total_num = 0;
    u32 err_cnt = 0;
    FILE *pfil = NULL;
    char *create_file_name = NULL;
    char *file_num_str;
    char *ext;

#if TCFG_NOR_FS_ENABLE
    extern int nor_fs_index;
    extern int recfs_scan_ex();
    nor_fs_index = recfs_scan_ex();
    last_num = nor_fs_index + 1;
    create_file_name = zalloc(strlen(path) + strlen(folder) + strlen(filename) + 3);
    if (!create_file_name) {
        log_w("malloc err");
        return NULL;
    }
    strcat(create_file_name, path);
    strcat(create_file_name, folder + 1);
    strcat(create_file_name, "/");
    strcat(create_file_name, filename);

    file_num_str = strrchr(create_file_name, '.') - 4;

    if (last_num > 9999) {
        last_num = 0;
    }
    file_num_str[0] = last_num / 1000 + '0';
    file_num_str[1] = last_num % 1000 / 100 + '0';
    file_num_str[2] = last_num % 100 / 10 + '0';
    file_num_str[3] = last_num % 10 + '0';
    log_i("createfile:%s ", create_file_name);
    if (file_index) {
        *file_index = last_num;
    }

    pfil = fopen(create_file_name, "w+");

    if (create_file_name) {
        free(create_file_name);
    }
    return pfil;
#endif

    ext = strrchr(filename, '.');
    if (!ext) {
        log_w("ext err");
        return NULL;
    }
    ext++;
    ret = fget_encfolder_info(path, folder, ext, &last_num, &total_num);
    if (ret) {
        log_w("fget_encfolder_info err");
        return NULL;
    }
    log_i("foler:%s, total:%d, last:%d ", folder, total_num, last_num);
    last_num ++;
    if ((last_num >= 9999) && (total_num >= 9999)) {
        log_w("file num err");
        return NULL;
    }
    if (total_num == 0) {
        log_i("make dir");
        fmk_dir(path, folder, 0);
    }

    create_file_name = zalloc(strlen(path) + strlen(folder) + strlen(filename) + 3);
    if (!create_file_name) {
        log_w("malloc err");
        return NULL;
    }
    strcat(create_file_name, path);
    strcat(create_file_name, folder + 1);
    strcat(create_file_name, "/");
    strcat(create_file_name, filename);

    file_num_str = strrchr(create_file_name, '.') - 4;

_get_file:
    if (last_num > 9999) {
        last_num = 0;
    }
    file_num_str[0] = last_num / 1000 + '0';
    file_num_str[1] = last_num % 1000 / 100 + '0';
    file_num_str[2] = last_num % 100 / 10 + '0';
    file_num_str[3] = last_num % 10 + '0';
    log_i("createfile:%s ", create_file_name);
    if (file_index) {
        *file_index = last_num;
    }

    pfil = fopen(create_file_name, "r");
    if (pfil) {
        log_i("file already exists ");
        fclose(pfil);
        pfil = NULL;
        err_cnt ++;
        if (err_cnt >= 9999) {
            log_w("err cnt ");
            goto _exit;
        }
        last_num ++;
        goto _get_file;
    } else {
        pfil = fopen(create_file_name, "w+");
    }

_exit:
    if (create_file_name) {
        free(create_file_name);
    }
    return pfil;
}

#if 0
void file_create_test(void)
{
    struct storage_dev *dev = storage_dev_last();
    printf("%s,%d ", __func__, __LINE__);
    if (!dev) {
        printf("%s,%d ", __func__, __LINE__);
        return ;
    }
    printf("%s,%d ", __func__, __LINE__);
    printf("devname:%s, pa:%s, fs:%s ", dev->dev_name, dev->storage_path, dev->fs_type);
    /* void *fmnt = mount(dev->dev_name, dev->storage_path, dev->fs_type, 3, NULL); */
    /* if (!fmnt) { */
    /* printf("%s,%d ", __func__, __LINE__); */
    /* return ; */
    /* } */
    FILE *pfil = file_api_auto_create_file(dev->root_path, "/JL_REC", "AC690000.MP3", NULL);
    if (pfil) {
        u8 str[] = "This is a test string.";
        u8 buf[10];
        int i;
        int len;
        int addr;
        for (i = 0; i < 100; i++) {
            len = fwrite(pfil, str, sizeof(str));
            if (len != sizeof(str)) {
                log_i("write file ERR!");
            }
        }
        len = flen(pfil);
        addr = fpos(pfil);
        log_i("fl:%d, addr:%d ", len, addr);

        fseek(pfil, 0, SEEK_SET);
        len = fread(pfil, buf, sizeof(buf));
        if (len != sizeof(buf)) {
            log_i("read file ERR!");
        }
        put_buf(buf, sizeof(buf));

        fseek(pfil, addr, SEEK_SET);
        fclose(pfil);
    }
    /* unmount(dev->storage_path); */
}
#endif

#endif
