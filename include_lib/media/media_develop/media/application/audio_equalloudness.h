
#ifndef _AUDIO_LOUDNESS_API_H_
#define _AUDIO_LOUDNESS_API_H_

#include "system/includes.h"
#include "media/includes.h"
#include "equalloudness/loudness_api.h"

typedef struct _equalloudness_open_parm {
    u16 sr;               //采样率
    u8 channel;           //通道数
    u8 threadhold_vol;    //触发等响度软件数字音量阈值

    int (*alpha_cb)(float *alpha, u8 *volume, u8 threadhold_vol);//该函数根据，系统软件的数字音量,参数返回 alpha值
} equalloudness_open_parm;

typedef struct _equalloudness_update_parm {
    u8 threadhold_vol;    //触发等响度软件数字音量阈值
} equalloudness_update_parm;


typedef struct _equalloudness_hdl {
    void *work_buf;       //软件等响度 运行句柄及buf
    OS_MUTEX mutex;       //互斥锁

    /*输出的偏移*/
    u32 out_buf_size;     //输出buf大小
    s16 *out_buf;         //输出buf地址
    u32 out_points;       //当前已输出的点数
    u32 out_total;        //输出buf总点数

    void *buf;            //输入buf
    cbuffer_t cbuf;       //软件等响度，每次处理固定长度的数据，此处使用cbuf管理输入
    u8 init_ok: 1;        //是否初始化完成
    u8 prev_vol;          //记录当前数据流入的数字音量

    equalloudness_open_parm o_parm;   //打开时，传入参数

    struct audio_stream_entry entry;	// 音频流入口
} equal_loudness_hdl;

/*----------------------------------------------------------------------------*/
/**@brief    audio_equal_loudness_open, 等响度 打开
   @param    *_parm: 等响度初始化参数，详见结构体equalloudness_open_parm
   @return   等响度句柄
   @note
*/
/*----------------------------------------------------------------------------*/
equal_loudness_hdl *audio_equal_loudness_open(equalloudness_open_parm *_parm);
/*----------------------------------------------------------------------------*/
/**@brief   audio_equal_loudness_parm_update 等响度 参数更新
   @param    cmd:命令
   @param    *_parm:参数指针
   @return   0：成功  -1: 失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_equal_loudness_parm_update(equal_loudness_hdl *_hdl, u32 cmd, equalloudness_update_parm *_parm);
/*----------------------------------------------------------------------------*/
/**@brief   audio_equal_loudness_close 等响度关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_equal_loudness_close(equal_loudness_hdl *_hdl);
#endif

