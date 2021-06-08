#include "key_event_deal.h"
#include "key_driver.h"
#include "app_config.h"
#include "board_config.h"
#include "app_task.h"
#ifdef CONFIG_BOARD_AC695X_LCD
/***********************************************************
 *				bt 模式的 adkey table
 ***********************************************************/
#if TCFG_APP_BT_EN
const u16 bt_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF, KEY_POWEROFF_HOLD,
    },
    [1] = {
        KEY_OK, KEY_MENU,
    },
    [2] = {
        KEY_DOWN, KEY_VOLUME_DEC, KEY_VOLUME_DEC,
    },
    [3] = {
        KEY_UP, KEY_VOLUME_INC, KEY_VOLUME_INC,
    },
    [4] = {
        KEY_NULL, KEY_MODE,
    },

};
#endif

/***********************************************************
 *				fm 模式的 adkey table
 ***********************************************************/
#if TCFG_APP_FM_EN
const u16 fm_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF, KEY_POWEROFF_HOLD,
    },
    [1] = {
        KEY_OK, KEY_MENU,
    },
    [2] = {
        KEY_DOWN, KEY_VOLUME_DEC, KEY_VOLUME_DEC,
    },
    [3] = {
        KEY_UP, KEY_VOLUME_INC, KEY_VOLUME_INC,
    },
    [4] = {
        KEY_NULL, KEY_MODE,
    },
};
#endif

/***********************************************************
 *				linein 模式的 adkey table
 ***********************************************************/
#if TCFG_APP_LINEIN_EN
const u16 linein_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF, KEY_POWEROFF_HOLD,
    },
    [1] = {
        KEY_OK, KEY_MENU,
    },
    [2] = {
        KEY_DOWN, KEY_VOLUME_DEC, KEY_VOLUME_DEC,
    },
    [3] = {
        KEY_UP, KEY_VOLUME_INC, KEY_VOLUME_INC,
    },
    [4] = {
        KEY_NULL, KEY_MODE,
    },

};
#endif

/***********************************************************
 *				music 模式的 adkey table
 ***********************************************************/
#if TCFG_APP_MUSIC_EN
const u16 music_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF, KEY_POWEROFF_HOLD,
    },
    [1] = {
        KEY_OK, KEY_MENU,
    },
    [2] = {
        KEY_DOWN, KEY_VOLUME_DEC, KEY_VOLUME_DEC,
    },
    [3] = {
        KEY_UP, KEY_VOLUME_INC, KEY_VOLUME_INC,
    },
    [4] = {
        KEY_NULL, KEY_MODE,
    },

};
#endif

/***********************************************************
 *				pc 模式的 adkey table
 ***********************************************************/
#if TCFG_APP_PC_EN
const u16 pc_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF, KEY_POWEROFF_HOLD,
    },
    [1] = {
        KEY_OK, KEY_MENU,
    },
    [2] = {
        KEY_DOWN
    },
    [3] = {
        KEY_UP
    },
    [4] = {
        KEY_NULL, KEY_MODE,
    },

};
#endif

/***********************************************************
 *				record 模式的 adkey table
 ***********************************************************/
#if TCFG_APP_RECORD_EN
const u16 record_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF,	KEY_POWEROFF_HOLD,
    },
    [1] = {
        KEY_OK, KEY_MENU
    },
    [2] = {
        KEY_DOWN, KEY_VOLUME_DEC, KEY_VOLUME_DEC,
    },
    [3] = {
        KEY_UP, KEY_VOLUME_INC, KEY_VOLUME_INC,
    },
    [4] = {
        KEY_ENC, KEY_MODE,
    },
};
#endif

/***********************************************************
 *				rtc 模式的 adkey table
 ***********************************************************/
#if TCFG_APP_RTC_EN
const u16 rtc_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF, KEY_POWEROFF_HOLD,
    },
    [1] = {
        KEY_OK, KEY_MENU
    },
    [2] = {
        KEY_DOWN, KEY_VOLUME_DEC, KEY_VOLUME_DEC,
    },
    [3] = {
        KEY_UP, KEY_VOLUME_INC, KEY_VOLUME_INC,
    },
    [4] = {
        KEY_NULL, KEY_MODE,
    },
};
#endif

/***********************************************************
 *				spdif 模式的 adkey table
 ***********************************************************/
#if TCFG_APP_SPDIF_EN
const u16 spdif_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF,		KEY_POWEROFF_HOLD,	KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [1] = {
        KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL,		KEY_NULL,				KEY_NULL
    },
    [2] = {
        KEY_MUSIC_PP,		KEY_POWEROFF,	KEY_POWEROFF_HOLD,	KEY_NULL,		KEY_NULL,				KEY_NULL
    },
    [3] = {
        KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL,		KEY_NULL,				KEY_NULL
    },
    [4] = {
        KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL,		KEY_SPDIF_SW_SOURCE,	KEY_NULL
    },
    [5] = {
        KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL,		KEY_NULL,				KEY_NULL
    },
    [6] = {
        KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL,		KEY_SPDIF_SW_SOURCE,	KEY_NULL
    },
    [7] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [8] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [9] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
};
#endif

/***********************************************************
 *				idle 模式的 adkey table
 ***********************************************************/
const u16 idle_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_NULL,	    KEY_POWER_ON,		KEY_POWER_ON_HOLD,	KEY_NULL,   	KEY_NULL,			KEY_NULL
    },
    [1] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,   	KEY_NULL,           KEY_NULL
    },
    [2] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,   	KEY_NULL,           KEY_NULL
    },
    [3] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,   	KEY_NULL,			KEY_NULL
    },
    [4] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,   	KEY_NULL,			KEY_NULL
    },
    [5] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,	    KEY_NULL,			KEY_NULL
    },
    [6] = {
        KEY_NULL,		KEY_NULL,		    KEY_NULL,			KEY_NULL,		KEY_NULL,	        KEY_NULL
    },
    [7] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [8] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [9] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
};
#endif
