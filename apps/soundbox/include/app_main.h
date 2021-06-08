#ifndef APP_MAIN_H
#define APP_MAIN_H

typedef struct _APP_VAR {
    s8 music_volume;
    s8 call_volume;
    s8 wtone_volume;
    u8 opid_play_vol_sync;
    u8 aec_dac_gain;
    u8 aec_mic_gain;
    u8 rf_power;
    u8 poweron_charge;                  //开机进充电标志
    u8 goto_poweroff_flag;
    u8 goto_poweroff_cnt;
    u8 play_poweron_tone;
    u8 remote_dev_company;
    u8 siri_stu;
    int auto_stop_page_scan_timer;     //用于1拖2时，有一台连接上后，超过三分钟自动关闭Page Scan
    volatile int auto_shut_down_timer;
    volatile int wait_exit_timer;
    u16 auto_off_time;
    u16 warning_tone_v;
    u16 poweroff_tone_v;
    u32 start_time;
    s8  usb_mic_gain;
    u8  reverb_status;                 //用于tws+混响在pc模式时，同步关闭混响, 0:非pc模式
    u8 	cycle_mode;
} APP_VAR;

typedef struct _BT_USER_PRIV_VAR {
    //phone
    u8 phone_ring_flag: 1;
    u8 phone_num_flag: 1;
    u8 phone_income_flag: 1;
    u8 phone_call_dec_begin: 1;
    u8 phone_con_sync_num_ring: 1;
    u8 phone_con_sync_ring: 1;
    u8 emitter_or_receiver: 2;
    u16 get_phone_num_timecnt;

    u8 inband_ringtone;
    u8 phone_vol;
    u16 phone_timer_id;
    u8 last_call_type;
    u8 income_phone_num[30];
    u8 income_phone_len;
    s32 auto_connection_counter;
    s32 search_counter;
    int auto_connection_timer;
    u8 auto_connection_addr[6];
    int tws_con_timer;
    u8 tws_start_con_cnt;
    u8 tws_conn_state;
    bool search_tws_ing;
    int sniff_timer;
    bool fast_test_mode;
    u8 set_call_vol_flag;  /*有些客户希望通话默认给手机设置最大音量*/
} BT_USER_PRIV_VAR;

#define    BT_EMITTER_EN     1
#define    BT_RECEIVER_EN    2

typedef struct _BT_USER_COMM_VAR {
} BT_USER_COMM_VAR;

extern APP_VAR app_var;
extern BT_USER_PRIV_VAR bt_user_priv_var;

extern void app_main();

#define earphone (&bt_user_priv_var)

#endif
