#include "gSensor/gSensor_manage.h"
#include "device/device.h"
#include "app_config.h"
#include "debug.h"
#include "key_event_deal.h"
#include "btstack/avctp_user.h"
#include "app_main.h"
#include "tone_player.h"
#include "user_cfg.h"
/* #include "audio_config.h" */
/* #include "app_power_manage.h" */
/* #include "system/timer.h" */
/* #include "event.h" */

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if TCFG_GSENSOR_ENABLE

#define LOG_TAG             "[GSENSOR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static const struct gsensor_platform_data *platform_data;
G_SENSOR_INTERFACE *gSensor_hdl = NULL;
G_SENSOR_INFO  __gSensor_info = {.iic_delay = 30};
#define gSensor_info (&__gSensor_info)

#if TCFG_GSENOR_USER_IIC_TYPE
#define iic_init(iic)                       hw_iic_init(iic)
#define iic_uninit(iic)                     hw_iic_uninit(iic)
#define iic_start(iic)                      hw_iic_start(iic)
#define iic_stop(iic)                       hw_iic_stop(iic)
#define iic_tx_byte(iic, byte)              hw_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               hw_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         hw_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        hw_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    hw_iic_suspend(iic)
#define iic_resume(iic)                     hw_iic_resume(iic)
#else
#define iic_init(iic)                       soft_iic_init(iic)
#define iic_uninit(iic)                     soft_iic_uninit(iic)
#define iic_start(iic)                      soft_iic_start(iic)
#define iic_stop(iic)                       soft_iic_stop(iic)
#define iic_tx_byte(iic, byte)              soft_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               soft_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         soft_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        soft_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    soft_iic_suspend(iic)
#define iic_resume(iic)                     soft_iic_resume(iic)
#endif

void gSensor_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_KEY_EVENT;
    e.u.key.event = event;
    e.u.key.value = 0;
    sys_event_notify(&e);
}

void gSensor_int_io_detect(void *priv)
{
    u8 int_io_status = 0;
    u8 det_result = 0;
    int_io_status = gpio_read(platform_data->gSensor_int_io);
    //log_info("status %d\n",int_io_status);
    gSensor_hdl->gravity_sensor_ctl(GSENSOR_INT_DET, &int_io_status);
    if (gSensor_hdl->gravity_sensor_check == NULL) {
        return;
    }
    det_result = gSensor_hdl->gravity_sensor_check();
    if (det_result == 1) {
        log_info("GSENSOR_EVENT_CLICK\n");
        gSensor_event_to_user(KEY_EVENT_CLICK);
    } else if (det_result == 2) {
        log_info("GSENSOR_EVENT_DOUBLE_CLICK\n");
        gSensor_event_to_user(KEY_EVENT_DOUBLE_CLICK);
    } else if (det_result == 3) {
        log_info("GSENSOR_EVENT_THREE_CLICK\n");
        gSensor_event_to_user(KEY_EVENT_TRIPLE_CLICK);
    }
}

u8 gravity_sensor_command(u8 w_chip_id, u8 register_address, u8 function_command)
{
    u8 ret = 1;
    iic_start(gSensor_info->iic_hdl);
    if (0 == iic_tx_byte(gSensor_info->iic_hdl, w_chip_id)) {
        ret = 0;
        log_e("\n gsen iic wr err 0\n");
        goto __gcend;
    }

    delay(gSensor_info->iic_delay);

    if (0 == iic_tx_byte(gSensor_info->iic_hdl, register_address)) {
        ret = 0;
        log_e("\n gsen iic wr err 1\n");
        goto __gcend;
    }

    delay(gSensor_info->iic_delay);

    if (0 == iic_tx_byte(gSensor_info->iic_hdl, function_command)) {
        ret = 0;
        log_e("\n gsen iic wr err 2\n");
        goto __gcend;
    }

__gcend:
    iic_stop(gSensor_info->iic_hdl);

    return ret;
}

u8 _gravity_sensor_get_ndata(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len)
{
    u8 read_len = 0;

    iic_start(gSensor_info->iic_hdl);
    if (0 == iic_tx_byte(gSensor_info->iic_hdl, r_chip_id - 1)) {
        log_e("\n gsen iic rd err 0\n");
        read_len = 0;
        goto __gdend;
    }


    delay(gSensor_info->iic_delay);
    if (0 == iic_tx_byte(gSensor_info->iic_hdl, register_address)) {
        log_e("\n gsen iic rd err 1\n");
        read_len = 0;
        goto __gdend;
    }

    iic_start(gSensor_info->iic_hdl);
    if (0 == iic_tx_byte(gSensor_info->iic_hdl, r_chip_id)) {
        log_e("\n gsen iic rd err 2\n");
        read_len = 0;
        goto __gdend;
    }

    delay(gSensor_info->iic_delay);

    for (; data_len > 1; data_len--) {
        *buf++ = iic_rx_byte(gSensor_info->iic_hdl, 1);
        read_len ++;
    }

    *buf = iic_rx_byte(gSensor_info->iic_hdl, 0);
    read_len ++;

__gdend:

    iic_stop(gSensor_info->iic_hdl);
    delay(gSensor_info->iic_delay);

    return read_len;
}

//u8 gravity_sen_dev_cur;		/*当前挂载的Gravity sensor*/
int gravity_sensor_init(void *_data)
{
    int retval = 0;
    platform_data = (const struct gsensor_platform_data *)_data;
    gSensor_info->iic_hdl = platform_data->iic;
    retval = iic_init(gSensor_info->iic_hdl);

    log_e("\n  gravity_sensor_init\n");

    if (retval < 0) {
        log_e("\n  open iic for gsensor err\n");
        return retval;
    } else {
        log_info("\n iic open succ\n");
    }

    retval = -EINVAL;
    list_for_each_gsensor(gSensor_hdl) {
        if (!memcmp(gSensor_hdl->logo, platform_data->gSensor_name, strlen(platform_data->gSensor_name))) {
            retval = 0;
            break;
        }
    }

    if (retval < 0) {
        log_e(">>>gSensor_hdl logo err\n");
        return retval;
    }

    gpio_set_pull_up(platform_data->gSensor_int_io, 0);
    gpio_set_pull_down(platform_data->gSensor_int_io, 0);
    gpio_set_direction(platform_data->gSensor_int_io, 1);
    gpio_set_die(platform_data->gSensor_int_io, 1);


    if (gSensor_hdl->gravity_sensor_init()) {
        log_e(">>>>gSensor_Int ERROR\n");
    } else {
        log_info(">>>>gSensor_Int SUCC\n");
        gSensor_info->init_flag  = 1;
        sys_s_hi_timer_add(NULL, gSensor_int_io_detect, 2); //2ms
    }
    return 0;
}


#endif // CONFIG_GSENSOR_ENABLE


