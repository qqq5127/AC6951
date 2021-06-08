
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_online_cfg.h"
#include "online_db/online_db_deal.h"
#include "application/audio_eq_drc_apply.h"
#include "config/config_interface.h"

/* #define LOG_TAG     "[APP-EQ]" */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
/* #include "debug.h" */

const u8 audio_eq_sdk_name[16] 		= "AC695N";
#if TCFG_EQ_DIVIDE_ENABLE
const u8 audio_eq_ver[4] 			= {0, 7, 3, 0};//四声道独立eq版本
#else
const u8 audio_eq_ver[4] 			= {0, 7, 2, 0};//四声道eq使用同一效果、立体声、单声道的eq版本
#endif

//eq_cfg_hw.bin中播歌eq曲线，当作用户自定义模式，参与效果切换.
//通话宽频上下行eq曲线也对应放到phone_eq_tab_normal、ul_eq_tab_normal
//EQ_FILE_CP_TO_CUSTOM 1使能时，同时板极文件中 TCFG_USE_EQ_FILE 配 0
#define EQ_FILE_CP_TO_CUSTOM  0


/*************************非用户配置区*************************/
#if TCFG_EQ_ONLINE_ENABLE
#undef EQ_FILE_CP_TO_CUSTOM
#define EQ_FILE_CP_TO_CUSTOM  0
#endif
/*************************************************************/

#if (TCFG_EQ_ENABLE != 0)

#if !TCFG_USE_EQ_FILE
const struct eq_seg_info eq_tab_normal[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    0 << 20, (int)(0.7f * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,    0 << 20, (int)(0.7f * (1 << 24))},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,   0 << 20, (int)(0.7f * (1 << 24))},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   0 << 20, (int)(0.7f * (1 << 24))},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0 << 20, (int)(0.7f * (1 << 24))},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0 << 20, (int)(0.7f * (1 << 24))},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0 << 20, (int)(0.7f * (1 << 24))},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0 << 20, (int)(0.7f * (1 << 24))},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0 << 20, (int)(0.7f * (1 << 24))},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0 << 20, (int)(0.7f * (1 << 24))},

