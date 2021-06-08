#include "app_config.h"
#include "smartbox_setting_sync.h"
#include "smartbox_setting_opt.h"
#include "adv_time_stamp_setting.h"

#if (SMART_BOX_EN && TCFG_USER_TWS_ENABLE)
static void smartbox_sync_tws_func_t(void *data, u16 len, bool rx)
{
    if (rx) {
        deal_sibling_setting((u8 *)data);
    }
}

REGISTER_TWS_FUNC_STUB(adv_tws_sync) = {
    .func_id = TWS_FUNC_ID_ADV_SETTING_SYNC,
    .func    = smartbox_sync_tws_func_t,
};

static void tws_app_opt_sync_call_fun(int cmd)
{
    struct sys_event event;

    event.type = SYS_BT_EVENT;
    event.arg = (void *)SYS_BT_EVENT_FROM_APP_OPT_TWS;

    event.u.bt.event = APP_OPT_TWS_EVENT_SYNC_FUN_CMD;
    event.u.bt.args[0] = 0;
    event.u.bt.args[1] = 0;
    event.u.bt.args[2] = cmd;
    sys_event_notify(&event);
}

TWS_SYNC_CALL_REGISTER(tws_tone_sync) = {
    .uuid = TWS_FUNC_APP_OPT_UUID,
    .func = tws_app_opt_sync_call_fun,
};

#if 1//RCSP_SMARTBOX_ADV_EN
static void adv_sync_time_stamp_func_t(void *data, u16 len, bool rx)
{
    if (rx) {
        deal_sibling_time_stamp_setting_switch(data, len);
    }
}

REGISTER_TWS_FUNC_STUB(adv_time_stamp_sync) = {
    .func_id = TWS_FUNC_ID_TIME_STAMP_SYNC,
    .func    = adv_sync_time_stamp_func_t,
};


static void adv_sync_reset_sync_func_t(int args)
{
    extern void cpu_reset();
    cpu_reset();
}

TWS_SYNC_CALL_REGISTER(adv_reset_sync) = {
    .uuid = TWS_FUNC_ID_ADV_RESET_SYNC,
    .func    = adv_sync_reset_sync_func_t,
};
#endif

#endif
