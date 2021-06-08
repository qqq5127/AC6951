#ifndef _UI_SYS_PARAM__H_
#define _UI_SYS_PARAM__H_


void set_sys_param_default(void);

void sys_param_write2vm(void);

void sys_param_init(void);

void set_backlight_time(u16 time);

u16 get_backlight_time(void);

void set_backlight_brightness(u16 brightness);

u16 get_backlight_brightness(void);
u16 get_backlight_brightness_item(void);




#endif

