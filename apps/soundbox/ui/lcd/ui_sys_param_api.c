#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_style.h"
#include "ui/ui_api.h"
#include "app_action.h"
#include "system/timer.h"
#include "font/language_list.h"
#include "res/resfile.h"
#include "syscfg_id.h"

#if (TCFG_UI_ENABLE && TCFG_SPI_LCD_ENABLE)
#include "ui/ui_sys_param.h"

struct SYS_INFO {
    u32 version_log;                 //版本日期
    u32 space_log;                   //内存大小
};

struct UI_DISPLAY {
    u16 backlight_time;             //背光时间
    u16 backlight_brightness;       //背光亮度
};

typedef struct _UI_SYS_PARAM {
    u16 cur_auto_close_time;        //当前设置的关机时间
    struct SYS_INFO sys_infomation; //系统信息
    struct UI_DISPLAY display;      //显示信息
} UI_SYS_PARAM;

//背光亮度
static const char table_system_lcd_value[] = {
    30,
    60,
    80,
    100,
};

//背光时间
static const u16 table_system_lcd_protect[] = {
    0,
    10,
    15,
    30,
    60,
};

//自动关机时间
static const u16 table_system_auto_close[] = {
    0,
    15 * 60,
    30 * 60,
    60 * 60,
    120 * 60,
    180 * 60,
};

static UI_SYS_PARAM sys_param;
volatile static int system_auto_close_timer;                 //自动关机定时器
volatile static int system_backlight_timer;                 //背光时间
#define __this 	(&sys_param)

extern void sys_enter_soft_poweroff(void *priv);
void auto_power_off_timer_close(void);
//*********************************************************************************//
//                                 配置开始                                        //
//*********************************************************************************//
static void sys_ui_backlight_close()
{
    printf("\n<<<<%s >>>>> \n", __FUNCTION__);
    system_backlight_timer = 0;
    ui_backlight_close();
}

void set_sys_param_default(void)
{
    __this->display.backlight_time = 0;
    __this->display.backlight_brightness = 0;
    __this->cur_auto_close_time = 0;
    auto_power_off_timer_close();
}

void sys_param_write2vm(void)
{
    syscfg_write(CFG_UI_SYS_INFO, __this, sizeof(UI_SYS_PARAM));
}

void sys_param_init(void)
{
    int ret = 0;
    r_printf("sys_param_init\n");
    ret = syscfg_read(CFG_UI_SYS_INFO, __this, sizeof(UI_SYS_PARAM));
    if (ret != sizeof(UI_SYS_PARAM)) {
        r_printf("sys_param read err\n");
        memset(__this, 0, sizeof(UI_SYS_PARAM)); //暂时默认都为0；
        set_sys_param_default();
        sys_param_write2vm();
    }
    if (__this->cur_auto_close_time && (!system_auto_close_timer)) {
        //每次重新开机把上次设置的自动关机时间设置进去
        system_auto_close_timer = sys_s_hi_timerout_add(NULL, sys_enter_soft_poweroff, (table_system_auto_close[__this->cur_auto_close_time ] * 1000));
    }

    if (__this->display.backlight_time && (!system_backlight_timer)) {
        system_backlight_timer = sys_s_hi_timerout_add(NULL, sys_ui_backlight_close, (table_system_lcd_protect[ __this->display.backlight_time] * 1000));
    }

}

//*********************************************************************************//
//                                 系统信息配置                                    //
//*********************************************************************************//
void set_version_log(u32 log)
{
    __this->sys_infomation.version_log = log;
}

u32 get_version_log(void)
{
    return __this->sys_infomation.version_log;
}

void set_space_log(u32 log)
{
    __this->sys_infomation.space_log = log;
}

u32 get_space_log(void)
{
    return __this->sys_infomation.space_log;
}

