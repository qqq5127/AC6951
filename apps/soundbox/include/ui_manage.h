#ifndef __UI_MANAGE_H_
#define __UI_MANAGE_H_

#include "typedef.h"
#include "asm/pwm_led.h"

typedef enum {
    STATUS_NULL = 0,
    STATUS_POWERON,
    STATUS_POWEROFF,
    STATUS_POWERON_LOWPOWER,
    STATUS_BT_INIT_OK,
    STATUS_BT_CONN,
    STATUS_BT_SLAVE_CONN_MASTER,
    STATUS_BT_MASTER_CONN_ONE,
    STATUS_BT_DISCONN,
    STATUS_BT_TWS_CONN,
    STATUS_BT_TWS_DISCONN,
    STATUS_PHONE_INCOME,
    STATUS_PHONE_OUT,
    STATUS_PHONE_ACTIV,
    STATUS_CHARGE_START,
    STATUS_CHARGE_FULL,
    STATUS_CHARGE_CLOSE,
    STATUS_CHARGE_ERR,
    STATUS_LOWPOWER,
    STATUS_CHARGE_LDO5V_OFF,
    STATUS_EXIT_LOWPOWER,
    STATUS_NORMAL_POWER,
    STATUS_POWER_NULL,

    STATUS_MUSIC_MODE,
    STATUS_MUSIC_PLAY,
    STATUS_MUSIC_PAUSE,
    STATUS_LINEIN_MODE,
    STATUS_LINEIN_PLAY,
    STATUS_LINEIN_PAUSE,
    STATUS_PC_MODE,
    STATUS_PC_PLAY,
    STATUS_PC_PAUSE,
    STATUS_FM_MODE,
    STATUS_RECORD_MODE,
    STATUS_SPDIF_MODE,
    STATUS_RTC_MODE,
    STATUS_DEMO_MODE,

} UI_STATUS;

typedef struct __LED_REMAP_STATUS {
    u8 charge_start;    //开始充电
    u8 charge_full;     //充电完成
    u8 power_on;        //开机
    u8 power_off;       //关机
    u8 lowpower;        //低电
    u8 max_vol;         //最大音量
    u8 phone_in;        //来电
    u8 phone_out;       //去电
    u8 phone_activ;     //通话中
    u8 bt_init_ok;      //蓝牙初始化完成
    u8 bt_connect_ok;   //蓝牙连接成功
    u8 bt_disconnect;   //蓝牙断开
    u8 tws_connect_ok;   //蓝牙连接成功
    u8 tws_disconnect;   //蓝牙断开
    u8 music_play;   	//音乐模式播放音乐
    u8 music_pause;   	//音乐模式停止播放
    u8 linein_play;   	//Linein模式播放音乐
    u8 linein_mute;   	//Linein模式静音
} _GNU_PACKED_ LED_REMAP_STATUS;


void ui_pwm_led_init(const struct led_platform_data *user_data);
int  ui_manage_init(void);
void ui_update_status(u8 status);
u8 get_ui_busy_status();
void *led_get_remap_t(void);

#endif
