#ifndef __LYRICS_API_H__
#define __LYRICS_API_H__

#include "ui/lyrics.h"
#include "ui/ui.h"

///<保存歌词时间标签到flash, 可以解决较长歌词文件不兼容问题,要使能该功能，需要做以下几点：
//1、配置isd_tools.cfg文件(参考vm区间分配, LRIF_LEN 默认设置为64K)
//		LRIF_ADDR=AUTO;
//		LRIF_LEN=0x10000;
//		LRIF_OPT=1;
//2、使能LRC_ENABLE_SAVE_LABEL_TO_FLASH
//3、配置时间标签临时缓存buf大小LABEL_TEMP_BUF_LEN(2048表示可以解释最长包含1024个时间标签的歌词文件)
//注意：如果flash空间不够， 只能不是能LRC_ENABLE_SAVE_LABEL_TO_FLASH,但是不兼容长歌词文件
#define LRC_ENABLE_SAVE_LABEL_TO_FLASH		1  //是否使能保存歌词时间标签到flash， 1：保存， 0：不保存


//宏定义区
#define LRC_DISPLAY_TEXT_ID   DVcTxt1_11
#define LRC_DISPLAY_TEXT_LEN   32

#define  ONCE_READ_LENGTH     ALIGN_4BYTE(128)   ///<涉及内存对齐问题，值最好是4的倍数(最大允许值为255)
#define  ONCE_DIS_LENGTH      ALIGN_4BYTE(64)    ///显示歌词的缓存长度
#define  LABEL_TEMP_BUF_LEN   ALIGN_4BYTE(2048)  ///暂用2K缓存时间标签


#if(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX)
#define LRC_TEXT_ID_FIR  MUSIC_FILE             //根据实际文本框的ID号修改
#define LRC_TEXT_ID_SEC  MUSIC_LYRICS 			//根据实际文本框的ID号修改
#endif

extern int lrc_init(void);
extern void lrc_exit(void);
extern int lrc_find(FILE *file, FILE **newFile, void *ext_name);
extern bool lrc_analysis_api(FILE *file, FILE **newFile);
extern void lrc_set_analysis_flag(u8 flag);
extern bool lrc_show_api(int text_id, u16 dbtime_s, u8 btime_100ms);
extern bool lrc_get_api(u16 dbtime_s, u8 btime_100ms);

#endif//__LYRICS_API_H__
