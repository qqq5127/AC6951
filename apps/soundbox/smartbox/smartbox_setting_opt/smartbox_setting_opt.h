#ifndef __SMARTBOX_SETTING_OPT_H__
#define __SMARTBOX_SETTING_OPT_H__

#include "system/includes.h"

#define REGISTER_APP_SETTING_OPT(smart_opt) \
int smart_opt##_setting_init(void) {\
   return register_smartbox_setting_opt_setting(&smart_opt);\
}\
late_initcall(smart_opt##_setting_init);

#pragma pack(1)
typedef struct _SMARTBOX_SETTING_OPT {
    struct _SMARTBOX_SETTING_OPT *next;
    bool need_opt;
    u32 data_len;
    int setting_type;
    int syscfg_id;
    int (*get_setting)(u8 *setting_data);
    void (*set_setting)(u8 *setting_data);
    void (*deal_opt_setting)(u8 *setting_data, u8 write_vm, u8 tws_sync);

    // 上面是必填，下面是选填
    int (*custom_setting_init)(void);
    int (*custom_vm_info_update)(void);
    int (*custom_setting_update)(u8 *setting_data);
    int (*custom_setting_key_event_update)(int event, void *param);
    int (*custom_sibling_setting_deal)(u8 *setting_data);
    int (*custom_setting_release)(void);
    int (*set_setting_extra_handle)(void *setting_data, void *param);
    int (*get_setting_extra_handle)(void *setting_data, void *param);
} SMARTBOX_SETTING_OPT;
#pragma pack()

void deal_sibling_setting(u8 *buf);
u8 smartbox_read_data_from_vm(u8 syscfg_id, u8 *buf, u8 buf_len);
int register_smartbox_setting_opt_setting(void *opt_param);

SMARTBOX_SETTING_OPT *get_smartbox_setting_opt_hdl(u16 setting_type);
void set_smartbox_opt_setting(SMARTBOX_SETTING_OPT *setting_opt, u8 *data);
int get_smartbox_opt_setting(SMARTBOX_SETTING_OPT *setting_opt, u8 *data);
int set_setting_extra_handle(SMARTBOX_SETTING_OPT *setting_opt_hdl, void *setting_data, void *param);
int get_setting_extra_handle(SMARTBOX_SETTING_OPT *setting_opt_hdl, void *setting_data, void *param);
int smartbox_opt_key_event_update(int event, void *param);

void smart_setting_init(void);
void update_smartbox_setting(u8 type);
void update_info_from_vm_info(void);
void smartbox_opt_release(void);

//属于弹窗
void modify_bt_name_and_reset(u32 msec);
#endif
