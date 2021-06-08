#include "btstack/avctp_user.h"
#include "app_config.h"
#include "gma.h"
#include "key_event_deal.h"
#include "app_config.h"

#if (GMA_EN)

_WEAK_ bool is_siri_active(void)
{
    return 0;//(app_var.siri_stu == 1) || (app_var.siri_stu == 2);
}

bool tm_gma_msg_deal(u32 param)
{
    switch (param) {

    case KEY_SEND_SPEECH_START:
        printf("gma MSG_SEND_SPEECH_START\n");

        if (BT_STATUS_TAKEING_PHONE == get_bt_connect_status()) {
            printf("phone ing...\n");
            break;
        }

        if (gma_connect_success() && (get_curr_channel_state()&A2DP_CH)) {
        } else {
            if (get_curr_channel_state()&A2DP_CH) {
                //<GMA未连接， 但是A2DP已连接， 点击唤醒键， 提示TTS【请打开小度APP】
            } else {
                //<蓝牙完全关闭状态， 用户按唤醒键， 提示TTS【蓝牙未连接， 请用手机蓝牙和我连接吧】
            }
            break;
        }

#if 0
        if (is_siri_active()) {
            printf("siri activing...\n");
            break;
        }
#endif

        printf("gma start mic status report \n");
        if (gma_mic_status_report(START_MIC) == 0) {
            gma_hw_start_speech();
        }
        break;
#if 1
    case KEY_SEND_SPEECH_STOP:
        printf("MSG_SPEECH_STOP\n");
        gma_hw_stop_speech();
        break;
#endif
    default:
        break;
    }

    return FALSE;
}

void gma_event_post(u32 type, u8 event)
{
    struct sys_event e;
    e.type = SYS_BT_EVENT;
    e.arg  = (void *)type;
    e.u.bt.event = event;
    sys_event_notify(&e);
}

void phone_call_begin_ai(void)
{
    printf(">>>>> gma phone coming ai mic close \n");
    int mic_coder_busy_flag(void);
    if (mic_coder_busy_flag()) {
        gma_hw_stop_speech();
    }

}

void phone_call_end_ai(void)
{

}

#endif
