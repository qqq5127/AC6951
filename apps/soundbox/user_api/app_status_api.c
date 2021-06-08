#include "user_api/app_status_api.h"
#include "system/event.h"
#include "app_config.h"
extern void app_status_6083_handler(enum APP_STATUS status);

void app_status_handler(enum APP_STATUS status)//必须在对应板级定义对应处理接口
{
#ifdef CONFIG_BOARD_AC6083A
    app_status_6083_handler(status);
#endif
    return;
}

void app_status_bt_event(struct bt_event *e)
{
    switch (e->event) {
    case BT_STATUS_SECOND_CONNECTED:
        app_status_handler(APP_STATUS_BT_SECOND_CONNECTED);
        break;
    case BT_STATUS_FIRST_CONNECTED:
        app_status_handler(APP_STATUS_BT_SECOND_DISCONNECT);
        break;
    case BT_STATUS_FIRST_DISCONNECT:
        app_status_handler(APP_STATUS_BT_FIRST_DISCONNECT);
        break;
    case BT_STATUS_SECOND_DISCONNECT:
        app_status_handler(APP_STATUS_BT_SECOND_DISCONNECT);
        break;
    case BT_STATUS_PHONE_INCOME:
        app_status_handler(APP_STATUS_BT_PHONE_INCOME);
        break;
    case BT_STATUS_PHONE_OUT:
        app_status_handler(APP_STATUS_BT_PHONE_OUT);
        break;
    case BT_STATUS_PHONE_ACTIVE:
        app_status_handler(APP_STATUS_BT_PHONE_ACTIVE);
        break;
    case BT_STATUS_PHONE_HANGUP:
        app_status_handler(APP_STATUS_BT_PHONE_HANGUP);
        break;
    case BT_STATUS_PHONE_NUMBER:
        app_status_handler(APP_STATUS_BT_PHONE_NUMBER);
        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
        app_status_handler(APP_STATUS_BT_SCO_STATUS_CHANGE);
        break;
    case BT_STATUS_VOICE_RECOGNITION:
        app_status_handler(APP_STATUS_BT_VOICE_RECOGNITION);
        break;
    case BT_STATUS_A2DP_MEDIA_START:
        app_status_handler(APP_STATUS_BT_A2DP_MEDIA_START);
        break;
    case BT_STATUS_INIT_OK:
        app_status_handler(APP_STATUS_BT_INIT_OK);
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        app_status_handler(APP_STATUS_BT_A2DP_MEDIA_STOP);
        break;
    default:
        break;
    }
}

