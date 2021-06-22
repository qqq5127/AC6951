#include "key_event_deal.h"
#include "key_driver.h"
#include "app_config.h"
#include "board_config.h"
#include "app_task.h"

#ifdef CONFIG_BOARD_AC695X_DEMO
/***********************************************************
 *				bt 模式�?adkey table
 ***********************************************************/
#if TCFG_APP_BT_EN
const u16 bt_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_NULL,	KEY_POWEROFF,	KEY_POWEROFF_HOLD,			KEY_NULL,		KEY_NULL,	KEY_NULL
    },
    [1] = {
        KEY_CHANGE_MODE,	KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [2] = {
        KEY_MUSIC_PREV, KEY_VOL_DOWN,		KEY_VOL_DOWN,	KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [3] = {
        KEY_MUSIC_NEXT,	KEY_VOL_UP,			KEY_VOL_UP,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [4] = {
        KEY_MUSIC_PP,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,	KEY_NULL
    },
    [5] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,		KEY_NULL
    },
    [6] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
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
 *				fm 模式�?adkey table
 ***********************************************************/
#if TCFG_APP_FM_EN
const u16 fm_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
		//单击						 //长按 				 //hold 				//抬起						//双击								//三击
		[0] = {
				KEY_NULL, KEY_POWEROFF, KEY_POWEROFF_HOLD,			KEY_NULL, 	KEY_NULL, KEY_NULL
		},
		[1] = {
				KEY_CHANGE_MODE,	KEY_NULL, 	KEY_NULL, 	KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[2] = {
				KEY_FM_PREV_STATION, KEY_VOL_DOWN, 	KEY_VOL_DOWN, KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[3] = {
				KEY_FM_NEXT_STATION, KEY_VOL_UP, 		KEY_VOL_UP, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[4] = {
				KEY_NULL, 	KEY_FM_SCAN_ALL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, KEY_NULL
		},
		[5] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 	KEY_NULL
		},
		[6] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[7] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[8] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[9] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},

};
#endif

/***********************************************************
 *				linein 模式�?adkey table
 ***********************************************************/
#if TCFG_APP_LINEIN_EN
const u16 linein_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
		[0] = {
				KEY_NULL, KEY_POWEROFF, KEY_POWEROFF_HOLD,			KEY_NULL, 	KEY_NULL, KEY_NULL
		},
		[1] = {
				KEY_CHANGE_MODE,	KEY_NULL, 	KEY_NULL, 	KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[2] = {
				KEY_NULL, KEY_VOL_DOWN, 	KEY_VOL_DOWN, KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[3] = {
				KEY_NULL, KEY_VOL_UP, 		KEY_VOL_UP, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[4] = {
				KEY_NULL, 	KEY_FM_SCAN_ALL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, KEY_NULL
		},
		[5] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 	KEY_NULL
		},
		[6] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[7] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[8] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[9] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},

};
#endif

/***********************************************************
 *				music 模式�?adkey table
 ***********************************************************/
#if TCFG_APP_MUSIC_EN
const u16 music_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
		[0] = {
				KEY_NULL, KEY_POWEROFF, KEY_POWEROFF_HOLD,			KEY_NULL, 	KEY_NULL, KEY_NULL
		},
		[1] = {
				KEY_CHANGE_MODE,	KEY_NULL, 	KEY_NULL, 	KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[2] = {
				KEY_MUSIC_PREV, KEY_VOL_DOWN, 	KEY_VOL_DOWN, KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[3] = {
				KEY_MUSIC_NEXT, KEY_VOL_UP, 		KEY_VOL_UP, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[4] = {
				KEY_MUSIC_PP, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, KEY_NULL
		},
		[5] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 	KEY_NULL
		},
		[6] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[7] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[8] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[9] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},

};
#endif

/***********************************************************
 *				pc 模式�?adkey table
 ***********************************************************/
#if TCFG_APP_PC_EN
const u16 pc_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
		[0] = {
				KEY_NULL, KEY_POWEROFF, KEY_POWEROFF_HOLD,			KEY_NULL, 	KEY_NULL, KEY_NULL
		},
		[1] = {
				KEY_CHANGE_MODE,	KEY_NULL, 	KEY_NULL, 	KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[2] = {
				KEY_MUSIC_PREV, KEY_VOL_DOWN, 	KEY_VOL_DOWN, KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[3] = {
				KEY_MUSIC_NEXT, KEY_VOL_UP, 		KEY_VOL_UP, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[4] = {
				KEY_MUSIC_PP, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, KEY_NULL
		},
		[5] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 	KEY_NULL
		},
		[6] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[7] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[8] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[9] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},

};
#endif

/***********************************************************
 *				record 模式�?adkey table
 ***********************************************************/
#if TCFG_APP_RECORD_EN
const u16 record_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF,		KEY_POWEROFF_HOLD,	KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [1] = {
        KEY_NULL,			KEY_VOL_DOWN,	KEY_VOL_DOWN,	KEY_NULL,		KEY_NULL,				KEY_NULL
    },
    [2] = {
        KEY_NULL,			KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL,				KEY_NULL
    },
    [3] = {
        KEY_NULL,			KEY_VOL_UP,		KEY_VOL_UP,		KEY_NULL,		KEY_NULL,				KEY_NULL
    },
    [4] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [5] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_ENC_START,			KEY_NULL
    },
    [6] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
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
 *				rtc 模式�?adkey table
 ***********************************************************/
#if TCFG_APP_RTC_EN
const u16 rtc_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF,		KEY_POWEROFF_HOLD,	KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [1] = {
        KEY_RTC_DOWN,		KEY_RTC_DOWN,	KEY_RTC_DOWN,	KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [2] = {
        KEY_RTC_SW_POS,		KEY_RTC_SW,		KEY_NULL,		KEY_NULL,		KEY_NULL,				KEY_NULL
    },
    [3] = {
        KEY_RTC_UP,			KEY_RTC_UP,		KEY_RTC_UP,		KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [4] = {
        KEY_NULL,			KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_REVERB_OPEN,				KEY_NULL
    },
    [5] = {
        KEY_NULL,			KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL,				KEY_NULL
    },
    [6] = {
        KEY_NULL,			KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL,				KEY_NULL
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
 *				spdif 模式�?adkey table
 ***********************************************************/
#if TCFG_APP_SPDIF_EN
const u16 spdif_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
		[0] = {
				KEY_NULL, KEY_POWEROFF, KEY_POWEROFF_HOLD,			KEY_NULL, 	KEY_NULL, KEY_NULL
		},
		[1] = {
				KEY_CHANGE_MODE,	KEY_NULL, 	KEY_NULL, 	KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[2] = {
				KEY_NULL, KEY_VOL_DOWN, 	KEY_VOL_DOWN, KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[3] = {
				KEY_NULL, KEY_VOL_UP, 		KEY_VOL_UP, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[4] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, KEY_NULL
		},
		[5] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 	KEY_NULL
		},
		[6] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[7] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[8] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},
		[9] = {
				KEY_NULL, 	KEY_NULL, 		KEY_NULL, 		KEY_NULL, 	KEY_NULL, 		KEY_NULL
		},

};
#endif

/***********************************************************
 *				idle 模式�?adkey table
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
