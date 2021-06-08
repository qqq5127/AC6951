#ifndef __EFFECT_CFG_H__
#define __EFFECT_CFG_H__

#include "generic/typedef.h"
#include "system/includes.h"
// #include "reverb/reverb_api.h"
#include "audio_mic/mic_stream.h"
// #include "pitchshifter/pitchshifter_api.h"
// #include "asm/noisegate.h"
#include "application/audio_noisegate.h"
// #include "audio_mic/digital_vol.h"
#include "application/audio_eq.h"
#include "application/audio_howling.h"
#include "application/audio_pitch.h"
#include "application/audio_echo_reverb.h"
#include "app_config.h"
#define MODE_SEQ_TO_INDEX(SEQ)			(SEQ-0x2000)
#define MODE_INDEX_TO_SEQ(index)		(index+0x2000)

#define EFFECT_EQ_SECTION_MAX 5

#ifdef EQ_SECTION_MAX
#if EFFECT_EQ_SECTION_MAX > EQ_SECTION_MAX
#undef EFFECT_EQ_SECTION_MAX
#define EFFECT_EQ_SECTION_MAX  EQ_SECTION_MAX
#endif
#endif

typedef struct {
    /* unsigned short crc; */
    unsigned short id;
    unsigned short len;
}  EFFECTS_FILE_HEAD;


struct __effect_mode_attr {
    u8   	 num;
    u16 	*cmd_tab;
    char    *name;
};

struct __effect_mode_cfg {
    u8	 debug_en: 1;
    u8	 revert:   7;
    u8	 mode_max;
    u16	 sample_rate;
    struct __effect_mode_attr  *attr;
    const char *file_path;
};

typedef struct {
    float global_gain;
    int seg_num;
    int enable_section;
#if (TCFG_EQ_ENABLE)
    struct eq_seg_info seg[EFFECT_EQ_SECTION_MAX];
#endif
}  EFFECTS_EQ_PARM_SET;


typedef struct {
    int gain;
}  EFFECTS_MIC_GAIN_PARM;

typedef struct PITCH_PARM_SET2 {
    u32 effect_v;//变声模式：变调/变声0/变声1/变声2/机器音
    u32 pitch;		//变音时 pitch 对饮shiftv, effectv 填 EFFECT_VOICECHANGE_KIN0
    u32 formant_shift;//变音对应formant
} PITCH_PARM_SET2;


typedef struct NOISE_PARM {
    int attacktime;// 0 - 15000ms
    int releasetime;//  0 - 300ms
    int threadhold;//    -92 - 0 db   （传下时转化为mdb,(thr * 1000)）
    int gain;//   0 - 1               (传下来时扩大30bit，(int)(gain * （1 <<30）))
} NOISE_PARM_SET;


// 高音:
typedef struct HIGH_SOUND {
    int cutoff_frequency;//:截止频率:  1800
    int highest_gain;//最高增益：0
    int lowest_gain;// 最低增益：-12000
} HIGH_SOUND_PARM_SET;

// 低音：
typedef struct LOW_SOUND {
    int cutoff_frequency;//截止频率：600
    int highest_gain;//最高增益：0
    int lowest_gain;//最低增益:  -12000
} LOW_SOUND_PARM_SET;

// 喊麦滤波器:
typedef struct SHOUT_WHEAT {
    int center_frequency;//中心频率: 800
    int bandwidth;//带宽:   4000
    int occupy;//占比:   100%
} SHOUT_WHEAT_PARM_SET;


