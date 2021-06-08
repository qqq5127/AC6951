#include "common/dev_status.h"
#include "dev_manager.h"
#include "dev_update.h"
#include "app_config.h"
#include "dev_multiplex_api.h"
#include "app_task.h"
#include "smartbox/smartbox.h"
#include "music/music.h"


usb_dev g_usb_id = (usb_dev) - 1;
//*----------------------------------------------------------------------------*/
/**@brief    设备上线预处理
   @param
   			 event：设备事件
   @return
   @note	 根据特殊情景，这里可以处理一些例如io复用处理
*/
/*----------------------------------------------------------------------------*/
static int dev_manager_add_prepare(struct sys_event *event)
{
    int ret = 0;
    switch ((u32)event->arg) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        //传入0 sd0 1：sd1
        mult_sd_online_mount_before(!!((int)event->arg - DRIVER_EVENT_FROM_SD0), g_usb_id);
        break;
#if TCFG_UDISK_ENABLE || TCFG_HOST_AUDIO_ENABLE
    case DEVICE_EVENT_FROM_OTG:
        ///host mount
        mult_usb_mount_before(g_usb_id);

        ret = usb_host_mount(g_usb_id,
                             MOUNT_RETRY,
                             MOUNT_RESET,
                             MOUNT_TIMEOUT);
        break;
#endif
    }
    return ret;
}
//*----------------------------------------------------------------------------*/
/**@brief    设备上线后处理
   @param
   			 event：设备事件
   			 err：设备上线错误码
   @return
   @note	 根据特殊情景，这里可以处理一些例如io复用处理
*/
/*----------------------------------------------------------------------------*/
static int dev_manager_add_after(struct sys_event *event, int err)
{
    int ret = 0;
    switch ((u32)event->arg) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        //传入0 sd0 1：sd1
        mult_sd_online_mount_after(!!((int)event->arg - DRIVER_EVENT_FROM_SD0), g_usb_id, err);
        break;
#if     TCFG_UDISK_ENABLE
    case DEVICE_EVENT_FROM_OTG:
        mult_usb_online_mount_after(g_usb_id, err);
        break;
#endif
    }
    return ret;
}
//*----------------------------------------------------------------------------*/
/**@brief    设备下线预处理
   @param
   			 event：设备事件
   @return
   @note	 根据特殊情景，这里可以处理一些例如io复用处理
*/
/*----------------------------------------------------------------------------*/
static int dev_manager_del_prepare(struct sys_event *event)
{
    int ret = 0;
    switch ((u32)event->arg) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        mult_sd_offline_before((char *)event->u.dev.value, g_usb_id);
        break;
#if TCFG_UDISK_ENABLE || TCFG_HOST_AUDIO_ENABLE
    case DEVICE_EVENT_FROM_OTG:
        ///host umount
        usb_host_unmount(g_usb_id);
        break;
#endif
    }
    return ret;
}
//*----------------------------------------------------------------------------*/
/**@brief    设备下线后处理
   @param
   			 event：设备事件
   			 err：设备下线错误码
   @return
   @note	 根据特殊情景，这里可以处理一些例如io复用处理
*/
/*----------------------------------------------------------------------------*/
static int dev_manager_del_after(struct sys_event *event, int err)
{
    int ret = 0;
    switch ((u32)event->arg) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        break;
#if TCFG_UDISK_ENABLE
    case DEVICE_EVENT_FROM_OTG:
        mult_usb_mount_offline(g_usb_id);
        g_usb_id = (usb_dev) - 1;
        break;
#endif
    }
    return ret;
}
//*----------------------------------------------------------------------------*/
/**@brief    设备事件响应处理
   @param
   			 event：设备事件
   @return
   @note	 设备上线和下线会动态添加到设备管理链表中，
   			 通过dev_manager系列接口可以进行设备操作，例如：例如查找、检查在线等
*/
/*----------------------------------------------------------------------------*/
int dev_status_event_filter(struct sys_event *event)
{
    int ret = true;
    int err = 0;
    char *add = NULL;
    char *del = NULL;
    switch ((u32)event->arg) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        printf("DEVICE_EVENT_FROM_SD%d\n", (u32)event->arg - DRIVER_EVENT_FROM_SD0);
        if (event->u.dev.event == DEVICE_EVENT_IN) {
            ///上线处理, 设备管理挂载
            printf("DEVICE_EVENT_IN\n");
            add = (char *)event->u.dev.value;
        } else {
            ///下线处理,设备管理卸载
            printf("DEVICE_EVENT_OUT\n");
            del = (char *)event->u.dev.value;
        }
        break;
#if TCFG_UDISK_ENABLE || TCFG_HOST_AUDIO_ENABLE
    case DEVICE_EVENT_FROM_OTG:
        printf("DEVICE_EVENT_FROM_OTG %d", event->u.dev.event);
        g_usb_id = ((char *)event->u.dev.value)[2] - '0';
        if (((char *)event->u.dev.value)[0] == 'h') {
            ///是host, 准备挂载设备, usb host mount/umout
            if (event->u.dev.event == DEVICE_EVENT_IN) {
                ///加入到设备管理列表
                printf("DEVICE_EVENT_IN\n");
                add = (char *)"udisk0";
            } else {
                printf("DEVICE_EVENT_OUT\n");
                del = (char *)"udisk0";
            }
        }
        break;
    case DEVICE_EVENT_FROM_USB_HOST:
        printf("DEVICE_EVENT_FROM_USB_HOST %s", (const char *)event->u.dev.value);
        break;
#endif
    default:
        ret = false;
        break;
    }

    ////dev manager add/del操作
    if (add) {
        err = dev_manager_add_prepare(event);
        if (!err) {
            err = dev_manager_add(add);
            if (!err) {
#if (MUSIC_DEV_ONLINE_START_AFTER_MOUNT_EN)
                music_task_dev_online_start();
#endif
#if (TCFG_DEV_UPDATE_IF_NOFILE_ENABLE == 0)
                if (app_check_curr_task(APP_PC_TASK) == false) {
                    ///检查设备升级
                    dev_update_check(add);
                }
#endif/*TCFG_DEV_UPDATE_IF_NOFILE_ENABLE*/
            } else {
                ret = false;
            }
        } else {
            ret = false;
        }
        dev_manager_add_after(event, err);
    }
    if (del) {
        dev_manager_del_prepare(event);
        dev_manager_del(del);
        dev_manager_del_after(event, 0);
    }
    SMARTBOX_UPDATE(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_DEV_INFO));
    return ret;
}

