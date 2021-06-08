#include "ui/ui_api.h"
#include "system/includes.h"
#include "system/sys_time.h"
#include "rtc/rtc_ui.h"

#if TCFG_APP_RTC_EN
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))

struct rtc_ui_opr {
    void *dev_handle;
    struct ui_rtc_display ui_rtc;
};

static struct rtc_ui_opr *__this = NULL;

void ui_rtc_temp_finsh(u8 menu)//子菜单被打断或者显示超时
{
    switch (menu) {
    default:
        break;
    }
}

static void ui_led7_show_year(void *hd, u16 Year)
{

    LCD_API *dis = (LCD_API *)hd;
    u8 tmp_buf[5] = {0};
    itoa4(Year, (u8 *)&tmp_buf[0]);
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string(tmp_buf);
    dis->lock(0);
}

static void led7_show_rtc_string(void *hd, const char *buf)
{
    LCD_API *dis = (LCD_API *)hd;

    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)buf);
    dis->lock(0);
}


static void led7_flash_char(void *hd, u8 index)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->FlashChar(BIT(index));
}

static void led7_rtc_flash_icon(void *hd, u32 icon)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->flash_icon(icon);
}


static void ui_led7_show_curtime(void *hd, u8 Hour, u8 Min)
{
    LCD_API *dis = (LCD_API *)hd;
    u8 tmp_buf[5] = {0};
    itoa2(Hour, (u8 *)&tmp_buf[0]);
    itoa2(Min, (u8 *)&tmp_buf[2]);

    dis->lock(1);
    dis->setXY(0, 0);
    dis->Clear_FlashChar(BIT(0) | BIT(1) | BIT(2) | BIT(3));
    dis->show_string(tmp_buf);
    dis->flash_icon(LED7_2POINT);
    dis->lock(0);
}


static void ui_led7_show_RTC_time(void *hd, u8 Hour, u8 Min)
{
    LCD_API *dis = (LCD_API *)hd;
    u8 tmp_buf[5] = {0};
    itoa2(Hour, (u8 *)&tmp_buf[0]);
    itoa2(Min, (u8 *)&tmp_buf[2]);

    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string(tmp_buf);
    dis->show_icon(LED7_2POINT);
    dis->lock(0);
}

static void ui_led7_show_date(void *hd, u16 Year, u8 Month, u8 Day)
{

    LCD_API *dis = (LCD_API *)hd;
    u8 tmp_buf[5] = {0};
    itoa2(Month, (u8 *)&tmp_buf[0]);
    itoa2(Day, (u8 *)&tmp_buf[2]);

    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string(tmp_buf);
    dis->lock(0);
}



static int ui_led7_show_RTC_user(void *hd, struct ui_rtc_display *rtc)
{
    int ret = true;
    if (rtc == NULL) {
        return false;
    }

    switch (rtc->rtc_menu) {

    /* case UI_RTC_ACTION_SHOW_TIME: */
    /* ui_led7_show_RTC_time(hd,rtc->time.Hour, rtc->time.Min); */
    /* break; */

    case UI_RTC_ACTION_SHOW_DATE:
        ui_led7_show_date(hd, rtc->time.Year, rtc->time.Month, rtc->time.Day);
        break;

    case UI_RTC_ACTION_YEAR_SET:
        ui_led7_show_year(hd, rtc->time.Year);
        led7_flash_char(hd, 0);
        led7_flash_char(hd, 1);
        led7_flash_char(hd, 2);
        led7_flash_char(hd, 3);
        break;

    case UI_RTC_ACTION_MONTH_SET:
        ui_led7_show_date(hd, rtc->time.Year, rtc->time.Month, rtc->time.Day);
        led7_flash_char(hd, 0);
        led7_flash_char(hd, 1);
        break;

    case UI_RTC_ACTION_DAY_SET:
        ui_led7_show_date(hd, rtc->time.Year, rtc->time.Month, rtc->time.Day);
        led7_flash_char(hd, 2);
        led7_flash_char(hd, 3);
        break;

    case UI_RTC_ACTION_HOUR_SET:
        ui_led7_show_RTC_time(hd, rtc->time.Hour, rtc->time.Min);
        led7_flash_char(hd, 0);
        led7_flash_char(hd, 1);
        break;

    case UI_RTC_ACTION_MINUTE_SET:
        ui_led7_show_RTC_time(hd, rtc->time.Hour, rtc->time.Min);
        led7_flash_char(hd, 2);
        led7_flash_char(hd, 3);
        break;

    case UI_RTC_ACTION_STRING_SET:
        led7_show_rtc_string(hd, rtc->str);
        break;

    default:
        ret = false;
        break;
    }

    return ret;
}




struct ui_rtc_display *rtc_ui_get_display_buf()
{
    if (__this) {
        return &(__this->ui_rtc);
    } else {
        return NULL;
    }
}



static void *ui_open_rtc(void *hd)
{

    struct sys_time current_time;
    if (!__this) {
        __this =  zalloc(sizeof(struct rtc_ui_opr));
    }
    __this->dev_handle = dev_open("rtc", NULL);

    if (!__this->dev_handle) {
        free(__this);
        __this = NULL;
        return NULL;
    }

    ui_set_auto_reflash(500);//设置主页500ms自动刷新
    dev_ioctl(__this->dev_handle, IOCTL_GET_SYS_TIME, (u32)&current_time);
    printf("rtc_read_sys_time: %d-%d-%d %d:%d:%d\n",
           current_time.year,
           current_time.month,
           current_time.day,
           current_time.hour,
           current_time.min,
           current_time.sec);


    if (hd) {
        LCD_API *dis = (LCD_API *)hd;
        dis->lock(1);
        dis->clear();
        dis->setXY(0, 0);
        dis->lock(0);
    }
    return NULL;
}


static void ui_close_rtc(void *hd, void *private)
{
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return;
    }
    if (__this) {
        free(__this);
        __this = NULL;
    }
}

static void ui_rtc_main(void *hd, void *private) //主界面显示
{
    if (!hd) {
        return;
    }
    struct sys_time current_time;
    dev_ioctl(__this->dev_handle, IOCTL_GET_SYS_TIME, (u32)&current_time);
#if 1
    printf("rtc_read_sys_time: %d-%d-%d %d:%d:%d\n",
           current_time.year,
           current_time.month,
           current_time.day,
           current_time.hour,
           current_time.min,
           current_time.sec);
#endif
    ui_led7_show_curtime(hd, current_time.hour, current_time.min);
}


static int ui_rtc_user(void *hd, void *private, u8 menu, u32 arg)//子界面显示 //返回true不继续传递 ，返回false由common统一处理
{
    int ret = true;
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return false;
    }
    switch (menu) {

    case MENU_ALM_SET:
    case MENU_RTC_SET:
        ui_led7_show_RTC_user(hd, &(__this->ui_rtc));
        break;
    default:
        ret = false;
        break;
    }

    return ret;

}



const struct ui_dis_api rtc_main = {
    .ui      = UI_RTC_MENU_MAIN,
    .open    = ui_open_rtc,
    .ui_main = ui_rtc_main,
    .ui_user = ui_rtc_user,
    .close   = ui_close_rtc,
};


#endif
#endif
