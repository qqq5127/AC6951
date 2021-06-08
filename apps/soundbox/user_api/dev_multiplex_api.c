#include "includes.h"
#include "app_config.h"
#include "dev_multiplex_api.h"
#include "usb/otg.h"
#include "dev_manager.h"
#include "music/music_player.h"


#define LOG_TAG_CONST       APP_ACTION
#define LOG_TAG             "[APP_ACTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


extern u8 sd_io_suspend(u8 sdx, u8 sd_io);
extern u8 sd_io_resume(u8 sdx, u8 sd_io);

static u8 msg_notify_enable = 0xff;


static u8 sd_first_online = 0;
static u16 resume = 0;

#define USB_ACTIVE 0x1
#define SD_ACTIVE  0x2

#define MULT_IO_IDLE    (0)
#define MULT_IO_ACTIVE  (BIT(0))
#define MULT_IO_SUSPEND (BIT(1))

#define USB_RESET_DELAY     20

struct mult_io {
    u8 sd_status: 3;
    u8 usb_status: 3;
    u8 active: 2;
};

struct mult_io mult_flag = {0};


/*----------------------------------------------------------------------------*/
/**@brief    库的调用函数 重载了库驱动 如果返回真则可以发送sd卡的上下线消息
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0

int sd_notify_enable(int sd_id)
{
    return msg_notify_enable & BIT(sd_id);
}

#endif

/*----------------------------------------------------------------------------*/
/**@brief    设置是否 sd卡可以推上下线消息
   @param    1:可以推 0：不推
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void set_sd_notify_enable(int sd_id, u8 en)
{
    if (en) {
        msg_notify_enable |=  BIT(sd_id);
    } else {
        msg_notify_enable &= ~BIT(sd_id);
    }
}



void mult_sdio_resume_clean()
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    if (resume) {
        sd_io_resume(TCFG_DM_MULTIPLEX_WITH_SD_PORT, 2);//sd_data0 io 状态会被还原
        resume = 0;
    }
#endif
}


void mult_sdio_resume()
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    if (resume) {
        resume--;
        if (!resume) {
            sd_io_resume(TCFG_DM_MULTIPLEX_WITH_SD_PORT, 2);//sd_data0 io 状态会被还原
        }
    }
#endif
}

int mult_sdio_suspend()
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    if (resume) {
        printf("%s resume count = %d\n", __FUNCTION__, resume);
        return 0;
    }
__try_t:
    if (sd_io_suspend(TCFG_DM_MULTIPLEX_WITH_SD_PORT, 2)) {
        os_time_dly(1);
        printf("wait %s\n", __FUNCTION__);
        goto __try_t;
    }
    resume++;
    //配置绑定的sd_data0 io 为高阻
    gpio_set_direction(TCFG_USB_SD_MULTIPLEX_IO, 1);
    gpio_set_pull_up(TCFG_USB_SD_MULTIPLEX_IO, 0);
    gpio_set_pull_down(TCFG_USB_SD_MULTIPLEX_IO, 0);
    gpio_set_die(TCFG_USB_SD_MULTIPLEX_IO, 0);

#endif
    return 0;
}



void mult_usb_suspend(usb_dev id)
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    if (id != (usb_dev) - 1 && usb_otg_online(id) == HOST_MODE) {
        printf("mult_usb_suspend\n");
        usb_otg_io_suspend(id);
    }
#endif
}



int mult_usb_resume(usb_dev id, u8 mount, u32 delay)
{
    int err = -1;
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    if (id != (usb_dev) - 1 && usb_otg_online(id) == HOST_MODE) {
        usb_otg_io_resume(id);
        /*
         * 有一部分U盘在DM和SD DAT0复用时，插入U盘或者模式切到播U盘，
         * 需要把DPDM拉低一段时间，让U盘自己认为插入了主机
         */
        if (delay) {
            usb_otg_suspend(id, 0);
            os_time_dly(delay);
            usb_otg_resume(id);
        }

        if (mount) {
            if (dev_manager_online_check_by_logo("udisk0", 0)) {
                dev_manager_unmount("udisk0");
            }
            if (usb_host_remount(id, MOUNT_RETRY, MOUNT_RESET, MOUNT_TIMEOUT, 0)) {
                log_error("udisk remount fail\n");
            } else {
                err = dev_manager_mount("udisk0");
                return err;
            }
        } else {
            err = 0;
        }
    }
