#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "tone_player.h"
#include "asm/charge.h"
#include "app_charge.h"
#include "app_main.h"
#include "app_online_cfg.h"
#include "app_power_manage.h"
#include "gSensor/gSensor_manage.h"
#include "ui_manage.h"
#include "vm.h"
#include "app_chargestore.h"
#include "key_event_deal.h"
#include "asm/pwm_led.h"
#include "user_cfg.h"
#include "sleep/sleep.h"
#include "ui/ui_api.h"
#include "clock_cfg.h"
#include "dev_manager.h"
#include "user_api/app_status_api.h"

#if TCFG_APP_SLEEP_EN


static u16 power_off_timer = 0;
static u16 led_blink_timer1 = 0;
static u16 led_blink_timer2 = 0;
static u16 power_on_timer = 0;

extern void app_status_handler(enum APP_STATUS status);

//*----------------------------------------------------------------------------*/
/**@brief    sleep 按键消息入口
  @param    无
  @return   1、消息已经处理，不需要发送到common  0、消息发送到common处理
  @note
 */
/*----------------------------------------------------------------------------*/
static int sleep_key_event_opr(struct sys_event *event)
{
    int ret = false;
    int err = 0;

    int key_event = event->u.key.event;
    int key_value = event->u.key.value;//

    log_i("key_event:%d \n", key_event);
    switch (key_event) {
    default:
        break;
    }

    return ret;
}


//*----------------------------------------------------------------------------*/
/**@brief    sleep 软关机处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void sleep_power_off()
{
    if (get_charge_online_flag() == 0) {
        app_var.goto_poweroff_flag = 1;
        app_task_switch_to(APP_POWEROFF_TASK);
    } else {
        app_task_switch_to(APP_IDLE_TASK);
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    sleep 关机闪灯处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void poweroff_led_blink1()
{
    app_status_handler(APP_STATUS_POWER_OFF);
}

static void poweroff_led_blink2()
{
    app_status_handler(APP_STATUS_POWER_CLOSE);
}

//*----------------------------------------------------------------------------*/
/**@brief    sleep 开机闪灯处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void poweron_led_blink()
{
    app_status_handler(APP_STATUS_POWER_ON);

}

//*----------------------------------------------------------------------------*/
/**@brief    sleep 模式初始化
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void sleep_task_start(void)
{
    sys_key_event_enable();

    clock_idle(REC_IDLE_CLOCK);
    power_off_timer = sys_timeout_add(NULL, sleep_power_off, 30000);
    led_blink_timer1 = sys_timeout_add(NULL, poweroff_led_blink1, 28000);
    led_blink_timer2 = sys_timeout_add(NULL, poweroff_led_blink2, 29500);
    power_on_timer = sys_timeout_add(NULL, poweron_led_blink, 500);
}


//*----------------------------------------------------------------------------*/
/**@brief    sleep 退出
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

static void sleep_task_close()
{
    sys_timeout_del(power_off_timer);
    sys_timeout_del(led_blink_timer1);
    sys_timeout_del(led_blink_timer2);
    return;
}

//*----------------------------------------------------------------------------*/
/**@brief    sleep 模式活跃状态 所有消息入口
   @param    无
   @return   1、当前消息已经处理，不需要发送comomon
   			 0、当前消息不是linein处理的，发送到common统一处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int sleep_sys_event_handler(struct sys_event *event)
{
    const char *logo = NULL;
    int err = 0;
    switch (event->type) {
    case SYS_KEY_EVENT:
        return 1;
    case SYS_DEVICE_EVENT:
        ///所有设备相关的事件不能返回true， 必须给留给公共处理的地方响应设备上下线
        switch ((u32)event->arg) {
        case DRIVER_EVENT_FROM_SD0:
        case DRIVER_EVENT_FROM_SD1:
        case DRIVER_EVENT_FROM_SD2:
            logo = (char *)event->u.dev.value;
        case DEVICE_EVENT_FROM_OTG:
            if ((u32)event->arg == DEVICE_EVENT_FROM_OTG) {
                logo = (char *)"udisk0";
            }

            if (event->u.dev.event == DEVICE_EVENT_IN) {
            } else if (event->u.dev.event == DEVICE_EVENT_OUT) {
            }
            break;//DEVICE_EVENT_FROM_USB_HOST
        }//switch((u32)event->arg)
        break;//SYS_DEVICE_EVENT
    default:
        break;;
    }//switch (event->type)

    return false;
}


//*----------------------------------------------------------------------------*/
/**@brief    sleep 主任务
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void app_sleep_task()
{
    int res;
    int msg[32];
    sleep_task_start();

    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (sleep_sys_event_handler((struct sys_event *)(&msg[1])) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));    //由common统一处理
            }
            break;
        default:
            break;
        }

        if (app_task_exitting()) {
            sleep_task_close();
            return;
        }
    }
}

#else


void app_sleep_task()
{

}

#endif
