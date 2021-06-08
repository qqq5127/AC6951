#include "smartbox/config.h"
#include "smartbox/function.h"
#include "smartbox/smartbox.h"
#include "smartbox/event.h"
#include "smartbox/func_cmd_common.h"
#include "fm_emitter/fm_emitter_manage.h"

#include "le_smartbox_module.h"
#include "smartbox_setting_opt.h"
#include "dev_manager.h"
#include "btstack/avctp_user.h"

#include "app_main.h"
#include "app_action.h"

#if (SMART_BOX_EN)

#define ASSERT_SET_LEN(len, limit) 	\
									do{	\
										if(len >= limit){	\
										}else{				\
											return 0;   \
										}\
									}while(0);


#define ASSERT_SET_LEN_RETURN_NULL(len, limit) 	\
									do{	\
										if(len >= limit){	\
										}else{				\
											return ;   \
										}\
									}while(0);

#define FUNCTION_UPDATE_MAX_LEN			(256)

#pragma pack(1)
struct _DEV_info {
    u8 status;
    u32 usb_handle;
    u32 sd0_handle;
    u32 sd1_handle;
    u32 flash_handle;
};
#pragma pack()

typedef bool (*func_set)(void *priv, u8 *data, u16 len);
typedef u32(*func_get)(void *priv, u8 *buf, u16 buf_size, u32 mask);
static const func_set set_tab[FUNCTION_MASK_MAX] = {
    [BT_FUNCTION_MASK] = bt_func_set,
    [MUSIC_FUNCTION_MASK] = music_func_set,
    [RTC_FUNCTION_MASK] = rtc_func_set,
    [LINEIN_FUNCTION_MASK] = linein_func_set,
    [FM_FUNCTION_MASK] = fm_func_set,
    [FMTX_FUNCTION_MASK] = NULL,
};

static const func_get get_tab[FUNCTION_MASK_MAX] = {
    [BT_FUNCTION_MASK] = bt_func_get,
    [MUSIC_FUNCTION_MASK] = music_func_get,
    [RTC_FUNCTION_MASK] = rtc_func_get,
    [LINEIN_FUNCTION_MASK] = linein_func_get,
    [FM_FUNCTION_MASK] = fm_func_get,
    [FMTX_FUNCTION_MASK] = NULL,
};

static void common_function_attr_vol_set(void *priv, u8 attr, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    if (BT_CALL_HANGUP != get_call_status()) {
        smart->err_code = -1;
        return;
    }
    ASSERT_SET_LEN_RETURN_NULL(len, 1);
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_VOL_SETTING);
    if (setting_opt_hdl) {
        set_smartbox_opt_setting(setting_opt_hdl, data);
    }
}
static void common_function_attr_eq_set(void *priv, u8 attr, u8 *data, u16 len)
{
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_EQ_SETTING);
    if (setting_opt_hdl) {
        set_smartbox_opt_setting(setting_opt_hdl, data);
        u32 mask = BIT(attr);
        smartbox_msg_post(USER_MSG_SMARTBOX_SET_EQ_PARAM, 2, (int)priv, mask);
    }
}
static void common_function_attr_fmtx_freq_set(void *priv, u8 attr, u8 *data, u16 len)
{
#if (TCFG_APP_FM_EMITTER_EN && TCFG_FM_EMITTER_INSIDE_ENABLE)
    ASSERT_SET_LEN_RETURN_NULL(len, 2);
    u16 freq = READ_BIG_U16(data);
    smartbox_msg_post(USER_MSG_SMARTBOX_SET_FMTX_FREQ, 2, (int)priv, (int)freq);
#endif
}
static void common_function_attr_bt_emitter_sw_set(void *priv, u8 attr, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    ASSERT_SET_LEN_RETURN_NULL(len, 1);
    smart->emitter_sw = data[0];
    smartbox_msg_post(USER_MSG_SMARTBOX_SET_BT_EMITTER_SW, 1, (int)priv);
}
static void common_function_attr_bt_emitter_connect_state_set(void *priv, u8 attr, u8 *data, u16 len)
{
    ASSERT_SET_LEN_RETURN_NULL(len, 7);
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    memcpy(smart->emitter_con_addr, data + 1, 6);
    smartbox_msg_post(USER_MSG_SMARTBOX_SET_BT_EMITTER_CONNECT_STATES, 2, (int)priv, (int)data[0]);
}