#endif
    return err;
}




int dev_sd_change_usb()
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    if ((mult_flag.sd_status == MULT_IO_ACTIVE) ||
        (mult_flag.usb_status == MULT_IO_SUSPEND)) {
        printf("sd_change_usb start \n");
        mult_sdio_suspend();
        mult_flag.sd_status = MULT_IO_SUSPEND;
        mult_usb_resume(g_usb_id, 1, USB_RESET_DELAY);
        mult_flag.usb_status = MULT_IO_ACTIVE;
        mult_flag.active = USB_ACTIVE;
        printf("sd_change_usb finsh \n");
    }
#endif
    return 0;
}


int dev_usb_change_sd()
{

#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    if ((mult_flag.usb_status == MULT_IO_ACTIVE) ||
        (mult_flag.sd_status == MULT_IO_SUSPEND)) {
        printf("dev_usb_change_sd start \n");
        mult_usb_suspend(g_usb_id);
        os_time_dly(1);//
        mult_flag.usb_status = MULT_IO_SUSPEND;
        mult_sdio_resume();
        mult_flag.sd_status = MULT_IO_ACTIVE;
        mult_flag.active = SD_ACTIVE;
        printf("dev_usb_change_sd finsh \n");
    }
    return 0;
#endif
    return true;
}


int mult_sd_online_mount_before(int sd_dev, usb_dev usb_id)
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    if (TCFG_DM_MULTIPLEX_WITH_SD_PORT != sd_dev) {
        return 0;
    }
    sd_first_online = 1;
    music_player_stop(1);//停止解码
    if (mult_flag.active == USB_ACTIVE) {
        mult_usb_suspend(g_usb_id);
    }
    mult_sdio_resume_clean();
    mult_flag.usb_status = MULT_IO_SUSPEND;
    mult_flag.sd_status = MULT_IO_ACTIVE;
    mult_flag.active = SD_ACTIVE;
#endif
    return 0;
}


int mult_sd_online_mount_after(int sd_dev, usb_dev usb_id, int err)
{
    int ret = 0;
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    if (TCFG_DM_MULTIPLEX_WITH_SD_PORT != sd_dev) {
        return 0;
    }

    if (err) {
        mult_flag.sd_status = MULT_IO_SUSPEND;
        mult_sdio_suspend();
        mult_usb_resume(usb_id, 1, USB_RESET_DELAY);
        mult_flag.usb_status = MULT_IO_ACTIVE;
        mult_flag.active = USB_ACTIVE;
    }
#endif
    return ret;
}