#if (EQ_SECTION_MAX > 10)
    //10段之后频率值设置96k,目的是让10段之后的eq走直通
    {10, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {11, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {12, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {13, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {14, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {15, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {16, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {17, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {18, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {19, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

#if (EQ_SECTION_MAX > 20)
    {20, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {21, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {22, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {23, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {24, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {25, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {26, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {27, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {28, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {29, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {30, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {31, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif
};

const struct eq_seg_info eq_tab_rock[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    -2 << 20, (int)(0.7f * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     0 << 20, (int)(0.7f * (1 << 24))},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    2 << 20, (int)(0.7f * (1 << 24))},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    4 << 20, (int)(0.7f * (1 << 24))},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   -2 << 20, (int)(0.7f * (1 << 24))},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  -2 << 20, (int)(0.7f * (1 << 24))},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0 << 20, (int)(0.7f * (1 << 24))},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0 << 20, (int)(0.7f * (1 << 24))},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   4 << 20, (int)(0.7f * (1 << 24))},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4 << 20, (int)(0.7f * (1 << 24))},

#if (EQ_SECTION_MAX > 10)
    //10段之后频率值设置96k,目的是让10段之后的eq走直通
    {10, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {11, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {12, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {13, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {14, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {15, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {16, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {17, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {18, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {19, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

#if (EQ_SECTION_MAX > 20)
    {20, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {21, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {22, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {23, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {24, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {25, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {26, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {27, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {28, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {29, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {30, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {31, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

};

const struct eq_seg_info eq_tab_pop[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,     3 << 20, (int)(0.7f * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     1 << 20, (int)(0.7f * (1 << 24))},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    0 << 20, (int)(0.7f * (1 << 24))},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   -2 << 20, (int)(0.7f * (1 << 24))},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   -4 << 20, (int)(0.7f * (1 << 24))},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  -4 << 20, (int)(0.7f * (1 << 24))},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  -2 << 20, (int)(0.7f * (1 << 24))},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0 << 20, (int)(0.7f * (1 << 24))},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   1 << 20, (int)(0.7f * (1 << 24))},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  2 << 20, (int)(0.7f * (1 << 24))},

#if (EQ_SECTION_MAX > 10)
    //10段之后频率值设置96k,目的是让10段之后的eq走直通
    {10, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {11, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {12, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {13, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {14, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {15, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {16, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {17, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {18, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {19, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

#if (EQ_SECTION_MAX > 20)
    {20, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {21, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {22, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {23, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {24, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {25, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {26, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {27, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {28, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {29, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {30, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {31, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

};

const struct eq_seg_info eq_tab_classic[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,     0 << 20, (int)(0.7f * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     8 << 20, (int)(0.7f * (1 << 24))},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    8 << 20, (int)(0.7f * (1 << 24))},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    4 << 20, (int)(0.7f * (1 << 24))},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,    0 << 20, (int)(0.7f * (1 << 24))},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,   0 << 20, (int)(0.7f * (1 << 24))},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0 << 20, (int)(0.7f * (1 << 24))},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0 << 20, (int)(0.7f * (1 << 24))},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   2 << 20, (int)(0.7f * (1 << 24))},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  2 << 20, (int)(0.7f * (1 << 24))},

#if (EQ_SECTION_MAX > 10)
    //10段之后频率值设置96k,目的是让10段之后的eq走直通
    {10, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {11, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {12, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {13, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {14, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {15, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {16, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {17, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {18, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {19, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

#if (EQ_SECTION_MAX > 20)
    {20, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {21, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {22, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {23, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {24, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {25, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {26, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {27, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {28, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {29, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {30, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {31, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

};

const struct eq_seg_info eq_tab_country[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,     -2 << 20, (int)(0.7f * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     0 << 20, (int)(0.7f * (1 << 24))},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    0 << 20, (int)(0.7f * (1 << 24))},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    2 << 20, (int)(0.7f * (1 << 24))},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,    2 << 20, (int)(0.7f * (1 << 24))},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,   0 << 20, (int)(0.7f * (1 << 24))},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0 << 20, (int)(0.7f * (1 << 24))},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0 << 20, (int)(0.7f * (1 << 24))},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   4 << 20, (int)(0.7f * (1 << 24))},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4 << 20, (int)(0.7f * (1 << 24))},

#if (EQ_SECTION_MAX > 10)
    //10段之后频率值设置96k,目的是让10段之后的eq走直通
    {10, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {11, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {12, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {13, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {14, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {15, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {16, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {17, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {18, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {19, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

#if (EQ_SECTION_MAX > 20)
    {20, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {21, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {22, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {23, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {24, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {25, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {26, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {27, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {28, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {29, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {30, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {31, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif
};

const struct eq_seg_info eq_tab_jazz[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,     0 << 20, (int)(0.7f * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     0 << 20, (int)(0.7f * (1 << 24))},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    0 << 20, (int)(0.7f * (1 << 24))},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    4 << 20, (int)(0.7f * (1 << 24))},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,    4 << 20, (int)(0.7f * (1 << 24))},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,   4 << 20, (int)(0.7f * (1 << 24))},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0 << 20, (int)(0.7f * (1 << 24))},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   2 << 20, (int)(0.7f * (1 << 24))},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   3 << 20, (int)(0.7f * (1 << 24))},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4 << 20, (int)(0.7f * (1 << 24))},

#if (EQ_SECTION_MAX > 10)
    //10段之后频率值设置96k,目的是让10段之后的eq走直通
    {10, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {11, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {12, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {13, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {14, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {15, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {16, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {17, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {18, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {19, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

#if (EQ_SECTION_MAX > 20)
    {20, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {21, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {22, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {23, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {24, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {25, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {26, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {27, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {28, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {29, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {30, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {31, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif


};
struct eq_seg_info eq_tab_custom[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    0 << 20, (int)(0.7f * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,    0 << 20, (int)(0.7f * (1 << 24))},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,   0 << 20, (int)(0.7f * (1 << 24))},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   0 << 20, (int)(0.7f * (1 << 24))},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0 << 20, (int)(0.7f * (1 << 24))},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0 << 20, (int)(0.7f * (1 << 24))},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0 << 20, (int)(0.7f * (1 << 24))},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0 << 20, (int)(0.7f * (1 << 24))},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0 << 20, (int)(0.7f * (1 << 24))},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0 << 20, (int)(0.7f * (1 << 24))},

#if (EQ_SECTION_MAX > 10)
    //10段之后频率值设置96k,目的是让10段之后的eq走直通
    {10, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {11, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {12, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {13, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {14, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {15, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {16, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {17, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {18, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {19, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

#if (EQ_SECTION_MAX > 20)
    {20, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {21, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {22, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {23, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {24, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {25, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {26, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {27, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {28, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {29, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {30, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
    {31, EQ_IIR_TYPE_BAND_PASS, 96000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif

};


const EQ_CFG_SEG *eq_type_tab[EQ_MODE_MAX] = {
    eq_tab_normal, eq_tab_rock, eq_tab_pop, eq_tab_classic, eq_tab_jazz, eq_tab_country, eq_tab_custom
};
// 默认系数表，每个表对应的总增益,用户可修改
float type_gain_tab[EQ_MODE_MAX] = {0, 0, 0, 0, 0, 0, 0};
#endif



__attribute__((weak)) u32 get_eq_mode_tab(void)
{
#if !TCFG_USE_EQ_FILE
    return (u32)eq_type_tab;
#else
    return 0;
#endif
}

#if (EQ_SECTION_MAX==9)
static const u8 eq_mode_use_idx[] = {
    0,	1,	2,	3,	4,	5,	/*6,*/	7,	8,	9
};
#elif (EQ_SECTION_MAX==8)
static const u8 eq_mode_use_idx[] = {
    0,	/*1,*/	2,	3,	4,	5,	6,	7,	/*8,*/	9
};
#elif (EQ_SECTION_MAX==7)
static const u8 eq_mode_use_idx[] = {
    0,	/*1,*/	2,	3,	4,	5,	/*6,*/	7,	/*8,*/	9
};
#elif (EQ_SECTION_MAX==6)
static const u8 eq_mode_use_idx[] = {
    0,	/*1,*/	2,	3,	4,	/*5,*/	/*6,*/	7,	/*8,*/	9
};
#elif (EQ_SECTION_MAX==5)
static const u8 eq_mode_use_idx[] = {
    /*0,*/	1,	/*2,*/	3,	/*4,*/	5,	/*6,*/	7,	/*8,*/	9
};
#else
static const u8 eq_mode_use_idx[] = {
    0,	1,	2,	3,	4,	5,	6,	7,	8,	9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
};
#endif
/*
 *通话下行eq系数表
 * */
#if TCFG_EQ_ENABLE && TCFG_PHONE_EQ_ENABLE
#if EQ_FILE_CP_TO_CUSTOM
struct eq_seg_info phone_eq_tab_normal[] = {
#else
const struct eq_seg_info phone_eq_tab_normal[] = {
#endif
    {0, EQ_IIR_TYPE_HIGH_PASS, 200,   0 << 20, (int)(0.7f  * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 300,   0 << 20, (int)(0.7f  * (1 << 24))},
    {2, EQ_IIR_TYPE_BAND_PASS, 400,   0 << 20, (int)(0.7f  * (1 << 24))},
    {3, EQ_IIR_TYPE_BAND_PASS, 450,   0 << 20, (int)(0.7f  * (1 << 24))},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0 << 20, (int)(0.7f  * (1 << 24))},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0 << 20, (int)(0.7f  * (1 << 24))},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0 << 20, (int)(0.7f  * (1 << 24))},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0 << 20, (int)(0.7f  * (1 << 24))},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0 << 20, (int)(0.7f  * (1 << 24))},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0 << 20, (int)(0.7f  * (1 << 24))},
};
#endif

/*
 *通话上行eq系数表
 * */
#if EQ_FILE_CP_TO_CUSTOM
struct eq_seg_info ul_eq_tab_normal[] = {
#else
const struct eq_seg_info ul_eq_tab_normal[] = {
#endif
    {0, EQ_IIR_TYPE_HIGH_PASS, 200,   0 << 20, (int)(0.7f  * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 300,   0 << 20, (int)(0.7f  * (1 << 24))},
    {2, EQ_IIR_TYPE_BAND_PASS, 400,   0 << 20, (int)(0.7f  * (1 << 24))},
    {3, EQ_IIR_TYPE_BAND_PASS, 450,   0 << 20, (int)(0.7f  * (1 << 24))},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0 << 20, (int)(0.7f  * (1 << 24))},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0 << 20, (int)(0.7f  * (1 << 24))},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0 << 20, (int)(0.7f  * (1 << 24))},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0 << 20, (int)(0.7f  * (1 << 24))},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0 << 20, (int)(0.7f  * (1 << 24))},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0 << 20, (int)(0.7f  * (1 << 24))},
};



#define SONG_SECTION  EQ_SECTION_MAX
#define CALL_SECTION  3//下行段数,小于等于SONG_SECTION
#define UL_SECTION    3//上行段数,小于等于SONG_SECTION
/*
 *下行的宽频和窄频段数需一致，上行的宽频和窄频段数需要一致
 *表的每一项顺序不可修改
 * */
eq_tool_cfg eq_tool_tab[] = {
    {call_eq_mode,	(u8 *)"通话宽频下行EQ", 0x3000, CALL_SECTION, 1, {EQ_ONLINE_CMD_CALL_EQ_SEG, 0}},
    {call_narrow_eq_mode,	(u8 *)"通话窄频下行EQ", 0x3001, CALL_SECTION, 1, {EQ_ONLINE_CMD_CALL_EQ_SEG, 0}},
    {aec_eq_mode,	(u8 *)"通话宽频上行EQ", 0x3002, UL_SECTION,   1, {EQ_ONLINE_CMD_AEC_EQ_SEG,  0}},
    {aec_narrow_eq_mode,	(u8 *)"通话窄频上行EQ", 0x3003, UL_SECTION,   1, {EQ_ONLINE_CMD_AEC_EQ_SEG,  0}},
    {song_eq_mode,	(u8 *)"普通音频EQ", 	0x3004, SONG_SECTION, 2, {EQ_ONLINE_CMD_SONG_EQ_SEG, EQ_ONLINE_CMD_SONG_DRC}},
#ifdef DAC_OUTPUT_FRONT_LR_REAR_LR
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_LR)
    {fr_eq_mode,	(u8 *)"FR_EQ",      	0x3005, SONG_SECTION, 2, {EQ_ONLINE_CMD_SONG_EQ_SEG, EQ_ONLINE_CMD_SONG_DRC}},
#if TCFG_EQ_DIVIDE_ENABLE
    {rl_eq_mode,	(u8 *)"RL_EQ",      	0x3006, SONG_SECTION, 2, {EQ_ONLINE_CMD_SONG_EQ_SEG, EQ_ONLINE_CMD_SONG_DRC}},
    {rr_eq_mode,	(u8 *)"RR_EQ",      	0x3007, SONG_SECTION, 2, {EQ_ONLINE_CMD_SONG_EQ_SEG, EQ_ONLINE_CMD_SONG_DRC}},
#endif
#endif
#endif
};

/*----------------------------------------------------------------------------*/
/**@brief    eq 段数更新,需要在eq_init前就准备好
   @param    mode:call_eq_mode\call_narrow_eq_section等模式
   @param    section:段数最大为EQ_SECTION_MAX
   @return
   @note     下行的宽频和窄频段数需一致，上行的宽频和窄频段数需要一致
*/
/*----------------------------------------------------------------------------*/
void set_eq_tool_tab_section(u8 mode, u8 section)
{
#if TCFG_EQ_ONLINE_ENABLE
    eq_tool_tab[mode].section = section;
#endif
}

void drc_default_init(EQ_CFG *eq_cfg, u8 mode)
{
#if TCFG_DRC_ENABLE
    int i = mode;
    if (eq_cfg && eq_cfg->drc) {
        //限幅器的初始值
        int th = 0;//db -60db~0db
        int threshold = round(pow(10.0, th / 20.0) * 32768); // 0db:32768, -60db:33
        eq_cfg->cfg_parm[i].drc_parm.parm.drc.nband = 1;
        eq_cfg->cfg_parm[i].drc_parm.parm.drc.type = 1;
        eq_cfg->cfg_parm[i].drc_parm.parm.drc._p.limiter[0].attacktime = 5;
        eq_cfg->cfg_parm[i].drc_parm.parm.drc._p.limiter[0].releasetime = 500;
        eq_cfg->cfg_parm[i].drc_parm.parm.drc._p.limiter[0].threshold[0] = threshold;
        eq_cfg->cfg_parm[i].drc_parm.parm.drc._p.limiter[0].threshold[1] = 32768;
    }
#endif

}
#if EQ_FILE_CP_TO_CUSTOM
//eq_cfg_hw.bin中播歌eq曲线，当作用户自定义模式，参与效果切换.
//通话宽频上下行eq曲线也对应放到phone_eq_tab_normal、ul_eq_tab_normal
//EQ_FILE_CP_TO_CUSTOM 1使能时，同时板极文件中 TCFG_USE_EQ_FILE 配 0
static void eq_file_cp_to_custon_mode_fun(EQ_CFG *eq_cfg)
{
    if (eq_cfg->eq_type == EQ_TYPE_FILE) {
        eq_cfg->eq_type = EQ_TYPE_MODE_TAB;
        for (int i = 0; i < eq_cfg->mode_num; i++) {
            if ((i == song_eq_mode) || (i == call_eq_mode) || (i == aec_eq_mode)) { //播歌eq  通话宽频上下行eq
                u32 seg_num    = eq_cfg->cfg_parm[i].song_eq_parm.parm.par.seg_num;
                if (seg_num > eq_tool_tab[i].section) {
                    seg_num = eq_tool_tab[i].section;
                }
                void *tar = NULL;
                if (i == call_eq_mode) {
#if TCFG_PHONE_EQ_ENABLE
                    tar = eq_cfg->phone_eq_tab;
#endif
                } else if (i == aec_eq_mode) {
                    tar = eq_cfg->ul_eq_tab;
                } else if (i == song_eq_mode) {
                    tar = eq_tab_custom;
                    eq_cfg->type_gain_tab[EQ_MODE_MAX - 1] = eq_cfg->cfg_parm[i].song_eq_parm.parm.par.global_gain;
                }
                if (tar) {
                    memcpy(tar, eq_cfg->cfg_parm[i].song_eq_parm.parm.seg, seg_num * sizeof(EQ_CFG_SEG));
                    eq_cfg->seg_num[i] = seg_num;
                }
            }
        }
    }
}
#endif


/*----------------------------------------------------------------------------*/
/**@brief    在线调试，应答接口
   @param    *packet
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int ci_send_cmd(void *priv, u32 id, u8 *packet, int size)
{
    EQ_CFG *eq_cfg = (EQ_CFG *)priv;
    ASSERT(eq_cfg);
    if (eq_cfg->app) {
#if APP_ONLINE_DEBUG
        if (EQ_CONFIG_ID == id) {
            app_online_db_ack(eq_cfg->parse_seq, packet, size);
        }
#endif
    } else {
#if TCFG_ONLINE_ENABLE
        ci_send_packet(id, packet, size);
#endif
    }
    return 0;
}


int eq_init(void)
{
    audio_eq_init();
    eq_adjust_parm parm = {0};
#if TCFG_EQ_ONLINE_ENABLE
    parm.online_en = 1;
#endif
    if (config_filter_coeff_fade_en) {
        parm.fade_en = 1;
    }

#if TCFG_USE_EQ_FILE
    parm.file_en = 1;
#endif

#if EQ_FILE_CP_TO_CUSTOM
    parm.file_en = 1;
    parm.type_gain_tab = type_gain_tab;
#endif

#if TCFG_DRC_ENABLE
    parm.drc = 1;
#endif

#if TCFG_USER_TWS_ENABLE
    parm.tws = 1;
#endif

#if APP_ONLINE_DEBUG
    parm.app = 1;
#endif

#if (RCSP_ADV_EN)&&(JL_EARPHONE_APP_EN)&&(TCFG_DRC_ENABLE == 0)
    parm.limit_zero = 1;
#endif

#if TCFG_EQ_DIVIDE_ENABLE
    parm.stero = 1;
    parm.mode_num = 8;
#endif

    if (!parm.stero) {
        parm.mode_num = 5;// 一共有多少个模式
        /* #ifdef DAC_OUTPUT_FRONT_LR_REAR_LR */
        /* #if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_LR) */
        /* parm.mode_num = 6; */
        /* #endif */
        /* #endif */
    }

#if TCFG_PHONE_EQ_ENABLE
    parm.phone_eq_tab = (void *)phone_eq_tab_normal;
    parm.phone_eq_tab_size = ARRAY_SIZE(phone_eq_tab_normal);
#endif
    parm.ul_eq_tab = (void *)ul_eq_tab_normal;
    parm.ul_eq_tab_size = ARRAY_SIZE(ul_eq_tab_normal);

    parm.eq_tool_tab = eq_tool_tab;


    parm.eq_mode_use_idx = (u8 *)eq_mode_use_idx;
    parm.eq_type_tab = (void *)get_eq_mode_tab();
    parm.type_num = EQ_MODE_MAX;

    parm.section_max = EQ_SECTION_MAX;

    EQ_CFG *eq_cfg = eq_cfg_open(&parm);
    if (eq_cfg) {
        eq_cfg->priv = eq_cfg;
        eq_cfg->send_cmd = ci_send_cmd;
#if APP_ONLINE_DEBUG
        if (eq_cfg->app) {
            app_online_db_register_handle(DB_PKT_TYPE_EQ, eq_app_online_parse);
        }
#endif

#if EQ_FILE_CP_TO_CUSTOM
        eq_file_cp_to_custon_mode_fun(eq_cfg);
#else
        for (int i = 0; i < eq_cfg->mode_num; i++) {
            if (eq_cfg->eq_type == EQ_TYPE_MODE_TAB) {
                set_global_gain(eq_cfg, i, 0);
                drc_default_init(eq_cfg, i);
            }
        }
#endif
    }
    return 0;
}
__initcall(eq_init);


#endif
