#include "includes.h"
#include "ui/ui_api.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "btstack/avctp_user.h"

#if (TCFG_APP_BT_EN)

#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))

static void led7_show_bt(void *hd)
{

    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" bt");
    dis->lock(0);
}

static void led7_show_call(void *hd)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" CAL");
    dis->lock(0);
}



static void led7_fm_show_freq(void *hd, void *private, u32 arg)
{
    u8 bcd_number[5] = {0};
    LCD_API *dis = (LCD_API *)hd;
    u16 freq = 0;
    freq = arg;

    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    itoa4(freq, (u8 *)bcd_number);
    if (freq > 999 && freq <= 1999) {
        bcd_number[0] = '1';
    } else {
        bcd_number[0] = ' ';
    }
    dis->show_string(bcd_number);
    dis->show_icon(LED7_DOT);
    dis->lock(0);
}


static void led7_show_wait(void *hd)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" Lod");
    dis->lock(0);
}



static void *ui_open_bt(void *hd)
{
    void *private = NULL;
    ui_set_auto_reflash(500);//设置主页500ms自动刷新
    return private;
}

static void ui_close_bt(void *hd, void *private)
{
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return;
    }
    if (private) {
        free(private);
    }
}

static void ui_bt_main(void *hd, void *private) //主界面显示
{
    if (!hd) {
        return;
    }

#if TCFG_APP_FM_EMITTER_EN

    if (BT_STATUS_TAKEING_PHONE == get_bt_connect_status()) {
        led7_show_call(hd);
    } else {
        u16 fre = fm_emitter_manage_get_fre();
        if (fre != 0) {
            led7_fm_show_freq(hd, private, fre);
        } else {
            led7_show_wait(hd);
        }
    }
#else
    if (BT_STATUS_TAKEING_PHONE == get_bt_connect_status()) {
        led7_show_call(hd);
    } else {
        led7_show_bt(hd);
    }
#endif
}


static int ui_bt_user(void *hd, void *private, u8 menu, u32 arg)//子界面显示 //返回true不继续传递 ，返回false由common统一处理
{
    int ret = true;
    LCD_API *dis = (LCD_API *)hd;
    if (!hd) {
        return false;
    }

    switch (menu) {
    case MENU_BT:
        led7_show_bt(hd);
        break;

    default:
        ret = false;
    }

    return ret;

}

const struct ui_dis_api bt_main = {
    .ui      = UI_BT_MENU_MAIN,
    .open    = ui_open_bt,
    .ui_main = ui_bt_main,
    .ui_user = ui_bt_user,
    .close   = ui_close_bt,
};



#endif
#endif
