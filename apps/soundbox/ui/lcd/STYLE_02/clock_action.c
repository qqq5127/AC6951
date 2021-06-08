#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_style.h"
#include "ui/ui_api.h"
#include "app_task.h"
#include "system/timer.h"
#include "key_event_deal.h"
#include "audio_config.h"
#include "jiffies.h"
#include "system/timer.h"
#include "system/includes.h"
#include "rtc/alarm.h"
#include "app_power_manage.h"
#include "asm/charge.h"
#ifndef CONFIG_MEDIA_NEW_ENABLE
#include "application/audio_eq_drc_apply.h"
#else
#include "audio_eq.h"
#endif

#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))
#if (TCFG_APP_RTC_EN)

#define STYLE_NAME  JL

extern int ui_hide_main(int id);
extern int ui_show_main(int id);
extern void key_ui_takeover(u8 on);
extern void power_off_deal(struct sys_event *event, u8 step);

struct rtc_ui_opr {
    void *dev_handle;
    int timer;
};

static struct rtc_ui_opr *__this = NULL;


/* static int test_timer =0; */

/************************************************************
                         电池控件事件
 ************************************************************/

static void battery_timer(void *priv)
{
    int  incharge = 0;//充电标志
    int id = (int)(priv);
    static u8 percent = 0;
    static u8 percent_last = 0;
    if (1 || get_charge_online_flag()) { //充电时候图标动态效果
        if (percent > get_vbat_percent()) {
            percent = 0;
        }
        ui_battery_set_level_by_id(id, percent, incharge); //充电标志,ui可以显示不一样的图标
        percent += 20;
    } else {

        percent = get_vbat_percent();
        if (percent != percent_last) {
            ui_battery_set_level_by_id(id, percent, incharge); //充电标志,ui可以显示不一样的图标,需要工具设置
            percent_last = percent;
        }

    }
}

static int battery_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_battery *battery = (struct ui_battery *)ctr;
    static u32 timer = 0;
    int  incharge = 0;//充电标志

    switch (e) {
    case ON_CHANGE_INIT:
        ui_battery_set_level(battery, get_vbat_percent(), incharge);
        if (!timer) {
            timer = sys_timer_add((void *)battery->elm.id, battery_timer, 1 * 1000); //传入的id就是BT_BAT
        }
        break;
    case ON_CHANGE_FIRST_SHOW:
        break;
    case ON_CHANGE_RELEASE:
        if (timer) {
            sys_timer_del(timer);
            timer = 0;
        }
        break;
    default:
        return false;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(CLOCK_BAT)
.onchange = battery_onchange,
 .ontouch = NULL,
};




static void timer_add_update(void *p)
{
    struct utime time_r;
    struct sys_time current_time;
    dev_ioctl(__this->dev_handle, IOCTL_GET_SYS_TIME, (u32)&current_time);

    time_r.year = current_time.year;//, jiffies /1 %3000;
    time_r.month = current_time.month;//jiffies /2 %12;
    time_r.day = current_time.day;//jiffies /3 %30;

    time_r.hour = current_time.hour;//jiffies /4 % 60;
    time_r.min = current_time.min;//jiffies /5 %60;
    time_r.sec = current_time.sec;//jiffies /6 %60;

    if (ui_get_disp_status_by_id(CLOCK_TIME_YMD) > 0) {
        ui_time_update_by_id(CLOCK_TIME_YMD, &time_r);
    }

    if (ui_get_disp_status_by_id(CLOCK_TIME_HMS) > 0) {
        ui_time_update_by_id(CLOCK_TIME_HMS, &time_r);
    }
}


/************************************************************
                         时钟主页窗口控件
 ************************************************************/
static int clock_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***clock_mode_onchange***\n");
        /* key_ui_takeover(1); */

        if (!__this) {
            __this =  zalloc(sizeof(struct rtc_ui_opr));
        }
        __this->dev_handle = dev_open("rtc", NULL);

        if (!__this->dev_handle) {
            free(__this);
            __this = NULL;
            break;
        }

        if (!__this->timer) {
            __this->timer = sys_timer_add(NULL, timer_add_update, 500);
        }


        /*
         * 注册APP消息响应
         */
        break;
    case ON_CHANGE_RELEASE:

        if (__this && __this->timer) {
            sys_timer_del(__this->timer);
            __this->timer = 0;
        }

        if (__this && __this->dev_handle) {
            dev_close(__this->dev_handle);
            __this->dev_handle = 0;
        }

        if (__this) {
            free(__this);
            __this = NULL;
        }

        break;
    default:
        return false;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(ID_WINDOW_CLOCK)
