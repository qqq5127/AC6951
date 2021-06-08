#include "smartbox/func_cmd/music_func.h"
#include "smartbox/func_cmd_common.h"
#include "smartbox/smartbox.h"
#include "smartbox/function.h"
#include "smartbox/config.h"

#include "key_event_deal.h"

#if (SMART_BOX_EN && TCFG_APP_MUSIC_EN)
#include "music/music_player.h"

#pragma pack(1)
struct _MUSIC_STATUS_info {
    u8 status;
    u32 cur_time;
    u32 total_time;
    u8 cur_dev;
};
#pragma pack()

enum {
    MUSIC_FUNC_PP = 0x1,
    MUSIC_FUNC_PREV,
    MUSIC_FUNC_NEXT,
    MUSIC_FUNC_MODE,
    MUSIC_FUNC_EQ_MODE,
    MUSIC_FUNC_REWIND,
    MUSIC_FUNC_FAST_FORWORD,
};


enum {
    REPEAT_MODE_ALL = 0x1,
    REPEAT_MODE_DEV,
    REPEAT_MODE_ONE,
    REPEAT_MODE_RANDOM,
    REPEAT_MODE_FOLDER,
};

const u8 smartbox_repeat_mode_remap[] = {
    [FCYCLE_ALL] = REPEAT_MODE_ALL,
    [FCYCLE_ONE] = REPEAT_MODE_ONE,
    [FCYCLE_FOLDER] = REPEAT_MODE_FOLDER,
    [FCYCLE_RANDOM] = REPEAT_MODE_RANDOM,
};


#define SMARTBOX_MUSIC_FILE_NAME_MAX_LIMIT			(128)

// 本文件内用到
static u8 music_func_add_one_attr_ex(u8 *buf, u16 max_len, u8 offset, u8 type, u8 *data, u8 size, u8 att_size)
{
    if (offset + size + 2 > max_len) {
        printf("add attr err\n");
        return 0;
    }
    buf[offset] = att_size + 1;
    buf[offset + 1] = type;
    memcpy(&buf[offset + 2], data, size);
    return size + 2;
}

// 本文件内用到
static u8 mucis_func_add_one_attr_continue(u8 *buf, u16 max_len, u8 offset, u8 type, u8 *data, u8 size)
{
    if ((offset + size) > max_len) {
        printf("add attr err 2\n");
        return 0;
    }
    memcpy(&buf[offset], data, size);
    return size;
}

