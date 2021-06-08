#ifndef AUDIO_ADC_H
#define AUDIO_ADC_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "generic/atomic.h"

/*无电容电�?/
#define SUPPORT_MIC_CAPLESS          1

#define LADC_STATE_INIT			1
#define LADC_STATE_OPEN      	2
#define LADC_STATE_START     	3
#define LADC_STATE_STOP      	4

#define FPGA_BOARD          	0

#define LADC_MIC                0
#define LADC_LINEIN0            1
#define LADC_LINEIN1            2
#define LADC_LINEIN             3


/* 通道选择 */
#define AUDIO_ADC_MIC_L		    BIT(0)
#define AUDIO_ADC_MIC_R		    BIT(1)
#define AUDIO_ADC_LINE0_L		BIT(2)
#define AUDIO_ADC_LINE0_R		BIT(3)
#define AUDIO_ADC_LINE1_L		BIT(4)
#define AUDIO_ADC_LINE1_R		BIT(5)
#define AUDIO_ADC_LINE2_L		BIT(6)
#define AUDIO_ADC_LINE2_R		BIT(7)
#define AUDIO_ADC_MIC_CH		BIT(0)

#define AUDIO_ADC_LINE0_LR		(AUDIO_ADC_LINE0_L | AUDIO_ADC_LINE0_R)
#define AUDIO_ADC_LINE1_LR		(AUDIO_ADC_LINE1_L | AUDIO_ADC_LINE1_R)
#define AUDIO_ADC_LINE2_LR		(AUDIO_ADC_LINE2_L | AUDIO_ADC_LINE2_R)

#define LADC_CH_MIC_L			BIT(0)
#define LADC_CH_MIC_R			BIT(1)
#define LADC_CH_LINE0_L			BIT(2)
#define LADC_CH_LINE0_R			BIT(3)
#define LADC_CH_LINE1_L			BIT(4)
#define LADC_CH_LINE1_R			BIT(5)

#define LADC_MIC_MASK			(BIT(0) | BIT(1))
#define LADC_LINE0_MASK			(BIT(2) | BIT(3))
#define LADC_LINE1_MASK			(BIT(4) | BIT(5))

/*LINEIN通道定义*/
#define AUDIO_LIN0L_CH			BIT(0)//PA0
#define AUDIO_LIN0R_CH			BIT(1)//PA1
#define AUDIO_LIN1L_CH			BIT(2)//PB7
#define AUDIO_LIN1R_CH			BIT(3)//PB8
#define AUDIO_LIN2L_CH			BIT(4)//PB9
#define AUDIO_LIN2R_CH			BIT(5)//PB10
#define AUDIO_LIN_DACL_CH		BIT(6)
#define AUDIO_LIN_DACR_CH		BIT(7)
#define AUDIO_LIN0_LR			(AUDIO_LIN2L_CH | AUDIO_LIN2R_CH)
#define AUDIO_LIN1_LR			(AUDIO_LIN1L_CH | AUDIO_LIN1R_CH)
#define AUDIO_LIN2_LR			(AUDIO_LIN2L_CH | AUDIO_LIN2R_CH)

struct ladc_port {
    u8 channel;
};

struct adc_platform_data {
    u8 mic_channel;
    u8 mic_ldo_isel; //MIC通道电流档位选择
    u8 mic_capless;  //MIC免电容方�?    u8
    u8 mic_bias_res; //MIC免电容方案需要设置，影响MIC的偏置电�?1:16K 2:7.5K 3:5.1K 4:6.8K 5:4.7K 6:3.5K 7:2.9K  8:3K  9:2.5K 10:2.1K 11:1.9K  12:2K  13:1.8K 14:1.6K  15:1.5K 16:1K 31:0.6K
    u8 mic_ldo_vsel : 2;//00:2.3v 01:2.5v 10:2.7v 11:3.0v
    u8 mic_bias_inside : 1;//MIC电容隔直模式使用内部mic偏置(PC7)
    u8 mic_bias_keep : 1;//保持内部mic偏置输出
    u8 reserved: 4;
    u8 ladc_num;
    const struct ladc_port *ladc;
};

struct capless_low_pass {
    u16 bud; //快调边界
    u16 count;
    u16 pass_num;
    u16 tbidx;
};

struct audio_adc_output_hdl {
    struct list_head entry;
    void *priv;
    void (*handler)(void *, s16 *, int);
};

/*Audio adc模块的数据结�*/
struct audio_adc_hdl {
    struct list_head head;//采样数据输出链表�?    const struct adc_platform_data *pd;//adc硬件相关配置
    atomic_t ref;	//adc模块引用记录
    u8 channel;		//adc打开通道统计
    u8 input;		//adc输入记录
    u8 state;		//adc状�?    u8 mic_ldo_state : 1;//当前micldo是否打开//省电容mic数据结构
#if SUPPORT_MIC_CAPLESS
    struct capless_low_pass lp;
    int last_dacr32;
#endif/*SUPPORT_MIC_CAPLESS*/
};

