#include "music/music.h"
#include "music_player.h"
#include "system/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "app_main.h"
#include "clock_cfg.h"
#include "app_action.h"
#include "breakpoint.h"
#include "key_event_deal.h"
#include "tone_player.h"
#include "dev_multiplex_api.h"
#include "ui/ui_api.h"
#include "bt/bt.h"
#include "smartbox/smartbox.h"
#include "ui_manage.h"
#include "audio_dec.h"
#include "common/dev_status.h"
#include "clock_cfg.h"
#include "system/fs/fs.h"
#include "user_api/app_status_api.h"

/*************************************************************
   此文件函数主要是music模式按键处理和事件处理

	void app_music_task()
   music模式主函数

	static int music_sys_event_handler(struct sys_event *event)
   music模式系统事件所有处理入口

	static void music_task_close(void)
	music模式退出

**************************************************************/

#define LOG_TAG_CONST       APP_MUSIC
#define LOG_TAG             "[APP_MUSIC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_APP_MUSIC_EN

static u8 music_idle_flag = 1;

///模式参数结构体
struct __music_task_parm {
    u8 type;
    int val;
};

///music模式控制结构体
struct __music {
    struct __music_task_parm task_parm;
    u16 file_err_counter;//错误文件统计
    u8 file_play_direct;//0:下一曲， 1：上一曲
    u8 scandisk_break;//扫描设备打断标志
    char device_tone_dev[16];
#if SD_BAUD_RATE_CHANGE_WHEN_SCAN
    u32 old_speed;
#endif
};
struct __music music_hdl;
#define __this (&music_hdl)

static struct __breakpoint *breakpoint = NULL;


static void music_player_play_start(void);

///设备提示音使能
#define MUSIC_DEVICE_TONE_EN				0
#if (MUSIC_DEVICE_TONE_EN)
struct __dev_tone {
    char *logo;
    char *phy_logo;
    char *tone_path;
    u16	 index;
};

enum {
    ///0x1000起始为了不要跟提示音的IDEX_TONE_重叠了
    DEVICE_INDEX_UDISK = 0x1000,
    DEVICE_INDEX_UDISK_REC,
    DEVICE_INDEX_SD0,
    DEVICE_INDEX_SD0_REC,
    DEVICE_INDEX_SD1,
    DEVICE_INDEX_SD1_REC,
};
const struct __dev_tone device_tone[] = {
    {"udisk0", 		"udisk0", 	TONE_RES_ROOT_PATH"tone/udisk.*", 	DEVICE_INDEX_UDISK}			,
    {"udisk0_rec",  "udisk0", 	TONE_RES_ROOT_PATH"tone/udisk.*", 	DEVICE_INDEX_UDISK_REC}		,
    {"sd0",  		"sd0", 		TONE_RES_ROOT_PATH"tone/sd.*", 	 	DEVICE_INDEX_SD0}		 	,
    {"sd0_rec",  	"sd0", 		TONE_RES_ROOT_PATH"tone/sd.*", 	 	DEVICE_INDEX_SD0_REC}		,
    {"sd1",  		"sd1", 		TONE_RES_ROOT_PATH"tone/sd.*", 	 	DEVICE_INDEX_SD1}			,
    {"sd1_rec",  	"sd1", 		TONE_RES_ROOT_PATH"tone/sd.*", 	 	DEVICE_INDEX_SD1_REC}		,
};
static void  music_tone_play_end_callback(void *priv, int flag);
int music_device_tone_play(char *logo)
{
    if (logo == NULL) {
        return false;
    }
    printf("__this->device_tone_dev = %s, logo =%s\n", __this->device_tone_dev, logo);
    char *phy_logo = dev_manager_get_phy_logo(dev_manager_find_spec(logo, 0));
    if (phy_logo && (strcmp(__this->device_tone_dev, phy_logo) == 0)) {
        log_i("[%s, %d]the same phy dev, no need device tone!!\n", logo, __LINE__);
        return false;
    }
    for (int i = 0; i < ARRAY_SIZE(device_tone); i++) {
        if (strcmp(device_tone[i].logo, logo) == 0) {
            log_i("[%s, %d]device_tone play \n", logo, __LINE__);
            memset(__this->device_tone_dev, 0, sizeof(__this->device_tone_dev));
            memcpy(__this->device_tone_dev, device_tone[i].phy_logo, strlen(device_tone[i].phy_logo));
            tone_play_with_callback_by_name(device_tone[i].tone_path, 1, music_tone_play_end_callback, (void *)device_tone[i].index);
            return true;
        }
    }
    log_i("[%s, %d]device_tone play \n", logo, __LINE__);
    return false;
}
#endif



