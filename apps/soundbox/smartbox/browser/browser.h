#ifndef __BROWSER_H__
#define __BROWSER_H__

#include "typedef.h"
#include "app_config.h"

///smartbox 对应的设备id枚举
enum {
    BS_UDISK = 0,
    BS_SD0,
    BS_SD1,
    BS_FLASH,
    BS_AUX,
    BS_DEV_MAX,
};

//smartbox浏览设备映射
char *smartbox_browser_dev_remap(u32 index);
//smartbox获取文件浏览后缀配置
char *smartbox_browser_file_ext(void);
//smartbox获取文件浏览后缀配置数据长度
u16 smartbox_browser_file_ext_size(void);
//smartbox限制文件名长度（包括文件夹,如：xxx~~~, 省略以"~~~"表示）
u16 file_name_cut(u8 *name, u16 len, u16 len_limit);
//smartbox文件浏览开始
void smartbox_browser_start(u8 *data, u16 len);
//smartbox文件浏览停止
void smartbox_browser_stop(void);
//smartbox文件浏览繁忙查询
bool smartbox_browser_busy(void);

#endif//__SMARTBOX_H__

