#include "smartbox/config.h"
#include "le_common.h"
#include "smartbox/event.h"
#include "le_smartbox_adv.h"
#include "le_smartbox_module.h"
#include "custom_cfg.h"
#include "smartbox_adv_bluetooth.h"

#if (SMART_BOX_EN)

#define RCSP_DEBUG_EN
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                putchar(x)
#define rcsp_printf                    printf
#define rcsp_printf_buf(x,len)         put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_printf_buf(...)
#endif

#ifndef JL_SMART_BOX_CUSTOM_APP_EN
#define RCSP_USE_BLE      0
#define RCSP_USE_SPP      1
#define RCSP_CHANNEL_SEL  RCSP_USE_SPP
#endif




u8 get_defalut_bt_channel_sel(void)
{
#if (RCSP_CHANNEL_SEL)
    return RCSP_CHANNEL_SEL;
#else
    return 0;
#endif
}



void smartbox_spp_setting(void)
{
    bt_3th_dev_type_spp();
    set_connect_flag(2);
    bt_ble_adv_ioctl(BT_ADV_SET_NOTIFY_EN, 1, 1);
}

void smartbox_config(struct smartbox *smart)
{
    if (smart == NULL) {
        return ;
    }

    ///配置app支持功能展示, 跟进具体方案进行配置
#if TCFG_APP_BT_EN
    smart->function_mask |= BIT(BT_FUNCTION_MASK);
#endif
#if TCFG_APP_MUSIC_EN
    smart->function_mask |= BIT(MUSIC_FUNCTION_MASK);
#endif
#if (TCFG_APP_LINEIN_EN && !SOUNDCARD_ENABLE)
    smart->function_mask |= BIT(LINEIN_FUNCTION_MASK);
#endif
#if TCFG_APP_RTC_EN
    smart->function_mask |= BIT(RTC_FUNCTION_MASK);
#endif
#if TCFG_APP_FM_EN
    smart->function_mask |= BIT(FM_FUNCTION_MASK);
#endif
#if TCFG_COLORLED_ENABLE
    smart->function_mask |= BIT(COLOR_LED_MASK);
#endif

#if (TCFG_APP_MUSIC_EN)
    smart->music_icon_mask = DEV_ICON_ALL_DISPLAY
                             | BIT(USB_ICON_DISPLAY)
                             | BIT(SD0_ICON_DISPLAY)
                             | BIT(SD1_ICON_DISPLAY)
                             ;
#endif

#if (RCSP_SDK_TYPE)
    ///sdk类型
    smart->sdk_type = RCSP_SDK_TYPE;
#endif

    ///OTA升级类型
    extern const int support_dual_bank_update_en;
    smart->ota_type = support_dual_bank_update_en;

#if (BT_CONNECTION_VERIFY)
    ///是否需要握手配置	0-校验，1-不校验
    smart->auth_check = BT_CONNECTION_VERIFY;
    JL_rcsp_set_auth_flag(smart->auth_check);
#endif

    ///是否使能发射器功能配置
    smart->emitter_en = 0;

#if (RCSP_ADV_FIND_DEVICE_ENABLE)
    ///是否支持查找设备
    smart->find_dev_en = RCSP_ADV_FIND_DEVICE_ENABLE;
#endif
    ///是否支持游戏模式
    smart->game_mode_en = 0;
    ///是否支持md5升级
    smart->md5_support = UPDATE_MD5_ENABLE;
    ///是否支持卡拉ok功能
    smart->karaoke_en = RCSP_ADV_KARAOKE_SET_ENABLE;

#ifdef RCSP_SOUND_EFFECT_FUNC_DISABLE
    ///是否屏蔽声效
    smart->sound_effects_disable = RCSP_SOUND_EFFECT_FUNC_DISABLE;
#endif

    bt_3th_type_dev_select(RCSP_BLE);
    bt_3th_set_ble_callback_priv(smart);
    bt_3th_set_spp_callback_priv(smart);
}


#endif//SMART_BOX_EN

