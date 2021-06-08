#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_style.h"
#include "ui/ui_api.h"
#include "app_task.h"
#include "system/timer.h"
#include "key_event_deal.h"
#include "asm/charge.h"
#include "record/record.h"
#include "encode/encode_write.h"
#include "audio_config.h"
#include "audio_enc/audio_enc.h"
#include "app_power_manage.h"
#include "audio_dec/audio_dec_file.h"
#include "audio_enc/audio_enc.h"
#ifndef CONFIG_MEDIA_NEW_ENABLE
#include "application/audio_eq_drc_apply.h"
#else
#include "audio_eq.h"
#endif

#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))

#define STYLE_NAME  JL

extern int ui_hide_main(int id);
extern int ui_show_main(int id);
extern void key_ui_takeover(u8 on);
extern void power_off_deal(struct sys_event *event, u8 step);

extern const char *music_file_get_cur_name(int *len, int *is_unicode);

typedef struct _RECORD_UI_OPR {
    u8 start: 1;
    u8 menu: 1;
    u8 rec_play: 1;
    u8 eq_mode : 3;
    int timer;
    int cur_num;
    u32 second;
} RECORD_UI_OPR;

static RECORD_UI_OPR *__this = NULL;
char path[64] = {0};//用于获取录音文件的路径

/************************************************************
                        获取录音文件的名字
************************************************************/
char *rec_get_filename()
{
    char *name = path;
    int ret, len;
    memset(path, 0, 64);
    ret = last_enc_file_path_get(path);
    if (ret) {
        return NULL;
    }
    len = strlen(path);

    while (len--) {
        if (*name++ == '.') {
            name -= 9;
            break;
        }
    }
    if (len <= 0) {
        return NULL;
    }
    len = strlen(name) + 1;
    printf("\n--func=%s, line=%d,record_action--len:%d,name:%s\n", __FUNCTION__, __LINE__, len, name);
    return name;
}
/************************************************************
                      录音模式UI消息处理函数
************************************************************/
static int record_start(const char *type, u32 arg)
{
    const char *file_name = NULL;
    int len = 0;
    printf("__FUNCTION__ =%s type =%s %d\n", __FUNCTION__, type, arg);

    if (type && !strcmp(type, "first_show")) {
        if (arg) {
            ui_text_set_text_by_id(REC_FILE_NAME, "------", 6, FONT_DEFAULT);
        }
        return 0;
    }

    if (type && !strcmp(type, "show_filename")) {
        file_name = rec_get_filename();
        len = strlen(file_name) + 1;
        if (len && file_name) {
            printf("\n--func=%s, line=%d,len:%d,file_name:%s\n", __FUNCTION__, __LINE__, len, file_name);
            ui_text_set_text_by_id(REC_FILE_NAME, file_name, len, FONT_DEFAULT);
        }
        return 0;
    }

    if (type && !strcmp(type, "dev")) {
        if (arg && (!strncmp((char *)arg, "udisk", 5))) {
            ui_pic_show_image_by_id(REC_DEV, 0); //显示udik
        } else {
            ui_pic_show_image_by_id(REC_DEV, 1); //显示sd
        }
        return 0;
    }

    return 0;
}

static const struct uimsg_handl ui_msg_handler[] = {
    { "record_start", record_start }, /* */
    { NULL, NULL},      /* 必须以此结尾！ */
};

