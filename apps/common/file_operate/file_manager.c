#include "file_manager.h"
#include "app_config.h"

#define MODE_FIX 0
#if MODE_FIX
static int file_manager_mode_deal(struct vfscan *fs, int sel_mode, int *arg)
{

    int err;
    int fnum = 0;
    int file_start = 1;
    int file_end = fs->file_number;

    if (file_end == 0) {
        return -ENOENT;
    }
    struct ffolder folder = {0};
    fget_folder(fs, &folder);
    /* if ((scan->cycle_mode == FCYCLE_FOLDER) && (scan->ff_api.fileTotalInDir) */
    /*     && ((scan->ff_api.fileTotalOutDir + scan->ff_api.fileTotalInDir) <= scan->ff_api.totalFileNumber) */
    /*    ) { */

    if ((fs->cycle_mode == FCYCLE_FOLDER) && (folder.fileTotal)
        && ((folder.fileStart + folder.fileTotal - 1) <= file_end)
       ) {
        file_start = folder.fileStart;
        file_end = folder.fileStart + folder.fileTotal - 1;
    }
    switch (sel_mode) {
    case FSEL_LAST_FILE:
        fnum = fs->file_number;
        break;
    case FSEL_FIRST_FILE:
        fnum = 1;
        break;
    case FSEL_AUTO_FILE:
        /* if (scan->ff_api.fileNumber == 0) { */
        /*     return -EINVAL; */
        /* } */
        if (fs->cycle_mode == FCYCLE_ONE) {
            /* fnum = scan->ff_api.fileNumber; */
            fnum = fs->file_counter;
            break;
        }
    case FSEL_NEXT_FILE:
        /* if (scan->ff_api.fileNumber == 0) { */
        /*     return -EINVAL; */
        /* } */
        if (fs->cycle_mode == FCYCLE_RANDOM) {
            fnum = rand32() % (file_end - file_start + 1) + file_start;
            if (fnum == fs->file_counter) {
                fnum = fs->file_counter + 1;
            }
        } else {
            fnum = fs->file_counter + 1;
        }
        /* if (fnum > scan->last_file_number) { */
        if (fnum > fs->file_number) {
            if (fs->cycle_mode == FCYCLE_LIST) {
                return -ENOENT;
            } else if (fs->cycle_mode == FCYCLE_FOLDER) {
                fnum = 	file_start;
            } else {
                fnum = 1;
            }
        }
        if (fnum > file_end) {
            fnum = file_start;
        } else if (fnum < file_start) {
            fnum = file_end;
        }
        break;
    case FSEL_PREV_FILE:
        /* if (scan->ff_api.fileNumber == 0) { */
        /*     return -EINVAL; */
        /* } */
        if (fs->cycle_mode == FCYCLE_RANDOM) {
            fnum = rand32() % (file_end - file_start + 1) + file_start;
            if (fnum == fs->file_counter) {
                fnum = fs->file_counter + 1;
            }
        } else {
            fnum = fs->file_counter - 1;
        }
        /* if ((scan->ff_api.fileNumber | BIT(15)) != scan->cur_file_number) { */
        /*     fnum -= scan->last_file_number - scan->ff_api.totalFileNumber; */
        /* } */
        /* scan->last_file_number = scan->ff_api.totalFileNumber; */
        if (fs->cycle_mode == FCYCLE_LIST) {
            break;
        }
        if (fnum > file_end) {
            fnum = file_start;
        } else if (fnum < file_start) {
            fnum = file_end;
        }
        break;

    case FSEL_NEXT_FOLDER_FILE:
        /* fnum = scan->ff_api.fileTotalOutDir + scan->ff_api.fileTotalInDir + 1; */
        fnum = folder.fileStart + folder.fileTotal;
        if (fnum > fs->file_number) {
            fnum = 1;
        }
        break;
    case FSEL_PREV_FOLDER_FILE:
        /* if ((scan->ff_api.fileTotalOutDir + 1) > 1) { */
        if (folder.fileStart > 1) {
            fnum = folder.fileStart - 1;
        } else {
            fnum = fs->file_number;
        }
        break;
    default:
        return -EINVAL;
    }

    if ((sel_mode != FSEL_NEXT_FOLDER_FILE) && (sel_mode != FSEL_PREV_FOLDER_FILE)) {
        if (fnum < file_start) {
            fnum = file_start;
        } else if (fnum > file_end) {
            fnum = file_end;
        }
    }
    *arg = fnum;
    return 0;
}
#endif

FILE *file_manager_select(struct __dev *dev, struct vfscan *fs, int sel_mode, int arg, struct __scan_callback *callback)
{
    FILE *_file = NULL;
    //clock_add_set(SCAN_DISK_CLK);
    if (callback && callback->enter) {
        callback->enter(dev);//扫描前处理， 可以在注册的回调里提高系统时钟等处理
    }
#if MODE_FIX
    if ((sel_mode != FSEL_BY_SCLUST) && (sel_mode != FSEL_BY_PATH)) {
        if (file_manager_mode_deal(fs, sel_mode, &arg)) {
            return NULL;
        }
        sel_mode = FSEL_BY_NUMBER;
    }
#endif
    _file = fselect(fs, sel_mode, arg);
    //clock_remove_set(SCAN_DISK_CLK);
    if (callback && callback->exit) {
        callback->exit(dev);//扫描后处理， 可以在注册的回调里还原到enter前的状态
    }
    return _file;
}


