#include "smartbox/config.h"
#include "syscfg_id.h"
#include "le_smartbox_module.h"

#include "adv_time_stamp_setting.h"
#include "smartbox_setting_sync.h"
#include "smartbox_setting_opt.h"
#include "smartbox_adv_bluetooth.h"

#include "smartbox_rcsp_manage.h"

//#if (SMART_BOX_EN && RCSP_SMARTBOX_ADV_EN)
#if (SMART_BOX_EN)
extern int get_bt_tws_connect_status();
static u32 adv_time_stamp = 0;
static int get_adv_time_stamp(u8 *time_stamp)
{
    memcpy(time_stamp, (u8 *)&adv_time_stamp, sizeof(adv_time_stamp));
    return 0;
}

static void set_adv_time_stamp(u8 *time_stamp)
{
    memcpy((u8 *)&adv_time_stamp, time_stamp, sizeof(adv_time_stamp));
}

void sync_setting_by_time_stamp(void)
{
    tws_api_send_data_to_sibling((u8 *)&adv_time_stamp, sizeof(adv_time_stamp), TWS_FUNC_ID_TIME_STAMP_SYNC);
}

void deal_sibling_time_stamp_setting_switch(void *data, u16 len)
{
    u32 time_stamp = *((u32 *)data);
    if (adv_time_stamp > time_stamp) {
        JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_ADV_SETTING_SYNC, NULL, 0);
    }
}

void deal_adv_setting_gain_time_stamp(void)
{
#if TCFG_USER_TWS_ENABLE
    extern u8 adv_info_device_request(u8 * buf, u16 len);
    static u8 tws_pair = 0;
    u8 buf = 2;
    u8 ret = 0;
    if (get_bt_tws_connect_status()) {
        tws_pair = 1;
    } else {
        if (tws_pair) {
            tws_pair = 0;
            ret = adv_info_device_request(&buf, sizeof(buf));
        }
    }
#endif
}

// 1、写入VM
static void update_time_stamp_vm_value(u8 *time_stamp)
{
    syscfg_write(CFG_RCSP_ADV_TIME_STAMP, time_stamp, 4);
}
// 2、同步对端
static void adv_time_stamp_sync(u8 *time_stamp)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_smartbox_setting(ATTR_TYPE_TIME_STAMP);
    }
#endif
}

static void deal_time_stamp_setting(u8 *time_stamp, u8 write_vm, u8 tws_sync)
{
    if (time_stamp) {
        set_adv_time_stamp(time_stamp);
    }
    if (write_vm) {
        update_time_stamp_vm_value((u8 *)&adv_time_stamp);
    }
    if (tws_sync) {
        adv_time_stamp_sync((u8 *)&adv_time_stamp);
    }
}

static int adv_time_stamp_sibling_settting_deal(u8 *setting_data)
{
    u32 time_stamp = (setting_data[0] | (setting_data[1] << 8) | (setting_data[2] << 16) | (setting_data[3] << 24));
    set_adv_time_stamp((u8 *)&time_stamp);
    return 0;
}

static int time_stamp_set_setting_extra_handle(void *setting_data, void *param)
{
    u8 dlen = *((u8 *)param);
    u8 *time_stamp_setting_data = (u8 *) setting_data;
    adv_time_stamp = (((time_stamp_setting_data[0] << 8) | time_stamp_setting_data[1]) << 8 | time_stamp_setting_data[2]) << 8 | time_stamp_setting_data[3];

    u8 bt_name[32] = {0};
    syscfg_read(CFG_BT_NAME, bt_name, sizeof(bt_name));
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_EDR_NAME);
    if (setting_opt_hdl && setting_opt_hdl->set_setting) {
        setting_opt_hdl->set_setting(bt_name);
    }
    return 0;
}

static SMARTBOX_SETTING_OPT adv_time_stamp_opt = {
    .data_len = 4,
    .setting_type = ATTR_TYPE_TIME_STAMP,
    .syscfg_id = CFG_RCSP_ADV_TIME_STAMP,
    .deal_opt_setting = deal_time_stamp_setting,
    .set_setting = set_adv_time_stamp,
    .get_setting = get_adv_time_stamp,
    .custom_setting_init = NULL,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = adv_time_stamp_sibling_settting_deal,
    .custom_setting_release = NULL,
    .set_setting_extra_handle = time_stamp_set_setting_extra_handle,
    .get_setting_extra_handle = NULL,
};
REGISTER_APP_SETTING_OPT(adv_time_stamp_opt);

#endif