u32 music_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    u16 offset = 0;
#if (TCFG_APP_MUSIC_EN)
    u8 app = app_get_curr_task();
    if (app != APP_MUSIC_TASK) {
        return 0;
    }
    ///获取当前播放状态
    struct smartbox *smart = (struct smartbox *) priv;
    FILE *file = music_player_get_file_hdl();
    if (file == NULL) {
        printf("music_function_info file err\n");
        return 0;
    }

    if (mask & BIT(MUSIC_INFO_ATTR_STATUS)) {
        /* printf("MUSIC_INFO_ATTR_STATUS\n"); */
        struct _MUSIC_STATUS_info music_info;
        music_info.status = music_player_get_play_status();
        music_info.cur_time = app_htonl(music_player_get_dec_cur_time());
        music_info.total_time = app_htonl(music_player_get_dec_total_time());
        char *logo = music_player_get_dev_cur();
        char *tmp = NULL;
        if (logo) {
            for (int i = 0; i < BS_DEV_MAX; i++) {
                tmp = smartbox_browser_dev_remap(i);
                if (tmp && strcmp(tmp, logo) == 0) {
                    music_info.cur_dev = i;
                    offset += add_one_attr(buf, buf_size, offset, MUSIC_INFO_ATTR_STATUS, (u8 *)&music_info, sizeof(music_info));
                    break;
                }
            }
        }
    }

    if (mask & BIT(MUSIC_INFO_ATTR_FILE_NAME)) {
        /* printf("MUSIC_INFO_ATTR_FILE_NAME\n"); */
        u8 *lfn_buf = zalloc(512);
        if (lfn_buf) {
            int lfn_len = fget_name(file, lfn_buf, 512);
            lfn_len = file_name_cut(lfn_buf, lfn_len, SMARTBOX_MUSIC_FILE_NAME_MAX_LIMIT);
            struct vfs_attr tmp_attr = {0};
            fget_attrs(file, &tmp_attr);
            u32 clust = app_htonl(tmp_attr.sclust);
            /* u32 clust = tmp_attr.sclust; */
            /* printf("clust %x\n", clust); */
            u8 code_type = 1;
            u8 *tmp_buf = lfn_buf;
            if (lfn_buf[0] == '\\' && lfn_buf[1] == 'U') {
                code_type = 0;
                lfn_len -= 2;
                tmp_buf += 2;
            }

            offset += music_func_add_one_attr_ex(buf, buf_size, offset, MUSIC_INFO_ATTR_FILE_NAME, (u8 *)&clust, sizeof(u32), lfn_len + sizeof(code_type) + sizeof(clust));
            offset += mucis_func_add_one_attr_continue(buf, buf_size, offset, MUSIC_INFO_ATTR_FILE_NAME, (u8 *)&code_type, 1);
            offset += mucis_func_add_one_attr_continue(buf, buf_size, offset, MUSIC_INFO_ATTR_FILE_NAME, tmp_buf, lfn_len);
            free(lfn_buf);
        }
    }

    if (mask & BIT(MUSIC_INFO_ATTR_FILE_PLAY_MODE)) {
        /* printf("MUSIC_INFO_ATTR_FILE_PLAY_MODE\n"); */
        u8 play_mode = music_player_get_repeat_mode();
        /* printf("play_mode = %d\n", play_mode); */
        play_mode = smartbox_repeat_mode_remap[play_mode];
        /* printf("remap = %d\n", play_mode); */
        offset += add_one_attr(buf, buf_size, offset, MUSIC_INFO_ATTR_FILE_PLAY_MODE, &play_mode, 1);
    }
#endif
    return offset;
}

bool music_func_set(void *priv, u8 *data, u16 len)
{
    /* printf("%s, %d\n", __func__, data[0]); */
#if (TCFG_APP_MUSIC_EN)
    switch (data[0]) {
    case MUSIC_FUNC_PP:
        app_task_put_key_msg(KEY_MUSIC_PP, 0);
        break;
    case MUSIC_FUNC_PREV:
        app_task_put_key_msg(KEY_MUSIC_PREV, 0);
        break;
    case MUSIC_FUNC_NEXT:
        app_task_put_key_msg(KEY_MUSIC_NEXT, 0);
        break;
    case MUSIC_FUNC_MODE:
        app_task_put_key_msg(KEY_MUSIC_CHANGE_REPEAT, 0);
        break;
    case MUSIC_FUNC_REWIND:
        /* printf("MUSIC_FUNC_REWIND = %d\n", (int)(data[1] << 8 | data[2])); */
        music_player_fr((int)(data[1] << 8 | data[2]));
        break;
    case MUSIC_FUNC_FAST_FORWORD:
        /* printf("MUSIC_FUNC_FAST_FORWORD = %d\n", (int)(data[1] << 8 | data[2])); */
        music_player_ff((int)(data[1] << 8 | data[2]));
        break;
    default:
        break;
    }

    SMARTBOX_UPDATE(MUSIC_FUNCTION_MASK,
                    BIT(MUSIC_INFO_ATTR_STATUS) | BIT(MUSIC_INFO_ATTR_FILE_PLAY_MODE));
#endif
    return true;
}

void music_func_stop(void)
{
    if (music_player_get_play_status() != FILE_DEC_STATUS_PLAY) {
        app_task_put_key_msg(KEY_MUSIC_PP, 0);
    }
}

#else

u32 music_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    return 0;
}

bool music_func_set(void *priv, u8 *data, u16 len)
{
    return true;
}

void music_func_stop(void)
{

}

#endif

