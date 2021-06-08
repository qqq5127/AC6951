#include "smartbox/func_cmd/fm_func.h"
#include "smartbox/func_cmd_common.h"
#include "smartbox/function.h"
#include "smartbox/config.h"
#include "smartbox/event.h"
#include "app_action.h"

#include "key_event_deal.h"

#if (SMART_BOX_EN)

#if (TCFG_APP_FM_EN)
#include "fm/fm_api.h"
#include "fm/fm_rw.h"

#pragma pack(1)
struct _FM_STATUS_INFO {
    u8 status;
    u8 cur_channel;
    u16 cur_fre;
    u8 mode;
};
#pragma pack()

#define FM_INFO_ATTR_STATUS                    (0)
#define FM_INFO_ATTR_FRE                       (1)

#define  SCANE_ALL        					(0x01)
#define  SCANE_DOWN        					(0x02)
#define  SCANE_UP          					(0x03)

enum {
    FM_FUNC_MUSIC_PP = 0x1,
    FM_FUNC_PREV_FREQ,
    FM_FUNC_NEXT_FREQ,
    FM_FUNC_PREV_STATION,
    FM_FUNC_NEXT_STATION,
    FM_FUNC_SCAN,
    FM_FUNC_SET_STATION,
    FM_FUNC_DEL_STATION,
    FM_FUNC_SET_FRE,
    FM_FUNC_DEL_FRE,
};

enum {
    FM_FUNC_SCAN_ALL = 0x0,
    FM_FUNC_SCAN_ALL_DOWN,
    FM_FUNC_SCAN_ALL_UP,
    FM_FUNC_SCAN_STOP,
};

enum {
    FM_FUNC_STATUS_PLAY = 0x01,
    FM_FUNC_STATUS_PAUSE,
    FM_FUNC_STATUS_SCANING,
};

extern u8 fm_get_scan_flag(void);
extern u8 fm_get_fm_dev_mute(void);

static u8 fm_get_status(u8 scan_all_en)
{
    u8 scan_flag = fm_get_scan_flag();
    u8 fm_dev_mute = fm_get_fm_dev_mute();
    if (SCANE_DOWN == scan_flag || SCANE_UP == scan_flag) {
        return FM_FUNC_STATUS_SCANING;
    } else if (SCANE_ALL == scan_flag) {
        if (scan_all_en && 0 == fm_dev_mute) {
            smartbox_function_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
        }
        return FM_FUNC_STATUS_SCANING;
    } else if (0 == fm_dev_mute) {
        return FM_FUNC_STATUS_PLAY;
    } else {
        return FM_FUNC_STATUS_PAUSE;
    }
}

static u16 fm_scan_timer = 0;
static void fm_scan_state_func(void *priv)
{
    /* printf("fm_scan_state_func\n"); */
    if (FM_FUNC_STATUS_SCANING == fm_get_status(1)) {
        return;
    }
    if (fm_scan_timer) {
        sys_hi_timer_del(fm_scan_timer);
        fm_scan_timer = 0;
    }
    // 更新
    smartbox_function_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
}

bool fm_func_set(void *priv, u8 *data, u16 len)
{
    /* printf("fm_func_set\n"); */
    /* put_buf(data, len); */
    u8 fun_cmd = data[0];
    u16 cmd_param = 0;
    u16 param_len = len - 1;
    switch (fun_cmd) {
    case FM_FUNC_MUSIC_PP:
        printf("fm pp\n");
        fm_volume_pp();
        smartbox_function_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
        break;
    case FM_FUNC_PREV_FREQ:
        printf("fm prev fre\n");
        fm_prev_freq();
        smartbox_function_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
        break;
    case FM_FUNC_NEXT_FREQ:
        printf("fm next fre\n");
        fm_next_freq();
        smartbox_function_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
        break;
    case FM_FUNC_PREV_STATION:
        printf("fm prev station\n");
        fm_prev_station();
        smartbox_function_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
        break;
    case FM_FUNC_NEXT_STATION:
        printf("fm next station\n");
        fm_next_station();
        smartbox_function_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
        break;
    case FM_FUNC_SCAN:
        printf("fm scan\n");
        if (1 == param_len) {
            cmd_param = data[1];
            switch (cmd_param) {
            case FM_FUNC_SCAN_ALL:
                printf("fm scan all\n");
                fm_scan_all();
                break;
            case FM_FUNC_SCAN_ALL_DOWN:
                printf("fm scan prev\n");
                fm_scan_up();
                break;
            case FM_FUNC_SCAN_ALL_UP:
                printf("fm scan next\n");
                fm_scan_down();
                break;
            case FM_FUNC_SCAN_STOP:
                printf("fm scan stop\n");
                if (FM_FUNC_STATUS_SCANING == fm_get_status(0)) {
                    fm_scan_stop();
                }
                break;
            default:
                printf("fm scan default\n");
                break;
            }
            if (0 == fm_scan_timer) {
                fm_scan_timer = sys_hi_timer_add(NULL, fm_scan_state_func, 1000);
            }
        }
        break;
    case FM_FUNC_SET_STATION:
        printf("fm sel station\n");
        extern void fm_sel_station(u8 channel);
        if (1 == param_len) {
            cmd_param = data[1];
            fm_sel_station(cmd_param);
        }
        break;
    case FM_FUNC_DEL_STATION:
        printf("fm del station\n");
        break;
    case FM_FUNC_SET_FRE:
        printf("fm sel fre\n");
        extern u8 fm_set_fre(u16 fre);
        printf("param_len : %d\n", param_len);
        for (u8 i = 0; i < param_len; i++) {
            printf("data[%d] : 0x%x\n", i, data[i + 1]);
        }
        if (2 == param_len) {
            cmd_param = (data[1] << 8 | data[2]) * 10;
            printf("fm target = %d\n", cmd_param);
            fm_set_fre(cmd_param);
        }
        break;
    case FM_FUNC_DEL_FRE:
        printf("fm del ...\n");
        break;
    default:
        break;
    }

    switch (fun_cmd) {
    case FM_FUNC_SET_FRE:
    case FM_FUNC_SET_STATION:
        // 更新状态
        smartbox_msg_post(USER_MSG_SMARTBOX_FM_UPDATE_STATE, 1, (int)priv);
        break;
    }

    return true;
}

