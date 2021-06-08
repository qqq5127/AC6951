#include "ui/ui_api.h"
#include "fm/fm_manage.h"

#if TCFG_APP_FM_EN
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))
void ui_fm_temp_finsh(u8 menu)//子菜单被打断或者显示超时
{
    switch (menu) {
    default:
        break;
    }
}

static void led7_show_fm(void *hd)
{


    LCD_API *dis = (LCD_API *)hd;

    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" FM");
    dis->lock(0);
}


static void led7_fm_show_freq(void *hd, u32 arg)
{
    u8 bcd_number[5] = {0};
    LCD_API *dis = (LCD_API *)hd;
    u16 freq = arg;

    if (freq > 1080 && freq <= 1080 * 10) {
        freq = freq / 10;
    }


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
    dis->show_icon(LED7_FM);
    dis->lock(0);
}

static void led7_fm_show_station(void *hd, u32 arg)
{
    u8 bcd_number[5] = {0};
    LCD_API *dis = (LCD_API *)hd;
    u16 freq = arg;

    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    sprintf((char *)bcd_number, "P%03d", arg);
    dis->show_string(bcd_number);
    dis->lock(0);
}


static void *ui_open_fm(void *hd)
{
    return NULL;
}

static void ui_close_fm(void *hd, void *private)
{
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return ;
    }
    if (private) {
        free(private);
    }
}

static void ui_fm_main(void *hd, void *private) //主界面显示
{
    if (!hd) {
        return;
    }
    u16 fre = fm_manage_get_fre();
    if (fre != 0) {
        led7_fm_show_freq(hd, fre);
    }
}


static int ui_fm_user(void *hd, void *private, u8 menu, u32 arg)//子界面显示 //返回true不继续传递 ，返回false由common统一处理
{
    int ret = true;
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return false;
    }

    switch (menu) {
    case MENU_FM_STATION:
        led7_fm_show_station(hd, arg);
        break;
    default:
        ret = false;
    }

    return ret;

}



const struct ui_dis_api fm_main = {
    .ui      = UI_FM_MENU_MAIN,
    .open    = ui_open_fm,
    .ui_main = ui_fm_main,
    .ui_user = ui_fm_user,
    .close   = ui_close_fm,
};


#endif
#endif
