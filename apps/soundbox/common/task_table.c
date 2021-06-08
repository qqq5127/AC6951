#include "system/includes.h"
#include "app_config.h"


/*任务列表, 注意:stack_size设置为32*n*/
const struct task_info task_info_table[] = {
    {"app_core",            1,     896,  768  },
    {"sys_event",           6,     256,   0    },
    {"btctrler",            4,     512,   384  },
    {"btencry",             1,     512,   128  },
#if TCFG_USER_TWS_ENABLE
    {"tws",                 5,     512,   128  },
#endif
#if (TCFG_USER_TWS_ENABLE || TCFG_USER_BLE_ENABLE || TCFG_USER_EMITTER_ENABLE)
    {"btstack",             3,     768,   256  },
#else
    {"btstack",             3,     512,   256  },
#endif
#if TCFG_DEC2TWS_TASK_ENABLE
    {"local_dec",           3,     768,   128  },
#endif
#if ((TCFG_USER_TWS_ENABLE && TCFG_MIC_EFFECT_ENABLE)||(SOUNDCARD_ENABLE))
    {"audio_dec",           3,     768 + 128 + 128,   128  },
#else
    {"audio_dec",           3,     768 + 32,   128  },
#endif
    {"audio_enc",           3,     512,   128  },
#if TCFG_AUDIO_DEC_OUT_TASK
    {"audio_out",           2,     384,   0},
#endif
    {"aec",					2,	   768,   0	},
    {"aec_dbg",				3,	   128,   128	},
    {"update",				1,	   512,   0		},
#if(USER_UART_UPDATE_ENABLE)
    {"uart_update",	        1,	   256,   128	},
#endif
    {"systimer",		    6,	   128,   0		},
    {"dev_mg",           	3,     512,   512  },
#ifndef CONFIG_SOUNDBOX_FLASH_256K
    {"usb_msd",           	1,     512,   128   },
    {"usb_audio",           5,     256,   256  },
    {"plnk_dbg",            5,     256,   256  },
    {"adc_linein",          2,     768,   128  },
    /* {"src_write",           1,     768,   256 	}, */
    {"fm_task",             3,     512,   128  },
#endif
    {"enc_write",           1,     768,   0 	},
#if (RCSP_BTMATE_EN || RCSP_ADV_EN)
    {"rcsp_task",			4,	   768,   128	},
#endif
#if (TCFG_SPI_LCD_ENABLE||TCFG_SIMPLE_LCD_ENABLE)
    {"ui",           	    2,     768,   256  },
#else
    {"ui",                  3,     384 - 64,   128  },
#endif

#if(TCFG_CHARGE_BOX_ENABLE)
    {"chgbox_n",            6,     512,   128  },
#endif
#if (SMART_BOX_EN)
    {"smartbox",            4,     768,   128  },
#endif//SMART_BOX_EN
#if (GMA_EN)
    {"tm_gma",              3,     768,   256   },
#endif
#if RCSP_FILE_OPT
    {"file_bs",				1,	   768,	  64  },
#endif
#if TCFG_KEY_TONE_EN
    {"key_tone",            5,     256,   32},
#endif
#if TCFG_MIXER_CYCLIC_TASK_EN
    {"mix_out",             5,     256,   0},
#endif

    {"mic_stream",          5,     768,   128  },
#if(TCFG_HOST_AUDIO_ENABLE)
    {"uac_play",            6,     768,   0    },
    {"uac_record",          6,     768,   32   },
#endif
    {0, 0},
};


