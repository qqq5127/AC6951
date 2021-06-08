#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_api.h"
#include "system/timer.h"
#include "key_event_deal.h"
#include "audio_config.h"
#include "jiffies.h"
#include "app_power_manage.h"
#include "asm/charge.h"
#include "audio_dec_file.h"
#include "music/music_player.h"
#ifndef CONFIG_MEDIA_NEW_ENABLE
#include "application/audio_eq_drc_apply.h"
#else
#include "audio_eq.h"
#endif





#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))

#define STYLE_NAME  JL//必须要

extern int ui_hide_main(int id);
extern int ui_show_main(int id);
extern void key_ui_takeover(u8 on);


//文件浏览部分

#define TEXT_NAME_LEN (128)
#define TEXT_SHORT_NAME_LEN (13)//8.3+1
#define TEXT_PAGE     (4)
struct text_name_t {
    u16 len;
    u8  unicode;
    u8 fname[TEXT_NAME_LEN];
    u8 fsname[TEXT_SHORT_NAME_LEN];
    int index;
};



struct grid_set_info {
    int flist_index;  //文件列表首项所指的索引
    int open_index;  //所打开的文件所指的索引(未)
    int cur_total;
    const char *disk_name[4];
    u8  in_scan;//是否在盘内
    u8  disk_total;
    FILE *file;
    struct vfscan *fs;
    struct text_name_t text_list[TEXT_PAGE];
    struct vfs_attr attr[TEXT_PAGE];
#if (TCFG_LFN_EN)
    u8  lfn_buf[512];
#endif//TCFG_LFN_EN

};

static struct grid_set_info *handler = NULL;
#define __this 	(handler)
#define sizeof_this     (sizeof(struct grid_set_info))

static u32 LAYOUT_FNAME_LIST_ID[] = {
    FILE_LAYOUT_0,
    FILE_LAYOUT_1,
    FILE_LAYOUT_2,
    FILE_LAYOUT_3,
};

static u32 TEXT_FNAME_ID[] = {
    FILE_BROWSE_TEXT_0,
    FILE_BROWSE_TEXT_1,
    FILE_BROWSE_TEXT_2,
    FILE_BROWSE_TEXT_3,
};

static u32 PIC_FNAME_ID[] = {
    FILE_BROWSE_PIC_0,
    FILE_BROWSE_PIC_1,
    FILE_BROWSE_PIC_2,
    FILE_BROWSE_PIC_3,
};




static int __get_ui_max_num()
{
    return sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]);
}

static const u8 MUSIC_SCAN_PARAM[] = "-t"
                                     "MP1MP2MP3"
                                     " -sn -d"
                                     ;


static int file_select_enter(int from_index)
{

    int one_page_num = sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]);
    int i = 0;
    i = from_index % one_page_num;
    if (__this->attr[i].attr == F_ATTR_DIR) { //判断是不是文件夹文件属性
        if (!from_index) {
            __this->fs = fscan_exitdir(__this->fs);
        } else {
            __this->fs = fscan_enterdir(__this->fs, __this->text_list[i].fsname);
        }

        if (!__this->fs) {
            ui_hide(MUSIC_FILE_LAYOUT);
            ui_hide(MUSIC_MENU_LAYOUT);
            return false;
        }


        __this->cur_total = __this->fs->file_number + 1;
        return TRUE;
    } else {
        /* if (!strcmp(cur->name, APP_NAME_MUSIC)) {//当前在目标app内，则只要显示出目标页面即可 */
        app_task_put_key_msg(KEY_MUSIC_PLAYE_BY_DEV_SCLUST, __this->attr[i].sclust);
        /* music_play_file_by_dev_sclust(__this->dev->logo, __this->attr[i].sclust);///this is a demo */
        ui_hide(MUSIC_FILE_LAYOUT);
        ui_hide(MUSIC_MENU_LAYOUT);
        /* } */
    }
    return false;
}

static int file_list_flush(int from_index)
{
    FILE *f = NULL;
    int one_page_num = sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]);
    int i = 0;
    int end_index = from_index + one_page_num;
    char *name_buf = NULL;

    for (i = 0; i < sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]); i++) {
        memset(__this->text_list[i].fname, 0, TEXT_NAME_LEN);
        __this->text_list[i].len = 0;
    }

