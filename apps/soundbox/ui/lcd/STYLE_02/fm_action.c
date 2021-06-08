#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_style.h"
#include "ui/ui_api.h"
#include "app_task.h"
#include "system/timer.h"
#include "key_event_deal.h"
#include "audio_config.h"
#include "jiffies.h"
#include "fm/fm_api.h"
#include "fm/fm_rw.h"
#include "app_power_manage.h"
#include "asm/charge.h"
#ifndef CONFIG_MEDIA_NEW_ENABLE
#include "application/audio_eq_drc_apply.h"
#else
#include "audio_eq.h"
#endif




#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))
#if (TCFG_APP_FM_EN)

#define STYLE_NAME  JL

extern int ui_hide_main(int id);
extern int ui_show_main(int id);
extern void key_ui_takeover(u8 on);
extern int fm_server_msg_post(int msg);
extern void fm_volume_up();
extern void fm_volume_down();
extern void power_off_deal(struct sys_event *event, u8 step);

extern u8 fm_get_cur_channel(void);

static int fm_fre_handler(const char *type, u32 arg)
{
    printf(">>%s\n", __FUNCTION__);
    u16 fre = fm_manage_get_fre();//8750

    struct unumber num;
    num.type = TYPE_NUM;
    num.numbs =  2;
    num.number[0] = fm_get_cur_channel();
    num.number[1] = get_total_mem_channel();
    ui_number_update_by_id(FM_CH, &num);

    num.numbs =  2;
    num.number[0] = fre / 100 % 1000;
    num.number[1] = fre / 10 % 10;
    ui_number_update_by_id(FM_FREQ, &num);
    //显示进度图标是从87开始
    ui_slider_set_persent_by_id(FM_SLIDER, (fre - 8700) * 100 / (REAL_FREQ_MAX - 8700));

    return 0;
}

static const struct uimsg_handl ui_msg_handler[] = {
    { "fm_fre",        fm_fre_handler     }, /* 设置fm频率显示*/
    { NULL, NULL},      /* 必须以此结尾！ */
};


static int fm_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***fm_mode_onchange***\n");
        /* key_ui_takeover(1); */
        ui_register_msg_handler(ID_WINDOW_FM, ui_msg_handler);
        /*
         * 注册APP消息响应
         */
        break;
    case ON_CHANGE_RELEASE:
        /*
         * 要隐藏一下系统菜单页面，防止在系统菜单插入USB进入USB页面
         */
        break;
    default:
        return false;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(ID_WINDOW_FM)
.onchange = fm_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};



static void fm_layout_init(int id)
{
    u16 fre = fm_manage_get_fre();//8750

    struct unumber num;
    num.type = TYPE_NUM;
    num.numbs =  2;
    num.number[0] = fm_get_cur_channel();
    num.number[1] = get_total_mem_channel();
    ui_number_update_by_id(FM_CH, &num);

    num.numbs =  2;
    num.number[0] = fre / 100 % 1000;
    num.number[1] = fre / 10 % 10;
    ui_number_update_by_id(FM_FREQ, &num);
    //显示进度图标是从87开始
    ui_slider_set_persent_by_id(FM_SLIDER, (fre - 8700) * 100 / (REAL_FREQ_MAX - 8700));
}

static int fm_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        layout_on_focus(layout);
        ui_set_call(fm_layout_init, 0);
        break;
    case ON_CHANGE_RELEASE:
        layout_lose_focus(layout);
        break;
    case ON_CHANGE_FIRST_SHOW:
        break;
    default:
        break;
    }
    return FALSE;
}


static void fm_vol_timeout(void *p);
static int fm_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("fm_layout_onkey %d\n", e->value);
    switch (e->value) {
    case KEY_MENU:
        if (ui_get_disp_status_by_id(FM_MENU_LAYOUT) <= 0) {
            printf("FM_MENU_LAYOUT\n");
            ui_show(FM_MENU_LAYOUT);
        }
        break;
    case KEY_UP:
        app_task_put_key_msg(KEY_FM_PREV_FREQ, 0);
        break;
    case KEY_DOWN:
        app_task_put_key_msg(KEY_FM_NEXT_FREQ, 0);
        break;
    case KEY_VOLUME_INC:
    case KEY_VOLUME_DEC:
        if (ui_get_disp_status_by_id(FM_VOL_LAYOUT) <= 0) {
            ui_show(FM_VOL_LAYOUT);
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
    return true;
}




REGISTER_UI_EVENT_HANDLER(FM_LAYOUT)
.onchange = fm_layout_onchange,
 .onkey = fm_layout_onkey,
  .ontouch = NULL,
};




