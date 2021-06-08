#include "smartbox/config.h"
#include "smartbox/feature.h"
#include "btstack/avctp_user.h"
#include "smartbox/event.h"
#include "custom_cfg.h"
#include "JL_rcsp_packet.h"

#if (SMART_BOX_EN)

#pragma pack(1)
struct _SYS_info {
    u8 bat_lev;
    u8 sys_vol;
    u8 max_vol;
    u8 vol_is_sync;
};

struct _EDR_info {
    u8 addr_buf[6];
    u8 profile;
    u8 state;
};

struct _MUSIC_STATUS_info {
    u8 status;
    u32 cur_time;
    u32 total_time;
    u8 cur_dev;
};

struct _dev_version {
    u16 _sw_ver2: 4; //software l version
    u16 _sw_ver1: 4; //software m version
    u16 _sw_ver0: 4; //software h version
    u16 _hw_ver:  4; //hardware version
};
#pragma pack()

static u32 target_feature_attr_protocol_version(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 ver = get_rcsp_version();
    rlen = add_one_attr(buf, buf_size, offset, attr, &ver, 1);
    return rlen;
}
static u32 target_feature_attr_sys_info(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return 0;
    }
    struct _SYS_info sys_info = {0};
    extern void smartbox_get_cur_dev_vol_info(u8 * vol_info);
    extern void smartbox_get_max_vol_info(u8 * vol_info);
    extern u8 get_vbat_percent(void);
    sys_info.bat_lev = get_vbat_percent(); //get_battery_level() / 10;
    smartbox_get_max_vol_info(&sys_info.max_vol);
    smartbox_get_cur_dev_vol_info(&sys_info.sys_vol);
#if BT_SUPPORT_MUSIC_VOL_SYNC
    if (get_remote_vol_sync() || (1 == smart->A_platform)) {
        sys_info.vol_is_sync |= BIT(0);
        smart->dev_vol_sync = 1;
    }
