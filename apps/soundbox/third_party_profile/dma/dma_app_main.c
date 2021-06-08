#include "dma_app_main.h"
#include "dma_deal.h"
#include "3th_profile_api.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack_3th_protocol_user.h"
#include "key_event_deal.h"
#include "soundbox.h"
#include "bt_tws.h"
#include "bt_common.h"
#include "tone_player.h"
#include "app_main.h"
#include "clock_cfg.h"

#define DMA_APP_DEBUG_ENABLE
#ifdef DMA_APP_DEBUG_ENABLE
#define dma_app_printf printf
#else
#define dma_app_printf(...)
#endif//DMA_APP_DEBUG_ENABLE

#if (DUEROS_DMA_EN)

#define DMA_APP_SPEECH_TONE_EN			1

enum {
    DMA_APP_SPEECH_STATUS_IDLE = 0x0,
    DMA_APP_SPEECH_STATUS_START_REPORT,
    DMA_APP_SPEECH_STATUS_REC,
};

enum {
    DMA_TTS_CONNECTED_ALL_FINISH = 0,
    DMA_TTS_PROTOCOL_CONNECTED,
    DMA_TTS_CONNECTED_NEED_OPEN_APP,
    DMA_TTS_DISCONNECTED,
    DMA_TTS_DISCONNECTED_ALL,
    DMA_TTS_OPEN_APP,
    DMA_TTS_POWERON,
};


static const char *dma_notice_tab[] = {
    [DMA_TTS_CONNECTED_ALL_FINISH]		= TONE_RES_ROOT_PATH"tone/xd_ok.*",//所有连接完成【已连接，你可以按AI键来和我进行对话】
    [DMA_TTS_PROTOCOL_CONNECTED]		= TONE_RES_ROOT_PATH"tone/xd_con.*",//小度APP已连接，经典蓝牙未连接【请在手机上完成蓝牙配对】
    [DMA_TTS_CONNECTED_NEED_OPEN_APP]	= TONE_RES_ROOT_PATH"tone/xd_btcon.*",//经典蓝牙已连接，小度app未连接【已配对，请打开小度app进行连接】
    [DMA_TTS_DISCONNECTED]				= TONE_RES_ROOT_PATH"tone/xd_dis.*",//经典蓝牙已断开【蓝牙已断开，请在手机上完成蓝牙配对】
    [DMA_TTS_DISCONNECTED_ALL]			= TONE_RES_ROOT_PATH"tone/xd_alldis.*",//经典蓝牙和小度都断开了【蓝牙未连接，请用手机蓝牙和我连接吧】
    [DMA_TTS_OPEN_APP]					= TONE_RES_ROOT_PATH"tone/xd_app.*",//请打开小度app【请打开小度APP】
    [DMA_TTS_POWERON]					= TONE_RES_ROOT_PATH"tone/xd_power.*",//小度开机【已开机，等待连接】
};



static volatile u8 app_speech_start_status = 0;
static volatile u16 dma_app_speech_timer = 0;
static volatile u8 dma_connect_status = 0;
extern int ai_mic_rec_start(void);
extern int ai_mic_rec_close(void);
extern int ai_mic_is_busy(void);
extern u8 get_remote_dev_company(void);
extern void phone_call_close_ble_adv(void);
extern void phone_call_resume_ble_adv(void);
extern u8 get_app_connect_type(void);
extern void ble_ibeacon_adv(void);
extern void phone_call_change_ble_conn_param(u8 param_index);
extern void phone_call_begin_ai(void);

void mic_rec_clock_set(void)
{
    clock_add_set(AI_SPEECH_CLK);
}

void mic_rec_clock_recover(void)
{
    clock_remove_set(AI_SPEECH_CLK);
}

int dueros_stop_speech(void)
{
    if (app_speech_start_status == DMA_APP_SPEECH_STATUS_REC) {
        int err = app_task_put_key_msg(KEY_SEND_SPEECH_STOP, 0);
        if (err) {
            dma_app_printf("speech stop err !!");
            return -1;
        }
        printf("dma_hw_stop_speech >>>>>>>>>>\n");
    }
    return 0;
}

int dueros_start_speech(void)
{
    if (app_speech_start_status) {
        printf("dma_hw_start_speech aready!!\n");
        return 0;
    }
    int err = app_task_put_key_msg(KEY_APP_SEND_SPEECH_START, 0);
    if (err) {
        dma_app_printf("speech start err !!");
        return -1;
    }
    printf("dma_hw_start_speech >>>>>>>>>>\n");
    return 0;
}

static void dma_speech_stop(void)
{
    if (dma_app_speech_timer) {
        sys_timeout_del(dma_app_speech_timer);
        dma_app_speech_timer = 0;
    }
    app_speech_start_status = DMA_APP_SPEECH_STATUS_IDLE;
    dma_app_printf("dma_speech_stop\n");
    ///mqc mark
    ai_mic_rec_close();
}

static void dma_speech_timeout(void *priv)
{
    dma_app_printf("dma speech timeout !!! \n");
    dma_speech_stop();
}
static int dma_speech_start(void)
{
    if (ai_mic_is_busy()) {
        if (dma_app_speech_timer) {
            sys_timer_modify(dma_app_speech_timer, 8000);
        }
        return -1;
    }

    if (ai_mic_rec_start() == 0) {
        dma_app_speech_timer = sys_timeout_add(NULL, dma_speech_timeout, 8000);
        app_speech_start_status = DMA_APP_SPEECH_STATUS_REC;
        return 0;
    }
    dma_app_printf("speech start fail\n");
    return -1;
}