static int fm_menu_list_onkey(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int sel_item = 0;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    switch (e->value) {
    case KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
            app_task_put_key_msg(KEY_FM_SCAN_ALL, 0);
            ui_hide(FM_MENU_LAYOUT);
            break;
        case 1:
            /* fm_server_msg_post(FM_DEL_FREQ); */
            ui_hide(FM_MENU_LAYOUT);
            break;
        case 2:
            ui_hide(FM_MENU_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(FM_MENU_LIST)
.onchange = NULL,
 .onkey = fm_menu_list_onkey,
  .ontouch = NULL,
};



static u16 fm_timer = 0;
static void fm_vol_timeout(void *p)
{
    int id = (int)(p);
    if (ui_get_disp_status_by_id(id) == TRUE) {
        ui_hide(id);
    }
    fm_timer = 0;
}


static int fm_vol_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("bt_vol_layout_onkey %d\n", e->value);
    struct unumber num;
    u8 vol;

    if (fm_timer) {
        sys_timer_modify(fm_timer, 3000);
    }

    switch (e->value) {
    case KEY_MENU:
        break;
    case KEY_UP:
    case KEY_VOLUME_INC:
        fm_volume_up();
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(FM_VOL_NUM, &num);
        break;
    case KEY_DOWN:
    case KEY_VOLUME_DEC:
        fm_volume_down();
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(FM_VOL_NUM, &num);
        break;
    default:
        return false;
        break;
    }
    return true;
}


static void vol_init(int id)
{
    struct unumber num;
    num.type = TYPE_NUM;
    num.numbs =  1;
    num.number[0] = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
    ui_number_update_by_id(FM_VOL_NUM, &num);
}



static int fm_vol_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    struct unumber num;
    u8 vol;
    switch (e) {
    case ON_CHANGE_INIT:
        layout_on_focus(layout);
        if (!fm_timer) {
            fm_timer = sys_timeout_add((void *)FM_VOL_LAYOUT, fm_vol_timeout, 3000);
        }

        break;
    case ON_CHANGE_RELEASE:
        layout_lose_focus(layout);
        if (fm_timer) {
            sys_timeout_del(fm_timer);
            fm_timer = 0;
        }
        break;
    case ON_CHANGE_FIRST_SHOW:
        ui_set_call(vol_init, 0);
        break;
    default:
        break;
    }
    return FALSE;
}


REGISTER_UI_EVENT_HANDLER(FM_VOL_LAYOUT)
.onchange = fm_vol_layout_onchange,
 .onkey = fm_vol_layout_onkey,
  .ontouch = NULL,
};


/************************************************************
                         fm模式控件初始化
 ************************************************************/


static int fm_slider_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_slider *slider = (struct ui_slider *)ctr;
    u16 fre = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        fre = fm_manage_get_fre();//8750
        slider->persent = VIRTUAL_FREQ(fre) * 100 / MAX_CHANNEL;
        break;
    default:
        return FALSE;
    }
    return FALSE;
}



REGISTER_UI_EVENT_HANDLER(FM_SLIDER)
.onchange = fm_slider_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


static int fm_vol_num_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_number *num = (struct ui_number *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        num->nums =  1;
        num->number[0] = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(FM_VOL_NUM)
.onchange = fm_vol_num_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int fm_ch_num_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_number *num = (struct ui_number *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        num->nums =  2;
        num->number[0] = fm_get_cur_channel();
        num->number[1] = get_total_mem_channel();
        break;
    default:
        return FALSE;
    }
    return FALSE;
}



REGISTER_UI_EVENT_HANDLER(FM_CH)
.onchange = fm_ch_num_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


static int fm_freq_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_number *num = (struct ui_number *)ctr;
    u16 fre = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        fre = fm_manage_get_fre();//8750
        num->nums =  2;
        num->number[0] = fre / 100 % 1000;
        num->number[1] = fre / 10 % 10;
        break;
    default:
        return FALSE;
    }
    return FALSE;
}



REGISTER_UI_EVENT_HANDLER(FM_FREQ)
.onchange = fm_freq_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static void battery_timer(void *priv)
{
    int  incharge = 0;//充电标志
    int id = (int)(priv);
    static u8 percent = 0;
    static u8 percent_last = 0;
    if (get_charge_online_flag()) { //充电时候图标动态效果
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
REGISTER_UI_EVENT_HANDLER(FM_BAT)
.onchange = battery_onchange,
 .ontouch = NULL,
};



#endif //#if (TCFG_APP_FM_EN)
#endif
