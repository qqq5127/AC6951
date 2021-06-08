#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_api.h"
#include "app_task.h"
#include "system/timer.h"
#include "key_event_deal.h"
#include "audio_config.h"
#include "jiffies.h"
#include "btstack/avctp_user.h"
#include "app_power_manage.h"
#include "asm/charge.h"
#ifndef CONFIG_MEDIA_NEW_ENABLE
#include "application/audio_eq_drc_apply.h"
#else
#include "audio_eq.h"
#endif


#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))

#define STYLE_NAME  JL

REGISTER_UI_STYLE(STYLE_NAME)

extern void key_ui_takeover(u8 on);
extern u8 bt_sco_state(void);
extern void power_off_deal(struct sys_event *event, u8 step);
static int music_vol_handler(const char *type, u32 arg);

static int bt_status_handler(const char *type, u32 arg)
{
    if (ui_get_disp_status_by_id(BT_MENU_LAYOUT) == true) {
        return -1;
    }

    if (ui_get_disp_status_by_id(BT_VOL_LAYOUT) == true) {
        return -1;
    }

    if (type && (!strcmp(type, "event"))) {
        switch (arg) {
        case BT_STATUS_SECOND_CONNECTED:
        case BT_STATUS_FIRST_CONNECTED:
            printf("%s %d\n", __func__, __LINE__);
            ui_pic_show_image_by_id(BT_STATUS_PIC, 1);
            ui_text_show_index_by_id(BT_STATUS_TEXT, 1);
            break;
        case BT_STATUS_FIRST_DISCONNECT:
        case BT_STATUS_SECOND_DISCONNECT:
            printf("%s %d\n", __func__, __LINE__);
            ui_pic_show_image_by_id(BT_STATUS_PIC, 0);
            ui_text_show_index_by_id(BT_STATUS_TEXT, 0);
            break;
        case BT_STATUS_A2DP_MEDIA_START:
            printf("%s %d\n", __func__, __LINE__);
            ui_pic_show_image_by_id(BT_STATUS_PIC, 1);
            ui_text_show_index_by_id(BT_STATUS_TEXT, 2);
            break;
        case BT_STATUS_A2DP_MEDIA_STOP:
            printf("%s %d\n", __func__, __LINE__);
            ui_pic_show_image_by_id(BT_STATUS_PIC, 1);
            ui_text_show_index_by_id(BT_STATUS_TEXT, 1);
            break;

        }
    }
    return 0;
}




/************************************************************
    窗口app与ui的消息交互注册 app可以发生消息到ui进行显示
 ************************************************************/

static const struct uimsg_handl ui_msg_handler[] = {
    { "bt_status",        bt_status_handler     }, /* 设置音量 */
    { "music_vol",        music_vol_handler     }, /* 显示音量 */
    { NULL, NULL},      /* 必须以此结尾！ */
};




/************************************************************
                         蓝牙主页窗口控件
              可以在这个窗口注册各个布局需要用的资源
 ************************************************************/


static int bt_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    static int bt_status_timer = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***!!!!!!!!!!!! >>>>>>>>>>>>>>>>>>bt_mode_onchange***\n");
        ui_register_msg_handler(ID_WINDOW_BT, ui_msg_handler);//注册消息交互的回调
        /*
          * 注册APP消息响应
          */
        break;
    case ON_CHANGE_RELEASE:

        if (bt_status_timer) {
            sys_timeout_del(bt_status_timer);
            bt_status_timer = 0;
        }
        break;
    default:
        return false;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(ID_WINDOW_BT)
.onchange = bt_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};



static void bt_status_check(void *p)
{
    u8 status = get_bt_connect_status();
    u8 eq_mode = 0;
    if (status ==  BT_STATUS_WAITINT_CONN) {
        ui_pic_show_image_by_id(BT_STATUS_PIC, 0);
        ui_text_show_index_by_id(BT_STATUS_TEXT, 0);
    } else if (status == BT_STATUS_PLAYING_MUSIC) {
        ui_pic_show_image_by_id(BT_STATUS_PIC, 1);
        ui_text_show_index_by_id(BT_STATUS_TEXT, 2);
    } else {
        ui_pic_show_image_by_id(BT_STATUS_PIC, 1);
        ui_text_show_index_by_id(BT_STATUS_TEXT, 1);
    }
#if TCFG_EQ_ENABLE
    eq_mode = eq_mode_get_cur();
    ui_pic_show_image_by_id(BT_EQ, eq_mode);
#endif
}

static int bt_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        break;

    case ON_CHANGE_FIRST_SHOW:
        ui_set_call(bt_status_check, 0);
        break;

    case ON_CHANGE_SHOW_POST:
        //有显示动作
        break;

    case ON_CHANGE_RELEASE:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}



