#include "app_config.h"
#include "user_cfg.h"
#include "fs.h"
#include "string.h"
#include "system/includes.h"
#include "vm.h"
#include "btcontroller_config.h"
#include "app_main.h"
#include "media/includes.h"
#include "audio_config.h"
#include "asm/pwm_led.h"
#include "aec_user.h"
#include "app_power_manage.h"


#define LOG_TAG_CONST       USER_CFG
#define LOG_TAG             "[USER_CFG]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

void lp_winsize_init(struct lp_ws_t *lp);
void bt_max_pwr_set(u8 pwr, u8 pg_pwr, u8 iq_pwr, u8 ble_pwr);
void app_set_sys_vol(s16 vol_l, s16  vol_r);

BT_CONFIG bt_cfg = {
    .edr_name        = {'J', 'L', '-', 'B', 'R', '2', '2'},
    .mac_addr        = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    .tws_local_addr  = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    .rf_power        = 10,
    .dac_analog_gain = 25,
    .mic_analog_gain = 7,
    .tws_device_indicate = 0x6688,
};

AUDIO_CONFIG audio_cfg = {
    .max_sys_vol    = SYS_MAX_VOL,
    .default_vol    = SYS_DEFAULT_VOL,
    .tone_vol       = SYS_DEFAULT_TONE_VOL,
};

//======================================================================================//
//                                 		BTIF配置项表                               		//
//	参数1: 配置项名字                                			   						//
//	参数2: 配置项需要多少个byte存储														//
//	说明: 配置项ID注册到该表后该配置项将读写于BTIF区域, 其它没有注册到该表       		//
//		  的配置项则默认读写于VM区域.													//
//======================================================================================//
const struct btif_item btif_table[] = {
// 	 	item id 		   	   len   	//
    {CFG_BT_MAC_ADDR, 			6 },
    {CFG_BT_FRE_OFFSET,   		6 },   //测试盒矫正频偏值
    //{CFG_DAC_DTB,   			2 },
    //{CFG_MC_BIAS,   			1 },
    {0, 						0 },   //reserved cfg
};

//============================= VM 区域空间最大值 ======================================//
const int vm_max_size_config = VM_MAX_SIZE_CONFIG; //该宏在app_cfg中配置
//======================================================================================//

struct lp_ws_t lp_winsize = {
    .lrc_ws_inc = 480,      //260
    .lrc_ws_init = 160,
    .bt_osc_ws_inc = 100,
    .bt_osc_ws_init = 140,
    .osc_change_mode = 0,
};

#if CONFIG_TWS_PAIR_BY_BOTH_SIDES
static u8 tws_pair_by_both_sides_en = 0;

void bt_set_pair_code_en(u8 en)
{
    tws_pair_by_both_sides_en = en;
}
#endif

u16 bt_get_tws_device_indicate(u8 *tws_device_indicate)
{
#if CONFIG_TWS_PAIR_BY_BOTH_SIDES
    if (tws_pair_by_both_sides_en) {
        return bt_cfg.tws_device_indicate;
    } else {
        return bt_cfg.tws_device_indicate + 1;
    }
#else
    return bt_cfg.tws_device_indicate;
#endif
}

const u8 *bt_get_mac_addr()
{
    return bt_cfg.mac_addr;
}

void bt_update_mac_addr(u8 *addr)
{
    memcpy(bt_cfg.mac_addr, addr, 6);
}

static u8 bt_mac_addr_for_testbox[6] = {0};
void bt_get_vm_mac_addr(u8 *addr)
{
#if 0
    //中断不能调用syscfg_read;
    int ret = 0;

    ret = syscfg_read(CFG_BT_MAC_ADDR, addr, 6);
    if ((ret != 6)) {
        syscfg_write(CFG_BT_MAC_ADDR, addr, 6);
    }
#else

    memcpy(addr, bt_mac_addr_for_testbox, 6);
#endif
}