//*********************************************************************************//
//                                 自动关机配置                                    //
//*********************************************************************************//
void set_auto_poweroff_timer(int sel_item)
{
    int time = table_system_auto_close[sel_item];
    __this->cur_auto_close_time = sel_item;

    if (time == 0) {
        auto_power_off_timer_close();
        sys_param_write2vm();
        return;
    }

    if (system_auto_close_timer) {
        sys_s_hi_timer_modify(system_auto_close_timer, time * 1000);
    } else {
        system_auto_close_timer = sys_s_hi_timerout_add(NULL, sys_enter_soft_poweroff, (time * 1000));
    }
    sys_param_write2vm();
}

void auto_power_off_timer_close(void)
{
    if (system_auto_close_timer) {
        sys_s_hi_timeout_del(system_auto_close_timer);
        system_auto_close_timer = 0;
    }
}

u16 get_cur_auto_power_time(void)
{
    return __this->cur_auto_close_time;
}

//*********************************************************************************//
//                                 背光配置                                        //
//*********************************************************************************//
void set_backlight_time(u16 time)
{
    /* __this->display.backlight_time = table_system_lcd_protect[time]; */


    if (time >= sizeof(table_system_lcd_protect) / sizeof(table_system_lcd_protect[0])) {
        return;
    }
    __this->display.backlight_time = time;

    if (system_backlight_timer) {
        time = table_system_lcd_protect[__this->display.backlight_time];
        sys_s_hi_timer_modify(system_backlight_timer, time * 1000);
    }

    if (__this->display.backlight_time && (!system_backlight_timer)) {

        system_backlight_timer = sys_s_hi_timerout_add(NULL, sys_ui_backlight_close, (table_system_lcd_protect[ __this->display.backlight_time] * 1000));
    }

    if (!__this->display.backlight_time && system_backlight_timer) {
        sys_s_hi_timeout_del(system_backlight_timer);
        system_backlight_timer = 0;
    }


    sys_param_write2vm();
}

u16 get_backlight_time_item(void)
{
    return __this->display.backlight_time;
}

u16 get_backlight_time(void)
{
    return table_system_lcd_protect[__this->display.backlight_time];
}


void set_backlight_brightness(u16 brightness)
{
    /* __this->display.backlight_brightness = table_system_lcd_value[brightness]; */
    if (brightness >= sizeof(table_system_lcd_value) / sizeof(table_system_lcd_value[0])) {
        return;
    }

    __this->display.backlight_brightness = brightness;
    sys_param_write2vm();
}



u16 get_backlight_brightness_item(void)
{
    return __this->display.backlight_brightness;
}

u16 get_backlight_brightness(void)
{
    return table_system_lcd_value[__this->display.backlight_brightness];
}

//*********************************************************************************//
//                                 其他配置                                        //
//*********************************************************************************//
void set_sys_info_reset(void)
{
    set_sys_param_default();
    sys_param_write2vm();
}



extern int lcd_backlight_status();

static int app_key_event(struct sys_event *e)
{
    struct key_event *key = &e->u.key;
    int time;

    if (system_auto_close_timer) {
        time = table_system_auto_close[__this->cur_auto_close_time ];
        sys_s_hi_timer_modify(system_auto_close_timer, time * 1000);
    }


    if (system_backlight_timer) {
        time = table_system_lcd_protect[__this->display.backlight_time];
        sys_s_hi_timer_modify(system_backlight_timer, time * 1000);
    }



    if (!lcd_backlight_status()) {
        e->consumed = 1;//接管按键消息,app应用不会收到消息
        if (__this->display.backlight_time && (!system_backlight_timer)) {
            system_backlight_timer = sys_s_hi_timerout_add(NULL, sys_ui_backlight_close, (table_system_lcd_protect[ __this->display.backlight_time] * 1000));
            ui_backlight_open();
        }
        return TRUE;
    }

    return FALSE;
}

SYS_EVENT_HANDLER(SYS_KEY_EVENT,  app_key_event, 2);

#endif //TCFG_SPI_LCD_ENABLE
