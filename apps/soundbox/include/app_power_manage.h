#ifndef APP_POWER_MANAGE_H
#define APP_POWER_MANAGE_H

#include "typedef.h"
#include "system/event.h"

#define LOW_POWER_SHUTDOWN      320  //低电直接关机电压
#define LOW_POWER_OFF_VAL   	330  //低电关机电压
#define LOW_POWER_WARN_VAL   	340  //低电提醒电压
#define LOW_POWER_WARN_TIME   	(60 * 1000)  //低电提醒时间


enum {
    POWER_EVENT_POWER_NORMAL,
    POWER_EVENT_POWER_WARNING,
    POWER_EVENT_POWER_LOW,
    POWER_EVENT_POWER_CHANGE,
    POWER_EVENT_SYNC_TWS_VBAT_LEVEL,
    POWER_EVENT_POWER_CHARGE,
};

void app_power_set_tws_sibling_bat_level(u8 vbat, u8 percent);
void tws_sync_bat_level(void);
void power_event_to_user(u8 event);
int app_power_event_handler(struct device_event *dev);
u16 get_vbat_level(void);
u8 get_vbat_percent(void);
bool get_vbat_need_shutdown(void);
u8  battery_value_to_phone_level(u16 bat_val);
u8  get_self_battery_level(void);
u8  get_cur_battery_level(void);
void vbat_check_init(void);
void vbat_timer_delete(void);
void vbat_check(void *priv);
bool vbat_is_low_power(void);
void check_power_on_voltage(void);
void app_reset_vddiom_lev(u8 lev);
void vbat_timer_update(u32 msec);


#endif

