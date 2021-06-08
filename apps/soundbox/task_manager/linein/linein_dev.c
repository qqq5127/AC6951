
/*************************************************************
   此文件函数主要是linein 插入检测

   支持和sd卡 io复用检测



**************************************************************/
#include "app_config.h"
#include "system/event.h"
#include "system/init.h"
#include "system/timer.h"
#include "asm/power_interface.h"
#include "asm/adc_api.h"
#include "linein/linein.h"
#include "linein/linein_dev.h"
#include "gpio.h"
#include "asm/sdmmc.h"


#if TCFG_APP_LINEIN_EN

#define LINEIN_STU_HOLD		0
#define LINEIN_STU_ON		1
#define LINEIN_STU_OFF		2

#define LINEIN_DETECT_CNT   3//滤波计算


#define LOG_TAG_CONST       APP_LINEIN
#define LOG_TAG             "[APP_LINEIN_DEV]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

struct linein_dev_opr {
    struct linein_dev_data *dev;
    u8 cnt;//滤波计算
    u8 stu;//当前状态
    u16 timer;//定时器句柄
    u8 online: 1; //是否在线
    u8 active: 1; //进入sniff的判断标志
    u8 init: 1; //初始化完成标志
    u8 step: 3; //检测阶段
};
static struct linein_dev_opr linein_dev_hdl = {0};
#define __this 	(&linein_dev_hdl)

/*----------------------------------------------------------------------------*/
/*@brief   获取linein是否在线
  @param
  @return  1:在线 0：不在线
  @note    app通过这个接口判断linein是否在线
 */
/*----------------------------------------------------------------------------*/
u8 linein_is_online(void)
{
#if ((defined TCFG_LINEIN_DETECT_ENABLE) && (TCFG_LINEIN_DETECT_ENABLE == 0))
    return 1;
#else
    return __this->online;
#endif//TCFG_LINEIN_DETECT_ENABLE

}

/*----------------------------------------------------------------------------*/
/*@brief   设置inein是否在线
  @param    1:在线 0：不在线
  @return  null
  @note    检测驱动通过这个接口判断linein是否在线
 */
/*----------------------------------------------------------------------------*/
void linein_set_online(u8 online)
{
    __this->online = !!online;
}

/*----------------------------------------------------------------------------*/
/*@brief   发布设备上下线消息
  @param    上下线消息
 */
/*----------------------------------------------------------------------------*/
void linein_event_notify(u8 arg)
{
    struct sys_event event = {0};
    event.arg  = (void *)DEVICE_EVENT_FROM_LINEIN;
    event.type = SYS_DEVICE_EVENT;

    if (arg == DEVICE_EVENT_IN) {
        event.u.dev.event = DEVICE_EVENT_IN;
    } else if (arg == DEVICE_EVENT_OUT) {
        event.u.dev.event = DEVICE_EVENT_OUT;
    } else {
        return ;
    }
    sys_event_notify(&event);
}


/*----------------------------------------------------------------------------*/
/*@brief   检测前使能io
  @param    null
  @return  null
  @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
  (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
  */
/*----------------------------------------------------------------------------*/
static void linein_io_start(void)
{
    /* printf("<<<linein_io_start\n"); */
    struct linein_dev_data *linein_dev = (struct linein_dev_data *)__this->dev;
    if (__this->init) {
        return ;
    }
    __this->init = 1;
    if (linein_dev->down) {
        gpio_set_pull_down(linein_dev->port, 1);
    } else {
        gpio_set_pull_down(linein_dev->port, 0);
    }
    if (linein_dev->up) {
        gpio_set_pull_up(linein_dev->port, 1);
    } else {
        gpio_set_pull_up(linein_dev->port, 0);
    }
    if (linein_dev->ad_channel == (u8)NO_CONFIG_PORT) {
        gpio_set_die(linein_dev->port, 1);
    } else {
        gpio_set_die(linein_dev->port, 0);
    }
    gpio_set_hd(linein_dev->port, 0);
    gpio_set_hd0(linein_dev->port, 0);
    gpio_direction_input(linein_dev->port);
}

/*----------------------------------------------------------------------------*/
/*@brief   检测完成关闭使能io
  @param    null
  @return  null
  @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
  (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
 */
/*----------------------------------------------------------------------------*/
static void linein_io_stop(void)
{
    /* printf("<<<linein_io_stop\n"); */
    struct linein_dev_data *linein_dev = (struct linein_dev_data *)__this->dev;
    if (!__this->init) {
        return ;
    }
    __this->init = 0;
    gpio_direction_input(linein_dev->port);
    gpio_set_pull_up(linein_dev->port, 0);
    gpio_set_pull_down(linein_dev->port, 0);
    gpio_set_hd(linein_dev->port, 0);
    gpio_set_hd0(linein_dev->port, 0);
    gpio_set_die(linein_dev->port, 0);
}

/*----------------------------------------------------------------------------*/
/*@brief   检测是否在线
  @param    驱动句柄
  @return    1:有设备插入 0：没有设备插入
  @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
  (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
  */
/*----------------------------------------------------------------------------*/
static int linein_sample_detect(void *arg)
{
    struct linein_dev_data *linein_dev = (struct linein_dev_data *)arg;
    u8 cur_stu;

    if (linein_dev->ad_channel == (u8)NO_CONFIG_PORT) {
        linein_io_start();
        cur_stu = gpio_read(linein_dev->port) ? false : true;
        linein_io_stop();
        if (!linein_dev->up) {
            cur_stu	= !cur_stu;
        }
    } else {
        cur_stu = adc_get_value(linein_dev->ad_channel) > linein_dev->ad_vol ? false : true;
        /* printf("<%d> ", adc_get_value(linein_dev->ad_channel)); */
    }
    /* putchar('A' + cur_stu); */
    return cur_stu;
}

