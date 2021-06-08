#ifndef APP_ACTION_H
#define APP_ACTION_H

#include "app_config.h"
#include "system/app_core.h"
#include "system/task.h"
#include "common/user_msg.h"

#if (TCFG_TONE2TWS_ENABLE)
#include "bt_tws.h"
#endif

#define ACTION_APP_MAIN   			0x0001


#define APP_NAME_POWERON								"poweron"
#define APP_NAME_POWEROFF								"poweroff"
#define APP_NAME_BT										"bt"
#define APP_NAME_MUSIC									"music"
#define APP_NAME_FM										"fm"
#define APP_NAME_RECORD									"record"
#define APP_NAME_LINEIN									"linein"
#define APP_NAME_RTC									"rtc"
#define APP_NAME_PC										"pc"
#define APP_NAME_SPDIF						 			"spdif"
#define APP_NAME_IDLE									"idle"
#define APP_NAME_BOX									"box"

extern struct application app_begin[];
extern struct application app_end[];
#define list_for_each_app(p) \
	for (p=app_begin; p<app_end; p++)

struct application_reg {
    const char *tone_name;
    int (*tone_play_check)(void);
    void (*tone_prepare)(void);//模式提示音预处理， 在模式提示音前， 可以是显示定义
    int (*enter_check)(void);
    int (*exit_check)(void);
    int (*user_msg)(int msg, int argc, int *argv);
#if (TCFG_TONE2TWS_ENABLE)
    u8  tone_tws_cmd;
#endif
};


//int app_task_switch(const char *name, int action, void *param);
// int app_task_switch_last(void);
//int app_task_switch_target(void);//一般不使用, 目前只有蓝牙模式使用
//int app_task_target_check(const char *name);//一般不使用, 目前只有蓝牙模式使用
int app_task_next(void);

//int app_cur_task_check(char *name); // true or false


bool app_task_msg_post(int msg, int argc, ...);
void app_task_softkey_event_post(u8 event);
extern void app_mode_tone_play_by_tws(int cmd);

#define task_switch_to_bt()     app_task_switch_to(APP_BT_TASK)//app_task_switch(APP_NAME_BT, ACTION_APP_MAIN, NULL)


#endif

