#ifndef _AEC_USER_H_
#define _AEC_USER_H_

#include "generic/typedef.h"
#include "user_cfg.h"
#include "commproc.h"

#define AEC_DEBUG_ONLINE	0
#define AEC_READ_CONFIG		1

/*清晰语音处理模块定义*/
#define AEC_EN				BIT(0)	//回音消除
#define NLP_EN				BIT(1)	//回音压制
#define	ANS_EN				BIT(2)	//语音降噪
/*TDE_EN和TDEYE_EN只有在TCFG_AUDIO_SMS_ENABLE = SMS_TDE有效*/
#define TDE_EN				BIT(5)	//延时估计模块使能
#define TDEYE_EN			BIT(6)	//延时估计模块结果使用(前提是模块使能)

/*清晰语音处理模式定义*/
#define AEC_MODE_ADVANCE	(AEC_EN | NLP_EN | ANS_EN)	//高级模式
#define AEC_MODE_REDUCE		(NLP_EN | ANS_EN)			//精简模式
#define AEC_MODE_SIMPLEX	(ANS_EN)					//单工模式

/*
 *SMS模式选择
 *(1)SMS模式性能更好，同时也需要更多的ram和mips
 *(2)SMS_NORMAL和SMS_TDE功能一样，只是SMS_TDE内置了延时估计和延时补偿
 *可以更好的兼容延时不固定的场景
 */
#define SMS_DISABLE		0
#define SMS_NORMAL		1
#define SMS_TDE			2
#define TCFG_AUDIO_SMS_ENABLE	SMS_DISABLE

#if (TCFG_AUDIO_SMS_ENABLE == SMS_NORMAL)
#define aec_open		sms_init
#define aec_close		sms_exit
#define aec_in_data		sms_fill_in_data
#define aec_ref_data	sms_fill_ref_data
#elif (TCFG_AUDIO_SMS_ENABLE == SMS_TDE)
#define aec_open		sms_tde_init
#define aec_close		sms_tde_exit
#define aec_in_data		sms_tde_fill_in_data
#define aec_ref_data	sms_tde_fill_ref_data
#else
#define aec_open		aec_init
#define aec_close		aec_exit
#define aec_in_data		aec_fill_in_data
#define aec_ref_data	aec_fill_ref_data
#endif/*TCFG_AUDIO_SMS_ENABLE*/

/*
*********************************************************************
*                  Audio AEC Init
* Description: 初始化AEC模块
* Arguments  : sr 采样率(8000/16000)
* Return	 : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_aec_init(u16 sample_rate);

/*
*********************************************************************
*                  Audio AEC Open
* Description: 初始化AEC模块
* Arguments  : sr 			采样率(8000/16000)
*			   enablebit	使能模块(AEC/NLP/AGC/ANS...)
*			   out_hdl		自定义回调函数，NULL则用默认的回调
* Return	 : 0 成功 其他 失败
* Note(s)    : 该接口是对audio_aec_init的扩展，支持自定义使能模块以及
*			   数据输出回调函数
*********************************************************************
*/
int audio_aec_open(u16 sample_rate, s16 enablebit, int (*out_hdl)(s16 *data, u16 len));

/*
*********************************************************************
*                  Audio AEC Close
* Description: 关闭AEC模块
* Arguments  : None.
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_aec_close(void);

/*
*********************************************************************
*                  Audio AEC Input
* Description: AEC源数据输入
* Arguments  : buf	输入源数据地址
*			   len	输入源数据长度
* Return	 : None.
* Note(s)    : 输入一帧数据，唤醒一次运行任务处理数据，默认帧长256点
*********************************************************************
*/
void audio_aec_inbuf(s16 *buf, u16 len);

/*
*********************************************************************
*                  Audio AEC Reference
* Description: AEC模块参考数据输入
* Arguments  : buf	输入参考数据地址
*			   len	输入参考数据长度
* Return	 : None.
* Note(s)    : 声卡设备是DAC，默认不用外部提供参考数据
*********************************************************************
*/
void audio_aec_refbuf(s16 *buf, u16 len);

/*
*********************************************************************
*                  Audio AEC Output Query
* Description: 查询aec模块的输出数据缓存大小
* Arguments  : None.
* Return	 : 数据缓存大小
* Note(s)    : None.
*********************************************************************
*/
int audio_aec_output_data_size(void);

/*
*********************************************************************
*                  Audio AEC Output Read
* Description: 读取aec模块的输出数据
* Arguments  : buf  读取数据存放地址
*			   len	读取数据长度
* Return	 : 数据读取长度
* Note(s)    : None.
*********************************************************************
*/
int audio_aec_output_read(s16 *buf, u16 len);

#endif/*_AEC_USER_H_*/
