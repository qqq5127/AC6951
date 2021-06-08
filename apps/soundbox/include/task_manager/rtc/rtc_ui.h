//=================================================================================//
//                        			RTC模式数据结构                    			   //
//=================================================================================//
/*
显示内容, 每个模式下显示内容是固定的, 枚举是有穷的;
*/
enum rtc_menu_mode {
    UI_RTC_ACTION_SHOW_TIME,   //显示时间
    UI_RTC_ACTION_SHOW_DATE,   //显示日期
    UI_RTC_ACTION_YEAR_SET,    //年设置
    UI_RTC_ACTION_MONTH_SET,   //月设置
    UI_RTC_ACTION_DAY_SET,     //日设置
    UI_RTC_ACTION_HOUR_SET,    //时设置
    UI_RTC_ACTION_MINUTE_SET,  //分设置
    UI_RTC_ACTION_ALARM_UP,    //闹铃响
    UI_RTC_ACTION_STRING_SET,//设置字符
};

struct ui_rtc_time {
    u16 Year;
    u8 Month;
    u8 Day;
    u8 Hour;
    u8 Min;
    u8 Sec;
};


struct ui_rtc_display {
    enum rtc_menu_mode rtc_menu; //用于选择是否闪烁/常亮;
    struct ui_rtc_time time;
    const char *str;
};


struct ui_rtc_display *rtc_ui_get_display_buf();


