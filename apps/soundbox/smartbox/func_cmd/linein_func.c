#include "smartbox/func_cmd/linein_func.h"
#include "smartbox/func_cmd_common.h"
#include "smartbox/function.h"
#include "smartbox/config.h"
#include "smartbox/event.h"
#include "app_action.h"

#include "app_msg.h"
#include "key_event_deal.h"

#if SMART_BOX_EN

#if (TCFG_APP_LINEIN_EN && !SOUNDCARD_ENABLE)
#include "linein/linein.h"
#include "tone_player.h"

#define LINEIN_INFO_ATTR_STATUS		(0)
bool linein_func_set(void *priv, u8 *data, u16 len)
{
    if (0 != tone_get_status()) {
        return true;
    }
    u8 fun_cmd = data[0];
    u16 param_len = len - 1;
    switch (fun_cmd) {
    case 1:
        app_task_put_key_msg(KEY_MUSIC_PP, 0); //test demo
        break;
    default:
        break;
    }
    return true;
}

u32 linein_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    u16 offset = 0;
    if (mask & BIT(LINEIN_INFO_ATTR_STATUS)) {
        u8 status = linein_get_status();
        offset = add_one_attr(buf, buf_size, offset, LINEIN_INFO_ATTR_STATUS, (u8 *)&status, sizeof(status));
    }
    return offset;
}

void smartbox_linein_msg_deal(int msg, u8 ret)
{
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL) {
        return ;
    }
    switch (msg) {
    case  KEY_VOL_DOWN:
    case  KEY_VOL_UP:
        smartbox_function_update(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_VOL));
        break;
    case (int)-1:
    case KEY_MUSIC_PP:
    case KEY_LINEIN_START:
        smartbox_function_update(LINEIN_FUNCTION_MASK, BIT(LINEIN_INFO_ATTR_STATUS));
        break;
    }
}

void linein_func_stop(void)
{
    if (linein_get_status()) {
        app_task_put_key_msg(KEY_MUSIC_PP, 0);
    }
}




#else

bool linein_func_set(void *priv, u8 *data, u16 len)
{
    return true;
}

u32 linein_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    return 0;
}

void smartbox_linein_msg_deal(int msg, u8 ret)
{

}

void smartbot_linein_msg_deal(int msg)
{

}

void linein_func_stop(void)
{

}
#endif
#endif