.onchange = clock_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};






/************************************************************
                         时钟显示界面的布局控件
 ************************************************************/


static int clock_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    switch (e) {
    case ON_CHANGE_INIT:
        break;
    }
    return false;
}


static int clock_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("clock_layout_onkey %d\n", e->value);
    switch (e->value) {
    case KEY_MENU:
        if (ui_get_disp_status_by_id(CLOCK_MENU_LAYOUT) <= 0) {
            printf("CLOCK_MENU_LAYOUT\n");
            ui_show(CLOCK_MENU_LAYOUT);
        }
        break;
    case KEY_UP:
    case KEY_DOWN:
        break;
    case KEY_VOLUME_INC:
    case KEY_VOLUME_DEC:
        if (ui_get_disp_status_by_id(CLOCK_VOL_LAYOUT) <= 0) {
            ui_hide(CLOCK_LAYOUT);
            ui_show(CLOCK_VOL_LAYOUT);
        }
        break;
    case KEY_MODE:
        ui_hide_curr_main();
        ui_show_main(ID_WINDOW_SYS);
        break;
    case KEY_POWER_START:
    case KEY_POWER:
        power_off_deal(NULL, e->value - KEY_POWER_START);
        break;
    default:
        return false;
        break;
    }
    return false;
}




REGISTER_UI_EVENT_HANDLER(CLOCK_LAYOUT)
.onchange = clock_layout_onchange,
 .onkey = clock_layout_onkey,
  .ontouch = NULL,
};




/************************************************************
                         时钟显示菜单的列表控件
 ************************************************************/
static int clock_menu_list_onkey(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int sel_item = 0;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    switch (e->value) {
    case KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
            ui_show(CLOCK_TIME_SET_LAYOUT);
            break;
        case 1:
            ui_show(CLOCK_ALM_SET_LAYOUT);
            break;
        case 2:
            ui_hide(CLOCK_MENU_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}



REGISTER_UI_EVENT_HANDLER(CLOCK_MENU_LIST)
.onchange = NULL,
 .onkey = clock_menu_list_onkey,
  .ontouch = NULL,
};



/************************************************************
                         时钟音量设置布局控件
 ************************************************************/
static u16 vol_timer = 0;
static void clock_vol_lay_timeout(void *p)
{
    int id = (int)(p);
    if (ui_get_disp_status_by_id(id) == TRUE) {
        ui_hide(id);
        ui_show(CLOCK_LAYOUT);
    }
    vol_timer = 0;
}


static void vol_init(int id)
{
    struct unumber num;
    num.type = TYPE_NUM;
    num.numbs =  1;
    num.number[0] = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
    ui_number_update_by_id(CLOCK_VOL_NUM, &num);
}

static int clock_vol_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        if (!vol_timer) {
            vol_timer  = sys_timer_add((void *)layout->elm.id, clock_vol_lay_timeout, 3000);
        }
        break;

    case ON_CHANGE_FIRST_SHOW:
        ui_set_call(vol_init, 0);
        /* ui_number_update_by_id(CLOCK_VOL_NUM,&num); */
        break;

    case ON_CHANGE_SHOW_POST:
        break;

    case ON_CHANGE_RELEASE:
        if (vol_timer) {
            sys_timeout_del(vol_timer);
            vol_timer = 0;
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}


static int clock_vol_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("clock_vol_layout_onkey %d\n", e->value);
    struct unumber num;
    u8 vol;

    if (vol_timer) {
        sys_timer_modify(vol_timer, 3000);
    }


    switch (e->value) {
    case KEY_MENU:
        break;

    case KEY_UP:
    case KEY_VOLUME_INC:
        app_audio_volume_up(1);
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(CLOCK_VOL_NUM, &num);
        break;
    case KEY_DOWN:
    case KEY_VOLUME_DEC:
        app_audio_volume_down(1);
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(CLOCK_VOL_NUM, &num);
        break;
    default:
        return false;
        break;
    }
    return true;
}