#if TCFG_LFN_EN
static u8 music_file_name[128] = {0}; //长文件名
u16    music_file_name_len = 0;
#else
static u8 music_file_name[12 + 1] = {0}; //8.3+\0
u16    music_file_name_len = 0;
#endif

const char *music_file_get_cur_name(int *len, int *is_unicode)
{
    if (music_file_name[0] == '\\' && music_file_name[1] == 'U') {
        *is_unicode = 1 ;
        *len = music_file_name_len - 2;
        return (const char *)(music_file_name + 2);
    }
    *is_unicode = 0 ;
    *len = music_file_name_len;
    return (const char *)music_file_name;
}


static void music_set_dev_sync_mode(char *logo, u8 mode)
{
    if (logo) {
        struct imount *mount_hdl = NULL;
        u8 async_mode  = mode;

        if ((!memcmp(logo, "udisk0", strlen("udisk0")))
            || (!memcmp(logo, "udisk0_rec", strlen("udisk0_rec")))) {
            struct file_dec_hdl *dec = get_file_dec_hdl();
            if (dec) {
                mount_hdl = dev_manager_get_mount_hdl(dev_manager_find_spec(logo, 0));
                if (mount_hdl) {
                    if (dec->file_dec.decoder.dec_ops->coding_type == AUDIO_CODING_APE
                        || dec->file_dec.decoder.dec_ops->coding_type == AUDIO_CODING_FLAC
                        || dec->file_dec.decoder.dec_ops->coding_type == AUDIO_CODING_DTS
                        || dec->file_dec.decoder.dec_ops->coding_type == AUDIO_CODING_WAV) {
                        ///指定解码格式开启该功能
                        dev_ioctl(mount_hdl->dev.fd, IOCTL_SET_ASYNC_MODE, (u32)async_mode);
                    } else {
                        ///不指定的解码格式不开启
                        async_mode = 0;
                        dev_ioctl(mount_hdl->dev.fd, IOCTL_SET_ASYNC_MODE, (u32)async_mode);
                    }
                    printf("udisk, set usb ASYNC = %d\n", async_mode);
                }
            }
        } else {
            //不是udisk, 如果udisk在线， 将udisk设置成非AYSNC_MODE
            mount_hdl = dev_manager_get_mount_hdl(dev_manager_find_spec("udisk0", 0));
            if (mount_hdl) {
                async_mode = 0;
                printf("not udisk, set usb ASYNC = 0\n");
                dev_ioctl(mount_hdl->dev.fd, IOCTL_SET_ASYNC_MODE, (u32)async_mode);
            }
        }
    }
}


