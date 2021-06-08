#include "system/includes.h"
#include "ui/ui_api.h"
#include "fm_emitter/fm_emitter_manage.h"

#if TCFG_APP_LINEIN_EN
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))

void ui_linein_temp_finsh(u8 menu)//子菜单被打断或者显示超时
{
    switch (menu) {
    default:
        break;
    }
}

static void led7_show_aux(void *hd)
{

    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" AUX");
    dis->lock(0);
}


static void led7_fm_show_freq(void *hd, u32 arg)
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

static void led7_show_pause(void *hd)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" PAU");
    dis->lock(0);
}

static void *ui_open_linein(void *hd)
{
    /* ui_set_auto_reflash(500);//设置主页500ms自动刷新 */
    return NULL;
}

static void ui_close_linein(void *hd, void *private)
{
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return ;
    }
    if (private) {
        free(private);
    }
}

static void ui_linein_main(void *hd, void *private) //主界面显示
{
    if (!hd) {
        return;
    }

#if TCFG_APP_FM_EMITTER_EN
    extern u8 linein_get_status(void);
    if (linein_get_status()) {
        u16 fre = fm_emitter_manage_get_fre();
        if (fre != 0) {
            led7_fm_show_freq(hd, fre);
        } else {
            printf("ui get fmtx fre err !\n");
        }
    } else {
        led7_show_pause(hd);
    }
#else
    extern u8 linein_get_status(void);
    if (linein_get_status()) {
        led7_show_aux(hd);
    } else {
        led7_show_pause(hd);
    }
#endif
}


static int ui_linein_user(void *hd, void *private, u8 menu, u32 arg)//子界面显示 //返回true不继续传递 ，返回false由common统一处理
{
    int ret = true;
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return false;
    }
    switch (menu) {
    case MENU_AUX:
        led7_show_aux(hd);
        break;
    default:
        ret = false;
        break;
    }

    return ret;

}



const struct ui_dis_api linein_main = {
    .ui      = UI_AUX_MENU_MAIN,
    .open    = ui_open_linein,
    .ui_main = ui_linein_main,
    .ui_user = ui_linein_user,
    .close   = ui_close_linein,
};






#endif
#endif
