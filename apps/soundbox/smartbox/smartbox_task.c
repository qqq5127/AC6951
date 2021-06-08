#include "smartbox/smartbox_task.h"
#include "smartbox/smartbox.h"
#include "file_transfer.h"
#include "file_delete.h"
#include "dev_format.h"
#include "clock_cfg.h"
#include "app_task.h"

#include "le_smartbox_module.h"

#if (SMART_BOX_EN)
//这个模式主要是提供一个空模式， 处理一些需要占用时间不较长的交互处理， 处理做完之后退回到原来的模式
struct __action_event {
    u8	type;		//1:手机端触发, 0:固件触发
    u8 	OpCode_SN;
    u8	action;
};
static struct __action_event action_prepare = {0};

static void app_smartbox_task_get_ready(void)
{
    printf("%s\n", __FUNCTION__);
    if (action_prepare.type) {
        JL_CMD_response_send(JL_OPCODE_ACTION_PREPARE, JL_PRO_STATUS_SUCCESS, action_prepare.OpCode_SN, NULL, 0);
    }
}

static void app_smartbox_action_end_callback(void)
{
    if (app_get_curr_task() == APP_SMARTBOX_ACTION_TASK) {
        printf("action end callback!!\n");
        app_task_switch_back();
    }
}

static void app_smartbox_task_start(void)
{
    clock_add_set(SMARTBOX_ACTION_CLK);
    app_smartbox_task_get_ready();
    //根据不同的场景， 做不同的处理， 例如：初始化不同的UI显示
    switch (action_prepare.action)		{
    case SMARTBOX_TASK_ACTION_FILE_TRANSFER:
        file_transfer_init(app_smartbox_action_end_callback);
        break;
    case SMARTBOX_TASK_ACTION_FILE_DELETE:
        file_delete_init(app_smartbox_action_end_callback);
        break;
    case SMARTBOX_TASK_ACTION_DEV_FORMAT:
        dev_format_init(app_smartbox_action_end_callback);
        break;
    default:
        break;
    }
}

static void app_smartbox_task_stop(void)
{
    switch (action_prepare.action)		{
    case SMARTBOX_TASK_ACTION_FILE_TRANSFER:
        break;
    case SMARTBOX_TASK_ACTION_FILE_DELETE:
        break;
    case SMARTBOX_TASK_ACTION_DEV_FORMAT:
        break;
    default:
        break;
    }

    clock_remove_set(SMARTBOX_ACTION_CLK);
    printf("app_smartbox_task_stop\n");
}

static int app_smartbox_task_event_handle(struct sys_event *event)
{
    switch (action_prepare.action)		{
    case SMARTBOX_TASK_ACTION_FILE_TRANSFER:
        break;
    case SMARTBOX_TASK_ACTION_FILE_DELETE:
        break;
    case SMARTBOX_TASK_ACTION_DEV_FORMAT:
        break;
    default:
        break;
    }
    return 0;
}

void app_smartbox_task_prepare(u8 type, u8 action, u8 OpCode_SN)
{
    action_prepare.type = type;
    action_prepare.action = action;
    action_prepare.OpCode_SN = OpCode_SN;
    //切换模式
    if (app_get_curr_task() != APP_SMARTBOX_ACTION_TASK) {
        app_task_switch_to(APP_SMARTBOX_ACTION_TASK);
    } else {
        app_smartbox_task_get_ready();
    }
}


void app_smartbox_task(void)
{
    int msg[32];
    app_smartbox_task_start();
    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (app_smartbox_task_event_handle((struct sys_event *)(msg + 1)) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));
            }
            break;
        default:
            break;
        }

        if (app_task_exitting()) {
            app_smartbox_task_stop();
            return;
        }
    }
}
#else
void app_smartbox_task(void)
{

}
#endif//SMART_BOX_EN