REGISTER_UI_EVENT_HANDLER(CLOCK_VOL_LAYOUT)
.onchange = clock_vol_layout_onchange,
 .onkey = clock_vol_layout_onkey,
  .ontouch = NULL,
};

/***************************** 时间显示控件初始化 ************************************/
static int clock_timer_show_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_time *time = (struct ui_time *)ctr;
    struct sys_time current_time;

    switch (e) {
    case ON_CHANGE_INIT:
        dev_ioctl(__this->dev_handle, IOCTL_GET_SYS_TIME, (u32)&current_time);
        time->year = current_time.year;
        time->month = current_time.month;
        time->day =  current_time.day;
        time->hour = current_time.hour;
        time->min = current_time.min;
        time->sec = current_time.sec;
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(CLOCK_TIME_YMD)
.onchange = clock_timer_show_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(CLOCK_TIME_HMS)
.onchange = clock_timer_show_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};





/***************************** 日期时间设置 ************************************/
struct time_setting {
    const char *mark;
    u32 time_id;
    u16 time_value;
};
static  struct time_setting time_set_table[] = {
    {"year",  CLOCK_TIMER_YEAR,   0},
    {"month", CLOCK_TIMER_MONTH,  0},
    {"day",   CLOCK_TIMER_DAY,    0},
    {"hour",  CLOCK_TIMER_HOUR,   0},
    {"min",   CLOCK_TIMER_MINUTE, 0},
    {"sec",   CLOCK_TIMER_SECOND, 0},
};
enum sw_dir { /* 切换方向 */
    DIR_NEXT = 1,
    DIR_PREV,
    DIR_SET,
};
enum set_mod { /* 加减时间方向 */
    MOD_ADD = 1,
    MOD_DEC,
    MOD_SET,
};

