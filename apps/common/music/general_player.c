#include "app_config.h"
#include "key_event_deal.h"
#include "audio_enc.h"
#include "general_player.h"
#include "dev_manager.h"
#include "bt/bt.h"
#include "common/dev_status.h"
#include "app_task.h"
#include "app_msg.h"
#include "media/audio_decoder.h"
#include "file_operate/file_manager.h"
#include "audio_dec/audio_dec_file.h"
///播放参数，文件扫描时用，文件后缀等
static const char scan_parm[] = "-t"
#if (TCFG_DEC_MP3_ENABLE)
                                "MP1MP2MP3"
#endif
#if (TCFG_DEC_WMA_ENABLE)
                                "WMA"
#endif
#if ( TCFG_DEC_WAV_ENABLE || TCFG_DEC_DTS_ENABLE)
                                "WAVDTS"
#endif
#if (TCFG_DEC_FLAC_ENABLE)
                                "FLA"
#endif
#if (TCFG_DEC_APE_ENABLE)
                                "APE"
#endif
#if (TCFG_DEC_M4A_ENABLE)
                                "M4AAAC"
#endif
#if (TCFG_DEC_M4A_ENABLE || TCFG_DEC_ALAC_ENABLE)
                                "MP4"
#endif
#if (TCFG_DEC_AMR_ENABLE)
                                "AMR"
#endif
#if (TCFG_DEC_DECRYPT_ENABLE)
                                "SMP"
#endif
#if (TCFG_DEC_MIDI_ENABLE)
                                "MID"
#endif
                                " -sn -r"
                                ;

struct __general_player {
    struct __dev    *dev;
    struct vfscan   *fsn;
    FILE            *file;
    struct __scan_callback *scan_cb;
    u8  scandisk_break;
};

static struct __general_player *general_player = NULL;
#define __this general_player

int general_player_init(struct __scan_callback *scan_cb)
{
    printf("the general_player init\n");
    __this = zalloc(sizeof(struct __general_player));
    if (__this == NULL) {
        return -1;
    } else {
        __this->scan_cb = scan_cb;
        return 0;
    }
}

void general_player_exit()
{
    if (__this) {
        general_player_stop(1);
        free(__this);
        __this = NULL;
    }
}

void general_player_stop(u8 fsn_release)
{
    if (__this == NULL) {
        return ;
    }
    ///停止解码
    file_dec_close();
    if (__this->file) {
        fclose(__this->file);
        __this->file = NULL;
    }
    if (fsn_release && __this->fsn) {
        ///根据播放情景， 通过设定flag决定是否需要释放fscan， 释放后需要重新扫盘!!!
        dev_manager_scan_disk_release(__this->fsn);
        __this->fsn = NULL;
    }
}


//可以添加播放回调函数
static int general_player_decode_start(FILE *file)
{
    int ret = 0;

    if (file) {
        ///get file short name
        u8 file_name[12 + 1] = {0}; //8.3+\0
        fget_name(file, file_name, sizeof(file_name));
        log_i("\n");
        log_i("file name: %s\n", file_name);
        log_i("\n");
    }
    ret = file_dec_create(NULL, NULL);
    if (ret) {
        return GENERAL_PLAYER_ERR_NO_RAM;
    }
    ret = file_dec_open(file, NULL);
    if (ret) {
        return GENERAL_PLAYER_ERR_DECODE_FAIL;
    }
    return GENERAL_PLAYER_SUCC;
}

static const char *general_player_get_phy_dev(int *len)
{
    if (__this) {
        char *logo = dev_manager_get_logo(__this->dev);
        if (logo) {
            char *str = strstr(logo, "_rec");
            if (str) {
                ///录音设备,切换到音乐设备播放
                if (len) {
                    *len =  strlen(logo) - strlen(str);
                }
            } else {
                if (len) {
                    *len =  strlen(logo);
                }
            }
            return logo;
        }
    }
    if (len) {
        *len =  0;
    }
    return NULL;
}

