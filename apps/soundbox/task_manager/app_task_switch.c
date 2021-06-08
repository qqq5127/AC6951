#include "app_config.h"
#include "app_task.h"
#include "app_msg.h"
#include "key_driver.h"
#include "key_event_deal.h"
#include "audio_recorder_mix.h"

///模式配置表，这里可以配置切换模式的顺序，方案根据需求定义
static const u8 app_task_list[] = {
#if TCFG_APP_BT_EN
    APP_BT_TASK,
#endif
#if TCFG_APP_MUSIC_EN
    APP_MUSIC_TASK,
#endif
#if TCFG_APP_FM_EN
    APP_FM_TASK,
#endif
#if TCFG_APP_RECORD_EN
    APP_RECORD_TASK,
#endif
#if TCFG_APP_LINEIN_EN
    APP_LINEIN_TASK,
#endif
#if TCFG_APP_RTC_EN
    APP_RTC_TASK,
#endif
#if TCFG_APP_SLEEP_EN
    APP_SLEEP_TASK,
#endif
#if TCFG_APP_PC_EN
    APP_PC_TASK,
#endif
#if TCFG_APP_SPDIF_EN
    APP_SPDIF_TASK,
#endif
};

u8 app_prev_task = 0;
u8 app_curr_task = 0;
u8 app_next_task = 0;

//*----------------------------------------------------------------------------*/
/**@brief    模式按键映射处理
   @param    e:按键事件
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

int app_key_event_remap(struct sys_event *e)
{
    struct key_event *key = &e->u.key;
    int msg = KEY_NULL;
    switch (key->type) {
    case KEY_DRIVER_TYPE_IO:
#if TCFG_IOKEY_ENABLE
        msg = iokey_event_to_msg(app_curr_task, key);
#endif
        break;
    case KEY_DRIVER_TYPE_AD:
    case KEY_DRIVER_TYPE_RTCVDD_AD:
#if TCFG_ADKEY_ENABLE
        msg = adkey_event_to_msg(app_curr_task, key);
#endif
        break;
    case KEY_DRIVER_TYPE_IR:
#if TCFG_IRKEY_ENABLE
        msg = irkey_event_to_msg(app_curr_task, key);
#endif
        break;
    case KEY_DRIVER_TYPE_TOUCH:
#if TCFG_TOUCH_KEY_ENABLE
        msg = touch_key_event_to_msg(app_curr_task, key);
#endif
        break;
    case KEY_DRIVER_TYPE_RDEC:
#if TCFG_RDEC_KEY_ENABLE
        msg = rdec_key_event_to_msg(app_curr_task, key);
#endif
        break;

    case KEY_DRIVER_TYPE_SOFTKEY:
        msg = key->event;
        break;
    default:
        break;
    }

    if (msg == KEY_NULL) {
        e->consumed = 1;//接管按键消息,app应用不会收到消息
        return FALSE;
    }

    e->u.key.event = msg;
    e->u.key.value = 0;//
    return TRUE;//notify数据
}
SYS_EVENT_HANDLER(SYS_KEY_EVENT,  app_key_event_remap, 3);
//*----------------------------------------------------------------------------*/
/**@brief    模式退出检查
   @param    curr_task:当前模式
   @return   TRUE可以退出， FALSE不可以退出
   @note
*/
/*----------------------------------------------------------------------------*/
static int app_task_switch_exit_check(u8 curr_task)
{
    int ret = false;
    switch (curr_task) {
    case APP_BT_TASK:
        ret = bt_app_exit_check();
        break;
    default:
        ret = TRUE;
        break;

    }
    return ret;
}
//*----------------------------------------------------------------------------*/
/**@brief    模式进入检查
   @param    app_task:目标模式
   @return   TRUE可以进入， FALSE不可以进入
   @note	 例如一些需要设备在线的任务(music/record/linein等)，
   			 如果设备在线可以进入， 没有设备在线不进入可以在这里处理
*/
/*----------------------------------------------------------------------------*/
static int app_task_switch_check(u8 app_task)
{
    int ret = false;
    switch (app_task) {
#if TCFG_APP_MUSIC_EN
    case APP_MUSIC_TASK:
        ret = music_app_check();
        break;
#endif
#if TCFG_APP_LINEIN_EN
    case APP_LINEIN_TASK:
        ret = linein_app_check();
        break;
#endif
#if TCFG_APP_PC_EN
    case APP_PC_TASK:
        ret = pc_app_check();
        break;
#endif
    case APP_FM_TASK:
        ret = TRUE;
        break;
#if TCFG_APP_RECORD_EN
    case APP_RECORD_TASK:
        ret = record_app_check();
        break;
#endif
    default:
        ret = TRUE;
        break;

    }
    return ret;
}
//*----------------------------------------------------------------------------*/
/**@brief    切换到上一个模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void app_task_switch_prev()
{
    int i = 0;
    int cur_index = 0;

    if (app_next_task) {
        printf("app_task_switch_prev busy \n");
    }

    for (cur_index = 0; cur_index < ARRAY_SIZE(app_task_list); cur_index++) {
        if (app_curr_task == app_task_list[cur_index]) {//遍历当前索引
            break;
        }
    }

    for (i = cur_index; ;) { //遍历一圈
        if (i-- == 0) {
            i = ARRAY_SIZE(app_task_list) - 1;
        }
        if (i == cur_index) {
            return;
        }
        if (app_task_switch_to(app_task_list[i])) {
            return;
        }
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    切换到下一个模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void app_task_switch_next()
{
    int i = 0;
    int cur_index = 0;

    if (app_next_task) {
        printf("app_task_switch_next busy \n");
    }

    for (cur_index = 0; cur_index < ARRAY_SIZE(app_task_list); cur_index++) {
        if (app_curr_task == app_task_list[cur_index]) {//遍历当前索引
            break;
        }
    }

    for (i = cur_index ;;) { //遍历一圈
        if (++i >= ARRAY_SIZE(app_task_list)) {
            i = 0;
        }
        if (i == cur_index) {
            return;
        }
        if (app_task_switch_to(app_task_list[i])) {
            return;
        }
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    切换到指定模式
   @param    app_task:指定模式
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int app_task_switch_to(u8 app_task)
{
    if (app_curr_task == app_task) {
        return false;
    }

    if (!app_task_switch_check(app_task)) {
        return false;
    }

    if (!app_task_switch_exit_check(app_curr_task)) {
        return false;
    }

    printf("cur --- %x \n", app_curr_task);
    printf("new +++ %x \n", app_task);

    /* if(app_next_task) */
    /* printf("app_task_switch_to busy \n"); */
