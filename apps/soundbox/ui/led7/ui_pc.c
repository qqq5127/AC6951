#include "ui/ui_api.h"
#include "system/includes.h"

#if TCFG_APP_PC_EN
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))
void ui_pc_temp_finsh(u8 menu)//子菜单被打断或者显示超时
{
    switch (menu) {
    default:
        break;
    }
}

static void led7_show_pc(void *hd)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" PC");
    dis->show_icon(LED7_USB);
    dis->lock(0);
}




static void *ui_open_pc(void *hd)
{
    /* ui_set_auto_reflash(500);//设置主页500ms自动刷新 */
    return NULL;
}

static void ui_close_pc(void *hd, void *private)
{
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return ;
    }

    if (private) {
        free(private);
    }
}

static void ui_pc_main(void *hd, void *private) //主界面显示
{
    if (!hd) {
        return;
    }
    led7_show_pc(hd);
}


static int ui_pc_user(void *hd, void *private, u8 menu, u32 arg)//子界面显示 //返回true不继续传递 ，返回false由common统一处理
{
    int ret = true;
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return false;
    }
    switch (menu) {
    default:
        ret = false;
        break;
    }

    return ret;

}




const struct ui_dis_api pc_main = {
    .ui      = UI_PC_MENU_MAIN,
    .open    = ui_open_pc,
    .ui_main = ui_pc_main,
    .ui_user = ui_pc_user,
    .close   = ui_close_pc,
};













#endif
#endif