int general_player_scandisk_break(void)
{
    ///注意：
    ///需要break fsn的事件， 请在这里拦截,
    ///需要结合MUSIC_PLAYER_ERR_FSCAN错误，做相应的处理
    int msg[32] = {0};
    struct sys_event *event = NULL;
    char *logo = NULL;
    char *evt_logo = NULL;
    app_task_get_msg(msg, ARRAY_SIZE(msg), 0);
    switch (msg[0]) {
    case APP_MSG_SYS_EVENT:
        event = (struct sys_event *)(&msg[1]);
        switch (event->type) {
        case SYS_DEVICE_EVENT:
            switch ((u32)event->arg) {
            case DRIVER_EVENT_FROM_SD0:
            case DRIVER_EVENT_FROM_SD1:
            case DRIVER_EVENT_FROM_SD2:
                evt_logo = (char *)event->u.dev.value;
            case DEVICE_EVENT_FROM_OTG:
                if ((u32)event->arg == DEVICE_EVENT_FROM_OTG) {
                    evt_logo = (char *)"udisk0";
                }
                ///设备上下线底层推出的设备逻辑盘符是跟跟音乐设备一致的（音乐/录音设备, 详细看接口注释）
                int str_len   = 0;
                logo = general_player_get_phy_dev(&str_len);
                //logo = dev_manager_get_logo(__this->dev);
                ///响应设备插拔打断
                if (event->u.dev.event == DEVICE_EVENT_OUT) {
                    log_i("__func__ = %s logo=%s evt_logo=%s %d\n", __FUNCTION__, logo, evt_logo, str_len);
                    if (logo && (0 == memcmp(logo, evt_logo, str_len))) {
                        ///相同的设备才响应
                        __this->scandisk_break = 1;
                    }
                } else {
                    ///响应新设备上线
                    __this->scandisk_break = 1;
                }
                if (__this->scandisk_break == 0) {
                    log_i("__func__ = %s DEVICE_EVENT_OUT TODO\n", __FUNCTION__);
                    dev_status_event_filter(event);
                    log_i("__func__ = %s DEVICE_EVENT_OUT OK\n", __FUNCTION__);
                }
                break;
            }
            break;
        case SYS_BT_EVENT:
            if (bt_background_event_handler_filter(event)) {
                __this->scandisk_break = 1;
            }
            break;
        case SYS_KEY_EVENT:
            switch (event->u.key.event) {
            case KEY_CHANGE_MODE:
                ///响应切换模式事件
                __this->scandisk_break = 1;
                break;
            }
            break;
        }
        break;
    }
    if (__this->scandisk_break) {
        ///查询到需要打断的事件， 返回1， 并且重新推送一次该事件,跑主循环处理流程
        printf("\n--func=%s, line=%d\n", __FUNCTION__, __LINE__);
        sys_event_notify(event);
        printf("scandisk_break!!!!!!\n");
        return 1;
    } else {
        return 0;
    }
}

int general_play_by_sculst(char *logo, u32 sclust)
{
    int ret = 0;
    if (logo == NULL) {
        return GENERAL_PLAYER_ERR_PARM;
    }
    char *cur_logo = dev_manager_get_logo(__this->dev);
    if (cur_logo == NULL || strcmp(cur_logo, logo) != 0) {
        if (cur_logo != NULL) {
            general_player_stop(1);
        }
        __this->dev = dev_manager_find_spec(logo, 1);
        if (!__this->dev) {
            return GENERAL_PLAYER_ERR_DEV_NOFOUND;
        }
        __this->fsn = dev_manager_scan_disk(__this->dev, NULL, scan_parm, 0, __this->scan_cb);
        if (!__this->fsn) {
            return GENERAL_PLAYER_ERR_FSCAN;
        }
        __this->file = file_manager_select(__this->dev, __this->fsn, FSEL_BY_SCLUST, sclust, __this->scan_cb);
        if (!__this->file) {
            return GENERAL_PLAYER_ERR_FILE_NOFOUND;
        }
        ret = general_player_decode_start(__this->file);
        if (ret != GENERAL_PLAYER_SUCC) {
            return ret;
        }
    } else {
        general_player_stop(0);
        __this->file = file_manager_select(__this->dev, __this->fsn, FSEL_BY_SCLUST, sclust, __this->scan_cb);
        if (!__this->file) {
            return GENERAL_PLAYER_ERR_FILE_NOFOUND;
        }
        ret = general_player_decode_start(__this->file);
        if (ret != GENERAL_PLAYER_SUCC) {
            return ret;
        }
    }
    return GENERAL_PLAYER_ERR_NULL;
}

void general_player_test()
{
    int tmp = general_player_init(NULL);
    if (!tmp) {
        printf("the init succ\n");
        general_play_by_sculst((char *)"sd0", 6);
    } else {
        printf("the init fail\n");
    }
}
