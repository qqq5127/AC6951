#ifndef _MUSIC_UI_H__
#define _MUSIC_UI_H__

#if MUSIC_AB_RPT_EN
#define DECODE_AB_REPEAT 1
#else
#define DECODE_AB_REPEAT 0
#endif


typedef enum {
    SD0_DEVICE = 0x00,
    SD1_DEVICE,
    USB_DEVICE,
    NO_DEVICE = 0xff,
} UI_DEVICE;

#define   MUSIC_OPT_BIT_PLAY       (0<<0)
#define   MUSIC_OPT_BIT_DEL        (1<<0)
#define   MUSIC_OPT_BIT_PAUSE      (1<<1)
#define   MUSIC_OPT_BIT_FF         (1<<2)
#define   MUSIC_OPT_BIT_FR         (1<<3)
#define   MUSIC_OPT_BIT_SEL        (1<<4)

typedef struct _MUSIC_DIS_VAR {
    UI_DEVICE ui_curr_device;//插入的设备类型
    u32 ui_curr_file;//当前文件序号
    u32 ui_total_file;//总文件数
    u32 play_time;//播放时间
    void *file;//文件句柄
    u8  *eq_mode;
    u8  *play_mode;//播放类型，顺序播放 单曲循环 AB重复 随机
    u8 opt_state;//播放状态 暂停、播放、快进、快退
    u8 lrc_flag;

} MUSIC_DIS_VAR;


#endif
