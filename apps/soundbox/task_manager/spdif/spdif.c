
/*************************************************************
   此文件函数主要是spdif模式按键处理和事件处理

	void app_spdif_task()
   spdif模式主函数

	static int spdif_sys_event_handler(struct sys_event *event)
    spdif模式系统事件所有处理入口

	static void spdif_task_close(void)
	spdif模式退出

**************************************************************/


#include "app_config.h"
#include "key_event_deal.h"
#include "system/includes.h"
#include "tone_player.h"
#include "app_task.h"
#include "tone_player.h"
#include "media/includes.h"
#include "asm/audio_spdif.h"
#include "clock_cfg.h"

#include "ui/ui_api.h"
#include "ui_manage.h"


#if TCFG_APP_SPDIF_EN

extern struct audio_spdif_hdl spdif_slave_hdl;
/* #define LOG_TAG_CONST    APP_SPDIF */
#define LOG_TAG             "[APP_SPDIF]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"



static u8 source = 0;

///*----------------------------------------------------------------------------*/
/**@brief    spdif 硬件设置
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void spdif_open(void)
{
#if	TCFG_SPDIF_OUTPUT_ENABLE
    spdif_slave_hdl.output_port = SPDIF_OUT_PORT_A;//PB11
#endif
    spdif_slave_hdl.input_port  = SPDIF_IN_PORT_A;//PA9
    source  = 0;
    audio_spdif_slave_open(&spdif_slave_hdl);

    audio_spdif_slave_start(&spdif_slave_hdl);
#if TCFG_HDMI_ARC_ENABLE
    extern void hdmi_cec_init(void);
    hdmi_cec_init();
#endif
}


///*----------------------------------------------------------------------------*/
/**@brief    spdif 模式初始化
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void spdif_app_init(void)
{
    extern void spdif_dec_init(void);
    spdif_dec_init();
    ui_update_status(STATUS_SPDIF_MODE);
    clock_idle(SPDIF_IDLE_CLOCK);
    UI_SHOW_WINDOW(ID_WINDOW_SPDIF);
    sys_key_event_enable();
    spdif_open();
}


//*----------------------------------------------------------------------------*/
/**@brief    spdif 模式退出
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void spdif_task_close(void)
{
    UI_HIDE_CURR_WINDOW();
    audio_spdif_slave_close(&spdif_slave_hdl);
    log_info("APP_SPDIF_STOP1\n");
    extern void spdif_dec_close(void);
    spdif_dec_close();
#if TCFG_HDMI_ARC_ENABLE
    extern void hdmi_cec_close(void);
    hdmi_cec_close();
#endif
}

//*----------------------------------------------------------------------------*/
/**@brief    spdif 输入io口设置
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void switch_spdif_input_port(void)
{
    source++;
    if (source > 2) {
        source = 0;
    }
    printf("\n--func=%s\n", __FUNCTION__);
    switch (source) {
    case 0:
        audio_spdif_slave_switch_port(&spdif_slave_hdl, SPDIF_IN_PORT_A);
        break;
    case 1:
        audio_spdif_slave_switch_port(&spdif_slave_hdl, SPDIF_IN_PORT_C);
        break;
    case 2:
        audio_spdif_slave_switch_port(&spdif_slave_hdl, SPDIF_IN_PORT_D);
        break;
    default:
        break;
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    spdif 按键消息入口
   @param    无
   @return   1、消息已经处理，不需要发送到common  0、消息发送到common处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int spdif_key_event_opr(struct sys_event *event)
{
    int ret = true;
    int err = 0;

    int key_event = event->u.key.event;
    int key_value = event->u.key.value;//

    log_info("key_event:%d \n", key_event);

    switch (key_event) {
    case KEY_SPDIF_SW_SOURCE:
        log_info("KEY_SPDIF_SW_SOURCE \n");
        switch_spdif_input_port();
        break;
    default :
        ret = false;
        break;
    }
    return ret;
}

//*----------------------------------------------------------------------------*/
/**@brief    spdif 模式活跃状态 所有消息入口
   @param    无
   @return   1、当前消息已经处理，不需要发送comomon 0、当前消息不是linein处理的，发送到common统一处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int spdif_sys_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return spdif_key_event_opr(event);
    case SYS_DEVICE_EVENT:
        return false;
    default:
        return false;
    }
    return false;
}


//*----------------------------------------------------------------------------*/
/**@brief    spdif 主任务
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void app_spdif_task()
{

    int res;
    int msg[32];
    spdif_app_init();
    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (spdif_sys_event_handler((struct sys_event *)(&msg[1])) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));    //由common统一处理
            }
            break;
        default:
            break;
        }

        if (app_task_exitting()) {
            spdif_task_close();
            return;
        }
    }
}

#else


void app_spdif_task()
{

}




#endif