static void common_function_attr_high_low_set(void *priv, u8 attr, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    if (BT_CALL_HANGUP != get_call_status()) {
        smart->err_code = -1;
        return;
    }

    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_HIGH_LOW_VOL);
    if (!set_setting_extra_handle(setting_opt_hdl, data, NULL)) {
        set_smartbox_opt_setting(setting_opt_hdl, NULL);
    }
}

static void common_function_attr_misc_setting_set(void *priv, u8 attr, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    if (BT_CALL_HANGUP != get_call_status()) {
        smart->err_code = -1;
        return;
    }
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_MISC_SETTING);
    if (setting_opt_hdl) {
        if (!set_setting_extra_handle(setting_opt_hdl, data, NULL)) {
            set_smartbox_opt_setting(setting_opt_hdl, data);
        } else {
            smart->err_code = -1;
        }
    }
}

static void common_function_attr_color_led_setting_set(void *priv, u8 attr, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_COLOR_LED_SETTING);
    if (setting_opt_hdl) {
        set_smartbox_opt_setting(setting_opt_hdl, data);
    }
}

static void common_function_attr_karaoke_eq_setting_set(void *priv, u8 attr, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_KARAOKE_EQ_SETTING);
    if (setting_opt_hdl) {
        set_smartbox_opt_setting(setting_opt_hdl, data);
        u32 mask = BIT(attr);
        smartbox_msg_post(USER_MSG_SMARTBOX_SET_EQ_PARAM, 2, (int)priv, mask);
    }
}

static void common_function_attr_karaoke_setting_set(void *priv, u8 attr, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }

    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_KARAOKE_SETTING);
    if (setting_opt_hdl) {
        if (!set_setting_extra_handle(setting_opt_hdl, data, &len)) {
            set_smartbox_opt_setting(setting_opt_hdl, NULL);
        }
    }
}

static const attr_set_func common_function_set_tab[COMMON_FUNCTION_ATTR_TYPE_MAX] = {
    [COMMON_FUNCTION_ATTR_TYPE_BATTERY 				          ] = NULL,
    [COMMON_FUNCTION_ATTR_TYPE_VOL 					          ] = common_function_attr_vol_set,
    [COMMON_FUNCTION_ATTR_TYPE_DEV_INFO 			          ] = NULL,
    [COMMON_FUNCTION_ATTR_TYPE_ERROR_STATS			          ] = NULL,
    [COMMON_FUNCTION_ATTR_TYPE_EQ_INFO				          ] = common_function_attr_eq_set,//NULL,
    [COMMON_FUNCTION_ATTR_TYPE_BS_FILE_TYPE			          ] = NULL,
    [COMMON_FUNCTION_ATTR_TYPE_FUNCTION_MODE		          ] = NULL,
    [COMMON_FUNCTION_ATTR_TYPE_FMTX_FREQ			          ] = common_function_attr_fmtx_freq_set,
    [COMMON_FUNCTION_ATTR_TYPE_BT_EMITTER_SW		          ] = common_function_attr_bt_emitter_sw_set,
    [COMMON_FUNCTION_ATTR_TYPE_BT_EMITTER_CONNECT_STATES      ] = common_function_attr_bt_emitter_connect_state_set,
    [COMMON_FUNCTION_ATTR_TYPE_HIGH_LOW_SET			          ] = common_function_attr_high_low_set,
    [COMMON_FUNCTION_ATTR_TYPE_PRE_FETCH_ALL_EQ_INFO 	      ] = NULL,
    [COMMON_FUNCTION_ATTR_TYPE_PHONE_SCO_STATE_INFO 	      ] = NULL,
    [COMMON_FUNCTION_ATTR_TYPE_MISC_SETTING_INFO 	      	  ] = common_function_attr_misc_setting_set,
    [COMMON_FUNCTION_ATTR_TYPE_COLOR_LED_SETTING_INFO 		  ] = common_function_attr_color_led_setting_set,
    [COMMON_FUNCTION_ATTR_TYPE_PRE_FETCH_KARAOKE_EQ_INFO	  ] = NULL,
    [COMMON_FUNCTION_ATTR_TYPE_KARAOKE_EQ_SETTING_INFO 		  ] = common_function_attr_karaoke_eq_setting_set,
    [COMMON_FUNCTION_ATTR_TYPE_KARAOKE_SETTING_INFO 		  ] = common_function_attr_karaoke_setting_set,
};