int mult_sd_offline_before(void *logo, usb_dev usb_id)
{
    int ret = 0 ;
    /* u8 dev_logo[16]; */
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    int str_len   = 0;
    if (!sd_first_online) { //没有收到过上线消息不处理下线
        return 0;
    }
    if ((TCFG_DM_MULTIPLEX_WITH_SD_PORT == 0) && strcmp("sd0", logo)) {
        return 0;
    }
    if ((TCFG_DM_MULTIPLEX_WITH_SD_PORT == 1) && strcmp("sd1", logo)) {
        return 0;
    }
    printf("MULT_SD_OFFLINE_BEFORE\n");
    if (mult_flag.active == SD_ACTIVE) {
        const char *cur_logo = music_player_get_phy_dev(&str_len);

#if (TCFG_DM_MULTIPLEX_WITH_SD_PORT == 0)
        if (cur_logo && (0 == strncmp(cur_logo, "sd0", str_len))) {
            printf("SD0 STOP FIRST !!!");
            music_player_stop(1);//停止解码
        }

#else
        if (cur_logo && (0 == strncmp(cur_logo, "sd1", str_len))) {
            printf("SD1 STOP FIRST !!!");
            music_player_stop(1);//停止解码
        }
#endif

        if (dev_manager_check_by_logo(logo)) {
            dev_manager_del(logo);
        }
        /* sprintf(dev_logo, "%s%s", logo, "_rec"); */
        /* if (dev_manager_check_by_logo(dev_logo)) { */
        /* dev_manager_del(dev_logo); */
        /* } */
        mult_flag.sd_status = MULT_IO_IDLE;
        mult_sdio_suspend();
        mult_usb_resume(usb_id, 1, USB_RESET_DELAY);
        mult_flag.usb_status = MULT_IO_ACTIVE;
        mult_flag.active = USB_ACTIVE;

    } else if (mult_flag.active == USB_ACTIVE) {
        if (dev_manager_check_by_logo(logo)) {
            dev_manager_del(logo);
        }
        /* sprintf(dev_logo, "%s%s", logo, "_rec"); */
        /* if (dev_manager_check_by_logo(dev_logo)) { */
        /* dev_manager_del(dev_logo); */
        /* } */
        mult_sdio_resume();
        mult_flag.sd_status = MULT_IO_SUSPEND;
    }
#endif
    return ret;
}

int mult_usb_mount_before(usb_dev usb_id)
{
    u32 reset_delay = 0;;
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    music_player_stop(1);//停止解码
    if (mult_flag.active == SD_ACTIVE) {
        mult_sdio_suspend();
        reset_delay = USB_RESET_DELAY;
    }
    mult_usb_resume(usb_id, 0, reset_delay);
    mult_flag.sd_status = MULT_IO_SUSPEND;
    mult_flag.usb_status = MULT_IO_ACTIVE;
    mult_flag.active = USB_ACTIVE;
#endif
    return 0;
}




int mult_usb_online_mount_after(usb_dev usb_id, int err)
{
    int ret = 0;
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    if (err) {
        mult_flag.usb_status = MULT_IO_SUSPEND;
        mult_usb_suspend(usb_id);
        mult_sdio_resume();
        mult_flag.sd_status = MULT_IO_ACTIVE;
        mult_flag.active = SD_ACTIVE;
    }
#endif
    return ret;
}

int mult_usb_mount_offline(usb_dev usb_id)
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    mult_usb_suspend(usb_id);
    mult_sdio_resume();
    mult_flag.usb_status = MULT_IO_SUSPEND;
    if (mult_flag.active == USB_ACTIVE) {
        mult_flag.sd_status = MULT_IO_SUSPEND;
        mult_flag.active = 0;
    } else {
        mult_flag.sd_status = MULT_IO_ACTIVE;
        mult_flag.active = SD_ACTIVE;

    }
#endif
    return 0;

}





///*----------------------------------------------------------------------------*/
/**@brief   pc模式 usb和sd卡 io 复用初始化
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void pc_dm_multiplex_init()
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    mult_sdio_suspend();//supend sd卡总线
    set_sd_notify_enable(TCFG_DM_MULTIPLEX_WITH_SD_PORT, 0); //sd卡不允许发出上下线消息
    gpio_set_die(TCFG_USB_SD_MULTIPLEX_IO, 1);
    gpio_direction_output(TCFG_USB_SD_MULTIPLEX_IO, 0);
    os_time_dly(2);
    gpio_set_direction(TCFG_USB_SD_MULTIPLEX_IO, 1);
    gpio_set_pull_up(TCFG_USB_SD_MULTIPLEX_IO, 0);
    gpio_set_pull_down(TCFG_USB_SD_MULTIPLEX_IO, 0);
    gpio_set_die(TCFG_USB_SD_MULTIPLEX_IO, 0);
#endif
}


void pc_dm_multiplex_exit()
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    mult_sdio_resume();//resume sd卡总线
    usb_otg_resume(0);
    set_sd_notify_enable(TCFG_DM_MULTIPLEX_WITH_SD_PORT, 1); //sd卡允许发出上下线消息
#endif
}


