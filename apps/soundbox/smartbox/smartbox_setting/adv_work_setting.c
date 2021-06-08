#include "app_config.h"
#include "syscfg_id.h"
#include "le_smartbox_module.h"

#include "smartbox_setting_sync.h"
#include "smartbox_setting_opt.h"
#include "os/os_api.h"
#include "btstack/avctp_user.h"

//#if (SMART_BOX_EN && RCSP_SMARTBOX_ADV_EN)
#if (SMART_BOX_EN)
#if RCSP_ADV_WORK_SET_ENABLE

static u8 g_work_mode = 1;

extern int get_bt_tws_connect_status();
extern void bt_set_low_latency_mode(int enable);
static void set_work_setting(u8 *work_setting_info)
{
    g_work_mode = *work_setting_info;
}

static int get_work_setting(u8 *work_setting_info)
{
    *work_setting_info = g_work_mode;
    return 0;
}

static void adv_work_setting_vm_value(u8 *work_setting_info)
{
    /* syscfg_write(CFG_RCSP_ADV_WORK_SETTING, &work_setting_info, 1); */
}

static void adv_work_setting_sync(u8 *work_setting_info)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_smartbox_setting(ATTR_TYPE_WORK_MODE);
    }
#endif
}

static void __rcsp_work_set_call(void)
{
    //r_f_printf("__rcsp_work_set_call\n");
    if (1 == g_work_mode) {
        bt_set_low_latency_mode(0);
    } else if (2 == g_work_mode) {
        bt_set_low_latency_mode(1);
    }
}

static void rcsp_work_set_deal(void)
{
    int err;
    int msg[3];
    msg[0] = (int) __rcsp_work_set_call;
    msg[1] = 1;
    msg[2] = 0;

    err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
    //r_printf("err %x\n",err);


}

static void update_work_setting_state(void)
{
    //r_f_printf("set deal\n");
    rcsp_work_set_deal();
}

static void deal_work_setting(u8 *work_setting_info, u8 write_vm, u8 tws_sync)
{
    if (work_setting_info) {
        set_work_setting(work_setting_info);
    }
    if (write_vm) {
        adv_work_setting_vm_value(&g_work_mode);
    }
    if (tws_sync) {
        adv_work_setting_sync(&g_work_mode);
    }
    update_work_setting_state();
}


int ble_vendor_is_slow_state(void)
{
    if ((BT_STATUS_PLAYING_MUSIC == get_bt_connect_status()) && (2 == g_work_mode)) {
        return 1;
    } else {
        return 0;
    }
}

static int adv_work_opt_init(void)
{
    u8 work_setting_info = 0;
    if (smartbox_read_data_from_vm(CFG_RCSP_ADV_WORK_SETTING, &work_setting_info, sizeof(work_setting_info))) {
        set_work_setting(&work_setting_info);
        __rcsp_work_set_call();
    }
    return 0;
}

static int work_set_setting_extra_handle(void *setting_data, void *param)
{
    u8 dlen = *((u8 *)param);
    u8 *led_setting_data = (u8 *) setting_data;
    memcpy(&g_work_mode, led_setting_data, dlen);
    return 0;
}

static int work_get_setting_extra_handle(void *setting_data, void *param)
{
    int **setting_data_ptr = (int **)setting_data;
    *setting_data_ptr = &g_work_mode;
    return sizeof(g_work_mode);
}

static SMARTBOX_SETTING_OPT adv_work_opt = {
    .data_len = 1,
    .setting_type = ATTR_TYPE_WORK_MODE,
    .syscfg_id = CFG_RCSP_ADV_WORK_SETTING,
    .deal_opt_setting = deal_work_setting,
    .set_setting = set_work_setting,
    .get_setting = get_work_setting,
    .custom_setting_init = adv_work_opt_init,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = NULL,
    .custom_setting_release = NULL,
    .set_setting_extra_handle = work_set_setting_extra_handle,
    .get_setting_extra_handle = work_get_setting_extra_handle,
};

REGISTER_APP_SETTING_OPT(adv_work_opt);

#endif
#endif