static u32 common_function_attr_battery_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    extern u8 get_vbat_percent(void);
    u8 vbat = get_vbat_percent();
    rlen = add_one_attr(buf, buf_size, offset, attr, &vbat, sizeof(vbat));
    return rlen;
}
static u32 common_function_attr_vol_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    //extern s8 app_audio_get_volume(u8 state);
    //u8 cur_vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
    if (BT_CALL_HANGUP != get_call_status()) {
        return 0;
    }
    u8 cur_vol;
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_VOL_SETTING);
    if (setting_opt_hdl) {
        get_smartbox_opt_setting(setting_opt_hdl, (u8 *)&cur_vol);
        rlen = add_one_attr(buf, buf_size, offset, attr, &cur_vol, sizeof(cur_vol));
    }
    return rlen;
}
static u32 common_function_attr_dev_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    struct smartbox *smart = (struct smartbox *) priv;
    struct _DEV_info dev_info = {0};
    u32 dev_status_tmp = 0;

#if (TCFG_DEV_MANAGER_ENABLE)
    if (dev_manager_get_root_path_by_logo("udisk0")) {
        printf("dev [udisk0] online\n");
        dev_info.status |= BIT(BS_UDISK);
        dev_info.usb_handle |= app_htonl((u32)BS_UDISK);
    }

    if (dev_manager_get_root_path_by_logo("sd0")) {
        printf("dev [sd0] online\n");
        dev_info.status |= BIT(BS_SD0);
        dev_info.sd0_handle |= app_htonl((u32)BS_SD0);
    }

    if (dev_manager_get_root_path_by_logo("sd1")) {
        printf("dev [sd1] online\n");
        dev_info.status |= BIT(BS_SD1);
        dev_info.sd1_handle = app_htonl((u32)BS_SD1);
    }
#endif

    extern int linein_app_check(void);
    if (linein_app_check()) {
        dev_info.status |= BIT(BS_AUX);
    }

    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&dev_info, sizeof(dev_info));
    return rlen;
}
static u32 common_function_attr_error_states_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    return rlen;
}

static u32 common_function_attr_pre_fetch_all_eq_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 eq_pre_fetch_info[1  +  20  + (1 + 10) * 10] = {0}; // num + freq + all_gain_of_eq [max]
    u16 eq_per_fetch_size = sizeof(eq_pre_fetch_info);
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_EQ_SETTING);
    if (setting_opt_hdl) {
        eq_per_fetch_size = get_setting_extra_handle(setting_opt_hdl, eq_pre_fetch_info, &eq_per_fetch_size);
        if (eq_per_fetch_size) {
            rlen = add_one_attr(buf, buf_size, offset, attr, eq_pre_fetch_info, eq_per_fetch_size);
        }
    }
    return rlen;
}

static u32 common_function_attr_phone_sco_state_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 phone_state = 0;
    if (BT_CALL_HANGUP != get_call_status()) {
        phone_state = 1;
    }
    rlen = add_one_attr(buf, buf_size, offset, attr, &phone_state, sizeof(phone_state));
    return rlen;
}

static u32 common_function_attr_eq_param_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 eq_info[11] = {0};
    u16 eq_info_size = sizeof(eq_info);
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_EQ_SETTING);
    if (setting_opt_hdl) {
        eq_info_size = get_setting_extra_handle(setting_opt_hdl, eq_info, &eq_info_size);
        if (eq_info_size) {
            rlen = add_one_attr(buf, buf_size, offset, attr, eq_info, eq_info_size);
        }
    }
    return rlen;
}

static u32 common_function_attr_bs_file_type(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if RCSP_FILE_OPT
    /* printf("%s\n", smartbox_browser_file_ext()); */
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)smartbox_browser_file_ext(), smartbox_browser_file_ext_size());
#endif
    return rlen;
}
static u32 common_function_attr_function_mode_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    struct smartbox *smart = (struct smartbox *) priv;
    rlen = add_one_attr(buf, buf_size, offset, attr, &smart->cur_app_mode, sizeof(smart->cur_app_mode));
    return rlen;
}
static u32 common_function_attr_fmtx_freq_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if (TCFG_APP_FM_EMITTER_EN && TCFG_FM_EMITTER_INSIDE_ENABLE)
    u16 freq = fm_emitter_manage_get_fre();
    printf("freq %d\n", freq);
    freq = app_htons(freq);
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&freq, sizeof(freq));
#endif
    return rlen;
}
static u32 common_function_attr_bt_emitter_sw_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return 0;
    }
    u32 rlen = 0;
    u8 sw = smart->emitter_sw;
    rlen = add_one_attr(buf, buf_size, offset,  attr, &sw, 1);
    return rlen;
}
static u32 common_function_attr_bt_emitter_connect_state_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return 0;
    }

    u32 rlen = 0;
    u8 send_buf[7] = {0};
    /* if(smart->emitter_sw) */
    {
        send_buf[0] = smart->emitter_bt_state;
        memcpy(send_buf + 1, smart->emitter_con_addr, 6);
    }
    printf("stata = %d\n", send_buf[0]);
    put_buf(send_buf + 1, 6);
    rlen = add_one_attr(buf, buf_size, offset,  attr, send_buf, sizeof(send_buf));
    return rlen;
}
static u32 common_function_attr_high_low_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 data[8] = {0};
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_HIGH_LOW_VOL);
    if (setting_opt_hdl) {
        u8 size = get_setting_extra_handle(setting_opt_hdl, data, NULL);
        if (size) {
            rlen = add_one_attr(buf, buf_size, offset, attr, data, size);
        }
    }

    return rlen;
}

