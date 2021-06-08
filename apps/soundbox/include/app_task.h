#ifndef APP_TASK_H
#define APP_TASK_H

#include "system/task.h"
#include "app_msg.h"
#include "system/event.h"


enum {
    APP_POWERON_TASK  = 1,
    APP_POWEROFF_TASK = 2,
    APP_BT_TASK       = 3,
    APP_MUSIC_TASK    = 4,
    APP_FM_TASK       = 5,
    APP_RECORD_TASK   = 6,
    APP_LINEIN_TASK   = 7,
    APP_RTC_TASK      = 8,
    APP_SLEEP_TASK    = 9,
    APP_IDLE_TASK	  = 10,
    APP_PC_TASK       = 11,
    APP_SPDIF_TASK    = 12,
    APP_WATCH_UPDATE_TASK     = 13,
    APP_SMARTBOX_ACTION_TASK  = 14,
    APP_TASK_MAX_INDEX,
};


enum {
    APP_MSG_SYS_EVENT = Q_EVENT,

    /* 用户自定义消息 */
    APP_MSG_SWITCH_TASK = Q_USER + 1,
    APP_MSG_USER        = Q_USER + 2,

};


//切换到前一个有效模式
void app_task_switch_prev();
//切换到下一个有效模式
void app_task_switch_next();
//返回到之前的模式
int app_task_switch_back();
//切换到指定模式
int app_task_switch_to(u8 app_task);
//获取当前模式id
u8 app_get_curr_task();
//通过id检查是否是当前模式
u8 app_check_curr_task(u8 app);
//模式切换退出检测
u8 app_task_exitting();

extern u8 app_curr_task;
extern u8 app_next_task;
extern u8 app_prev_task;


//*********************************************************************************//
//                               模式进入检测配置                                 //
//*********************************************************************************//
extern int music_app_check(void);
extern int linein_app_check(void);
extern int pc_app_check(void);
extern int record_app_check(void);

//*********************************************************************************//
//                               模式退出检测配置                                 //
//*********************************************************************************//
extern u8 bt_app_exit_check();

//*********************************************************************************//
//                               模式入口                                 //
//*********************************************************************************//
extern void app_bt_task();
extern void app_music_task();
extern void app_linein_task();
extern void app_pc_task();
extern void app_record_task();
extern void app_spdif_task();
extern void app_rtc_task();
extern void app_fm_task();
extern void app_idle_task();
extern void app_poweroff_task();
extern void app_poweron_task();
extern void app_sleep_task();
extern void app_smartbox_task(void);



//*********************************************************************************//
//                               按键转换表                                 //
//*********************************************************************************//
extern u16 adkey_event_to_msg(u8 cur_task, struct key_event *key);
extern u16 iokey_event_to_msg(u8 cur_task, struct key_event *key);
extern u16 irkey_event_to_msg(u8 cur_task, struct key_event *key);
extern u16 rdec_key_event_to_msg(u8 cur_task, struct key_event *key);
extern u16 touch_key_event_to_msg(u8 cur_task, struct key_event *key);

//*********************************************************************************//
//                               公共消息处理                                //
//*********************************************************************************//
void app_default_event_deal(struct sys_event *event);
int app_common_key_msg_deal(struct sys_event *event);

#endif