static void record_check_add(void *ptr);
/************************************************************
                        录音模式主页窗口控件
************************************************************/
static int record_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***record_mode_onchange***\n");
        /* key_ui_takeover(1); */
        if (!__this) {
            __this = zalloc(sizeof(RECORD_UI_OPR));
        }
        ui_register_msg_handler(ID_WINDOW_REC, ui_msg_handler);
        /*
         * 注册APP消息响应
         */
        UI_MSG_POST("record_start:first_show=%1", 1); //录音模式上电显示空白路径
        if (!__this->timer) {
            __this->timer = sys_timer_add(NULL, record_check_add, 500);
        }
        break;
    case ON_CHANGE_RELEASE:
        puts("\n***record_mode_release***\n");
        if (__this->timer) {
            sys_timer_del(__this->timer);
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


REGISTER_UI_EVENT_HANDLER(ID_WINDOW_REC)
.onchange = record_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


/* extern void test_browser(); */
/************************************************************
                         录音模式布局控件按键事件
 ************************************************************/
static void record_layout_init(int id)
{
    ;
}

static int record_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_SHOW_PROBE:
        break;
    case ON_CHANGE_FIRST_SHOW:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

static int record_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("record_layout_onkey %d\n", e->value);
    switch (e->value) {
    case KEY_MENU:
        ui_show(REC_MENU_LAYOUT);
        break;
    case KEY_OK:
        /* music_play_file_pp(); */
        break;
    case KEY_ENC:
        app_task_put_key_msg(KEY_ENC_START, 0);
        break;
    case KEY_UP:
        break;
    case KEY_DOWN:
        break;
    case KEY_VOLUME_INC:
    case KEY_VOLUME_DEC:
        ui_show(REC_VOL_LAYOUT);
        break;
    case KEY_MODE:
        UI_HIDE_CURR_WINDOW();
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

REGISTER_UI_EVENT_HANDLER(REC_LAYOUT)
.onchange = record_layout_onchange,
 .onkey = record_layout_onkey,
  .ontouch = NULL,
};


/************************************************************
                         录音模式的定时器
 ************************************************************/
static void record_check_add(void *ptr)
{
    struct utime time_r;
    char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));//最后活动设备

    if (!__this) {
        return;
    }

#if TCFG_EQ_ENABLE
    if (eq_mode_get_cur() != __this->eq_mode) {
        __this->eq_mode = eq_mode_get_cur();
        ui_pic_show_image_by_id(REC_EQ, __this->eq_mode);
    }
#endif

    if (true == recorder_is_encoding()) { //正在录音
        if (!__this->start) { //刚开始进行录音或者总系统设置出来后显示录音盘符,名字,正在录音文本以及播放标志
            ui_highlight_element_by_id(REC_START);
            ui_text_show_index_by_id(REC_TEXT, 1);
            UI_MSG_POST("record_start:show_filename=%2:dev=%2", 1, logo);
            __this->start  = 1;
            __this->second = 0;
        }

        if (__this->start == 1) { //显示录音时间
            __this->second = recorder_get_encoding_time();
        }
        time_r.hour = __this->second / 60 / 60;
        time_r.min = __this->second / 60 % 60;
        time_r.sec = __this->second % 60;
        ui_time_update_by_id(REC_TIME, &time_r);
    } else {//录音回放,回放结束以及还没开始录音处理
        if (__this->start == 1 || (file_dec_is_play() && !__this->rec_play)) { //回放时显示录音盘符,名字,正在录音文本以及播放标志,从系统设置菜单返回也跑这里
            ui_highlight_element_by_id(REC_START);
            ui_text_show_index_by_id(REC_TEXT, 1);
            UI_MSG_POST("record_start:show_filename=%2:dev=%2", 1, logo);
            __this->start = 0;
            __this->second = 0;
            __this->rec_play = 1;
            ui_text_show_index_by_id(REC_TEXT, 2);
        }
        if (__this->rec_play == 1) { //正在回放
            __this->second = file_dec_get_cur_time();
            if (file_dec_is_stop()) { //回放结束
                __this->rec_play = 0;
                __this->second = 0;
            }
            time_r.hour = __this->second / 60 / 60;
            time_r.min = __this->second / 60 % 60;
            time_r.sec = __this->second % 60;
            ui_time_update_by_id(REC_TIME, &time_r);
        } else { //回放结束或者还没开始录音处理
            ui_text_show_index_by_id(REC_TEXT, 0);
            ui_no_highlight_element_by_id(REC_START);
        }
    }
}

/************************************************************
                         录音模式的一些控件初始化事件
 ************************************************************/

static int record_timer_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_time *timer = (struct ui_time *)ctr;
    int sencond = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        timer->hour = sencond / 60 / 60;
        timer->min = sencond / 60 % 60;
        timer->sec = sencond % 60;
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(REC_TIME)
.onchange = record_timer_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

