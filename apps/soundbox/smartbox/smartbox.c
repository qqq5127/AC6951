#include "smartbox/smartbox.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "string.h"
#include "system/timer.h"
#include "app_core.h"
#include "spp_user.h"
#include "app_task.h"
#include "system/task.h"
#include "smartbox/config.h"
#include "smartbox/event.h"
#include "btstack_3th_protocol_user.h"
#include "smartbox_rcsp_manage.h"
#include "smartbox_setting_opt.h"

#if (SMART_BOX_EN)
#define SMARTBOX_TASK_NAME   "smartbox"

#ifndef JL_SMART_BOX_CUSTOM_APP_EN
#define SMARTBOX_SPP_INTERACTIVE_SUPPORT	1
#define SMARTBOX_BLE_INTERACTIVE_SUPPORT	1
#endif

struct smartbox *__this = NULL;

extern void cmd_recieve(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len);
extern void cmd_recieve_no_respone(void *priv, u8 OpCode, u8 *data, u16 len);
extern void cmd_respone(void *priv, u8 OpCode, u8 status, u8 *data, u16 len);
extern void data_recieve(void *priv, u8 OpCode_SN, u8 CMD_OpCode, u8 *data, u16 len);
extern void data_recieve_no_respone(void *priv, u8 CMD_OpCode, u8 *data, u16 len);
extern void data_respone(void *priv, u8 status, u8 CMD_OpCode, u8 *data, u16 len);


extern void JL_recieve_packet_parse_process(void);
void JL_rcsp_recieve_resume(void)
{
    int argv[3];
    argv[0] = (int)JL_recieve_packet_parse_process;
    argv[1] = 1;
    argv[2] = 0;
    int ret = os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
}

void JL_rcsp_resume_do(void)
{
    os_sem_post(&__this->sem);
    JL_rcsp_recieve_resume();
}

static void smartbox_process(void *p)
{
    int res;
    while (1) {
        os_sem_pend(&__this->sem, 0);
        JL_send_packet_process();
        /* JL_protocol_process(); */
    }
}


struct smartbox *smartbox_handle_get(void)
{
    return __this;
}

static u16 smartbox_timer = 0;

static void smartbox_process_timer()
{
    os_sem_post(&__this->sem);
}

extern int smartbox_user_spp_state_specific(u8 packet_type);

static BT_3TH_USER_CB bt_rcsp_callback = {
    .type                             = APP_TYPE_RCSP,

    .bt_config                        = 0
#if (SMARTBOX_BLE_INTERACTIVE_SUPPORT)
    | BT_CONFIG_BLE
#endif
#if (SMARTBOX_SPP_INTERACTIVE_SUPPORT)
    | BT_CONFIG_SPP
#endif
    ,

    .bt_3th_handler.priv              = NULL,
    .bt_3th_handler.fw_ready          = NULL,
    .bt_3th_handler.fw_send           = NULL,
    .bt_3th_handler.CMD_resp          = cmd_recieve,
    .bt_3th_handler.CMD_no_resp       = cmd_recieve_no_respone,
    .bt_3th_handler.CMD_recieve_resp  = cmd_respone,
    .bt_3th_handler.DATA_resp         = data_recieve,
    .bt_3th_handler.DATA_no_resp      = data_recieve_no_respone,
    .bt_3th_handler.DATA_recieve_resp = data_respone,
    .bt_3th_handler.wait_resp_timeout = NULL,
    .BT_3TH_spp_state_specific        = smartbox_user_spp_state_specific,
    .BT_3TH_event_handler             = rcsp_user_event_handler,
};

void smartbox_init(void)
{
    if (__this) {
        return;
    }

    btstack_3th_protocol_user_init(&bt_rcsp_callback);

    //set_jl_mtu_resv();
    ///设置rcsp最大发送buf， 即MTU
    set_jl_mtu_send(264);

    //如果支持大文件传输可以通过修改接收buf大小优化传输速度
    set_jl_rcsp_recieve_buf_size(4 * 1024);

    u32 size = rcsp_protocol_need_buf_size();
    u8 *ptr = zalloc(size);
    ASSERT(ptr, "no ram for rcsp !!\n");
    JL_protocol_init(ptr, size);

    struct smartbox *smart = (struct smartbox *)zalloc(sizeof(struct smartbox));
    ASSERT(smart, "no ram for smartbox !!\n");
    smartbox_config(smart);
    __this = smart;
    __this->smartbox_buf = ptr;

    os_sem_create(&__this->sem, 0);
    smartbox_timer = sys_timer_add(NULL, smartbox_process_timer, 500);

    ///从vm获取相关配置
    smart_setting_init();

    int err = task_create(smartbox_process, (void *)smart, SMARTBOX_TASK_NAME);
    if (err) {
        printf("smartbox creat fail %x\n", err);
    }
}

void smartbox_exit(void)
{
    //extern void rcsp_resume(void);
    //rcsp_resume();
    if (smartbox_timer) {
        sys_timer_del(smartbox_timer);
        smartbox_timer = 0;
    }
    task_kill(SMARTBOX_TASK_NAME);
    if (__this->smartbox_buf) {
        free(__this->smartbox_buf);
        __this->smartbox_buf = NULL;
    }
    if (__this) {
        free(__this);
        __this = NULL;
    }
    smartbox_opt_release();
}

#endif//SMART_BOX_EN