static void get_sys_time(struct sys_time *time)
{
    void *fd = dev_open("rtc", NULL);
    if (!fd) {
        memset(time, 0, sizeof(*time));
        return;
    }
    dev_ioctl(fd, IOCTL_GET_SYS_TIME, (u32)time);
    /* printf("get_sys_time : %d-%d-%d,%d:%d:%d\n", time->year, time->month, time->day, time->hour, time->min, time->sec); */
    dev_close(fd);
}
static u16 time_set_p = 0xff; /* time_set_table的当前设置指针 */
static u8 __time_search_by_mark(const char *mark)
{
    u16 p = 0;
    u16 table_sum = sizeof(time_set_table) / sizeof(struct time_setting);
    while (p < table_sum) {
        if (!strcmp(mark, time_set_table[p].mark)) {
            return p;
        }
        p++;
    }
    return -1;
}
static u8 __time_set_switch(enum sw_dir dir, const char *mark)
{
    u16 table_sum;
    u16 prev_set_p = time_set_p;
    u8 p;
    printf("__time_set_switch dir: %d\n", dir);
    table_sum = sizeof(time_set_table) / sizeof(struct time_setting);
    ASSERT(dir == DIR_NEXT || dir == DIR_PREV || dir == DIR_SET);
    switch (dir) {
    case DIR_NEXT:
        if (time_set_p >= (table_sum - 1)) {
            time_set_p = 0;
        } else {
            time_set_p++;
        }
        break;
    case DIR_PREV:
        if (time_set_p >= (table_sum - 1)) {
            time_set_p = 0;
        }


        if (time_set_p == 0) {
            time_set_p = (table_sum - 1);
        } else {
            time_set_p--;
        }
        break;
    case DIR_SET:
        p = __time_search_by_mark(mark);
        if (p == (u8) - 1) {
            return -1;
        }
        time_set_p = p;

        break;
    }
    if (prev_set_p != 0xff) {
        ui_no_highlight_element_by_id(time_set_table[prev_set_p].time_id);
    }

    ui_highlight_element_by_id(time_set_table[time_set_p].time_id);
    printf("__time_set_switch ok.\n");
    return 0;
}
static u8 __time_set_reset(void)
{
    struct sys_time time;

    time_set_p = 0xff;
    /*
     * 此处应该读取系统RTC时间
     */
    get_sys_time(&time);

    time_set_table[0].time_value = time.year;
    time_set_table[1].time_value = time.month;
    time_set_table[2].time_value = time.day;
    time_set_table[3].time_value = time.hour;
    time_set_table[4].time_value = time.min;
    time_set_table[5].time_value = time.sec;

    return 0;
}
const u16 leap_month_table[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static u8 __is_leap_year(u16 year)
{
    return (((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0));
}
static u8 __time_update_show(u8 pos) /* 更新单个时间控件的时间 */
{
    struct utime t;
    t.year  = time_set_table[0].time_value;
    t.month = time_set_table[1].time_value;
    t.day   = time_set_table[2].time_value;
    t.hour  = time_set_table[3].time_value;
    t.min   = time_set_table[4].time_value;
    t.sec   = time_set_table[5].time_value;
    ui_time_update_by_id(time_set_table[pos].time_id, &t);
    return 0;
}
static u8 __time_set_value(enum set_mod mod, u16 value)
{

    enum set_mod need_check = 0;
    u16 year, month, day;
    ASSERT(mod == MOD_ADD || mod == MOD_DEC || mod == MOD_SET);
    switch (mod) {
    case MOD_ADD:
        switch (time_set_p) {
        case 0: /* year */
            value = time_set_table[time_set_p].time_value + 1;
            if (value >= 2100) {
                value = 2099;
            }
            need_check  = MOD_ADD;
            break;
        case 1: /* month */
            value = time_set_table[time_set_p].time_value + 1;
            if (value > 12) {
                value = 1;
            }
            need_check  = MOD_ADD;
            break;
        case 2: /* day */
            month = time_set_table[1].time_value;
            ASSERT(month >= 1 && month <= 12);
            value = time_set_table[time_set_p].time_value + 1;
            if (value > leap_month_table[month - 1]) {
                value = 1;
            }
            need_check  = MOD_ADD;
            break;
        case 3: /* hour */
            value = time_set_table[time_set_p].time_value + 1;
            if (value > 23) {
                value = 0;
            }
            break;
        case 4: /* min */
            value = time_set_table[time_set_p].time_value + 1;
            if (value > 59) {
                value = 0;
            }
            break;
        case 5: /* sec */
            value = time_set_table[time_set_p].time_value + 1;
            if (value > 59) {
                value = 0;
            }
            break;
        default:
            /* ASSERT(0, "mod_add time_set_p:%d err!", time_set_p); */
            return 0;
            break;
        }
        break;
    case MOD_DEC:
        switch (time_set_p) {
        case 0: /* year */
            value = time_set_table[time_set_p].time_value - 1;
            if (value <= 2000) {
                value = 2001;
            }
            need_check  = MOD_DEC;
            break;
        case 1: /* month */
            value = time_set_table[time_set_p].time_value;
            if (value == 1) {
                value = 12;
            } else {
                value--;
            }
            need_check  = MOD_DEC;
            break;
        case 2: /* day */
            value = time_set_table[time_set_p].time_value;
            if (value == 1) {
                month = time_set_table[1].time_value;
                ASSERT(month >= 1 && month <= 12);
                value = leap_month_table[month - 1];
            } else {
                value--;
            }
            need_check = MOD_DEC;
            break;
        case 3: /* hour */
            value = time_set_table[time_set_p].time_value;
            if (value == 0) {
                value = 23;
            } else {
                value--;
            }
            break;
        case 4: /* min */
            value = time_set_table[time_set_p].time_value;
            if (value == 0) {
                value = 59;
            } else {
                value--;
            }
            break;
        case 5: /* sec */
            value = time_set_table[time_set_p].time_value;
            if (value == 0) {
                value = 59;
            } else {
                value--;
            }
            break;
        default:
            /* ASSERT(0, "mod_dec time_set_p:%d err!", time_set_p); */

            return 0;
            break;
        }
        break;
    case MOD_SET:
        switch (time_set_p) {
        case 0: /* year */
            need_check = MOD_SET;
            break;
        case 1: /* month */
            ASSERT(value >= 1 && value <= 12);
            need_check = MOD_SET;
            break;
        case 2: /* day */
            need_check = MOD_SET;
            break;
        case 3: /* hour */
            ASSERT(value >= 0 && value <= 23);
            break;
        case 4: /* min */
            ASSERT(value >= 0 && value <= 59);
            break;
        case 5: /* sec */
            ASSERT(value >= 0 && value <= 59);
            break;
        default:
            /* ASSERT(0, "mod_set time_set_p:%d err!", time_set_p); */

            return 0;
            break;
        }
        break;
    }
    time_set_table[time_set_p].time_value = value;

    if (need_check) {
        year = time_set_table[0].time_value;
        month = time_set_table[1].time_value;
        day = time_set_table[2].time_value;
        if (month == 2 && !__is_leap_year(year)) {
            if (day >= 29) {
                if (need_check == MOD_ADD) {
                    day = 1;
                } else if (need_check == MOD_DEC) {
                    day = 28;
                } else {
                    day = 28;
                }
                time_set_table[2].time_value = day;
                __time_update_show(2); /* 调整day数值显示 */
            }

        } else {
            if (day > leap_month_table[month - 1]) {
                day = leap_month_table[month - 1];
                time_set_table[2].time_value = day;
                __time_update_show(2); /* 调整day数值显示 */
            }
        }
    }
    __time_update_show(time_set_p); /* 更新当前位数值显示 */

    return 0;
}
static int __time_get(struct sys_time *t)
{
    t->year  = time_set_table[0].time_value;
    t->month = time_set_table[1].time_value;
    t->day   = time_set_table[2].time_value;
    t->hour  = time_set_table[3].time_value;
    t->min   = time_set_table[4].time_value;
    t->sec   = time_set_table[5].time_value;
    return 0;
}


extern void rtc_update_time_api(struct sys_time *time);
static int menu_sys_time_set()
{
    struct sys_time t;

    __time_get(&t);

    rtc_update_time_api(&t);
    return 0;
}



/************************************************************
                         时钟置布局控件
 ************************************************************/
//是时钟初始化消息入口
static void __time_set_switch_init(int id)
{
    __time_set_switch(DIR_SET, "year");//设置默认高亮的选项,可以是year day
    /* __time_set_switch(DIR_SET, NULL); */
}


static int time_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("time_layout_onchange onfocus.\n");
        layout_on_focus(layout);//设置焦点在本布局(如果布局下还有列表，焦点会最后显示出的列表这里，防止焦点不在，可以把焦点控制在本布局)
        __time_set_reset();//初始化一些信息
        break;
    case ON_CHANGE_RELEASE:
        puts("time_layout_onchange losefocus.\n");
        layout_lose_focus(layout);//失去本焦点
        break;
    case ON_CHANGE_FIRST_SHOW:
        puts("time_layout_onchange ON_CHANGE_FIRST_SHOW.\n");
        ui_set_call(__time_set_switch_init, 0);

        break;
    default:
        break;
    }
    return FALSE;
}