void bt_get_tws_local_addr(u8 *addr)
{
    memcpy(addr, bt_cfg.tws_local_addr, 6);
}

const char *bt_get_local_name()
{
    return (const char *)(bt_cfg.edr_name);
}

const char *bt_get_pin_code()
{
    return "0000";
}


void bt_reset_and_get_mac_addr(u8 *addr)
{
    get_random_number(addr, 6);
    syscfg_write(CFG_BT_MAC_ADDR, addr, 6);
    memcpy(bt_cfg.mac_addr, addr, 6);
}




extern STATUS_CONFIG status_config;
extern struct charge_platform_data charge_data;
/* extern struct dac_platform_data dac_data; */
extern struct adkey_platform_data adkey_data;
extern struct led_platform_data pwm_led_data;
extern struct adc_platform_data adc_data;

#include "fm_emitter/fm_emitter_manage.h"

u8 get_max_sys_vol(void)
{
#if TCFG_APP_FM_EMITTER_EN
    return FM_EMITTER_MAX_VOL;
#else
    return (audio_cfg.max_sys_vol);
#endif
}

#if 1
u8 get_tone_vol(void)
{
    if (!audio_cfg.tone_vol) {
        return (get_max_sys_vol());
    }
    if (audio_cfg.tone_vol > get_max_sys_vol()) {
        return (get_max_sys_vol());
    }

    return (audio_cfg.tone_vol);
}
#endif

#define USE_CONFIG_BIN_FILE                  0

#define USE_CONFIG_STATUS_SETTING            1                          //状态设置，包括灯状态和提示音
#define USE_CONFIG_AUDIO_SETTING             USE_CONFIG_BIN_FILE        //音频设置
#define USE_CONFIG_CHARGE_SETTING            USE_CONFIG_BIN_FILE        //充电设置
#define USE_CONFIG_KEY_SETTING               USE_CONFIG_BIN_FILE        //按键消息设置
#define USE_CONFIG_MIC_TYPE_SETTING          USE_CONFIG_BIN_FILE        //MIC类型设置
#define USE_CONFIG_LOWPOWER_V_SETTING        USE_CONFIG_BIN_FILE        //低电提示设置
#define USE_CONFIG_AUTO_OFF_SETTING          USE_CONFIG_BIN_FILE        //自动关机时间设置


