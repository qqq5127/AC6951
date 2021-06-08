#ifndef _SYS_PARAM_API_H_
#define _SYS_PARAM_API_H_

void sys_param_init(void);
void sys_param_write2vm(void);

void set_version_log(u32 log);
u32 get_version_log(void);
void set_space_log(u32 log);
u32 get_space_log(void);

void set_auto_poweroff_timer(int sel_item);
void auto_power_off_timer_close(void);
u16 get_cur_auto_power_time(void);

void set_backlight_time(u16 time);
u16 get_backlight_time(void);
u16 get_backlight_time_item(void);
void set_backlight_brightness(u16 brightness);
u16 get_backlight_brightness(void);
u16 get_backlight_brightness_item(void);

void set_sys_info_reset(void);
#endif /* _SYS_PARAM_API_H_ */