//是时钟消息入口,控制设置显示的选项和加减
static int time_layout_onkey(void *ctr, struct element_key_event *e)
{

    switch (e->value) {
    case KEY_OK:
        puts("time_layout_onkey KEY_OK.\n");
        __time_set_switch(DIR_NEXT, NULL);//切换选项

        break;
    case KEY_VOLUME_DEC:
    case KEY_DOWN:
        __time_set_value(MOD_ADD, 0);

        break;
    case KEY_VOLUME_INC:
    case KEY_UP:
        __time_set_value(MOD_DEC, 0);

        break;
    case KEY_MODE:
    case KEY_MENU:
        menu_sys_time_set();//保存设置
        ui_hide(CLOCK_TIME_SET_LAYOUT);//退出当前布局
        break;
    default:
        return false;
        break;
    }

    return true;
}

REGISTER_UI_EVENT_HANDLER(CLOCK_TIME_SET_LAYOUT)
.onchange = time_layout_onchange,
 .onkey = time_layout_onkey,
  .ontouch = NULL,
};



//如果不对控件进行初始化，默认显示0的，因此需要对控件进行初始先
/***************************** 系统时间各个数字动作 ************************************/
static int clock_timer_sys_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_time *time = (struct ui_time *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("timer_sys_onchange .\n");
        time->year = time_set_table[0].time_value;
        time->month = time_set_table[1].time_value;
        time->day = time_set_table[2].time_value;
        time->hour = time_set_table[3].time_value;
        time->min = time_set_table[4].time_value;
        time->sec = time_set_table[5].time_value;
        break;
#if 0
    case ON_CHANGE_SHOW_POST:
        printf("~~~~~~~~~~~~%x\n", time->text.elm.id);
        if (time->text.elm.highlight) {
            printf("~~~~~~~~~~~~%x\n", time->text.elm.id);
            /* ui_invert_element_by_id(time->text.elm.id); */
        }
        break;
#endif
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(CLOCK_TIMER_YEAR)
.onchange = clock_timer_sys_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(CLOCK_TIMER_MONTH)
.onchange = clock_timer_sys_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(CLOCK_TIMER_DAY)
.onchange = clock_timer_sys_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(CLOCK_TIMER_HOUR)
.onchange = clock_timer_sys_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(CLOCK_TIMER_MINUTE)
.onchange = clock_timer_sys_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(CLOCK_TIMER_SECOND)
.onchange = clock_timer_sys_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};