//*----------------------------------------------------------------------------*/
/**@brief    music 解码成功回调
   @param    priv:私有参数， parm:暂时未用
   @return
   @note	 此处可以做一些用户操作， 如断点保存， 显示， 获取播放信息等
*/
/*----------------------------------------------------------------------------*/
static void music_player_play_success(void *priv, int parm)
{
    char *logo = music_player_get_dev_cur();

    //使能usb预读功能
    music_set_dev_sync_mode(logo, 1);

    log_i("\n\n----------------music_player_play_success----------------------\n");
    log_i("cur dev = %s\n", logo);
    log_i("total dev = %d\n", dev_manager_get_total(1));
    log_i("cur filenum = %d\n", music_player_get_file_cur());
    log_i("totol filenum = %d\n", music_player_get_file_total());
    log_i("totol time = %d\n", music_player_get_dec_total_time());
    log_i("sclust = %d\n", music_player_get_file_sclust());
    log_i("dir_cur = %d\n", music_player_get_dir_cur());
    log_i("dir_total = %d\n", music_player_get_dir_total());
    log_i("file indir = %d\n", music_player_get_fileindir_number());
    music_file_name_len =  fget_name(music_player_get_file_hdl(), music_file_name, sizeof(music_file_name));
    int unicode = 0;
    if ((music_file_name[0] == '\\') && (music_file_name[1] == 'U')) {
        unicode = 1;
        music_file_name_len -= 2;
        log_i("cur file  = %s, len = %d, unicode = %d\n", music_file_name + 2, music_file_name_len, unicode);
    } else {
        log_i("cur file  = %s, len = %d, unicode = %d\n", music_file_name, music_file_name_len, unicode);
    }
    log_i("\n");



    ///save breakpoint, 只保存文件信息
    if (music_player_get_playing_breakpoint(breakpoint, 0) == true) {
        breakpoint_vm_write(breakpoint, logo);
    }

    int analaz =  music_player_lrc_analy_start();
    ///show ui
    UI_SHOW_MENU(MENU_FILENUM, 1000, music_player_get_file_cur(), NULL);
    UI_MSG_POST("music_start:show_lyric=%4:dev=%4:filenum=%4:total_filenum=%4", !analaz, logo, music_player_get_file_cur(), music_player_get_file_total());
    ///smartbox info update
    SMARTBOX_UPDATE(MUSIC_FUNCTION_MASK,
                    BIT(MUSIC_INFO_ATTR_STATUS) | BIT(MUSIC_INFO_ATTR_FILE_NAME) | BIT(MUSIC_INFO_ATTR_FILE_PLAY_MODE));

}
//*----------------------------------------------------------------------------*/
/**@brief    music 解码结束回调处理
   @param
   @return
   @note	此处统一将错误通过消息的方式发出， 在key msg中统一响应
*/
/*----------------------------------------------------------------------------*/
static void music_player_play_end(void *priv, int parm)
{
    log_i("music_player_play_end\n");
    ///这里推出消息， 目的是在music主流程switch case统一入口
    app_task_put_key_msg(KEY_MUSIC_PLAYER_END, parm);
}
//*----------------------------------------------------------------------------*/
/**@brief    music 解码错误回调
   @param
   @return
   @note	此处统一将错误通过消息的方式发出， 在key msg中统一响应
*/
/*----------------------------------------------------------------------------*/
static void music_player_decode_err(void *priv, int parm)
{
    log_i("music_player_decode_err\n");
    ///这里推出消息， 目的是在music主流程switch case统一入口
    app_task_put_key_msg(KEY_MUSIC_PLAYER_DEC_ERR, parm);
}
//*----------------------------------------------------------------------------*/
/**@brief    music 播放器扫盘打断接口
   @param
   @return  1:打断当前扫描，0:正常扫描
   @note	设备扫描耗时长的情况下， 此接口提供打断机制
*/
/*----------------------------------------------------------------------------*/
static int music_player_scandisk_break(void)
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
                logo = music_player_get_phy_dev(&str_len);
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
                //其他按键case 在这里增加
            }
            ///因为TWS转发sys_event_notify需要用原始的按键序号， 未经过按键表处理， 所以这里要特殊处理
            if (__this->scandisk_break) {
                app_task_put_key_msg(event->u.key.event, (int)event->u.key.value);
                printf("key break scan!!");
                return 1;
            }
            break;
        }
        break;
    }
    if (__this->scandisk_break) {
        ///查询到需要打断的事件， 返回1， 并且重新推送一次该事件,跑主循环处理流程
        sys_event_notify(event);
        printf("scandisk_break!!!!!!\n");
        return 1;
    } else {
        return 0;
    }
}

static const struct __player_cb music_player_callback = {
    .start 		= music_player_play_success,
    .end   		= music_player_play_end,
    .err		= music_player_decode_err,
//    .fsn_break  = music_player_scandisk_break,
};

static void scan_enter(struct __dev *dev)
{
#if SD_BAUD_RATE_CHANGE_WHEN_SCAN
    struct imount *mount_hdl = dev_manager_get_mount_hdl(dev);
    if (mount_hdl) {
        if ((!memcmp(dev_manager_get_logo(dev), "sd0", strlen("sd0")))
            || (!memcmp(dev_manager_get_logo(dev), "sd1", strlen("sd1")))) {
            dev_ioctl(mount_hdl->dev.fd, IOCTL_GET_SPEED, (u32)&__this->old_speed);   //获取sd速度
            if (SD_BAUD_RATE_CHANGE_WHEN_SCAN > __this->old_speed) {                  //设定速度大于获取的才加速
                dev_ioctl(mount_hdl->dev.fd, IOCTL_SET_SPEED, SD_BAUD_RATE_CHANGE_WHEN_SCAN);
            }
        }
    }
#endif
    clock_add_set(SCAN_DISK_CLK);
}

static void scan_exit(struct __dev *dev)
{
#ifdef SD_BAUD_RATE_CHANGE_WHEN_SCAN
    struct imount *mount_hdl = dev_manager_get_mount_hdl(dev);
    if (mount_hdl) {
        if ((!memcmp(dev_manager_get_logo(dev), "sd0", strlen("sd0")))
            || (!memcmp(dev_manager_get_logo(dev), "sd1", strlen("sd1")))) {
            if (SD_BAUD_RATE_CHANGE_WHEN_SCAN > __this->old_speed) {
                dev_ioctl(mount_hdl->dev.fd, IOCTL_SET_SPEED, __this->old_speed);//恢复原速
            }
        }
    }
#endif
    clock_remove_set(SCAN_DISK_CLK);
}

