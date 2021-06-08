#include "includes.h"
#include "ui/ui_api.h"
#include "fm_emitter/fm_emitter_manage.h"

#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))
static void led7_show_hi(void *hd)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" HI");
    dis->lock(0);
}

static void led7_show_volume(void *hd, u8 vol)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_char(' ');
    dis->show_char('V');
    dis->show_number(vol / 10);
    dis->show_number(vol % 10);
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

static void led7_show_bt(void *hd)
{

    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" bt");
    dis->lock(0);
}

#if TCFG_APP_FM_EMITTER_EN
static void led7_fm_ir_set_freq(void *hd, u16 freq)
{

    LCD_API *dis = (LCD_API *)hd;
    u8 bcd_number[5] = {0};	  ///<换算结果显示缓存
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    sprintf((char *)bcd_number, "%4d", freq);
    /* itoa4(freq,bcd_number); */
    if (freq > 1080) {
        dis->show_string((u8 *)" Err");
    } else if (freq >= 875) {
        dis->show_string(bcd_number);
        /* os_time_dly(100); */
        fm_emitter_manage_set_fre(freq);
        UI_REFLASH_WINDOW(TRUE);//设置回主页
    } else {
        dis->FlashChar(BIT(0) | BIT(1) | BIT(2) | BIT(3)); //设置闪烁
        dis->show_string(bcd_number);
    }
    dis->lock(0);

}
#endif


static void led7_fm_set_freq(void *hd, u32 arg)
{
    u8 bcd_number[5] = {0};
    LCD_API *dis = (LCD_API *)hd;
    u16 freq = 0;
    freq = arg;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->FlashChar(BIT(0) | BIT(1) | BIT(2) | BIT(3)); //设置闪烁
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



void ui_common(void *hd, void *private, u8 menu, u32 arg)//公共显示
{

    u16 fre = 0;

    if (!hd) {
        return;
    }

    switch (menu) {
    case MENU_POWER_UP:
        led7_show_hi(hd);
        break;
    case MENU_MAIN_VOL:
        led7_show_volume(hd, arg & 0xff);
        break;
    case MENU_WAIT:
        led7_show_wait(hd);
        break;
    case MENU_BT:
        led7_show_bt(hd);
        break;
    case MENU_IR_FM_SET_FRE:
#if TCFG_APP_FM_EMITTER_EN
        led7_fm_ir_set_freq(hd, arg);
#endif
        break;
    case MENU_FM_SET_FRE:

#if TCFG_APP_FM_EMITTER_EN
        fre = fm_emitter_manage_get_fre();
        led7_fm_set_freq(hd, arg);
#endif
        break;
    default:
        break;
    }
}

#endif
