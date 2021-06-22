#include "includes.h"
#include "ui/ui_api.h"


#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))

#include "app_task.h"
extern u8 app_curr_task;

static void ui_idle_main(void *hd, void *private) //主界面显�?
{
		LCD_API *dis = (LCD_API *)hd;

    if (!hd) {
        return;
    }
		dis->lock(1);
		if(app_curr_task == APP_IDLE_TASK)
		{
			dis->clear();
			dis->setXY(0, 0);
			dis->show_string((u8 *)"----");
			dis->FlashChar(BIT(0) | BIT(1) | BIT(2) | BIT(3)); //设置闪烁
		}
		else
		{
			dis->clear();
			dis->setXY(0, 0);
			dis->show_string((u8 *)"COAX");
		}
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