typedef struct {
    volatile u8	 	 flag: 		1;
        volatile u8	 	 update: 	1;
        volatile u8	 	 revert: 	6;
        REVERBN_PARM_SET val;
    }  EFFECTS_REVERB;
    typedef struct {
    volatile u8	 	 flag: 		1;
        volatile u8	 	 update: 	1;
        volatile u8	 	 revert: 	6;
        PITCH_PARM_SET2  val;
    }  EFFECTS_PARM2;
    typedef struct {
    volatile u8	 	 flag: 		1;
        volatile u8	 	 update: 	1;
        volatile u8	 	 revert: 	6;
        ECHO_PARM_SET    val;
    }  EFFECTS_ECHO;
    typedef struct {
    volatile u8	 	 flag: 		1;
        volatile u8	 	 update: 	1;
        volatile u8	 	 revert: 	6;
        NOISE_PARM_SET 	 val;
    }  EFFECTS_NOISE;
    typedef struct {
    volatile u8	 	 flag: 		1;
        volatile u8	 	 update: 	1;
        volatile u8	 	 revert: 	6;
        SHOUT_WHEAT_PARM_SET 	val;
    }  EFFECTS_SHOUT_WHEAT;
    typedef struct {
    volatile u8	 	 flag: 		1;
        volatile u8	 	 update: 	1;
        volatile u8	 	 revert: 	6;
        LOW_SOUND_PARM_SET 	val;
    }  EFFECTS_LOW_SOUND;
    typedef struct {
    volatile u8	 	 flag: 		1;
        volatile u8	 	 update: 	1;
        volatile u8	 	 revert: 	6;
        HIGH_SOUND_PARM_SET val;
    }  EFFECTS_HIGH_SOUND;
    typedef struct {
    volatile u8	 	 flag: 		1;
        volatile u8	 	 update: 	1;
        volatile u8	 	 revert: 	6;
        EFFECTS_MIC_GAIN_PARM   val;
    }  EFFECTS_MIC_GAIN;
    typedef struct {
    volatile u8	 	 flag: 		1;
        volatile u8	 	 update: 	1;
        volatile u8	 	 revert: 	6;
        EFFECTS_EQ_PARM_SET val;
    }  EFFECTS_EQ_PARM;


    struct __effect_mode_parm {
    EFFECTS_REVERB 		reverb;
    EFFECTS_PARM2  		pitch;
    EFFECTS_ECHO   		ehco;
    EFFECTS_NOISE  		noise;
    EFFECTS_SHOUT_WHEAT shout_wheat;
    EFFECTS_LOW_SOUND  	low_sound;
    EFFECTS_HIGH_SOUND 	high_sound;
    EFFECTS_MIC_GAIN   	mic_gain;
    EFFECTS_EQ_PARM 	eq;
};
struct __effect_mode {
    u32								 seg_num;
    u32								 eq_design_mask;
    float 							 global_gain;
    struct __effect_mode_parm 		 parm;
};

struct __tool_callback {
    void *priv;
    int (*cmd_func)(void *priv, u16 mode_index, u16 cmd_index, u8 *data, u32 len, u8 online);
    void (*change_mode)(u16 mode);
};


/*effetcs type*/
typedef enum {
    EFFECTS_TYPE_FILE = 0x01,
    EFFECTS_TYPE_ONLINE,
    EFFECTS_TYPE_MODE_TAB,
} EFFECTS_TYPE;

// 0x09 查询是否有密码
// 0x0A 密码是否正确
// 0x0B 查询文件大小
// 0x0C 读取文件内容

/*effects online cmd*/
typedef enum {
    EFFECTS_ONLINE_CMD_INQUIRE = 0x4,
    EFFECTS_ONLINE_CMD_GETVER = 0x5,

    EFFECTS_ONLINE_CMD_PASSWORD = 0x9,
    EFFECTS_ONLINE_CMD_VERIFY_PASSWORD = 0xA,
    EFFECTS_ONLINE_CMD_FILE_SIZE = 0xB,
    EFFECTS_ONLINE_CMD_FILE = 0xC,
    EFFECTS_EQ_ONLINE_CMD_GET_SECTION_NUM = 0xD,//工具查询 小机需要的eq段数
    EFFECTS_EQ_ONLINE_CMD_CHANGE_MODE = 0xE,//切换变声模式

    EFFECTS_ONLINE_CMD_MODE_COUNT = 0x100,//模式个数
    EFFECTS_ONLINE_CMD_MODE_NAME = 0x101,//模式的名字
    EFFECTS_ONLINE_CMD_MODE_GROUP_COUNT = 0x102,//模式下组的个数,4个字节
    EFFECTS_ONLINE_CMD_MODE_GROUP_RANGE = 0x103,//模式下组的id内容
    EFFECTS_ONLINE_CMD_EQ_GROUP_COUNT = 0x104,//eq组的id个数
    EFFECTS_ONLINE_CMD_EQ_GROUP_RANGE = 0x105,//eq组的id内容
    EFFECTS_ONLINE_CMD_MODE_SEQ_NUMBER = 0x106,//mode的编号

    EFFECTS_CMD_REVERB = 0x1001,
    EFFECTS_CMD_PITCH1 = 0x1002, //无用
    EFFECTS_CMD_PITCH2 = 0x1003,
    EFFECTS_CMD_ECHO  = 0x1004,
    EFFECTS_CMD_NOISE = 0x1005,
    //高音、低音、喊麦
    EFFECTS_CMD_HIGH_SOUND = 0x1006,
    EFFECTS_CMD_LOW_SOUND = 0x1007,
    EFFECTS_CMD_SHOUT_WHEAT = 0x1008,
    //混响eq
    EFFECTS_EQ_ONLINE_CMD_PARAMETER_SEG = 0x1009,
    EFFECTS_EQ_ONLINE_CMD_PARAMETER_TOTAL_GAIN = 0x100A,//无用
    EFFECTS_CMD_MIC_ANALOG_GAIN = 0x100B,
    //add xx


    EFFECTS_CMD_MAX,
} EQ_ONLINE_CMD;



int mic_eq_get_filter_info(void *eq, int sr, struct audio_eq_filter_info *info);

u16 effect_cfg_get_cur_mode(void);
void effect_cfg_change_mode(u16 mode);

#endif//__EFFECT_CFG_H__