#endif
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&sys_info, sizeof(sys_info));
    return rlen;
}
static u32 target_feature_attr_edr_addr(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    struct _EDR_info edr_info;
    extern const u8 *bt_get_mac_addr();
    u8 taddr_buf[6];
    memcpy(taddr_buf, bt_get_mac_addr(), 6);
    edr_info.addr_buf[0] =  taddr_buf[5];
    edr_info.addr_buf[1] =  taddr_buf[4];
    edr_info.addr_buf[2] =  taddr_buf[3];
    edr_info.addr_buf[3] =  taddr_buf[2];
    edr_info.addr_buf[4] =  taddr_buf[1];
    edr_info.addr_buf[5] =  taddr_buf[0];
    edr_info.profile = 0x0E;
    if (get_defalut_bt_channel_sel()) {
        edr_info.profile |= BIT(7);
    } else {
        edr_info.profile &= ~BIT(7);
    }
    extern u8 get_bt_connect_status(void);
    if (get_bt_connect_status() ==  BT_STATUS_WAITINT_CONN) {
        edr_info.state = 0;
    } else {
        edr_info.state = 1;
    }
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&edr_info, sizeof(struct _EDR_info));
    return rlen;
}
static u32 target_feature_attr_platform(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    return rlen;
}
static u32 target_feature_function_info(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return 0;
    }
    extern u8 get_cur_mode(u8 app_mode);

    u32 rlen = 0;
    u8 tmp_buf[6] = {0};
    u32 func_mask = app_htonl(smart->function_mask);
    u8 cur_fun = get_cur_mode(app_get_curr_task());
    memcpy(tmp_buf, (u8 *)&func_mask, 4);
    memcpy(tmp_buf + 4, &cur_fun, 1);
    memcpy(tmp_buf + 5, &smart->music_icon_mask, 1);
    rlen = add_one_attr(buf, buf_size, offset, attr, tmp_buf, sizeof(tmp_buf));
    return rlen;
}
static u32 target_feature_dev_version(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u16 ver = get_vid_pid_ver_from_cfg_file(GET_VER_FROM_EX_CFG);
    ver = READ_BIG_U16(&ver);
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&ver, sizeof(ver));
    return rlen;
}
static u32 target_feature_sdk_type(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return 0;
    }
    u32 rlen = 0;
    u8 sdk_type = smart->sdk_type;
    rlen = add_one_attr(buf, buf_size, offset, attr, &sdk_type, 1);
    return rlen;
}
static u32 target_feature_uboot_version(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 *uboot_ver_flag = (u8 *)(0x1C000 - 0x8);
    u8 uboot_version[2] = {uboot_ver_flag[0], uboot_ver_flag[1]};
    rlen = add_one_attr(buf, buf_size, offset, attr, uboot_version, sizeof(uboot_version));
    return rlen;
}
static u32 target_feature_ota_double_parition(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return 0;
    }
    u32 rlen = 0;
    u8 double_partition_value;
    u8 ota_loader_need_download_flag;
    u8 update_channel_sel;

    if (smart->ota_type) {
        double_partition_value = 0x1;
        ota_loader_need_download_flag = 0x00;
    } else {
        double_partition_value = 0x0;
        ota_loader_need_download_flag = 0x01;
    }
    update_channel_sel = 0x1;       //强制使用BLE升级
    u8 update_param[3] = {
        double_partition_value,
        ota_loader_need_download_flag,
        update_channel_sel,
    };

    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)update_param, sizeof(update_param));
    return rlen;
}
static u32 target_feature_ota_update_status(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 update_status_value = 0x0;
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&update_status_value, sizeof(update_status_value));
    return rlen;
}
static u32 target_feature_pid_vid(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u16 pvid[2] = {0};
    pvid[0] =  get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);
    pvid[1] =  get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
    pvid[0] = READ_BIG_U16(&pvid[0]);
    pvid[1] = READ_BIG_U16(&pvid[1]);
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&pvid, sizeof(pvid));
    return rlen;
}
static u32 target_feature_authkey(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if VER_INFO_EXT_COUNT
    u8 authkey_len = 0;
    u8 *local_authkey_data = NULL;
    get_authkey_procode_from_cfg_file(&local_authkey_data, &authkey_len, GET_AUTH_KEY_FROM_EX_CFG);
    if (local_authkey_data && authkey_len) {
        put_buf(local_authkey_data, authkey_len);
        rlen = add_one_attr(buf, buf_size, offset, attr, local_authkey_data, authkey_len);
    }
#endif
    return rlen;
}
static u32 target_feature_procode(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if VER_INFO_EXT_COUNT
    u8 procode_len = 0;
    u8 *local_procode_data = NULL;
    get_authkey_procode_from_cfg_file(&local_procode_data, &procode_len, GET_PRO_CODE_FROM_EX_CFG);
    if (local_procode_data && procode_len) {
        put_buf(local_procode_data, procode_len);
        rlen = add_one_attr(buf, buf_size, offset, attr, local_procode_data, procode_len);
    }
#endif
    return rlen;
}
static u32 target_feature_mtu(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u16 rx_max_mtu = JL_packet_get_rx_max_mtu();
    u16 tx_max_mtu = JL_packet_get_tx_max_mtu();
    u8 t_buf[4];
    t_buf[0] = (tx_max_mtu >> 8) & 0xFF;
    t_buf[1] = tx_max_mtu & 0xFF;
    t_buf[2] = (rx_max_mtu >> 8) & 0xFF;
    t_buf[3] = rx_max_mtu & 0xFF;
    rlen = add_one_attr(buf, buf_size, offset,  attr, t_buf, 4);
    return rlen;
}
static u32 target_feature_ble_only(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    /* u8 flag = 1; */

    extern void lib_make_ble_address(u8 * ble_address, u8 * edr_address);
    extern const u8 *bt_get_mac_addr();
    u8 taddr_buf[7];
    taddr_buf[0] = 0;
    lib_make_ble_address(taddr_buf + 1, (void *)bt_get_mac_addr());
    for (u8 i = 0; i < (6 / 2); i++) {
        taddr_buf[i + 1] ^= taddr_buf[7 - i - 1];
        taddr_buf[7 - i - 1] ^= taddr_buf[i + 1];
        taddr_buf[i + 1] ^= taddr_buf[7 - i - 1];
    }

    /* rlen = add_one_attr(buf, buf_size, offset,  attr, &flag, 1); */
    rlen = add_one_attr(buf, buf_size, offset,  attr, taddr_buf, sizeof(taddr_buf));
    return rlen;
}
static u32 target_feature_bt_emitter_info(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return 0;
    }

    u32 rlen = 0;
    u8 val = 0;
    val |= smart->emitter_en;
    val |= smart->emitter_sw;
    printf("val = %d, emitter_en = %d, emitter_sw = %d\n", val, smart->emitter_en, smart->emitter_sw);
    rlen = add_one_attr(buf, buf_size, offset,  attr, &val, 1);
    return rlen;
}

