#include "ui/ui_api.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "audio_enc/audio_enc.h"

#if TCFG_APP_RECORD_EN
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))

void ui_record_temp_finsh(u8 menu)//子菜单被打断或者显示超时
{
    switch (menu) {
    default:
        break;
    }
}

static void led7_show_record(void *hd)
{
    LCD_API *dis = (LCD_API *)hd;

    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" REC");
    dis->lock(0);
}

static void led_show_record_time(void *hd, int time)
{
    LCD_API *dis = (LCD_API *)hd;
    u8 tmp_buf[5] = {0};

    u8 Min = (u8)(time / 60 % 60);
    u8 Sec = (u8)(time % 60);
    printf("rec Min = %d, Sec = %d\n", Min, Sec);

    itoa2(Min, (u8 *)&tmp_buf[0]);
    itoa2(Sec, (u8 *)&tmp_buf[2]);

    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string(tmp_buf);
    dis->show_icon(LED7_2POINT);
    dis->lock(0);
}


#if TCFG_APP_FM_EMITTER_EN
static void led7_fm_show_freq(void *hd, u32 arg)
{
    u8 bcd_number[5] = {0};
    LCD_API *dis = (LCD_API *)hd;
    u16 freq = 0;
    if (arg) {
        freq = arg;
        fmtx_freq = arg;
    } else {
        freq = fmtx_freq;
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
    dis->lock(0);
}
#endif//TCFG_APP_FM_EMITTER_EN

static void *ui_open_record(void *hd)
{
    ui_set_auto_reflash(500);
    return NULL;
}

static void ui_close_record(void *hd, void *private)
{
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return ;
    }
    if (private) {
        free(private);
    }
}

static void ui_record_main(void *hd, void *private) //主界面显示
{
    /* printf("=====================\n"); */
    if (!hd) {
        return;
    }

#if TCFG_APP_FM_EMITTER_EN
    u16 fre = fm_emitter_manage_get_fre();
    if (fre != 0) {
        led7_fm_show_freq(hd, fre);
    }
#else
    if (recorder_is_encoding()) {
        int time = recorder_get_encoding_time();
        led_show_record_time(hd, time);
    } else {
        led7_show_record(hd);
    }
#endif
}


static int ui_record_user(void *hd, void *private, u8 menu, u32 arg)//子界面显示 //返回true不继续传递 ，返回false由common统一处理
{
    int ret = true;
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return false;
    }
    switch (menu) {
    case MENU_RECORD:
        led7_show_record(hd);
        break;
    default:
        ret = false;
        break;
    }

    return ret;

}



const struct ui_dis_api record_main = {
    .ui      = UI_RECORD_MENU_MAIN,
    .open    = ui_open_record,
    .ui_main = ui_record_main,
    .ui_user = ui_record_user,
    .close   = ui_close_record,
};

#endif
#endif//(TCFG_UI_ENABLE && TCFG_APP_RECORD_EN)