__BANK_INIT_ENTRY
void cfg_file_parse(u8 idx)
{
    u8 tmp[128] = {0};
    int ret = 0;

    memset(tmp, 0x00, sizeof(tmp));

    /*************************************************************************/
    /*                      CFG READ IN cfg_tools.bin                        */
    /*************************************************************************/

    //-----------------------------CFG_BT_NAME--------------------------------------//
    ret = syscfg_read(CFG_BT_NAME, tmp, 32);
    if (ret < 0) {
        log_info("read bt name err\n");
    } else if (ret >= LOCAL_NAME_LEN) {
        memset(bt_cfg.edr_name, 0x00, LOCAL_NAME_LEN);
        memcpy(bt_cfg.edr_name, tmp, LOCAL_NAME_LEN);
        bt_cfg.edr_name[LOCAL_NAME_LEN - 1] = 0;
    } else {
        memset(bt_cfg.edr_name, 0x00, LOCAL_NAME_LEN);
        memcpy(bt_cfg.edr_name, tmp, ret);
    }
    /* g_printf("bt name config:%s\n", bt_cfg.edr_name); */
    log_info("bt name config:%s\n", bt_cfg.edr_name);

    //-----------------------------CFG_TWS_PAIR_CODE_ID----------------------------//
    ret = syscfg_read(CFG_TWS_PAIR_CODE_ID, &bt_cfg.tws_device_indicate, 2);
    if (ret < 0) {
        log_debug("read pair code err\n");
        bt_cfg.tws_device_indicate = 0x8888;
    }
    /* g_printf("tws pair code config:\n"); */
    log_info("tws pair code config:\n");
    log_info_hexdump(&bt_cfg.tws_device_indicate, 2);

    //-----------------------------CFG_BT_RF_POWER_ID----------------------------//
    ret = syscfg_read(CFG_BT_RF_POWER_ID, &app_var.rf_power, 1);
    if (ret < 0) {
        log_debug("read rf err\n");
        app_var.rf_power = 10;
    }
#if TCFG_APP_BT_EN
    bt_max_pwr_set(app_var.rf_power, 5, 8, 9);
#endif
    /* g_printf("rf config:%d\n", app_var.rf_power); */
    log_info("rf config:%d\n", app_var.rf_power);

    //-----------------------------CFG_AEC_ID------------------------------------//
    /* g_printf("aec config:\n"); */
    log_info("aec config:\n");
    AEC_CONFIG aec;
    ret = syscfg_read(CFG_AEC_ID, &aec, sizeof(AEC_CONFIG));
    if (ret > 0) {
        log_info_hexdump(&aec, sizeof(AEC_CONFIG));
        app_var.aec_mic_gain = aec.mic_again;
        app_var.aec_dac_gain = aec.dac_again;
#if AEC_READ_CONFIG
#ifdef CONFIG_MEDIA_ORIGIN_ENABLE
        aec_cfg_fill(&aec);
#endif/*CONFIG_MEDIA_ORIGIN_ENABLE*/
#endif/*AEC_READ_CONFIG*/
    } else {
        app_var.aec_mic_gain = 6;
        app_var.aec_dac_gain = 22;
    }
    log_info("aec_cfg mic_gain:%d dac_gain:%d", app_var.aec_mic_gain, app_var.aec_dac_gain);

    //-----------------------------CFG_MIC_TYPE_ID------------------------------------//
#if USE_CONFIG_MIC_TYPE_SETTING
    /* g_printf("mic_type_config:\n"); */
    log_info("mic_type_config:\n");
    MIC_TYPE_CONFIG mic_type;
    ret = syscfg_read(CFG_MIC_TYPE_ID, &mic_type, sizeof(MIC_TYPE_CONFIG));
    if (ret > 0) {
        log_info_hexdump(&mic_type, sizeof(MIC_TYPE_CONFIG));
        adc_data.mic_capless = mic_type.type;
#if ((TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_LR) || (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_DUAL_LR_DIFF))
        if (adc_data.mic_capless == 1) { //四声道与双声道差分 使用不省电容接法mic_capless = 0
            log_error("adc_data.mic_capless err, CHECK !!!\n");
            ASSERT(0);
        }
#endif
        adc_data.mic_bias_res = mic_type.pull_up;
        adc_data.mic_ldo_vsel = mic_type.ldo_lev;
    }
#endif

#if TCFG_MC_BIAS_AUTO_ADJUST
    ret = syscfg_read(CFG_MC_BIAS, &adc_data.mic_bias_res, 1);
    log_info("mic_bias_res:%d\n", adc_data.mic_bias_res);
    if (ret != 1) {
        log_info("mic_bias_adjust NULL\n");
    }
    u8 mic_ldo_idx;
    ret = syscfg_read(CFG_MIC_LDO_VSEL, &mic_ldo_idx, 1);
    if (ret == 1) {
        adc_data.mic_ldo_vsel = mic_ldo_idx & 0x3;
        log_info("mic_ldo_vsel:%d,%d\n", adc_data.mic_ldo_vsel, mic_ldo_idx);
    } else {
        log_info("mic_ldo_vsel_adjust NULL\n");
    }
#endif

#if USE_CONFIG_STATUS_SETTING
    /* g_printf("status_config:\n"); */
    log_info("status_config:\n");
    STATUS_CONFIG *status = (STATUS_CONFIG *)tmp;
    ret = syscfg_read(CFG_UI_TONE_STATUS_ID, status, sizeof(STATUS_CONFIG));
    if (ret > 0) {
        memcpy((u8 *)&status_config, (u8 *)status, sizeof(STATUS_CONFIG));
        log_info_hexdump(&status_config, sizeof(STATUS_CONFIG));
    }
#endif

#if USE_CONFIG_AUDIO_SETTING
    /* g_printf("app audio_config:\n"); */
    log_info("app audio_config:\n");
    ret = syscfg_read(CFG_AUDIO_ID, (u8 *)&audio_cfg, sizeof(AUDIO_CONFIG));
    if (ret > 0) {
        log_info_hexdump((u8 *)&audio_cfg, sizeof(AUDIO_CONFIG));
#else
    {
#endif
        /* dac_data.max_ana_vol = audio_cfg.max_sys_vol; */
        /* if (dac_data.max_ana_vol > 30) { */
        /* dac_data.max_ana_vol = 30; */
        /* } */

        //// pc usb mic gain
        s8 usb_mic_gain = -1;
        ret = syscfg_read(VM_USB_MIC_GAIN, &usb_mic_gain, 1);
        if (ret < 0) {
            usb_mic_gain  = 7;
        }
        app_var.usb_mic_gain = usb_mic_gain;
        log_info("usb_mic_gain: %d\n", app_var.usb_mic_gain);

        s8 default_volume = -1;
        s8 music_volume = -1;
#if SYS_DEFAULT_VOL
        default_volume = audio_cfg.default_vol;
#else
        ret = syscfg_read(CFG_SYS_VOL, &default_volume, 1);
        if (ret < 0) {
            default_volume = -1;
        }
        ret = syscfg_read(CFG_MUSIC_VOL, &music_volume, 1);
        if (ret < 0) {
            music_volume = -1;
        }
#endif
        if (default_volume > audio_cfg.max_sys_vol) {
            default_volume = audio_cfg.max_sys_vol;
        }
        if (default_volume <= 0) {
            default_volume = audio_cfg.max_sys_vol / 2;
        }

#if (SMART_BOX_EN)
        app_var.music_volume = music_volume < 0 ? default_volume : music_volume;
#else
        app_var.music_volume = music_volume <= 0 ? default_volume : music_volume;
#endif
        app_var.wtone_volume = audio_cfg.tone_vol;
        app_var.call_volume = app_var.aec_dac_gain;
        app_var.opid_play_vol_sync = app_var.music_volume * 127 / audio_cfg.max_sys_vol;

        log_info("max vol:%d default vol:%d tone vol:%d vol_sync:%d\n", audio_cfg.max_sys_vol, default_volume, audio_cfg.tone_vol, app_var.opid_play_vol_sync);
    }

#if (USE_CONFIG_CHARGE_SETTING) && (TCFG_CHARGE_ENABLE)
    /* g_printf("app charge config:\n"); */
    log_info("app charge config:\n");
    CHARGE_CONFIG *charge = (CHARGE_CONFIG *)tmp;
    ret = syscfg_read(CFG_CHARGE_ID, charge, sizeof(CHARGE_CONFIG));
    if (ret > 0) {
        log_info_hexdump(charge, sizeof(CHARGE_CONFIG));
        log_info("sw:%d poweron_en:%d full_v:%d full_mA:%d charge_mA:%d\n",
                 charge->sw, charge->poweron_en, charge->full_v, charge->full_c, charge->charge_c);
        memcpy((u8 *)&charge_data, (u8 *)charge, sizeof(CHARGE_CONFIG));
    }
#endif

#if (USE_CONFIG_KEY_SETTING) && (TCFG_ADKEY_ENABLE || TCFG_IOKEY_ENABLE)
    /* g_printf("app key config:\n"); */
    log_info("app key config:\n");
    KEY_OP *key_msg = (KEY_OP *)tmp;
    ret = syscfg_read(CFG_KEY_MSG_ID, key_msg, sizeof(KEY_OP) * KEY_NUM_MAX);
    if (ret > 0) {
        log_info_hexdump(key_msg, sizeof(KEY_OP) * KEY_NUM);
        memcpy(key_table, key_msg, sizeof(KEY_OP) * KEY_NUM);
    }

    log_info("key_msg:");
    log_info_hexdump((u8 *)key_table, KEY_EVENT_MAX * KEY_NUM_MAX);
#endif


#if USE_CONFIG_LOWPOWER_V_SETTING
    /* g_printf("auto low power config:\n"); */
    log_info("auto low power config:\n");
    AUTO_LOWPOWER_V_CONFIG auto_lowpower;
    ret = syscfg_read(CFG_LOWPOWER_V_ID, &auto_lowpower, sizeof(AUTO_LOWPOWER_V_CONFIG));
    if (ret > 0) {
        app_var.warning_tone_v = auto_lowpower.warning_tone_v;
        app_var.poweroff_tone_v = auto_lowpower.poweroff_tone_v;
    }
    log_info("warning_tone_v:%d poweroff_tone_v:%d\n", app_var.warning_tone_v, app_var.poweroff_tone_v);
#else
    app_var.warning_tone_v = LOW_POWER_WARN_VAL;
    app_var.poweroff_tone_v = LOW_POWER_OFF_VAL;
    log_info("warning_tone_v:%d poweroff_tone_v:%d\n", app_var.warning_tone_v, app_var.poweroff_tone_v);
#endif

#if USE_CONFIG_AUTO_OFF_SETTING
    /* g_printf("auto off time config:\n"); */
    log_info("auto off time config:\n");
    AUTO_OFF_TIME_CONFIG auto_off_time;
    ret = syscfg_read(CFG_AUTO_OFF_TIME_ID, &auto_off_time, sizeof(AUTO_OFF_TIME_CONFIG));
    if (ret > 0) {
        app_var.auto_off_time = auto_off_time.auto_off_time * 60;
    }
    log_info("auto_off_time:%d\n", app_var.auto_off_time);
#else
    app_var.auto_off_time =  TCFG_AUTO_SHUT_DOWN_TIME;
    log_info("auto_off_time:%d\n", app_var.auto_off_time);
#endif

#if TCFG_APP_MUSIC_EN
    ret = syscfg_read(CFG_MUSIC_MODE, &app_var.cycle_mode, 1);
    if (ret < 0) {
        log_info("read music play mode err\n");
    }
    if (app_var.cycle_mode >= FCYCLE_MAX || app_var.cycle_mode == 0) {
        app_var.cycle_mode = FCYCLE_ALL;
    }
#endif

    /*************************************************************************/
    /*                      CFG READ IN VM                                   */
    /*************************************************************************/
#if TCFG_APP_BT_EN
    u8 mac_buf[6];
    u8 mac_buf_tmp[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    u8 mac_buf_tmp2[6] = {0, 0, 0, 0, 0, 0};
#if TCFG_USER_TWS_ENABLE
    int len = syscfg_read(CFG_TWS_LOCAL_ADDR, bt_cfg.tws_local_addr, 6);
    if (len != 6) {
        get_random_number(bt_cfg.tws_local_addr, 6);
        syscfg_write(CFG_TWS_LOCAL_ADDR, bt_cfg.tws_local_addr, 6);
        log_info(">>>init mac tws addr!!!\n");
    }
    log_info("tws_local_mac:");
    log_info_hexdump(bt_cfg.tws_local_addr, sizeof(bt_cfg.tws_local_addr));

#if CONFIG_TWS_USE_COMMMON_ADDR
    ret = syscfg_read(CFG_TWS_COMMON_ADDR, mac_buf, 6);
    if (ret != 6 || !memcmp(mac_buf, mac_buf_tmp, 6))
#endif
#endif
        do {
            ret = syscfg_read(CFG_BT_MAC_ADDR, mac_buf, 6);
            if ((ret != 6) || !memcmp(mac_buf, mac_buf_tmp, 6) || !memcmp(mac_buf, mac_buf_tmp2, 6)) {
#if (GMA_EN)
                void gma_bt_mac_addr_get(uint8_t *buf);
                gma_bt_mac_addr_get(mac_buf);
                //printf("gma mac :");
                //put_buf(mac_buf, 6);
#else
                get_random_number(mac_buf, 6);
#endif
                syscfg_write(CFG_BT_MAC_ADDR, mac_buf, 6);
                log_info(">>>init tws addr!!!\n");
            }
        } while (0);


    syscfg_read(CFG_BT_MAC_ADDR, bt_mac_addr_for_testbox, 6);

#if CONFIG_TWS_USE_COMMMON_ADDR
    if (!memcmp(bt_mac_addr_for_testbox, mac_buf_tmp, 6)) {
#if (GMA_EN)
        void gma_bt_mac_addr_get(uint8_t *buf);
        gma_bt_mac_addr_get(bt_mac_addr_for_testbox);
#else
        get_random_number(bt_mac_addr_for_testbox, 6);
#endif
        syscfg_write(CFG_BT_MAC_ADDR, bt_mac_addr_for_testbox, 6);
        log_info(">>>init mac addr!!!\n");
    }
#endif

#if (GMA_EN)
#if (GMA_USED_FIXED_TRI_PARA)
    int gma_select_ali_para_by_mac(u8 * mac);
    if (gma_select_ali_para_by_mac(mac_buf) != 0) {
        syscfg_read(CFG_BT_MAC_ADDR, mac_buf, 6);
    }

#else
    ///sync mac
    syscfg_read(CFG_BT_MAC_ADDR, mac_buf, 6);
#endif
#endif

    log_info("mac:");
    log_info_hexdump(mac_buf, sizeof(mac_buf));
    memcpy(bt_cfg.mac_addr, mac_buf, 6);

#if (CONFIG_BT_MODE != BT_NORMAL)
    const u8 dut_name[]  = "AC693x_DUT";
    const u8 dut_addr[6] = {0x12, 0x34, 0x56, 0x56, 0x34, 0x12};
    memcpy(bt_cfg.edr_name, dut_name, sizeof(dut_name));
    memcpy(bt_cfg.mac_addr, dut_addr, 6);
#endif

    /*************************************************************************/
    /*                      CFG READ IN isd_config.ini                       */
    /*************************************************************************/
    LRC_CONFIG lrc_cfg;
    ret = syscfg_read(CFG_LRC_ID, &lrc_cfg, sizeof(LRC_CONFIG));
    if (ret > 0) {
        log_info("lrc cfg:");
        log_info_hexdump(&lrc_cfg, sizeof(LRC_CONFIG));
        lp_winsize.lrc_ws_inc      = lrc_cfg.lrc_ws_inc;
        lp_winsize.lrc_ws_init     = lrc_cfg.lrc_ws_init;
        lp_winsize.bt_osc_ws_inc   = lrc_cfg.btosc_ws_inc;
        lp_winsize.bt_osc_ws_init  = lrc_cfg.btosc_ws_init;
        lp_winsize.osc_change_mode = lrc_cfg.lrc_change_mode;
    }
    /* printf("%d %d %d \n",lp_winsize.lrc_ws_inc,lp_winsize.lrc_ws_init,lp_winsize.osc_change_mode); */
    lp_winsize_init(&lp_winsize);
#endif
}

extern void hci_vendor_update_name(void);
int bt_modify_name(u8 *new_name)
{
    u8 new_len = strlen(new_name);

    if (new_len >= LOCAL_NAME_LEN) {
        new_name[LOCAL_NAME_LEN - 1] = 0;
    }

    if (strcmp(new_name, bt_cfg.edr_name)) {
        syscfg_write(CFG_BT_NAME, new_name, LOCAL_NAME_LEN);
        memcpy(bt_cfg.edr_name, new_name, LOCAL_NAME_LEN);
        hci_vendor_update_name();
        log_info("mdy_name sucess\n");
        return 1;
    }
    return 0;
}


char *get_edr_name(void)
{
    return bt_cfg.edr_name;
}