static u32 common_function_attr_misc_setting_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    extern u32 get_misc_setting_data_len(void);
    u32 data_len = get_misc_setting_data_len();
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_MISC_SETTING);
    if (data_len && setting_opt_hdl) {
        u8 *misc_data = zalloc(data_len);
        data_len = get_smartbox_opt_setting(setting_opt_hdl, misc_data);
        rlen = add_one_attr(buf, buf_size, offset, attr, misc_data, data_len);
        if (misc_data) {
            free(misc_data);
        }
    }
    return rlen;
}

static u32 common_function_attr_color_led_setting_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    extern u32 get_color_led_setting_data_len(void);
    u32 data_len = get_color_led_setting_data_len();
    u8 color_led_data[data_len];

    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_COLOR_LED_SETTING);
    if (setting_opt_hdl) {
        get_smartbox_opt_setting(setting_opt_hdl, color_led_data);
        rlen = add_one_attr(buf, buf_size, offset, attr, color_led_data, data_len);
    }
    return rlen;
}

static u32 common_function_attr_karaoke_eq_setting_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 karaoke_eq_info[9] = {0};
    u16 karaoke_eq_size = sizeof(karaoke_eq_info);
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_KARAOKE_EQ_SETTING);
    if (setting_opt_hdl) {
        karaoke_eq_size = get_setting_extra_handle(setting_opt_hdl, karaoke_eq_info, &karaoke_eq_size);
        if (karaoke_eq_size) {
            rlen = add_one_attr(buf, buf_size, offset, attr, karaoke_eq_info, karaoke_eq_size);
        }
    }
    return rlen;
}

static u32 common_function_attr_karaoke_setting_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 *data = NULL;
    u8 data_len = 0;
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_KARAOKE_SETTING);
    if (setting_opt_hdl) {
        data = (u8 *)zalloc(setting_opt_hdl->data_len);
        data_len = get_setting_extra_handle(setting_opt_hdl, data, NULL);
        if (data_len) {
            rlen = add_one_attr(buf, buf_size, offset, attr, data, data_len);
        }
    }
    if (data) {
        free(data);
    }
    return rlen;
}

static u32 common_function_attr_karaoke_pre_fetch_eq_setting_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 karaoke_eq_pre_fetch_info[1 + 8 * 2] = {0}; // num + freq * 2
    u16 karaoke_eq_pre_fetch_size = sizeof(karaoke_eq_pre_fetch_info);
    SMARTBOX_SETTING_OPT *setting_opt_hdl = get_smartbox_setting_opt_hdl(ATTR_TYPE_KARAOKE_EQ_SETTING);
    if (setting_opt_hdl) {
        karaoke_eq_pre_fetch_size = get_setting_extra_handle(setting_opt_hdl, karaoke_eq_pre_fetch_info, &karaoke_eq_pre_fetch_size);
        if (karaoke_eq_pre_fetch_size) {
            rlen = add_one_attr(buf, buf_size, offset, attr, karaoke_eq_pre_fetch_info, karaoke_eq_pre_fetch_size);
        }
    }
    return rlen;
}