struct adc_mic_ch {
    u8 gain;
    u8 buf_num;
    u16 buf_size;
    u16 sample_rate;
    s16 *bufs;
    struct audio_adc_hdl *hdl;
    //void (*handler)(struct adc_mic_ch *, s16 *, u16);
};

/*Audio adc通道数据结构*/
struct audio_adc_ch {
    u8 gain;		//adc通道的增�?    u8 buf_num;		//adc buf数量
    u8 ch;			//adc 通道索引
    u16 buf_size;	//adc buf大小
    u16 sample_rate;//adc通道采样�?    s16 *bufs;		//adc buf地址
    struct audio_adc_hdl *hdl;//adc模块句柄
    //void (*handler)(struct audio_adc_ch *, s16 *, u16);
};

/*
*********************************************************************
*                  Audio ADC Initialize
* Description: 初始化Audio_ADC模块的相关数据结�?* Arguments  : adc	ADC模块操作句柄
*			   pd	ADC模块硬件相关配置参数
* Note(s)    : None.
*********************************************************************
*/
void audio_adc_init(struct audio_adc_hdl *, const struct adc_platform_data *);

/*
*********************************************************************
*                  Audio ADC Output Callback
* Description: 注册adc采样输出回调函数
* Arguments  : adc		adc模块操作句柄
*			   output  	采样输出回调
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_adc_add_output_handler(struct audio_adc_hdl *, struct audio_adc_output_hdl *);

/*
*********************************************************************
*                  Audio ADC Output Callback
* Description: 删除adc采样输出回调函数
* Arguments  : adc		adc模块操作句柄
*			   output  	采样输出回调
* Return	 : None.
* Note(s)    : 采样通道关闭的时候，对应的回调也要同步删除，防止内存�?*              放出现非法访问情�?*********************************************************************
*/
void audio_adc_del_output_handler(struct audio_adc_hdl *adc,
                                  struct audio_adc_output_hdl *output);

/*
*********************************************************************
*                  Audio ADC IRQ Handler
* Description: Audio ADC中断回调函数
* Arguments  : adc  adc模块操作句柄
* Return	 : None.
* Note(s)    : (1)仅供Audio_ADC中断使用
*  			   (2)ADC(3通道)数据组织结构:
*  			   adc0(0) adc1(0)
*  			   adc2(0) adc0(1)
*  			   adc1(1) adc2(1)
*  			   ...	  adc0(n)
*  			   adc1(n) adc2(n)
*********************************************************************
*/
void audio_adc_irq_handler(struct audio_adc_hdl *adc);

/*
*********************************************************************
*                  Audio ADC Mic Open
* Description: 打开mic采样通道
* Arguments  : mic	mic采样通道句柄
*			   ch	mic通道索引
*			   hdl  adc模块操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_open(struct adc_mic_ch *mic, int ch, struct audio_adc_hdl *adc);

/*
*********************************************************************
*                  Audio ADC Mic Sample Rate
* Description: 设置mic采样�?* Arguments  : mic			mic操作句柄
*			   sample_rate	采样�?* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_set_sample_rate(struct adc_mic_ch *mic, int sample_rate);

/*
*********************************************************************
*                  Audio ADC Mic Gain
* Description: 设置mic增益
* Arguments  : mic	mic操作句柄
*			   gain	mic增益
* Return	 : 0 成功	其他 失败
* Note(s)    : MIC增益范围�?(0dB)~14(28dB),step:2dB
*********************************************************************
*/
int audio_adc_mic_set_gain(struct adc_mic_ch *mic, int gain);

/*
*********************************************************************
*                  Audio ADC Mic Buffer
* Description: 设置采样buf和采样长�?* Arguments  : mic		mic操作句柄
*			   bufs		采样buf地址
*			   buf_size	采样buf长度，即一次采样中断数据长�?*			   buf_num 	采样buf的数�?* Return	 : 0 成功	其他 失败
* Note(s)    : (1)需要的总buf大小 = buf_size * ch_num * buf_num
* 		       (2)buf_num = 2表示，第一次数据放在buf0，第二次数据放在
*			   buf1,第三次数据放在buf0，依此类推。如果buf_num = 0则表
*              示，每次数据都是放在buf0
*********************************************************************
*/
int audio_adc_mic_set_buffs(struct adc_mic_ch *mic, s16 *bufs, u16 buf_size, u8 buf_num);