/***************************** 闹钟日期时间设置 ************************************/

static const int alm_rep_list[] = {
    ALM_SUN,
    ALM_MON,
    ALM_TUE,
    ALM_WED,
    ALM_THU,
    ALM_FRI,
    ALM_SAT,
};


struct alm_data_rep {
    int  rep_set;
    int  mask;
};

static struct time_setting alm_set_table[] = {
    {"hour",  ALM_H,   0},
    {"min",   ALM_M, 0},
    {"rep",   0, 0},
    {"sw",    ALM_SW, 0},
};


struct alm_data_rep alm_rep[] = {
    {E_ALARM_MODE_ONCE,  0},
    //只响应一次
    {E_ALARM_MODE_EVERY_DAY,  0xff},
    //每天
    {
        E_ALARM_MODE_EVERY_MONDAY | E_ALARM_MODE_EVERY_TUESDAY | \
        E_ALARM_MODE_EVERY_WEDNESDAY | E_ALARM_MODE_EVERY_THURSDAY | \
        E_ALARM_MODE_EVERY_FRIDAY,  BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5)
    },
    //工作日

    {E_ALARM_MODE_EVERY_SUNDAY,  BIT(0)},
    {E_ALARM_MODE_EVERY_MONDAY,  BIT(1)},
    {E_ALARM_MODE_EVERY_TUESDAY,  BIT(2)},
    {E_ALARM_MODE_EVERY_WEDNESDAY,  BIT(3)},
    {E_ALARM_MODE_EVERY_THURSDAY,  BIT(4)},
    {E_ALARM_MODE_EVERY_FRIDAY,  BIT(5)},
    {E_ALARM_MODE_EVERY_SATURDAY,  BIT(6)},
    //独立日
};



static u16 alm_set_p = 0xff; /* time_set_table的当前设置指针 */

static u8 __alm_search_by_mark(const char *mark)
{
    u16 p = 0;
    u16 table_sum = sizeof(alm_set_table) / sizeof(struct time_setting);
    while (p < table_sum) {
        if (!strcmp(mark, alm_set_table[p].mark)) {
            return p;
        }
        p++;
    }
    return -1;
}

static u8 __alm_set_switch(enum sw_dir dir, const char *mark)
{
    u16 table_sum;
    u16 prev_set_p = alm_set_p;
    u8 p;
    printf("__time_set_switch dir: %d\n", dir);
    table_sum = sizeof(alm_set_table) / sizeof(struct time_setting);
    ASSERT(dir == DIR_NEXT || dir == DIR_PREV || dir == DIR_SET);
    switch (dir) {
    case DIR_NEXT:
        if (alm_set_p >= (table_sum - 1)) {
            alm_set_p = 0;
        } else {
            alm_set_p++;
        }
        break;
    case DIR_PREV:
        if (alm_set_p >= (table_sum - 1)) {
            alm_set_p = 0;
        }

        if (alm_set_p == 0) {
            alm_set_p = (table_sum - 1);
        } else {
            alm_set_p--;
        }
        break;
    case DIR_SET:
        p = __alm_search_by_mark(mark);
        if (p == (u8) - 1) {
            return -1;
        }
        alm_set_p = p;
        break;
    }

    if (prev_set_p != 0xff && alm_set_table[prev_set_p].time_id) {
        ui_no_highlight_element_by_id(alm_set_table[prev_set_p].time_id);
    }


    if (alm_set_table[alm_set_p].time_id) {
        ui_highlight_element_by_id(alm_set_table[alm_set_p].time_id);
    }

    printf("__time_set_switch ok.\n");
    return 0;
}