static void  dma_app_tone_play_end_callback(void *priv, int flag)
{
    u32 index = (u32)priv;
    switch (index) {
    case IDEX_TONE_SPEECH_START:
        dma_app_printf("IDEX_TONE_SPEECH_START\n");
        if (app_speech_start_status == DMA_APP_SPEECH_STATUS_START_REPORT) {
            //app启动的
            dma_speech_start();
        } else {
            //按键启动需要report一下
            app_speech_start_status = DMA_APP_SPEECH_STATUS_START_REPORT;
            if (dueros_dma_start_speech() == 0) {
                dma_speech_start();
            } else {
                app_speech_start_status = DMA_APP_SPEECH_STATUS_IDLE;
                printf("dueros_dma_start_speech fail\n");
            }
        }
        break;
    default:
        break;
    }
}

void dma_app_key_event_deal(struct sys_event *event)
{
    int ret = false;
    struct key_event *key = &event->u.key;
    int key_event = event->u.key.event;
    int key_value = event->u.key.value;
    switch (key_event) {
    case KEY_APP_SEND_SPEECH_START:
        dma_app_printf("KEY_APP_SEND_SPEECH_START \n");
        app_speech_start_status = DMA_APP_SPEECH_STATUS_START_REPORT;
        tone_play_with_callback_by_name(tone_table[IDEX_TONE_SPEECH_START], 1, dma_app_tone_play_end_callback, (void *)IDEX_TONE_SPEECH_START);
        break;
    case KEY_SEND_SPEECH_START:
        dma_app_printf("KEY_SEND_SPEECH_START \n");
        if (BT_STATUS_TAKEING_PHONE == get_bt_connect_status()) {
            dma_app_printf("phone ing...\n");
            break;
        }

        if (app_var.siri_stu) {
            dma_app_printf("siri activing...\n");
            break;;
        }

        if (get_curr_channel_state()&A2DP_CH) {
            if (dma_connect_status == 0) {
                //<DMA未连接， 但是A2DP已连接， 点击唤醒键， 提示TTS【请打开小度APP】
                tone_play_with_callback_by_name(dma_notice_tab[DMA_TTS_OPEN_APP], 1, NULL, 0);
                break;
            }
        } else {
            //<蓝牙未连接， 用户按唤醒键， 提示TTS【蓝牙未连接， 请用手机蓝牙和我连接吧】
            tone_play_with_callback_by_name(dma_notice_tab[DMA_TTS_DISCONNECTED_ALL], 1, NULL, 0);
            break;
        }
        tone_play_with_callback_by_name(tone_table[IDEX_TONE_SPEECH_START], 1, dma_app_tone_play_end_callback, (void *)IDEX_TONE_SPEECH_START);
        break;
    case KEY_SEND_SPEECH_STOP:
        dma_app_printf("KEY_SEND_SPEECH_STOP \n");
        dma_speech_stop();
        break;
    }
}
void dma_bt_status_event(struct bt_event *e)
{
    switch (e->event) {
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        if (dma_connect_status == 1) {
            tone_play_with_callback_by_name(dma_notice_tab[DMA_TTS_CONNECTED_ALL_FINISH], 1, NULL, 0);
        } else {
            tone_play_with_callback_by_name(dma_notice_tab[DMA_TTS_CONNECTED_NEED_OPEN_APP], 1, NULL, 0);
        }
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        tone_play_with_callback_by_name(dma_notice_tab[DMA_TTS_DISCONNECTED], 1, NULL, 0);
        break;
    case BT_STATUS_PHONE_INCOME:
        break;
    case BT_STATUS_PHONE_OUT:
        break;
    case BT_STATUS_PHONE_ACTIVE:
        break;
    case BT_STATUS_PHONE_HANGUP:
        break;
    case BT_STATUS_PHONE_NUMBER:
        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
        if (e->value != 0xff) {
            //电话激活， 先停止当前录音
        } else {

        }
        break;
    case BT_STATUS_VOICE_RECOGNITION:
        if (e->value) { //如果是siri语音状态，停止录音
        }
        break;
    default:
        break;
    }
}

void dma_event_deal(struct bt_event *bt)
{
    switch (bt->event) {
    case KEY_DUEROS_CONNECTED:
        dma_connect_status = 1;
        if (get_curr_channel_state() & A2DP_CH) {
            tone_play_with_callback_by_name(dma_notice_tab[DMA_TTS_CONNECTED_ALL_FINISH], 1, NULL, 0);
        } else {
            tone_play_with_callback_by_name(dma_notice_tab[DMA_TTS_PROTOCOL_CONNECTED], 1, NULL, 0);
        }
        break;
    case KEY_DUEROS_DISCONNECTED:
        dma_connect_status = 0;
        tone_play_with_callback_by_name(dma_notice_tab[DMA_TTS_PROTOCOL_CONNECTED], 1, NULL, 0);
        break;
    default:
        break;
    }
}

int dma_app_bt_state_init(void)
{
    dma_spp_init();
    return 0;
}
void dma_app_bredr_handle_reg(void)
{
    spp_data_deal_handle_register(user_spp_data_handler);
}
int dma_app_sys_event_handler_specific(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        dma_app_key_event_deal(event);
        break;
    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            dma_bt_status_event(&event->u.bt);
        } else if (((u32)event->arg == SYS_BT_AI_EVENT_TYPE_STATUS)) {
            dma_event_deal(&event->u.bt);
        }
        break;
    }
    return 0;
}

#endif//DUEROS_DMA_EN


