#include "file_delete.h"
#include "smartbox/smartbox.h"
#include "system/includes.h"
#include "system/fs/fs.h"
#include "btstack_3th_protocol_user.h"
#include "dev_manager.h"
#include "key_event_deal.h"
#include "app_task.h"
#include "common/dev_status.h"
#include "bt/bt.h"
#include "common/dev_status.h"
#include "app_task.h"
#include "app_msg.h"
#include "clock_cfg.h"
#include "file_operate/file_manager.h"

#if (SMART_BOX_EN && TCFG_DEV_MANAGER_ENABLE)

#define FILE_DELELET_TIMEOUT 				(10*1000)

#define FILE_DEL_DEBUG_EN
#ifdef FILE_DEL_DEBUG_EN
#define file_del_printf printf
#else
#define file_del_printf(...)
#endif

struct __file_del {
    struct __dev 			*dev;//当前设备节点
    struct vfscan			*fsn;//设备扫描句柄
    u8 						scandisk_break;
    u8						busy;
    u16 					timerout;
    void	(*end_callback)(void);
};

struct __file_del *file_d = NULL;
#define __this	   file_d

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

//*----------------------------------------------------------------------------*/
/**@brief    文件删除扫盘匹配文件过程系统消息获取处理接口
   @param
   @return   1：中断文件扫描处理， 0:继续扫盘
   @note	 可以在这个函数内相应所需的消息事件
*/
/*----------------------------------------------------------------------------*/
static int file_delete_scandisk_break(void)
{
    ///注意：
    ///需要break fsn的事件， 请在这里拦截,
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
                logo = dev_manager_get_phy_logo(__this->dev);
                ///响应设备插拔打断
                if (event->u.dev.event == DEVICE_EVENT_OUT) {
                    log_i("__func__ = %s logo=%s evt_logo=%s \n", __FUNCTION__, logo, evt_logo);
                    if (logo && evt_logo && (0 == strcmp(logo, evt_logo))) {
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
            if (__this->scandisk_break) {
                app_task_put_key_msg(event->u.key.event, (int)event->u.key.value);
                return -1;
            }
            break;
        }
        break;
    }
    if (__this->scandisk_break) {
        ///查询到需要打断的事件， 返回1， 并且重新推送一次该事件,跑主循环处理流程
        file_del_printf("\n--func=%s, line=%d\n", __FUNCTION__, __LINE__);
        sys_event_notify(event);
        file_del_printf("scandisk_break!!!!!!\n");
        return 1;
    } else {
        return 0;
    }
}

static void scan_enter(struct __dev *dev)
{
    __this->busy = 1;
    clock_add_set(SCAN_DISK_CLK);
}

static void scan_exit(struct __dev *dev)
{
    clock_remove_set(SCAN_DISK_CLK);
    __this->busy = 0;
}

static const struct __scan_callback scan_cb = {
    .enter = scan_enter,
    .exit = scan_exit,
    .scan_break = file_delete_scandisk_break,
};


//*----------------------------------------------------------------------------*/
/**@brief    文件删除超时处理
   @param
   @return
   @note	 这里是等待手机删除命令超时(即没有等到删除命令)
*/
/*----------------------------------------------------------------------------*/
static void file_delete_timeout(void *p)
{
    //批量删除超时， 退出删除流程
    if (__this && __this->timerout) {
        file_del_printf("file_delete_timeout !!\n");
        file_delete_end();
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    文件删除结束处理
   @param
   @return
   @note	 释放资源
*/
/*----------------------------------------------------------------------------*/
void file_delete_end(void)
{
    if (__this) {
        dev_manager_scan_disk_release(__this->fsn);
        if (__this->timerout) {
            sys_timeout_del(__this->timerout);
            __this->timerout = 0;
        }
        if (__this->end_callback) {
            __this->end_callback();
        }
        free(__this);
        __this = NULL;
        file_del_printf("file_delete_end\n");
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    文件删除开始处理
   @param	 OpCode_SN:命名包的sn码， 命名回复时用到
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void file_delete_start(u8 OpCode_SN, u8 *data, u16 len)
{
    FILE *file = NULL;
    u8 reason = 0;
    u8 last = data[0];
    u32 dev_handle = READ_BIG_U32(data + 1);
    u8  type = data[5];
    u32 sclust = READ_BIG_U32(data + 6);
    file_del_printf("last = %d,dev_handle = %d,sclust = %d,type = %d,\n", last, dev_handle, sclust, type);

    char *logo = smartbox_browser_dev_remap(dev_handle);
    if (logo == NULL) {
        goto __err;
    }

    if (__this == NULL) {
        __this = zalloc(sizeof(struct __file_del));
        if (__this == NULL) {
            goto __err;
        }
    }

    if (__this->timerout) {
        sys_timeout_del(__this->timerout);
        __this->timerout = 0;
    }

    if (__this->fsn == NULL) {
        __this->dev = dev_manager_find_spec(logo, 0);
        if (__this->dev) {
            __this->fsn = dev_manager_scan_disk(__this->dev, NULL, scan_parm, 0, &scan_cb);
            if (__this->fsn == NULL) {
                free(__this);
                __this = NULL;
                goto __err;
            }
            file_del_printf("%s, fscan ok\n", __FUNCTION__);
        }
    } else {
        file_del_printf("%s, fscan aready\n", __FUNCTION__);
        if (__this->busy) {
            //正在执行删除， 回复APP繁忙
            printf("file delete busy\n");
            JL_CMD_response_send(JL_OPCODE_FILE_DELETE, JL_PRO_STATUS_BUSY, OpCode_SN, NULL, 0);
            return ;
        }
    }

    file = file_manager_select(__this->dev, __this->fsn, FSEL_BY_SCLUST, sclust, &scan_cb);//根据文件簇号查找断点文件
    if (file) {
        fdelete(file);
        file_del_printf("delete file ok!!\n");
        JL_CMD_response_send(JL_OPCODE_FILE_DELETE, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
        if (last == 0) {
            if (__this->timerout == 0) {
                __this->timerout = sys_timeout_add(NULL, file_delete_timeout, FILE_DELELET_TIMEOUT);
            } else {
                sys_timer_modify(__this->timerout, FILE_DELELET_TIMEOUT);
            }
            //批量删除， 还没有删除完毕， 等待继续删除
            file_del_printf("wait delete file more\n");
            return;
        }
    } else {
        file_del_printf("file delete, no this file\n");
        JL_CMD_response_send(JL_OPCODE_FILE_DELETE, JL_PRO_STATUS_PARAM_ERR, OpCode_SN, NULL, 0);
    }
    file_delete_end();
    return ;

__err:
    JL_CMD_response_send(JL_OPCODE_FILE_DELETE, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0);
    file_delete_end();
}


//*----------------------------------------------------------------------------*/
/**@brief    文件删除初始化
   @param	 end_callback:文件删除结束回调处理
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void file_delete_init(void (*end_callback)(void))
{
    if (__this) {
        printf("file_delete_init aready err!!\n");
        return ;
    }
    __this = zalloc(sizeof(struct __file_del));
    if (__this == NULL) {
        printf("file_delete_init fail\n");
        if (end_callback) {
            end_callback();
        }
        return ;
    }
    __this->end_callback = end_callback;
    //启动定时器， 防止APP后续没有发执行删除的命令过来， 可以超时退出
    __this->timerout = sys_timeout_add(NULL, file_delete_timeout, 2000);
}
#else
void file_delete_init(void (*end_callback)(void))
{
}
void file_delete_start(u8 OpCode_SN, u8 *data, u16 len)
{
}
void file_delete_end(void)
{
}
#endif