static const attr_get_func target_common_function_get_tab[COMMON_FUNCTION_ATTR_TYPE_MAX] = {
    [COMMON_FUNCTION_ATTR_TYPE_BATTERY 				          ] = common_function_attr_battery_get,
    [COMMON_FUNCTION_ATTR_TYPE_VOL 					          ] = common_function_attr_vol_get,
    [COMMON_FUNCTION_ATTR_TYPE_DEV_INFO 			          ] = common_function_attr_dev_info_get,
    [COMMON_FUNCTION_ATTR_TYPE_ERROR_STATS			          ] = NULL,
    [COMMON_FUNCTION_ATTR_TYPE_EQ_INFO				          ] = common_function_attr_eq_param_get,
    [COMMON_FUNCTION_ATTR_TYPE_BS_FILE_TYPE			          ] = common_function_attr_bs_file_type,
    [COMMON_FUNCTION_ATTR_TYPE_FUNCTION_MODE		          ] = common_function_attr_function_mode_get,
    [COMMON_FUNCTION_ATTR_TYPE_FMTX_FREQ			          ] = common_function_attr_fmtx_freq_get,
    [COMMON_FUNCTION_ATTR_TYPE_BT_EMITTER_SW		          ] = common_function_attr_bt_emitter_sw_get,
    [COMMON_FUNCTION_ATTR_TYPE_BT_EMITTER_CONNECT_STATES      ] = common_function_attr_bt_emitter_connect_state_get,
    [COMMON_FUNCTION_ATTR_TYPE_HIGH_LOW_SET			          ] = common_function_attr_high_low_get,
    [COMMON_FUNCTION_ATTR_TYPE_PRE_FETCH_ALL_EQ_INFO 	      ] = common_function_attr_pre_fetch_all_eq_info_get,
    [COMMON_FUNCTION_ATTR_TYPE_PHONE_SCO_STATE_INFO 	      ] = common_function_attr_phone_sco_state_info_get,
    [COMMON_FUNCTION_ATTR_TYPE_MISC_SETTING_INFO 	      	  ] = common_function_attr_misc_setting_info_get,
    [COMMON_FUNCTION_ATTR_TYPE_COLOR_LED_SETTING_INFO 		  ] = common_function_attr_color_led_setting_info_get,
    [COMMON_FUNCTION_ATTR_TYPE_PRE_FETCH_KARAOKE_EQ_INFO	  ] = common_function_attr_karaoke_pre_fetch_eq_setting_info_get,
    [COMMON_FUNCTION_ATTR_TYPE_KARAOKE_EQ_SETTING_INFO 		  ] = common_function_attr_karaoke_eq_setting_info_get,
    [COMMON_FUNCTION_ATTR_TYPE_KARAOKE_SETTING_INFO 		  ] = common_function_attr_karaoke_setting_info_get,
};



static bool smartbox_common_function_set(void *priv, u8 *data, u16 len)
{
    printf("smartbox_common_function_set\n");
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return false;
    }
    put_buf(data, len);
    attr_set(priv, data, len, common_function_set_tab, COMMON_FUNCTION_ATTR_TYPE_MAX);
    if (smart->err_code) {
        smart->err_code = 0;
        return false;
    }
    return true;
}

static u32 smartbox_common_function_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    printf("smartbox_common_function_get, mask = %x\n", mask);
    return attr_get(priv, buf, buf_size, target_common_function_get_tab, COMMON_FUNCTION_ATTR_TYPE_MAX, mask);
}

bool smartbox_function_set(void *priv, u8 function, u8 *data, u16 len)
{
    if (function >= FUNCTION_MASK_MAX) {
        if (function == COMMON_FUNCTION) {
            return smartbox_common_function_set(priv, data, len);
        } else {
            return false;
        }
    }

    func_set func = set_tab[function];
    if (func) {
        return func(priv, data, len);
    }

    return false;
}

u32 smartbox_function_get(void *priv, u8 function, u8 *data, u16 len, u8 *buf, u16 buf_size)
{
    ASSERT_SET_LEN(len, 4);
    u32 mask = READ_BIG_U32(data);
    if (function >= FUNCTION_MASK_MAX) {
        if (function == COMMON_FUNCTION) {
            return smartbox_common_function_get(priv, buf, buf_size, mask);
        } else {
            return 0;
        }
    }

    func_get func = get_tab[function];
    if (func) {
        return func(priv, buf, buf_size, mask);
    }
    return 0;
}

bool smartbox_function_cmd_set(void *priv, u8 function, u8 *data, u16 len)
{
    if (COMMON_FUNCTION == function) {
        // 模式切换
        smartbox_msg_post(USER_MSG_SMARTBOX_MODE_SWITCH, 2, (int)priv, (int)data[0]);
        return true;
    }
    return smartbox_function_set(priv, function, data, len);
}

