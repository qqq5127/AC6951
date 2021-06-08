#ifndef _CHGBOX_CTRL_H_
#define _CHGBOX_CTRL_H_
#include "event.h"

#define STATUS_OFFLINE  0
#define STATUS_ONLINE   1

enum {
    USB_DET,        //usb检测
    LID_DET,        //盖子检测
    LDO_DET,        //升压成功检测
    WIRELESS_DET,   //无线充检测
    DET_MAX,
};

enum {
    KEY_POWER_CLICK,
    KEY_POWER_LONG,
    KEY_POWER_HOLD,
    KEY_POWER_UP,
    KEY_POWER_DOUBLE,
    KEY_POWER_THIRD,
};

//充电仓当然状态
enum {
    CHG_STATUS_COMM,      //开盖通信
    CHG_STATUS_CHARGE,    //合盖充电
    CHG_STATUS_LOWPOWER,  //充电仓电压低
};


//检测几次不在线后才认为耳机拔出
#define TCFG_EAR_OFFLINE_MAX     4
//发送shutdown的个数
#define TCFG_SEND_SHUT_DOWN_MAX  5
//发送closelid的个数 -- 需要 EAR_OFFLINE_MAX 大
#define TCFG_SEND_CLOSE_LID_MAX  5


typedef struct _SYS_INFO {
    volatile u8 charge: 1;        //是否处于充电状态
        volatile u8 ear_l_full: 1;     //左耳机是否充满
        volatile u8 ear_r_full: 1;     //右耳机是否充满
        volatile u8 earfull: 1;     //是否充满
        volatile u8 localfull: 1;   //本机是否充满
        volatile u8 led_flag: 1;    //led活跃状态
        volatile u8 lowpower_flag: 1; //低电标记
        volatile u8 power_on: 1;    //上电/唤醒

        volatile u8 pair_succ: 1;   //配对成功
        volatile u8 init_ok: 1;     //充电IC是否初始化成功
        volatile u8 chg_addr_ok: 1;     //获取广播地址成功
        volatile u8 current_limit: 1; //过流
        volatile u8 temperature_limit: 1;//过热过冷
        volatile u8 wireless_wakeup: 1; //标记无线充唤醒
        volatile u8 reserev: 2; //保留bit

        volatile u8 pair_status;    //处于配对状态
        volatile u8 shut_cnt;       //关机命令计数器
        volatile u8 lid_cnt;        //关盖命令计数器
        volatile u8 life_cnt;       //超时休眠
        volatile u8 force_charge;       //强制充电（耳机完全没电时需要先充电）
        volatile u8 chgbox_status;     //充电仓状态：开盖、合盖、低电等
        volatile u8 status[DET_MAX];
    } SYS_INFO;

    typedef struct _EAR_INFO {
    volatile u8 online[2];    //在线离线计数
    volatile u8 power[2];     //电量
    volatile u8 full_cnt[2];  //电量充满计数
} EAR_INFO;


extern SYS_INFO sys_info;
extern EAR_INFO ear_info;

u8 chargebox_check_output_short(void);
void chargebox_set_output_short(void);
void app_charge_box_ctrl_init(void);
int charge_box_ctrl_event_handler(struct chargebox_event *chg_event);
int charge_box_key_event_handler(u16 event);

void  chgbox_init_app(void);
#endif