static u8 __alm_set_reset(void)
{
    T_ALARM alarm;
    alm_set_p = 0xff;
    alarm_get_info(&alarm, 0);
    alm_set_table[0].time_value = alarm.time.hour;
    alm_set_table[1].time_value = alarm.time.min;

    for (int i = 0; i < sizeof(alm_rep) / sizeof(alm_rep[0]); i++) {
        if (alm_rep[i].rep_set == alarm.mode) {
            alm_set_table[2].time_value = i;
            break;
        }
    }

    alm_set_table[3].time_value = alarm.sw;
    return 0;
}

static u8 __alm_update_show(u8 pos) /* 更新单个时间控件的时间 */
{
    struct utime t;
    t.hour  = alm_set_table[0].time_value;
    t.min   = alm_set_table[1].time_value;
    int rep_set = alm_set_table[2].time_value;
    int sw = alm_set_table[3].time_value;

    if (pos == 2) { //
        for (int i = 0; i < sizeof(alm_rep_list) / sizeof(alm_rep_list[0]); i++) {
            if (BIT(i) & alm_rep[rep_set].mask) {
                ui_highlight_element_by_id(alm_rep_list[i]);
            } else {
                ui_no_highlight_element_by_id(alm_rep_list[i]);
            }
        }
        return 0;
    }

    if (pos == 3) { //开关
        if (sw) {
            ui_text_show_index_by_id(alm_set_table[pos].time_id, 1);
        } else {
            ui_text_show_index_by_id(alm_set_table[pos].time_id, 0);
        }

        return 0;
    }

    ui_time_update_by_id(alm_set_table[pos].time_id, &t);
    return 0;
}

static u8 __alm_set_value(enum set_mod mod, u16 value)
{
    enum set_mod need_check = 0;
    u16 year, month, day;
    ASSERT(mod == MOD_ADD || mod == MOD_DEC || mod == MOD_SET);
    switch (mod) {
    case MOD_ADD:
        switch (alm_set_p) {
        case 0: /* hour */
            value = alm_set_table[alm_set_p].time_value + 1;
            if (value > 23) {
                value = 0;
            }
            break;
        case 1: /* min */
            value = alm_set_table[alm_set_p].time_value + 1;
            if (value > 59) {
                value = 0;
            }
            break;
        case 2: /* rep */
            value = alm_set_table[alm_set_p].time_value + 1;
            if (value >= sizeof(alm_rep) / sizeof(alm_rep[0])) {
                value = 0;
            }
            break;
        case 3: /* sw */
            value = !alm_set_table[alm_set_p].time_value;
            break;
        default:
            /* ASSERT(0, "mod_add time_set_p:%d err!", time_set_p); */
            return 0;
            break;
        }
        break;
    case MOD_DEC:
        switch (alm_set_p) {
        case 0: /* hour */
            value = alm_set_table[alm_set_p].time_value;
            if (value == 0) {
                value = 23;
            } else {
                value--;
            }
            break;
        case 1: /* min */
            value = alm_set_table[alm_set_p].time_value;
            if (value == 0) {
                value = 59;
            } else {
                value--;
            }
            break;
        case 2://rep
            value = alm_set_table[alm_set_p].time_value;
            if (value == 0) {
                value =   sizeof(alm_rep) / sizeof(alm_rep[0]) - 1;
            } else {
                value--;
            }
            break;
        case 3: /* sw */
            value = !alm_set_table[alm_set_p].time_value;
            break;
        default:
            /* ASSERT(0, "mod_dec time_set_p:%d err!", time_set_p); */
            return 0;
            break;
        }
        break;
    case MOD_SET:
        switch (alm_set_p) {
        case 0: /* hour */
            ASSERT(value >= 0 && value <= 23);
            break;
        case 1: /* min */
            ASSERT(value >= 0 && value <= 59);
            break;
        default:
            /* ASSERT(0, "mod_set time_set_p:%d err!", time_set_p); */
            return 0;
            break;
        }
        break;
    }

    alm_set_table[alm_set_p].time_value = value;

    __alm_update_show(alm_set_p); /* 更新当前位数值显示 */

    return 0;
}


