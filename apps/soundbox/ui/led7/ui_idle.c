#include "includes.h"
#include "ui/ui_api.h"

#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))
static void ui_idle_main(void *hd, void *private) //主界面显示
{
    if (!hd) {
        return;
    }
    LCD_API *dis = (LCD_API *)hd;//清显示
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->clear_icon(0xffff);
    dis->lock(0);
}


const struct ui_dis_api idle_main = {
    .ui      = UI_IDLE_MENU_MAIN,
    .open    = NULL,
    .ui_main = ui_idle_main,
    .ui_user = NULL,
    .close   = NULL,
};
#endif