static const struct __scan_callback scan_cb = {
    .enter = scan_enter,
    .exit = scan_exit,
    .scan_break = music_player_scandisk_break,
};

//*----------------------------------------------------------------------------*/
/**@brief    music 模式提示音播放结束处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void  music_tone_play_end_callback(void *priv, int flag)
{
    u32 index = (u32)priv;
    char *logo = NULL;

    if (APP_MUSIC_TASK != app_get_curr_task()) {
        log_error("tone callback task out \n");
        return;
    }

    switch (index) {
    case IDEX_TONE_MUSIC:
        ///提示音播放结束， 启动播放器播放
        music_player_play_start();
        break;
#if (MUSIC_DEVICE_TONE_EN)
    case DEVICE_INDEX_UDISK:
    case DEVICE_INDEX_UDISK_REC:
    case DEVICE_INDEX_SD0:
    case DEVICE_INDEX_SD0_REC:
    case DEVICE_INDEX_SD1:
    case DEVICE_INDEX_SD1_REC:
        for (int i = 0; i < ARRAY_SIZE(device_tone); i++) {
            if (index == device_tone[i].index) {
                logo = device_tone[i].logo;
                break;
            }
        }
        app_task_put_key_msg(KEY_MUSIC_DEVICE_TONE_END, (int)logo);
        break;
#endif
    default:
        break;
    }
}


//*----------------------------------------------------------------------------*/
/**@brief    music 模式解码错误处理
   @param    err:错误码，详细错误码描述请看MUSIC_PLAYER错误码表枚举
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void music_player_err_deal(int err)
{
    u16 msg = KEY_NULL;
    char *logo = NULL;
    if (err != MUSIC_PLAYER_ERR_NULL && err != MUSIC_PLAYER_ERR_DECODE_FAIL) {
        __this->file_err_counter = 0;///清除错误文件累计
    }

    if (err != MUSIC_PLAYER_ERR_NULL && err != MUSIC_PLAYER_SUCC) {
        log_e("music player err = %d\n", err);
    }

    switch (err) {
    case MUSIC_PLAYER_SUCC:
        __this->file_err_counter = 0;
        break;
    case MUSIC_PLAYER_ERR_NULL:
        break;
    case MUSIC_PLAYER_ERR_POINT:
    case MUSIC_PLAYER_ERR_NO_RAM:
        msg = KEY_MUSIC_PLAYER_QUIT;//退出音乐模式
        break;
    case MUSIC_PLAYER_ERR_DECODE_FAIL:
        if (__this->file_err_counter >= music_player_get_file_total()) {
            __this->file_err_counter = 0;
            dev_manager_set_valid_by_logo(music_player_get_dev_cur(), 0);///将设备设置为无效设备
            if (dev_manager_get_total(1) == 0) {//参数为1 ：获取所有有效设备  参数0：获取所有设备
                msg = KEY_MUSIC_PLAYER_QUIT;//没有设备了，退出音乐模式
            } else {
                msg = KEY_MUSIC_AUTO_NEXT_DEV;///所有文件都是错误的， 切换到下一个设备
            }
        } else {
            __this->file_err_counter ++;
            if (__this->file_play_direct == 0) {
                msg = KEY_MUSIC_NEXT;//播放下一曲
            } else {
                msg = KEY_MUSIC_PREV;//播放上一曲
            }
        }
        break;
    case MUSIC_PLAYER_ERR_DEV_NOFOUND:
        log_e("MUSIC_PLAYER_ERR_DEV_NOFOUND \n");
        if (dev_manager_get_total(1) == 0) {//参数为1 ：获取所有有效设备  参数0：获取所有设备
            msg = KEY_MUSIC_PLAYER_QUIT;///没有设备在线， 退出音乐模式
        } else {
            msg = KEY_MUSIC_PLAYER_START;///没有找到指定设备， 播放之前的活动设备
        }
        break;

    case MUSIC_PLAYER_ERR_FSCAN:
        ///需要结合music_player_scandisk_break中处理的标志位处理
        if (__this->scandisk_break) {
            __this->scandisk_break = 0;
            ///此处不做任何处理， 打断的事件已经重发， 由重发事件执行后续处理
            break;
        }
    case MUSIC_PLAYER_ERR_DEV_READ:
    case MUSIC_PLAYER_ERR_DEV_OFFLINE:
        log_e("MUSIC_PLAYER_ERR_DEV_OFFLINE \n");
        logo = music_player_get_dev_cur();
        if (dev_manager_online_check_by_logo(logo, 1)) {
            ///如果错误失败在线， 并且是播放过程中产生的，先记录下断点
            if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
                music_player_stop(0);//先停止，防止下一步操作VM卡顿
                breakpoint_vm_write(breakpoint, logo);
            }
            if (err == MUSIC_PLAYER_ERR_FSCAN) {
                dev_manager_set_valid_by_logo(logo, 0);///将设备设置为无效设备
            } else {
                //针对读错误， 因为时间推到应用层有延时导致下一个模式判断不正常， 此处需要将设备卸载
                dev_manager_unmount(logo);
            }
        }
        if (dev_manager_get_total(1) == 0) {
            app_status_handler(APP_STATUS_MUSIC_QUIT);
            msg = KEY_MUSIC_PLAYER_QUIT;///没有设备在线， 退出音乐模式
        } else {
            msg = KEY_MUSIC_AUTO_NEXT_DEV;///切换设备
        }
        break;
    case MUSIC_PLAYER_ERR_FILE_NOFOUND:
        ///查找文件有扫盘的可能，也需要结合music_player_scandisk_break中处理的标志位处理
        if (__this->scandisk_break) {
            __this->scandisk_break = 0;
            ///此处不做任何处理， 打断的事件已经重发， 由重发事件执行后续处理
            break;
        }
    case MUSIC_PLAYER_ERR_PARM:
        logo = music_player_get_dev_cur();
        if (dev_manager_online_check_by_logo(logo, 1)) {
            if (music_player_get_file_total()) {
                msg = KEY_MUSIC_PLAYER_PLAY_FIRST;///有文件,播放第一个文件
                break;
            }
        }

        if (dev_manager_get_total(1) == 0) {
            msg = KEY_MUSIC_PLAYER_QUIT;//没有设备了，退出音乐模式
        } else {
            msg = KEY_MUSIC_AUTO_NEXT_DEV;
        }
        break;
    case MUSIC_PLAYER_ERR_FILE_READ://文件读错误
        msg = KEY_MUSIC_NEXT;//播放下一曲
        break;
    }
    if (msg != KEY_NULL) {
        app_task_put_key_msg(msg, 0);
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    music 按键消息入口
   @param    无
   @return   1、消息已经处理，common不再处理  0、消息发送到common处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int music_key_event_opr(struct sys_event *event)
{
    int ret = true;
    int err = MUSIC_PLAYER_ERR_NULL;
    u8 vol, auto_next_dev;
    int mode ;
    char *logo = NULL;

    int msg[2];
    msg[0] = event->u.key.event;
    msg[1] = event->u.key.value;//
    static int msg_demo = 0;

    log_i("music task msg = %d\n", msg[0]);

    switch (msg[0]) {
    case KEY_MUSIC_DEVICE_TONE_END:
#if (MUSIC_DEVICE_TONE_EN)
        logo = (char *)msg[1];
        log_i("KEY_MUSIC_DEVICE_TONE_END %s\n", logo);
        if (logo) {
            if (true == breakpoint_vm_read(breakpoint, logo)) {
                err = music_player_play_by_breakpoint(logo, breakpoint);
            } else {
                err = music_player_play_first_file(logo);
            }
        }
        break;
#endif
    case KEY_MUSIC_PLAYER_START:
        log_i("KEY_MUSIC_PLAYER_START !!\n");
        app_status_handler(APP_STATUS_MUSIC_PLAY);
        ///断点播放活动设备
        logo = dev_manager_get_logo(dev_manager_find_active(1));
        if (music_player_get_play_status() == FILE_DEC_STATUS_PLAY) {
            if (music_player_get_dev_cur() && logo) {
                ///播放的设备跟当前活动的设备是同一个设备，不处理
                if (0 == strcmp(logo, music_player_get_dev_cur())) {
                    log_w("the same dev!!\n");
                    break;
                }
            }
        }
#if (MUSIC_DEVICE_TONE_EN)
        if (music_device_tone_play(logo) == true) {
            break;
        }
#endif
        if (true == breakpoint_vm_read(breakpoint, logo)) {
            err = music_player_play_by_breakpoint(logo, breakpoint);
        } else {
            err = music_player_play_first_file(logo);
        }
        break;
    ///播放器退出处理
    case KEY_MUSIC_PLAYER_QUIT:
        log_i("KEY_MUSIC_PLAYER_QUIT !!\n");
        app_task_switch_next();
        break;
    ///结束消息处理
    case KEY_MUSIC_PLAYER_END:
        log_i("KEY_MUSIC_PLAYER_END\n");
        err = music_player_end_deal(msg[1]);
        break;
    //播放器解码错误处理
    case KEY_MUSIC_PLAYER_DEC_ERR:
        err = music_player_decode_err_deal(msg[1]);
        break;
    ///播放执行类消息
    case  KEY_MUSIC_PP:
        log_i("KEY_MUSIC_PP\n");
        err = music_player_pp();
        if (music_player_get_play_status() == FILE_DEC_STATUS_PLAY) {
            app_status_handler(APP_STATUS_MUSIC_PLAY);
            ui_update_status(STATUS_MUSIC_PLAY);
        } else {
            app_status_handler(APP_STATUS_MUSIC_PP);
            ui_update_status(STATUS_MUSIC_PAUSE);
        }
        printf("=============================================%d\n", music_player_get_play_status());
        break;
    case KEY_MUSIC_PLAYER_AUTO_NEXT:
        log_i("KEY_MUSIC_PLAYER_AUTO_NEXT\n");
        err = music_player_play_auto_next();
        break;
    case KEY_MUSIC_PLAYER_PLAY_FIRST:
        log_i("KEY_MUSIC_PLAYER_PLAY_FIRST\n");
        err = music_player_play_first_file(NULL);
        break;
    case  KEY_MUSIC_PREV:
        log_i("KEY_MUSIC_PREV\n");
        app_status_handler(APP_STATUS_MUSIC_FFR);
        __this->file_play_direct = 1;
        err = music_player_play_prev();
        break;
    case  KEY_MUSIC_NEXT:
        log_i("KEY_MUSIC_NEXT\n");
        app_status_handler(APP_STATUS_MUSIC_FFR);
        __this->file_play_direct = 0;
        err = music_player_play_next();
        break;
    case KEY_MUSIC_PLAYE_PREV_FOLDER:
        log_i("KEY_MUSIC_PLAYE_PREV_FOLDER\n");
        err = music_player_play_folder_prev();
        break;
    case KEY_MUSIC_PLAYE_NEXT_FOLDER:
        log_i("KEY_MUSIC_PLAYE_NEXT_FOLDER\n");
        err = music_player_play_folder_next();
        break;

    case KEY_MUSIC_AUTO_NEXT_DEV:
    case  KEY_MUSIC_CHANGE_DEV:
        log_i("KEY_MUSIC_CHANGE_DEV\n");
        auto_next_dev = ((msg[0] == KEY_MUSIC_AUTO_NEXT_DEV) ? 1 : 0);
        logo = music_player_get_dev_next(auto_next_dev);
        printf("next dev = %s\n", logo);
        if (logo == NULL) { ///找不到下一个设备，不响应设备切换
            break;
        }
        ///切换设备前先保存一下上一个设备的断点信息,包括文件和解码信息
        if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
            music_player_stop(0);//先停止，防止下一步操作VM卡顿
            breakpoint_vm_write(breakpoint, music_player_get_dev_cur());
        }
#if (MUSIC_DEVICE_TONE_EN)
        if (music_device_tone_play(logo) == true) {
            break;
        }
#endif
        if (true == breakpoint_vm_read(breakpoint, logo)) {
            err = music_player_play_by_breakpoint(logo, breakpoint);
        } else {
            err = music_player_play_first_file(logo);
        }
        break;
    case KEY_MUSIC_PLAYE_REC_FOLDER_SWITCH:
        log_i("KEY_MUSIC_PLAYE_REC_FOLDER_SWITCH\n");
#if (TCFG_RECORD_FOLDER_DEV_ENABLE)
        ///尝试保存断点
        if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
            breakpoint_vm_write(breakpoint, music_player_get_dev_cur());
        }
        if (true == breakpoint_vm_read(breakpoint, music_player_get_cur_music_dev())) {
            err = music_player_play_record_folder(NULL, breakpoint);
        } else {
            err = music_player_play_record_folder(NULL, NULL);
        }
#endif//TCFG_RECORD_FOLDER_DEV_ENABLE
        break;
    case KEY_MUSIC_PLAYE_BY_DEV_FILENUM:
        log_i("KEY_MUSIC_PLAYE_BY_DEV_FILENUM, file_number = %d\n", msg[1]);
        logo = dev_manager_get_logo(dev_manager_find_active(1));
        err = music_player_play_by_number(logo, msg[1]);
        break;
    case KEY_MUSIC_PLAYE_BY_DEV_SCLUST:
        log_i("KEY_MUSIC_PLAYE_BY_DEV_SCLUST\n");
        logo = dev_manager_get_logo(dev_manager_find_active(1));
        err = music_player_play_by_sclust(logo, msg[1]);
        break;
    case KEY_MUSIC_PLAYE_BY_DEV_PATH:
        log_i("KEY_MUSIC_PLAYE_BY_DEV_PATH\n");
        err = music_player_play_by_path((char *)"udisk0", "/sin.wav");///this is a demo
        break;


    ///非播放执行类消息
    case KEY_MUSIC_FF:
        log_i("KEY_MUSIC_FF\n");
        app_status_handler(APP_STATUS_MUSIC_FFR);
        music_player_ff(3);
        break;
    case KEY_MUSIC_FR:
        log_i("KEY_MUSIC_FR\n");
        app_status_handler(APP_STATUS_MUSIC_FFR);
        music_player_fr(3);
        break;
    case KEY_MUSIC_CHANGE_REPEAT:
        log_i("KEY_MUSIC_CHANGE_REPEAT\n");
        mode = music_player_change_repeat_mode();
        if (mode > 0) {
            UI_SHOW_MENU(MENU_MUSIC_REPEATMODE, 1000, mode, NULL);
        }
        break;
    case KEY_MUSIC_DELETE_FILE:
        log_i("KEY_MUSIC_DELETE_FILE\n");
        err = music_player_delete_playing_file();
        break;
    case KEY_MUSIC_PLAYER_AB_REPEAT_SWITCH:
        file_dec_ab_repeat_switch();
        break;

    default:
        ret = false;
        break;
    }


    ///错误处理
    music_player_err_deal(err);
    ///smartbox info update
    SMARTBOX_UPDATE(MUSIC_FUNCTION_MASK,
                    BIT(MUSIC_INFO_ATTR_STATUS) | BIT(MUSIC_INFO_ATTR_FILE_PLAY_MODE));

    return ret;
}


//*----------------------------------------------------------------------------*/
/**@brief    music 设备事件响应接口
   @param    无
   @return   1、当前消息已经处理，comomon不再做处理 0、common统一处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int music_sys_event_handler(struct sys_event *event)
{
    int err = 0;
    char *logo = NULL;
    char *evt_logo = NULL;
    switch (event->type) {
    case SYS_KEY_EVENT:
        return music_key_event_opr(event);
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
            int str_len   = 0;
            logo = music_player_get_phy_dev(&str_len);
            log_i("evt_logo =%s, logo = %s len =%d\n", evt_logo, logo, str_len);
            if (event->u.dev.event == DEVICE_EVENT_OUT) {
                if (logo == NULL) {
                    break;
                }
                if (logo && (0 == memcmp(logo, evt_logo, str_len))) {
                    ///相同的设备才响应
                    if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
                        breakpoint_vm_write(breakpoint, logo);
                    }
                    memset(__this->device_tone_dev, 0, sizeof(__this->device_tone_dev));
                    ///停止解码,防止设备掉线后还继续使用
                    music_player_stop(1);
                    ///重新选择活动设备播放
                    app_task_put_key_msg(KEY_MUSIC_PLAYER_START, 0);//卸载了设备再执行
                    log_i("KEY_MUSIC_PLAYER_START AFTER UMOUNT\n");
                }
            } else {
#if (MUSIC_DEV_ONLINE_START_AFTER_MOUNT_EN == 0)
                music_task_dev_online_start();
#endif
            }
            break;
        default://switch((u32)event->arg)
            break;
        }
        break;//SYS_DEVICE_EVENT
    default://switch (event->type)
        break;
    }

    return false;
}

//*----------------------------------------------------------------------------*/
/**@brief    music 模式切换前参数设置
   @param    type:播放方式,暂时支持：
  				  MUSIC_TASK_START_BY_NORMAL：首次播放按照正常断点播放
				  MUSIC_TASK_START_BY_SCLUST：首次播放按照簇号播放
			 val:播放参数
   @return
   @note	 首次播放执行参考music_player_play_start接口
*/
/*----------------------------------------------------------------------------*/
void music_task_set_parm(u8 type, int val)
{
    __this->task_parm.type = type;
    __this->task_parm.val = val;
}
//*----------------------------------------------------------------------------*/
/**@brief    music 模式首次播放
   @param    无
   @return
   @note	 切换到音乐模式前可以通过接口music_task_set_parm设置参数,
   			 进入音乐模式后会按照对应参数播放
*/
/*----------------------------------------------------------------------------*/
static void music_player_play_start(void)
{
    switch (__this->task_parm.type) {
    case MUSIC_TASK_START_BY_NORMAL:
        log_i("MUSIC_TASK_START_BY_NORMAL\n");
        app_task_put_key_msg(KEY_MUSIC_PLAYER_START, 0);
        break;
    case MUSIC_TASK_START_BY_SCLUST:
        log_i("MUSIC_TASK_START_BY_SCLUST\n");
        app_task_put_key_msg(KEY_MUSIC_PLAYE_BY_DEV_SCLUST, __this->task_parm.val);
        break;
    default:
        log_i("other MUSIC_TASK_START!!!\n");
        break;
    }
    ///提示音播放失败，直接推送KEY_MUSIC_PLAYER_START启动播放
}


