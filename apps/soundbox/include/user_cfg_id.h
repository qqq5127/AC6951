#ifndef _USER_CFG_ID_H_
#define _USER_CFG_ID_H_

//=================================================================================//
//                            与APP CASE相关配置项[1 ~ 60]                         //
//=================================================================================//
#define 	VM_FM_EMITTER_FREQ				 1
// #define 	VM_FM_EMITTER_DIG_VOL		 	 2
// #define 	VM_MUSIC_LAST_DEV	    		 3
// #define 	VM_WAKEUP_PND					 4
#define     VM_USB_MIC_GAIN             	 5
#define     VM_ALARM_0                  	 6
#define     VM_ALARM_1                  	 7
#define     VM_ALARM_2                  	 8
#define     VM_ALARM_3                  	 9
#define     VM_ALARM_4                  	 10
#define     VM_ALARM_MASK               	 11
#define     VM_ALARM_NAME_0             	 12
#define     VM_ALARM_NAME_1             	 13
#define     VM_ALARM_NAME_2             	 14
#define     VM_ALARM_NAME_3             	 15
#define     VM_ALARM_NAME_4             	 16
#define     VM_FM_INFO				    	 17
// #define		VM_RTC_TRIM				    	 18

#define		CFG_RCSP_ADV_HIGH_LOW_VOL		 19
#define     CFG_RCSP_ADV_EQ_MODE_SETTING     20
#define     CFG_RCSP_ADV_EQ_DATA_SETTING     21
#define     ADV_SEQ_RAND                     22
#define     CFG_RCSP_ADV_TIME_STAMP          23
#define     CFG_RCSP_ADV_WORK_SETTING        24
#define     CFG_RCSP_ADV_MIC_SETTING         25
#define     CFG_RCSP_ADV_LED_SETTING         26
#define     CFG_RCSP_ADV_KEY_SETTING         27
#define     CFG_RCSP_MISC_DRC_SETTING        28
#define     CFG_RCSP_MISC_REVERB_ON_OFF      29
#define     VM_ALARM_RING_NAME_0             30
#define     VM_ALARM_RING_NAME_1             31
#define     VM_ALARM_RING_NAME_2             32
#define     VM_ALARM_RING_NAME_3             33
#define     VM_ALARM_RING_NAME_4             34
#define     VM_COLOR_LED_SETTING			 35
#define     VM_KARAOKE_EQ_SETTING			 36

// #define 	CFG_FLASH_BREAKPOINT			 29
// #define 	CFG_USB_BREAKPOINT				 30
// #define 	CFG_SD0_BREAKPOINT				 31
// #define 	CFG_SD1_BREAKPOINT				 32
// #define 	CFG_MUSIC_DEVICE				 33
// #define 	CFG_FM_RECEIVER_INFO			 34
// #define 	CFG_FM_TRANSMIT_INFO			 35
// #define 	CFG_AAP_MODE_INFO				 36
#define     CFG_UI_SYS_INFO					 37
//
//
//
#define    VM_TWS_ROLE                       38

#if (VM_ITEM_MAX_NUM > 128)

#define		CFG_FLASH_BREAKPOINT0		128
#define		CFG_FLASH_BREAKPOINT1		129
#define		CFG_FLASH_BREAKPOINT2		130
#define		CFG_FLASH_BREAKPOINT3		131
#define		CFG_FLASH_BREAKPOINT4		132
#define		CFG_FLASH_BREAKPOINT5		133
#define		CFG_FLASH_BREAKPOINT6		134
#define		CFG_FLASH_BREAKPOINT7		135
#define		CFG_FLASH_BREAKPOINT8		136
#define		CFG_FLASH_BREAKPOINT9		137
#define 	CFG_USB_BREAKPOINT0			138
#define 	CFG_USB_BREAKPOINT1			139
#define 	CFG_USB_BREAKPOINT2			140
#define 	CFG_USB_BREAKPOINT3			141
#define 	CFG_USB_BREAKPOINT4			142
#define 	CFG_USB_BREAKPOINT5			143
#define 	CFG_USB_BREAKPOINT6			144
#define 	CFG_USB_BREAKPOINT7			145
#define 	CFG_USB_BREAKPOINT8			146
#define 	CFG_USB_BREAKPOINT9			147
#define 	CFG_SD0_BREAKPOINT0		    148
#define 	CFG_SD0_BREAKPOINT1		    149
#define 	CFG_SD0_BREAKPOINT2		    150
#define 	CFG_SD0_BREAKPOINT3		    151
#define 	CFG_SD0_BREAKPOINT4		    152
#define 	CFG_SD0_BREAKPOINT5		    153
#define 	CFG_SD0_BREAKPOINT6		    154
#define 	CFG_SD0_BREAKPOINT7		    155
#define 	CFG_SD0_BREAKPOINT8		    156
#define 	CFG_SD0_BREAKPOINT9		    157
#define 	CFG_SD1_BREAKPOINT0		    158
#define 	CFG_SD1_BREAKPOINT1		    159
#define 	CFG_SD1_BREAKPOINT2		    160
#define 	CFG_SD1_BREAKPOINT3		    161
#define 	CFG_SD1_BREAKPOINT4		    162
#define 	CFG_SD1_BREAKPOINT5		    163
#define 	CFG_SD1_BREAKPOINT6		    164
#define 	CFG_SD1_BREAKPOINT7		    165
#define 	CFG_SD1_BREAKPOINT8		    166
#define 	CFG_SD1_BREAKPOINT9		    167
#define     CFG_CHGBOX_ADDR             168
#define 	CFG_REMOTE_DN_00		    169
#define 	CFG_REMOTE_DN_01		    170
#define 	CFG_REMOTE_DN_02		    171
#define 	CFG_REMOTE_DN_03		    172
#define 	CFG_REMOTE_DN_04		    173
#define 	CFG_REMOTE_DN_05		    174
#define 	CFG_REMOTE_DN_06		    175
#define 	CFG_REMOTE_DN_07		    176
#define 	CFG_REMOTE_DN_08		    177
#define 	CFG_REMOTE_DN_09		    178
#define 	CFG_REMOTE_DN_10		    179
#define 	CFG_REMOTE_DN_11		    180
#define 	CFG_REMOTE_DN_12		    181
#define 	CFG_REMOTE_DN_13		    182
#define 	CFG_REMOTE_DN_14		    183
#define 	CFG_REMOTE_DN_15		    184
#define 	CFG_REMOTE_DN_16		    185
#define 	CFG_REMOTE_DN_17		    186
#define 	CFG_REMOTE_DN_18		    187
#define 	CFG_REMOTE_DN_19		    188
#define		CFG_MUSIC_MODE				189

#endif//(VM_ITEM_MAX_NUM > 128)

#endif /* #ifndef _USER_CFG_ID_H_ */