/************************************************************
                         录音模式主菜单列表按键事件
 ************************************************************/
static int record_enter_callback(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    switch (e->value) {
    case KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
            ui_show(REC_EQ_LAYOUT);
            break;
        case 1:
            ui_hide(REC_MENU_LAYOUT);
            break;
        default:
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}

REGISTER_UI_EVENT_HANDLER(REC_MENU_LIST)
.onchange = NULL,
 .onkey = record_enter_callback,
  .ontouch = NULL,
};

/************************************************************
                         录音模式eq菜单列表按键事件
 ************************************************************/

static const int eq_mode_pic[] = {
    REC_EQ_NORMAL_PIC,
    REC_EQ_ROCK_PIC,
    REC_EQ_POP_PIC,
    REC_EQ_CLASSIC_PIC,
    REC_EQ_JAZZ_PIC,
    REC_EQ_COUNTRY_PIC,
};

#if TCFG_EQ_ENABLE
static int eq_pic_common_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;
    switch (event) {
    case ON_CHANGE_INIT:
        if (pic->elm.id == eq_mode_pic[eq_mode_get_cur() % 6]) {
            ui_pic_set_image_index(pic, 1);
        }
        break;
    case ON_CHANGE_SHOW_POST:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(REC_EQ_NORMAL_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(REC_EQ_ROCK_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(REC_EQ_POP_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(REC_EQ_CLASSIC_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(REC_EQ_JAZZ_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(REC_EQ_COUNTRY_PIC)
.onchange = eq_pic_common_onchange,
};
#endif

static int record_eq_menu_list_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int list = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        printf("ON_CHANGE_INIT %d \n", grid->avail_item_num);
        break;
    }
    return false;
}

static int record_eq_callback(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    switch (e->value) {
    case KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
#if TCFG_EQ_ENABLE
            eq_mode_set(sel_item);
#endif
        case 6:
            ui_hide(REC_EQ_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}


REGISTER_UI_EVENT_HANDLER(REC_EQ_MENU_LIST)
.onchange = record_eq_menu_list_onchange,
 .onkey = record_eq_callback,
  .ontouch = NULL,
};



/************************************************************
                         音量设置布局控件
 ************************************************************/
static u16 vol_timer = 0;
static void record_vol_lay_timeout(void *p)
{
    int id = (int)(p);
    if (ui_get_disp_status_by_id(id) == TRUE) {
        ui_hide(id);
        //ui_show(CLOCK_LAYOUT);
    }
    vol_timer = 0;
}


static void vol_init(int id)
{
    struct unumber num;
    num.type = TYPE_NUM;
    num.numbs =  1;
    num.number[0] = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
    ui_number_update_by_id(REC_VOL_NUM, &num);
}

static int record_vol_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        if (!vol_timer) {
            vol_timer  = sys_timer_add((void *)layout->elm.id, record_vol_lay_timeout, 3000);
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


static int record_vol_layout_onkey(void *ctr, struct element_key_event *e)
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
        ui_number_update_by_id(REC_VOL_NUM, &num);
        break;
    case KEY_DOWN:
    case KEY_VOLUME_DEC:
        app_audio_volume_down(1);
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(REC_VOL_NUM, &num);
        break;
    default:
        return false;
        break;
    }
    return true;
}

REGISTER_UI_EVENT_HANDLER(REC_VOL_LAYOUT)
.onchange = record_vol_layout_onchange,
 .onkey = record_vol_layout_onkey,
  .ontouch = NULL,
};

/************************************************************
                         电池控件事件
 ************************************************************/

static void battery_timer(void *priv)
{
    int  incharge = 0;//充电标志
    int id = (int)(priv);
    static u8 percent = 0;
    static u8 percent_last = 0;
    if (1 || get_charge_online_flag()) { //充电时候图标动态效果 get_charge_online_flag()在内置充电的时候获取是否正在充电,外置充电根据电路来编写接口
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

REGISTER_UI_EVENT_HANDLER(REC_BAT)
.onchange = battery_onchange,
 .ontouch = NULL,
};

#endif