static int bt_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("bt_layout_onkey %d\n", e->value);
    switch (e->value) {
    case KEY_MENU:
        if (ui_get_disp_status_by_id(BT_MENU_LAYOUT) <= 0) {
            printf("BT_MENU_LAYOUT\n");
            ui_show(BT_MENU_LAYOUT);
        }
        break;
    case KEY_UP:
        r_printf("BT_MUSIC_PREV\n");
        app_task_put_key_msg(KEY_MUSIC_PREV, 0);
        break;
    case KEY_DOWN:
        r_printf("BT_MUSIC_NEXT\n");
        app_task_put_key_msg(KEY_MUSIC_NEXT, 0);
        break;
    case KEY_VOLUME_INC:
    case KEY_VOLUME_DEC:
        if (ui_get_disp_status_by_id(BT_VOL_LAYOUT) <= 0) {
            ui_show(BT_VOL_LAYOUT);
        }
        break;
    case KEY_OK:
        r_printf("BT_MUSIC_PP\n");
        app_task_put_key_msg(KEY_MUSIC_PP, 0);
        break;
    case KEY_MODE:
        ui_hide_main(ID_WINDOW_BT);
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
    return TRUE;
}




REGISTER_UI_EVENT_HANDLER(BT_LAYOUT)
.onchange = bt_layout_onchange,
 .onkey = bt_layout_onkey,
  .ontouch = NULL,
};


static u16 bt_timer = 0;
static void bt_vol_timeout(void *p)
{
    int id = (int)(p);
    if (ui_get_disp_status_by_id(id) == TRUE) {
        ui_hide(id);
    }
    bt_timer = 0;
}

static int music_vol_handler(const char *type, u32 arg)
{
    struct unumber num;
    if (type && (!strcmp(type, "vol"))) {
        if (ui_get_disp_status_by_id(BT_VOL_LAYOUT) == TRUE) {
            num.type = TYPE_NUM;
            num.numbs =  1;
            num.number[0] = arg;
            ui_number_update_by_id(BT_VOL_NUM, &num);
        }
    }
    return 0;
}



static void vol_init(int id)
{
    struct unumber num;
    num.type = TYPE_NUM;
    num.numbs =  1;
    num.number[0] = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
    ui_number_update_by_id(BT_VOL_NUM, &num);
}

static int bt_vol_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        if (!bt_timer) {
            bt_timer  = sys_timeout_add((void *)layout->elm.id, bt_vol_timeout, 3000);
        }
        layout_on_focus(layout);//设置焦点在本布局(如果布局下还有列表，焦点会最后显示出的列表这里，防止焦点不在，可以把焦点控制在本布局)
        break;

    case ON_CHANGE_FIRST_SHOW:
        ui_set_call(vol_init, 0);
        break;

    case ON_CHANGE_SHOW_POST:
        //有显示动作
        break;

    case ON_CHANGE_RELEASE:
        if (bt_timer) {
            sys_timeout_del(bt_timer);
            bt_timer = 0;
        }
        layout_lose_focus(layout);//失去本焦点
        break;
    default:
        return FALSE;
    }
    return FALSE;
}




static int bt_vol_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("bt_vol_layout_onkey %d\n", e->value);

    if (bt_timer) {
        sys_timer_modify(bt_timer, 3000);
    }

    switch (e->value) {
    case KEY_MENU:
        break;
    case KEY_UP:
    case KEY_VOLUME_INC:
        app_task_put_key_msg(KEY_VOL_UP, 0);
        break;
    case KEY_DOWN:
    case KEY_VOLUME_DEC:
        app_task_put_key_msg(KEY_VOL_DOWN, 0);
        break;
    default:
        return false;
        break;
    }
    return true;
}



REGISTER_UI_EVENT_HANDLER(BT_VOL_LAYOUT)
.onchange = bt_vol_layout_onchange,
 .onkey = bt_vol_layout_onkey,
  .ontouch = NULL,
};



static int bt_menu_list_onkey(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int sel_item = 0;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    switch (e->value) {
    case KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
            ui_show(BT_MENU_EQ_LAYOUT);
            break;
        case 1:
            ui_hide(BT_MENU_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}



REGISTER_UI_EVENT_HANDLER(BT_MENU_LIST)
.onchange = NULL,
 .onkey = bt_menu_list_onkey,
  .ontouch = NULL,
};



static int bt_eq_menu_list_onkey(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int sel_item = 0;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
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
            ui_pic_show_image_by_id(BT_EQ, sel_item);
#endif
        case 6:
            ui_hide(BT_MENU_EQ_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}


static const int eq_mode_pic[] = {
    BT_EQ_NORMAL_PIC,
    BT_EQ_ROCK_PIC,
    BT_EQ_POP_PIC,
    BT_EQ_CLASSIC_PIC,
    BT_EQ_JAZZ_PIC,
    BT_EQ_COUNTRY_PIC,
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

REGISTER_UI_EVENT_HANDLER(BT_EQ_NORMAL_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(BT_EQ_ROCK_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(BT_EQ_POP_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(BT_EQ_CLASSIC_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(BT_EQ_JAZZ_PIC)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(BT_EQ_COUNTRY_PIC)
.onchange = eq_pic_common_onchange,
};

#endif




static int bt_eq_menu_list_onchange(void *ctr, enum element_change_event e, void *arg)
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



REGISTER_UI_EVENT_HANDLER(BT_EQ_MENU_LIST)
.onchange = bt_eq_menu_list_onchange,
 .onkey = bt_eq_menu_list_onkey,
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
REGISTER_UI_EVENT_HANDLER(BT_BAT)
.onchange = battery_onchange,
 .ontouch = NULL,
};



#endif
