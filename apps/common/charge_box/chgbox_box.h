#ifndef _CHGBOX_BOX_H_
#define _CHGBOX_BOX_H_

#include "typedef.h"
#include "system/event.h"
#include "device/chargebox.h"

enum {
    CHGBOX_MSG_SEND_POWER_OPEN,
    CHGBOX_MSG_SEND_POWER_CLOSE,
    CHGBOX_MSG_SEND_CLOSE_LID,
    CHGBOX_MSG_SEND_SHUTDOWN,
    CHGBOX_MSG_SEND_PAIR,
};

enum {
    CHGBOX_EVENT_200MS,
    CHGBOX_EVENT_500MS,
    CHGBOX_EVENT_USB_IN,
    CHGBOX_EVENT_USB_OUT,
    CHGBOX_EVENT_OPEN_LID,
    CHGBOX_EVENT_CLOSE_LID,
    CHGBOX_EVENT_EAR_L_ONLINE,
    CHGBOX_EVENT_EAR_L_OFFLINE,
    CHGBOX_EVENT_EAR_R_ONLINE,
    CHGBOX_EVENT_EAR_R_OFFLINE,
    CHGBOX_EVENT_ENTER_LOWPOWER,
    CHGBOX_EVENT_EXIT_LOWPOWER,
    CHGBOX_EVENT_EAR_FULL,
    CHGBOX_EVENT_LOCAL_FULL,
    CHGBOX_EVENT_NEED_SHUTDOWN,
    CHGBOX_EVENT_WL_DATA_OVER,
    CHGBOX_EVENT_WIRELESS_ONLINE,
    CHGBOX_EVENT_WIRELESS_OFFLINE,
    CHGBOX_EVENT_ENTER_CURRENT_PROTECT,
    CHGBOX_EVENT_EXIT_CURRENT_PROTECT,
    CHGBOX_EVENT_ENTER_TEMP_PROTECT,
    CHGBOX_EVENT_EXIT_TEMP_PROTECT,
    CHGBOX_EVENT_HANDSHAKE_OK,
};

#if(TCFG_CHARGE_BOX_ENABLE)
extern void chargebox_init(const struct chargebox_platform_data *data);
extern void chargebox_open(u8 l_r, u8 mode);
extern void chargebox_close(u8 l_r);
extern u8 chargebox_write(u8 l_r, u8 *data, u8 len);
extern void chargebox_set_baud(u8 l_r, u32 baudrate);

#define CHARGEBOX_PLATFORM_DATA_BEGIN(data)\
    static const struct chargebox_platform_data data = {

#define CHARGEBOX_PLATFORM_DATA_END()\
        .baudrate               = 9600,\
        .init                   = chargebox_init,\
        .open                   = chargebox_open,\
        .close                  = chargebox_close,\
        .write                  = chargebox_write,\
        .set_baud               = chargebox_set_baud,\
    };
#else
#define CHARGEBOX_PLATFORM_DATA_BEGIN(data)\
    static const struct chargebox_platform_data data = {

#define CHARGEBOX_PLATFORM_DATA_END()\
        .baudrate               = 9600,\
        .init                   = NULL,\
        .open                   = NULL,\
        .close                  = NULL,\
        .write                  = NULL,\
        .set_baud               = NULL,\
    };
#endif

extern void app_chargebox_send_mag(int msg);
extern void app_chargebox_event_to_user(u8 event);
extern u8 chgbox_adv_addr_scan(void);
#endif    //_APP_CHARGEBOX_H_