static int menu_sys_alm_set()
{
    u8 index = 0;
    T_ALARM alarm;
    alarm.time.hour  = alm_set_table[0].time_value ;
    alarm.time.min   = alm_set_table[1].time_value;
    alarm.mode       = alm_rep[alm_set_table[2].time_value].rep_set;
    alarm.sw         = alm_set_table[3].time_value;
    alarm.index      = index;
    alarm_add(&alarm, index);
    return 0;
}


static void __alm_set_switch_init(int id)
{
    __alm_set_switch(DIR_SET, NULL);
}

static int alm_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("time_layout_onchange onfocus.\n");
        layout_on_focus(layout);
        __alm_set_reset();
        break;
    case ON_CHANGE_RELEASE:
        puts("time_layout_onchange losefocus.\n");
        layout_lose_focus(layout);
        break;
    case ON_CHANGE_FIRST_SHOW:
        puts("time_layout_onchange ON_CHANGE_FIRST_SHOW.\n");
        ui_set_call(__time_set_switch_init, 0);

        break;
    default:
        break;
    }
    return FALSE;
}
static int alm_layout_onkey(void *ctr, struct element_key_event *e)
{

    switch (e->value) {
    case KEY_OK:
        puts("time_layout_onkey KEY_OK.\n");
        __alm_set_switch(DIR_NEXT, NULL);

        break;
    case KEY_DOWN:
        __alm_set_value(MOD_ADD, 0);

        break;
    case KEY_UP:
        __alm_set_value(MOD_DEC, 0);

        break;
    case KEY_MODE:
    case KEY_MENU:
        menu_sys_alm_set();
        ui_hide(CLOCK_ALM_SET_LAYOUT);
        break;
    default:
        return false;
        break;
    }

    return true;
}

REGISTER_UI_EVENT_HANDLER(CLOCK_ALM_SET_LAYOUT)
.onchange = alm_layout_onchange,
 .onkey = alm_layout_onkey,
  .ontouch = NULL,
};



/***************************** 闹钟各个数字动作 ************************************/
static int alm_timer_sys_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_time *time = (struct ui_time *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("timer_sys_onchange .\n");
        time->hour = alm_set_table[0].time_value;
        time->min  = alm_set_table[1].time_value;
        break;
    case ON_CHANGE_SHOW_POST:
#if 0
        printf("~~~~~~~~~~~~%x\n", time->text.elm.id);
        if (time->text.elm.highlight) {
            printf("~~~~~~~~~~~~%x\n", time->text.elm.id);
            /* ui_invert_element_by_id(time->text.elm.id); */
        }
#endif
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(ALM_H)
.onchange = alm_timer_sys_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(ALM_M)
.onchange = alm_timer_sys_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


static int alm_timer_rep_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        for (int i = 0; i < sizeof(alm_rep_list) / sizeof(alm_rep_list[0]); i++) {
            if (text->elm.id == alm_rep_list[i]) {
                if (BIT(i) & alm_rep[alm_set_table[2].time_value].mask) {
                    ui_highlight_element(&text->elm);
                    /* text->elm.highlight = 1; */
                }
                break;
            }
        }
        break;
    case ON_CHANGE_SHOW_POST:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(ALM_SUN)
.onchange = alm_timer_rep_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(ALM_MON)
.onchange = alm_timer_rep_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(ALM_TUE)
.onchange = alm_timer_rep_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(ALM_WED)
.onchange = alm_timer_rep_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


REGISTER_UI_EVENT_HANDLER(ALM_THU)
.onchange = alm_timer_rep_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(ALM_FRI)
.onchange = alm_timer_rep_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(ALM_SAT)
.onchange = alm_timer_rep_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int alm_timer_sw_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        if (alm_set_table[3].time_value) {
            ui_text_set_index(text, 1);
        } else {
            ui_text_set_index(text, 0);
        }
        break;
    case ON_CHANGE_SHOW_POST:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}


REGISTER_UI_EVENT_HANDLER(ALM_SW)
.onchange = alm_timer_sw_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

#endif //#if (TCFG_APP_RTC_EN)
#endif