u32 fm_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    u16 offset  = 0;
    if (mask & BIT(FM_INFO_ATTR_STATUS)) {
        extern u8 fm_get_cur_channel(void);
        extern u16 fm_get_cur_fre(void);
        extern u8 fm_get_mode(void);

        struct _FM_STATUS_INFO fm_status_info;
        u16 fre = fm_get_cur_fre();
        fre = app_htons(fre);
        fm_status_info.cur_fre = fre;
        fm_status_info.cur_channel = fm_get_cur_channel();
        fm_status_info.status = fm_get_status(0);
        if (FM_FUNC_STATUS_PLAY != fm_status_info.status) {
            fm_status_info.status = 0;
        }
        fm_status_info.mode = fm_get_mode();

        /* printf("fm_func_get, %d, %d, %d, %d\n", fm_status_info.cur_fre, fm_status_info.cur_channel, fm_status_info.status, fm_status_info.mode); */
        offset = add_one_attr(buf, buf_size, offset, FM_INFO_ATTR_STATUS, (u8 *)&fm_status_info, sizeof(fm_status_info));
    }

    if (mask & BIT(FM_INFO_ATTR_FRE)) {
        FM_INFO fm_info = {0};
        fm_read_info(&fm_info);
        /* printf("get_fm_channel_fre_info:\n"); */
        /* put_buf(&fm_info, sizeof(fm_info)); */
        u8 fm_info_size = fm_info.total_chanel * 3 + 1;
        u8 *fm_fre_info = zalloc(fm_info_size);
        if (fm_fre_info) {
            u8 *ptr = fm_fre_info;
            u16 fm_fre = 0;
            u8 total = 1;
            u8 i;
            u8 k;
            *ptr++ = fm_info.total_chanel;
            for (i = 0; i < (MEM_FM_LEN); i++) {
                for (k = 0; k < 8; k++) {
                    if (fm_info.dat[i] & BIT(k)) {
                        fm_fre = i * 8 + k + 874;
                        fm_fre = app_htons(fm_fre);
                        *ptr++ = total++;
                        memcpy(ptr, &fm_fre, sizeof(fm_fre));
                        ptr += 2;
                    }
                }
            }
            offset += add_one_attr(buf, buf_size, offset, FM_INFO_ATTR_FRE, fm_fre_info, fm_info_size);
            free(fm_fre_info);
        }
    }

    return offset;
}

void smartbot_fm_msg_deal(int msg)
{
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL) {
        return ;
    }
    switch (msg) {
    case KEY_MUSIC_PP:
    case KEY_FM_PREV_FREQ:
    case KEY_FM_NEXT_FREQ:
    case KEY_FM_PREV_STATION:
    case KEY_FM_NEXT_STATION:
    case (int)-1:
        smartbox_function_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
        break;
    case KEY_FM_SCAN_ALL:
    case KEY_FM_SCAN_ALL_UP:
    case KEY_FM_SCAN_ALL_DOWN:
    case KEY_FM_SCAN_DOWN:
    case KEY_FM_SCAN_UP:
        if (!fm_get_scan_flag()) {
            smartbox_function_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
        }
        break;
    }
}

void fm_func_stop(void)
{
    if (fm_get_fm_dev_mute) {
        fm_volume_pp();
    }
}





#else

bool fm_func_set(void *priv, u8 *data, u16 len)
{
    return true;
}

u32 fm_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    return 0;
}

void smartbot_fm_msg_deal(int msg)
{

}

void fm_func_stop(void)
{

}
#endif
#endif
