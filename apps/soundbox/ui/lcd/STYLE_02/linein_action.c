#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_style.h"
#include "ui/ui_api.h"
#include "audio_config.h"
#include "audio_enc/audio_enc.h"
#include "app_power_manage.h"
#include "app_task.h"
#include "key_event_deal.h"
#include "linein/linein.h"
#include "asm/charge.h"
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

/************************************************************
                        linein模式主页窗口控件
************************************************************/
static int linein_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***linein_mode_onchange***\n");
        /* key_ui_takeover(1); */
        break;
    case ON_CHANGE_RELEASE:
        puts("\n***linein_mode_release***\n");
        break;
    default:
        return false;
    }
    return false;
}


REGISTER_UI_EVENT_HANDLER(ID_WINDOW_LINEIN)
.onchange = linein_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

/************************************************************
                         linein模式布局控件按键事件
 ************************************************************/
static void linein_ui_first_show()
{
    u8 eq_mode = 0;
    ui_pic_show_image_by_id(LINE_IN_PIC, 0);
    ui_text_show_index_by_id(LINE_IN_TEXT, 0);
    ui_no_highlight_element_by_id(LINE_IN_STATUS);
#if TCFG_EQ_ENABLE
    eq_mode = eq_mode_get_cur();
    ui_pic_show_image_by_id(LINE_IN_EQ, eq_mode);
#endif
}

static int linein_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_SHOW_PROBE:
        break;
    case ON_CHANGE_FIRST_SHOW:
        ui_set_call(linein_ui_first_show, 0);
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

static int linein_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("record_layout_onkey %d\n", e->value);
    switch (e->value) {
    case KEY_MENU:
        ui_show(LINE_IN_MENU_LAYOUT);
        break;
    case KEY_OK:
        app_task_put_key_msg(KEY_MUSIC_PP, 0);
        if (linein_get_status()) { //显示暂停开始图标
            ui_highlight_element_by_id(LINE_IN_STATUS);
        } else {
            ui_no_highlight_element_by_id(LINE_IN_STATUS);
        }
        break;
    case KEY_UP:
        break;
    case KEY_DOWN:
        break;
    case KEY_VOLUME_INC:
    case KEY_VOLUME_DEC:
        ui_show(LINE_IN_VOL_LAYOUT);
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

REGISTER_UI_EVENT_HANDLER(LINE_IN_LAYOUT)
.onchange = linein_layout_onchange,
 .onkey = linein_layout_onkey,
  .ontouch = NULL,
};

/************************************************************
                         LINEIN模式主菜单列表按键事件
 ************************************************************/

static int linein_enter_callback(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    switch (e->value) {
    case KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
            ui_show(LINE_IN_EQ_LAYOUT);
            break;
        case 1:
            ui_hide(LINE_IN_MENU_LAYOUT);
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

REGISTER_UI_EVENT_HANDLER(LINE_IN_MENU_LIST)
.onchange = NULL,
 .onkey = linein_enter_callback,
  .ontouch = NULL,
};

/************************************************************
                         LINEIN模式eq菜单列表按键事件
 ************************************************************/

static const int eq_mode_pic[] = {
    LINE_IN_EQ_NORMAL_PIC,
    LINE_IN_EQ_ROCK_PIC,
    LINE_IN_EQ_POP_PIC,
    LINE_IN_EQ_CLASSIC_PIC,
    LINE_IN_EQ_JAZZ_PIC,
    LINE_IN_EQ_COUNTRY_PIC,
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

REGISTER_UI_EVENT_HANDLER(LINE_IN_EQ_NORMAL_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(LINE_IN_EQ_ROCK_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(LINE_IN_EQ_POP_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(LINE_IN_EQ_CLASSIC_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(LINE_IN_EQ_JAZZ_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(LINE_IN_EQ_COUNTRY_PIC)
.onchange = eq_pic_common_onchange,
};
#endif

static int linein_eq_menu_list_onchange(void *ctr, enum element_change_event e, void *arg)
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

static int linein_eq_callback(void *ctr, struct element_key_event *e)
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
            ui_pic_show_image_by_id(LINE_IN_EQ, sel_item);
#endif
        case 6:
            ui_hide(LINE_IN_EQ_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}


REGISTER_UI_EVENT_HANDLER(LINE_IN_EQ_MENU_LIST)
.onchange = linein_eq_menu_list_onchange,
 .onkey = linein_eq_callback,
  .ontouch = NULL,
};

/************************************************************
                         音量设置布局控件
 ************************************************************/
static u16 vol_timer = 0;
static void linein_vol_lay_timeout(void *p)
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
    ui_number_update_by_id(LINE_IN_VOL_NUM, &num);
}

static int linein_vol_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        if (!vol_timer) {
            vol_timer  = sys_timer_add((void *)layout->elm.id, linein_vol_lay_timeout, 3000);
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


static int linein_vol_layout_onkey(void *ctr, struct element_key_event *e)
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
        linein_key_vol_up();
        //app_audio_volume_up(1);
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(LINE_IN_VOL_NUM, &num);
        break;
    case KEY_DOWN:
    case KEY_VOLUME_DEC:
        linein_key_vol_down();
        // app_audio_volume_down(1);
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(LINE_IN_VOL_NUM, &num);
        break;
    default:
        return false;
        break;
    }
    return true;
}

REGISTER_UI_EVENT_HANDLER(LINE_IN_VOL_LAYOUT)
.onchange = linein_vol_layout_onchange,
 .onkey = linein_vol_layout_onkey,
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
    if (1 || get_charge_online_flag()) { //充电时候图标动态效果,get_charge_online_flag()在内置充电的时候获取是否正在充电,外置充电根据电路来编写接口
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
REGISTER_UI_EVENT_HANDLER(LINE_IN_BAT)
.onchange = battery_onchange,
 .ontouch = NULL,
};

#endif