//*----------------------------------------------------------------------------*/
/**@brief    music 模式新设备上线处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void music_task_dev_online_start(void)
{
    u8 save = 0;
    char *logo = music_player_get_dev_cur();
    if (logo && breakpoint) {
        ///新设备上线, 先记录当前设备断点， 然后播放活动设备
        if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
            save = 1;
            //这里不要直接记忆断点， 解码停了之后再记忆
            //breakpoint_vm_write(breakpoint, logo);
        }
    }
    ///停止解码，播放新活动设备
    music_player_stop(1);
    if (save && breakpoint) {
        breakpoint_vm_write(breakpoint, logo);
    }
    app_task_put_key_msg(KEY_MUSIC_PLAYER_START, 0);
    log_i("KEY_MUSIC_PLAYER_START AFTER MOUNT\n");
    //先挂载了设备再执行
}

//*----------------------------------------------------------------------------*/
/**@brief    music 模式初始化处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void music_task_start()
{
    printf("music task start!!!!!\n");
    music_idle_flag = 0;
    ui_update_status(STATUS_MUSIC_MODE);

#if (TCFG_LRC_LYRICS_ENABLE)
    extern int lrc_init(void);
    lrc_init();
#endif

    ///显示初始化
    UI_SHOW_WINDOW(ID_WINDOW_MUSIC);
    UI_SHOW_MENU(MENU_WAIT, 0, 0, NULL);
    ///时钟初始化
    clock_idle(MUSIC_IDLE_CLOCK);
    ///按键使能
    sys_key_event_enable();


    ///播放器初始化
    struct __player_parm parm = {0};
    parm.cb = &music_player_callback;
    parm.scan_cb = &scan_cb;
    music_player_creat(NULL, &parm);
    ///获取断点句柄， 后面所有断点读/写都需要用到
    breakpoint = breakpoint_handle_creat();
    ///初始化一些参数
    __this->file_err_counter = 0;
    __this->file_play_direct = 0;
    __this->scandisk_break = 0;
    memset(__this->device_tone_dev, 0, sizeof(__this->device_tone_dev));
}
//*----------------------------------------------------------------------------*/
/**@brief    music 模式退出处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void music_task_close()
{
    UI_HIDE_CURR_WINDOW();
    tone_play_stop_by_path(tone_table[IDEX_TONE_MUSIC]);//停止播放提示音
    char *logo = music_player_get_dev_cur();
    if (music_player_get_playing_breakpoint(breakpoint, 1) == true) {
        breakpoint_vm_write(breakpoint, logo);
    }

    music_set_dev_sync_mode(logo, 0);

    breakpoint_handle_destroy(&breakpoint);
    music_player_destroy();
#if (TCFG_LRC_LYRICS_ENABLE)
    extern void lrc_exit(void);
    lrc_exit();
#endif
    memset(__this, 0, sizeof(struct __music));
    music_idle_flag = 1;
}
//*----------------------------------------------------------------------------*/
/**@brief    music 在线检测  切换模式判断使用
   @param    无
   @return   1 设备在线 0 设备不在线
   @note
*/
/*----------------------------------------------------------------------------*/
int music_app_check(void)
{
    if (dev_manager_get_total(1)) {
        return true;
    }
    return false;
}

//*----------------------------------------------------------------------------*/
/**@brief    music 主任务
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void app_music_task()
{
    int res;
    int msg[32];
    music_task_start();
		set_pa_mode(2);

#if (MUSIC_DEVICE_TONE_EN)
    music_player_play_start();
#else
#if TCFG_DEC2TWS_ENABLE
    extern void set_tws_background_connected_flag(u8 flag);
    extern u8 get_tws_background_connected_flag();
    if (get_tws_background_connected_flag()) { //不播放提示音
        music_player_play_start();
        set_tws_background_connected_flag(0);
    } else
#endif
    {
        tone_play_with_callback_by_name(tone_table[IDEX_TONE_MUSIC], 1, music_tone_play_end_callback, (void *)IDEX_TONE_MUSIC);
    }
#endif


    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);
        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (music_sys_event_handler((struct sys_event *)(&msg[1])) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));
            }
            break;
        default:
            break;
        }
        if (app_task_exitting()) {
            music_task_close();
            return;
        }
    }
}

static u8 music_idle_query(void)
{
    return music_idle_flag;
}
REGISTER_LP_TARGET(music_lp_target) = {
    .name = "music",
    .is_idle = music_idle_query,
};

#else

void app_music_task()
{

}

#endif