__find:
    i = from_index % one_page_num;
    if (from_index == 0) {
        sprintf(__this->text_list[i].fname, "%s", "..");
        sprintf(__this->text_list[i].fsname, "%s", "..");
        __this->attr[i].attr = F_ATTR_DIR;
        __this->text_list[i].unicode = 0;
        __this->text_list[i].len = strlen(__this->text_list[i].fname);
    } else {
        f = fselect(__this->fs, FSEL_BY_NUMBER, from_index);
    }

    if (f) {
        name_buf = malloc(TEXT_NAME_LEN);
        __this->text_list[i].len = fget_name(f, name_buf, TEXT_NAME_LEN);
        if (name_buf[0] == '\\' && name_buf[1] == 'U') {
            __this->text_list[i].len   -= 2;
            __this->text_list[i].unicode = 1;
            memcpy(__this->text_list[i].fname, name_buf + 2, TEXT_NAME_LEN - 2);
        } else {
            __this->text_list[i].unicode = 0;
            memcpy(__this->text_list[i].fname, name_buf, TEXT_NAME_LEN);
        }
        free(name_buf);
        name_buf = NULL;
        printf("\n--func=%s, line=%d\n", __FUNCTION__, __LINE__);
        printf("flush [%d]=%s\n", i, __this->text_list[i].fname);
        fget_attrs(f, &__this->attr[i]);
        if (__this->attr[i].attr == F_ATTR_DIR) { //判断是不是文件夹文件属性,文件夹需要获取短文件名
            fget_name(f, __this->text_list[i].fsname, TEXT_SHORT_NAME_LEN);
        }
        fclose(f);
        f = NULL;
    }

    from_index++;
    if (from_index < __this->cur_total && from_index < end_index) {
        goto __find;
    }


    for (i = 0; i < sizeof(TEXT_FNAME_ID) / sizeof(TEXT_FNAME_ID[0]); i++) {
        if (__this->text_list[i].len) {
            if (__this->text_list[i].unicode) {
                ui_text_set_textw_by_id(TEXT_FNAME_ID[i], __this->text_list[i].fname, __this->text_list[i].len, FONT_ENDIAN_SMALL, FONT_DEFAULT | FONT_HIGHLIGHT_SCROLL);
            } else {
                ui_text_set_text_by_id(TEXT_FNAME_ID[i], __this->text_list[i].fname, __this->text_list[i].len, FONT_DEFAULT);
            }
            if (__this->attr[i].attr == F_ATTR_DIR) {
                ui_pic_show_image_by_id(PIC_FNAME_ID[i], 1);
            } else {
                ui_pic_show_image_by_id(PIC_FNAME_ID[i], 0);
            }
            if (ui_get_disp_status_by_id(PIC_FNAME_ID[i]) != true) {
                ui_show(PIC_FNAME_ID[i]);
            }
        } else {
            ui_text_set_text_by_id(TEXT_FNAME_ID[i], __this->text_list[i].fname, __this->text_list[i].len, FONT_DEFAULT);
            if (ui_get_disp_status_by_id(PIC_FNAME_ID[i]) == true) {
                ui_hide(PIC_FNAME_ID[i]);
            }
        }

    }
    return 0;
}


static int file_browse_enter_onchane(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int fnum = 0;

    switch (e) {
    case ON_CHANGE_INIT:
        if (!__this) {
            __this = zalloc(sizeof_this);
        }

        if (!__this->fs) {
            if (!dev_manager_get_total(1)) {
                break;
            }

            void *dev = dev_manager_find_active(1);
            if (!dev) {
                break;
            }
            __this->fs = fscan(dev_manager_get_root_path(dev), MUSIC_SCAN_PARAM, 9);
#if (TCFG_LFN_EN)
            if (__this->fs) {
                fset_lfn_buf(__this->fs, __this->lfn_buf);
            }
#endif
            printf(">>> file number=%d \n", __this->fs->file_number);
            fnum = (__this->fs->file_number + 1 > __get_ui_max_num()) ? __get_ui_max_num() : __this->fs->file_number + 1;
            __this->cur_total = __this->fs->file_number + 1;
        }
        break;
    case ON_CHANGE_RELEASE:
        if (__this->fs) {
            fscan_release(__this->fs);
            __this->fs = NULL;
        }

        if (__this) {
            free(__this);
            __this = NULL;
        }

        break;
    case ON_CHANGE_FIRST_SHOW:
        if (__this->open_index) {
            //从回放返回文件列表时
            ui_set_call(file_list_flush, __this->flist_index);
        } else {
            ui_set_call(file_list_flush, 0);
            /* file_list_flush(0); */
            //刚进去文件列表时
            __this->flist_index = 0;
        }
        break;
    default:
        return false;
    }
    return false;
}

static int file_browse_onkey(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    sel_item = ui_grid_cur_item(grid);
    switch (e->value) {
    case KEY_OK:
        if (file_select_enter(__this->flist_index)) {
            ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item]);
            __this->flist_index = 0;
            file_list_flush(__this->flist_index);
            ui_grid_set_item(grid, 0);
            ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[0]);
        }


        break;
    case KEY_DOWN:

        sel_item++;

        __this->flist_index += 1;

        if (sel_item >= __this->cur_total || __this->flist_index >= __this->cur_total) {
            //大于文件数
            ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item - 1]);
            __this->flist_index = 0;
            file_list_flush(__this->flist_index);
            ui_grid_set_item(grid, 0);
            ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[0]);
            ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
            return TRUE;
        }
        ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
        if (sel_item >= TEXT_PAGE) {
            ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item - 1]);
            file_list_flush(__this->flist_index);
            ui_grid_set_item(grid, 0);
            ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[0]);
            return true; //不返回到首项
        }

        return FALSE;

        break;
    case KEY_UP:
        if (sel_item == 0) {
            __this->flist_index = __this->flist_index ? __this->flist_index - 1 : __this->cur_total - 1;
            ui_no_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[sel_item]);
            file_list_flush(__this->flist_index / TEXT_PAGE * TEXT_PAGE);
            ui_grid_set_item(grid, __this->flist_index % TEXT_PAGE);
            ui_highlight_element_by_id(LAYOUT_FNAME_LIST_ID[__this->flist_index % TEXT_PAGE]);
            ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
            return true; //不跳转到最后一项
        }
        __this->flist_index--;
        ui_vslider_set_persent_by_id(MUSIC_FILE_SLIDER, (__this->flist_index + 1) * 100 / __this->cur_total);
        return FALSE;
        break;

    default:
        return false;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(MUSIC_FILE_BROWSE)
.onchange = file_browse_enter_onchane,
 .onkey = file_browse_onkey,
  .ontouch = NULL,
};























#endif