void smartbox_function_update(u8 function, u32 mask)
{
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL || 0 == JL_rcsp_get_auth_flag()) {
        return ;
    }
    u32 rlen = 0;
    u8 *buf = zalloc(FUNCTION_UPDATE_MAX_LEN);
    if (buf == NULL) {
        printf("no ram err!!\n");
        return ;
    }
    buf[0] = function;
    if (function >= FUNCTION_MASK_MAX) {
        if (function == COMMON_FUNCTION) {
            rlen = attr_get((void *)smart, buf + 1, FUNCTION_UPDATE_MAX_LEN - 1, target_common_function_get_tab, COMMON_FUNCTION_ATTR_TYPE_MAX, mask);
        }
    } else {
        func_get func = get_tab[function];
        if (func) {
            rlen = func(smart, buf + 1, FUNCTION_UPDATE_MAX_LEN - 1, mask);
        }
    }
    if (rlen) {
        JL_CMD_send(JL_OPCODE_SYS_INFO_AUTO_UPDATE, buf, rlen + 1, JL_NOT_NEED_RESPOND);
    }
    free(buf);
}

void smartbox_update_bt_emitter_connect_state(u8 state, u8 *addr)
{
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL) {
        return ;
    }
    smart->emitter_bt_state = state;
    memcpy(smart->emitter_con_addr, addr, 6);
    smartbox_function_update(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_BT_EMITTER_CONNECT_STATES));
}

void smartbox_update_dev_state(u32 event)
{
    switch (event) {
    case DEVICE_EVENT_FROM_ALM:
        printf("DEVICE_EVENT_FROM_ALM\n");
        extern void smartbot_rtc_msg_deal(int msg);
        smartbot_rtc_msg_deal(-1);
        break;
    case DEVICE_EVENT_FROM_LINEIN:
        smartbox_function_update(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_DEV_INFO));
        break;
    }
}

u8 get_cur_mode(u8 app_mode)
{
    u8 current_mode;
    switch (app_mode) {
    case APP_MUSIC_TASK:
        current_mode = MUSIC_FUNCTION;
        break;
    case APP_RTC_TASK:
        current_mode = RTC_FUNCTION;
        break;
    case APP_LINEIN_TASK:
        current_mode = LINEIN_FUNCTION;
        break;
    case APP_FM_TASK:
        current_mode = FM_FUNCTION;
        break;
    case APP_BT_TASK:
    default:
        current_mode = BT_FUNCTION;
        break;
    }
    return current_mode;
}

void function_change_inform(u8 app_mode, u8 ret)
{
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL) {
        return ;
    }
    if (LINEIN_FUNCTION == smart->cur_app_mode) {
        smartbox_function_update(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_DEV_INFO));
    }
    smart->cur_app_mode = get_cur_mode(app_mode);
    smartbox_function_update(COMMON_FUNCTION, BIT(COMMON_FUNCTION_ATTR_TYPE_FUNCTION_MODE));
}

void smartbox_function_setting_stop(void)
{
    struct smartbox *smart = smartbox_handle_get();
    if (smart == NULL) {
        return ;
    }
    switch (smart->cur_app_mode) {
    case BT_FUNCTION:
        printf("BT_FUNCTION STOP\n");
#if TCFG_APP_BT_EN
        // 蓝牙模式是否播放
#endif
        break;
    case MUSIC_FUNCTION:
        printf("MUSIC_FUNCTION STOP\n");
#if TCFG_APP_MUSIC_EN
        // 音乐模式是否播放
        extern void music_func_stop(void);
        music_func_stop();
#endif
        break;
    case RTC_FUNCTION:
        printf("RTC_FUNCTION STOP\n");
#if TCFG_APP_RTC_EN
        // RTC模式是否播放
        extern void rtc_func_stop(void);
        rtc_func_stop();
#endif
        break;
    case LINEIN_FUNCTION:
        printf("LINEIN_FUNCTION STOP\n");
#if TCFG_APP_LINEIN_EN
        // linein模式是否播放
        extern void linein_func_stop(void);
        linein_func_stop();
#endif
        break;
    case FM_FUNCTION:
        printf("FM_FUNCTION STOP\n");
#if TCFG_APP_FM_EN
        // fm模式是否播放
        extern void fm_func_stop(void);
        fm_func_stop();
#endif
        break;
    case FMTX_FUNCTION:
        break;
    }
    // 调用各种设置的release函数
    smartbox_opt_release();
}

#endif