#if (defined SMART_BOX_EN) && (SMART_BOX_EN)
    extern void function_change_inform(u8 app_mode, u8 ret);
    function_change_inform(app_task, TRUE);
#endif

#if (RECORDER_MIX_EN)
    recorder_mix_stop();
#endif/*RECORDER_MIX_EN*/

    app_prev_task = app_curr_task;
    app_next_task = app_task;
    app_task_put_usr_msg(APP_MSG_SWITCH_TASK, 0);

    return TRUE;
}

//*----------------------------------------------------------------------------*/
/**@brief    跳回到原来的模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int app_task_switch_back()
{
    if (app_prev_task == 0) {
        return -EINVAL;
    }
    return app_task_switch_to(app_prev_task);
}

//*----------------------------------------------------------------------------*/
/**@brief    模式切换退出检测
   @param
   @return   1:响应退出模式， 0:不响应
   @note
*/
/*----------------------------------------------------------------------------*/
u8 app_task_exitting()//
{
    struct sys_event clear_key_event = {.type =  SYS_KEY_EVENT, .arg = (void *)DEVICE_EVENT_FROM_KEY};
    if (app_next_task != 0) {
        app_curr_task = app_next_task;
        app_next_task = 0;
        sys_key_event_disable();
        sys_event_clear(&clear_key_event);
        return 1;
    }
    return 0;
}

//*----------------------------------------------------------------------------*/
/**@brief    获取当前模式
   @param
   @return   当前模式id
   @note
*/
/*----------------------------------------------------------------------------*/
u8 app_get_curr_task()
{
    return app_curr_task;
}

//*----------------------------------------------------------------------------*/
/**@brief    通过指定id检查是否是当前模式
   @param
   @return   true:是当前模式，false:不是当前模式
   @note
*/
/*----------------------------------------------------------------------------*/
u8 app_check_curr_task(u8 app)
{
    if (app == app_curr_task) {
        return true;
    }
    return false;
}