/*
*********************************************************************
*                  Audio ADC Mic Start
* Description: 启动mic采样
* Arguments  : mic	mic操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_start(struct adc_mic_ch *mic);

/*
*********************************************************************
*                  Audio ADC Mic Close
* Description: 关闭mic采样
* Arguments  : mic	mic操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_close(struct adc_mic_ch *mic);

/*
*********************************************************************
*                  Audio MIC Mute
* Description: mic静音使能控制
* Arguments  : mute 静音使能
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_set_mic_mute(u8 mute);

/*
*********************************************************************
*                  Audio ADC Linein Open
* Description: 打开linein采样通道
* Arguments  : adc	adc采样通道句柄
*			   ch   linein通道索引
*			   hdl	adc采样模块操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_linein_open(struct audio_adc_ch *adc, int ch, struct audio_adc_hdl *hdl);

/*
*********************************************************************
*                  Audio ADC Linein Sample Rate
* Description: 设置linein采样�?* Arguments  : ch			linein采样通道操作句柄
*			   sample_rate	采样�?* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_linein_set_sample_rate(struct audio_adc_ch *ch, int sample_rate);

/*
*********************************************************************
*                  Audio ADC Linein Gain
* Description: 设置linein增益
* Arguments  : ch			linein采样通道操作句柄
*			   sample_rate	采样�?* Return	 : 0 成功	其他 失败
* Note(s)    : (1)linein adc的增益范围：0(-8dB)~15(7dB),8(0dB),step:1dB
*			   (2)增益设置规则�?*			   A:默认linein两个通道的增益一�?
*				audio_adc_linein_set_gain(ch,gain);
*			   B:如果要分开设置，可以把参数gain分成高低16bit来设�?
*				audio_adc_linein_set_gain(ch,(gain_l | (gain_r << 16)));
*********************************************************************
*/
int audio_adc_linein_set_gain(struct audio_adc_ch *ch, int gain);

/*
*********************************************************************
*                  Audio ADC Linein Start
* Description: 启动linein采样
* Arguments  : adc	adc采样通道句柄
*			   ch   linein通道索引
*			   hdl	adc采样模块操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_linein_start(struct audio_adc_ch *ch);

/*
*********************************************************************
*                  Audio ADC Linein Close
* Description: 关闭linein采样
* Arguments  : adc	adc采样通道句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_linein_close(struct audio_adc_ch *ch);

/*
*********************************************************************
*                  Audio ADC Buffer
* Description: 设置采样buf和采样长�?* Arguments  : ch		adc通道操作句柄
*			   bufs		采样buf地址
*			   buf_size	采样buf长度，即一次采样中断数据长�?*			   buf_num 	采样buf的数�?* Return	 : 0 成功	其他 失败
* Note(s)    : (1)需要的总buf大小 = buf_size * ch_num * buf_num
* 		       (2)buf_num = 2表示，第一次数据放在buf0，第二次数据放在
*			   buf1,第三次数据放在buf0，依此类推。如果buf_num = 0则表
*              示，每次数据都是放在buf0
*********************************************************************
*/
int audio_adc_set_buffs(struct audio_adc_ch *ch, s16 *bufs, u16 buf_size, u8 buf_num);

/*
*********************************************************************
*                  Audio ADC Start
* Description: 启动audio adc采样
* Arguments  : linein_ch	linein采样通道句柄
*			   mic_ch	    mic采样通道句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : 该接口支持同时操作mic和linein采样通道
*********************************************************************
*/
int audio_adc_start(struct audio_adc_ch *linein_ch, struct adc_mic_ch *mic_ch);

/*
*********************************************************************
*                  Audio ADC Close
* Description: 关闭linein采样
* Arguments  : linein_ch	linein采样通道句柄
*			   mic_ch	    mic采样通道句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : 该接口支持同时操作mic和linein采样通道
*********************************************************************
*/
int audio_adc_close(struct audio_adc_ch *linein_ch, struct adc_mic_ch *mic_ch);

/*
*********************************************************************
*                  Audio ADC Mic Pre_Gain
* Description: 设置mic第一�?前级增益
* Arguments  : en 前级增益使能(0:6dB 1:0dB)
* Return	 : None.
* Note(s)    : 前级增益只有0dB�?dB两个档位，使能即�?dB，否则为6dB
*********************************************************************
*/
void audio_mic_0dB_en(bool en);

/*
*********************************************************************
*                  Audio ADC MIC Control
* Description: mic通道使能控制
* Arguments  : en 使能控制�?* Return	 : None.
* Note(s)    : 扩展接口，GPIO使用冲突
*********************************************************************
*/
void audio_adc_mic_ctl(u8 en);


/*
*********************************************************************
*                  Audio ADC MIC LDO EN
* Description: mic ldo使能
* Arguments  : en 使能控制�? pd adc结构�?* Return	 : None.
* Note(s)    : 开关mic的ldo
*********************************************************************
*/

int audio_mic_ldo_en(u8 en, struct adc_platform_data *pd);









#endif/*AUDIO_ADC_H*/