/*----------------------------------------------------------------------------*/
/*@brief   sd cmd 复用检测是否在线
  @param    驱动句柄
  @return    1:有设备插入 0：没有设备插入
  @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
  (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
  */
/*----------------------------------------------------------------------------*/
static int linein_sample_mult_sd(void *arg)
{
    struct linein_dev_data *linein_dev = (struct linein_dev_data *)arg;
    linein_io_start();
    u16 ad_value = 0;
    u8 cur_stu;
    if (linein_dev->ad_channel == (u8)NO_CONFIG_PORT) {
        cur_stu = gpio_read(linein_dev->port) ? false : true;
    } else {
        adc_enter_occupy_mode(linein_dev->ad_channel);
        if (adc_occupy_run()) {
            ad_value = adc_get_occupy_value();
            cur_stu = ad_value > linein_dev->ad_vol ? false : true;
            /* printf("\n<%d>\n", ad_value); */
        } else {
            cur_stu = __this->stu;
        }
        adc_exit_occupy_mode();
    }

    /* putchar('A'+cur_stu); */
    /* putchar(cur_stu); */
    linein_io_stop();
    return cur_stu;
}


/*----------------------------------------------------------------------------*/
/*@brief   注册的定时器回调检测函数
  @param    驱动句柄
  @return    null
  @note    定时进行检测
  */
/*----------------------------------------------------------------------------*/
static void linein_detect(void *arg)
{
    int cur_stu;
    struct sys_event event = {0};
#if ((TCFG_LINEIN_MULTIPLEX_WITH_SD == ENABLE))
    if (sd_io_suspend(TCFG_LINEIN_SD_PORT, 0) == 0) {//判断sd 看是否空闲
        cur_stu = linein_sample_mult_sd(arg);
        sd_io_resume(TCFG_LINEIN_SD_PORT, 0);//使用完，回复sd
    } else {
        return;
    }
#else
    if (__this->step == 0) {
        __this->step = 1;
        sys_timer_modify(__this->timer, 10);//猜测是检测状态变化的时候改变定时器回调时间
        return ;
    }
    cur_stu = linein_sample_detect(arg);
    if (!__this->active) {
        __this->step = 0;
        sys_timer_modify(__this->timer, 500);//猜测是检测状态不变化的时候改变定时器回调时间
    }
#endif

    if (cur_stu != __this->stu) {
        __this->stu = cur_stu;
        __this->cnt = 0;
        __this->active = 1;
    } else {
        __this->cnt ++;
    }

    if (__this->cnt < LINEIN_DETECT_CNT) {//滤波计算
        return;
    }

    __this->active = 0;//检测一次完成


    /* putchar(cur_stu); */

    if ((linein_is_online()) && (!__this->stu)) {
        linein_set_online(false);
        linein_event_notify(DEVICE_EVENT_OUT);//发布下线消息
    } else if ((!linein_is_online()) && (__this->stu)) {
        linein_set_online(true);
        linein_event_notify(DEVICE_EVENT_IN);//发布上线消息
    }
    return;
}

void linein_detect_timer_add()
{
    if (!__this || !(__this->dev)) {
        return;
    }
    if (__this->timer == 0) {
        __this->timer = sys_timer_add(__this->dev, linein_detect, 25);
    }
}
void linein_detect_timer_del()
{
    if (__this && __this->timer) {
        sys_timer_del(__this->timer);
        __this->timer = 0;
    }
}

static int linein_driver_init(const struct dev_node *node,  void *arg)
{
    struct linein_dev_data *linein_dev = (struct linein_dev_data *)arg;
    if (!linein_dev->enable) {
        linein_set_online(true);
        return 0;
    }
#if ((defined TCFG_LINEIN_DETECT_ENABLE) && (TCFG_LINEIN_DETECT_ENABLE == 0))
    linein_set_online(true);//没有配置detcct 默认在线
    return 0;
#endif

    if (linein_dev->port == (u8)NO_CONFIG_PORT) {
        linein_set_online(true);//配置的io 不在范围 ，默认在线
        return 0;
    }
    linein_set_online(false);

#if (!(TCFG_LINEIN_MULTIPLEX_WITH_SD))//复用情况和io检测仅在使用的时候配置io
    if (linein_dev->ad_channel != (u8)NO_CONFIG_PORT) {
        linein_io_start();//初始化io
        adc_add_sample_ch(linein_dev->ad_channel);
    }
#endif

    __this->dev = linein_dev;
    __this->timer = sys_timer_add(arg, linein_detect, 50);
    return 0;
}

const struct device_operations linein_dev_ops = {
    .init = linein_driver_init,
};


/*----------------------------------------------------------------------------*/
/*@brief   注册的定功耗检测函数
  @param
  @return    null
  @note   用于防止检测一次未完成进入了低功耗
  */
/*----------------------------------------------------------------------------*/
static u8 linein_dev_idle_query(void)
{
    return !__this->active;
}

REGISTER_LP_TARGET(linein_dev_lp_target) = {
    .name = "linein_dev",
    .is_idle = linein_dev_idle_query,
};

#endif