static u32 target_feature_md5_game_support(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return 0;
    }

    u32 rlen = 0;
    u8 md5_support = 0;
    if (md5_support) {
        md5_support = UPDATE_MD5_ENABLE;
    }
    /* if(get_work_setting() == 2) */
    if (smart->game_mode_en) {
        md5_support |= BIT(1);
    }
    if (smart->find_dev_en) {
        md5_support |= BIT(2);
    }
    if (smart->karaoke_en) {
        md5_support |= BIT(3);
    }
    if (smart->sound_effects_disable) {
        md5_support |= BIT(4);
    }
    if (md5_support) {
        rlen = add_one_attr(buf, buf_size, offset,  attr, &md5_support, 1);
    }

    return rlen;
}


enum {
    FEATURE_ATTR_TYPE_PROTOCOL_VERSION = 0,
    FEATURE_ATTR_TYPE_SYS_INFO         = 1,
    FEATURE_ATTR_TYPE_EDR_ADDR         = 2,
    FEATURE_ATTR_TYPE_PLATFORM         = 3,
    FEATURE_ATTR_TYPE_FUNCTION_INFO    = 4,
    FEATURE_ATTR_TYPE_DEV_VERSION      = 5,
    FEATURE_ATTR_TYPE_SDK_TYPE         = 6,
    FEATURE_ATTR_TYPE_UBOOT_VERSION    = 7,
    FEATURE_ATTR_TYPE_DOUBLE_PARITION  = 8,
    FEATURE_ATTR_TYPE_UPDATE_STATUS    = 9,
    FEATURE_ATTR_TYPE_DEV_VID_PID      = 10,
    FEATURE_ATTR_TYPE_DEV_AUTHKEY      = 11,
    FEATURE_ATTR_TYPE_DEV_PROCODE      = 12,
    FEATURE_ATTR_TYPE_DEV_MAX_MTU      = 13,
    FEATURE_ATTR_TYPE_CONNECT_BLE_ONLY = 17,
    FEATURE_ATTR_TYPE_BT_EMITTER_INFO  = 18,
    FEATURE_ATTR_TYPE_MD5_GAME_SUPPORT = 19,
    FEATURE_ATTR_TYPE_MAX,
};

static const attr_get_func target_feature_mask_get_tab[FEATURE_ATTR_TYPE_MAX] = {
    [FEATURE_ATTR_TYPE_PROTOCOL_VERSION  ] = target_feature_attr_protocol_version,
    [FEATURE_ATTR_TYPE_SYS_INFO          ] = target_feature_attr_sys_info,
    [FEATURE_ATTR_TYPE_EDR_ADDR          ] = target_feature_attr_edr_addr,
    [FEATURE_ATTR_TYPE_PLATFORM          ] = target_feature_attr_platform,
    [FEATURE_ATTR_TYPE_FUNCTION_INFO     ] = target_feature_function_info,
    [FEATURE_ATTR_TYPE_DEV_VERSION       ] = target_feature_dev_version,
    [FEATURE_ATTR_TYPE_SDK_TYPE          ] = target_feature_sdk_type,
    [FEATURE_ATTR_TYPE_UBOOT_VERSION     ] = target_feature_uboot_version,
    [FEATURE_ATTR_TYPE_DOUBLE_PARITION   ] = target_feature_ota_double_parition,
    [FEATURE_ATTR_TYPE_UPDATE_STATUS     ] = target_feature_ota_update_status,
    [FEATURE_ATTR_TYPE_DEV_VID_PID       ] = target_feature_pid_vid,
    [FEATURE_ATTR_TYPE_DEV_AUTHKEY       ] = target_feature_authkey,
    [FEATURE_ATTR_TYPE_DEV_PROCODE       ] = target_feature_procode,
    [FEATURE_ATTR_TYPE_DEV_MAX_MTU       ] = target_feature_mtu,
    [FEATURE_ATTR_TYPE_CONNECT_BLE_ONLY  ] = target_feature_ble_only,
    [FEATURE_ATTR_TYPE_BT_EMITTER_INFO   ] = target_feature_bt_emitter_info,
    [FEATURE_ATTR_TYPE_MD5_GAME_SUPPORT  ] = target_feature_md5_game_support,
};

u32 target_feature_parse_packet(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    printf("target_feature_parse_packet, mask = %x\n", mask);
    return attr_get(priv, buf, buf_size, target_feature_mask_get_tab, FEATURE_ATTR_TYPE_MAX, mask);
}
#endif//SMART_BOX_EN


