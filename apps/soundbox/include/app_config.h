#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * 系统打印总开关
 */


#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif

#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)

#define CONFIG_DEBUG_ENABLE

#ifndef CONFIG_DEBUG_ENABLE
//#define CONFIG_DEBUG_LITE_ENABLE  //轻量级打印开关, 默认关闭
#endif

//*********************************************************************************//
//						APP应用默认配置	                                           //
//注意！！！注意！！！注意！！！         	                                       //
//1、以下配置各个app应用方向的默认宏定义	                                       //
//2、禁止直接修改这里的配置														   //
//3、应用方向的配置如果与以下默认配置不一致， 必须在自己板卡中进行重定义           //
//4、应用方向的配置,在这里只定义大的功能方向， 细节宏定义请在板卡中定义            //
//*********************************************************************************//

// #define TCFG_MIXER_EXT_ENABLE											0

#define TCFG_PC_BACKMODE_ENABLE											0

#define TCFG_ENC_WRITE_FILE_ENABLE		                                1

#define TCFG_MEDIA_LIB_USE_MALLOC										1

#define TCFG_AEC_ENABLE													1

#define TCFG_DEV_UPDATE_IF_NOFILE_ENABLE								1

#define TCFG_DEV_MANAGER_ENABLE											1

#define SD_BAUD_RATE_CHANGE_WHEN_SCAN                                   (12000000L)

//*********************************************************************************//
//						APP应用默认配置请在此之前定义	                           //
//						APP应用默认配置请在此之前定义	                           //
//						APP应用默认配置请在此之前定义	                           //
//*********************************************************************************//
#include "asm/clock_define.h"
#include "board_config.h"
#include "btcontroller_mode.h"
#include "user_cfg_id.h"


#define STYLE_JL_WTACH              (1)//彩屏demo
#define STYLE_JL_SOUNDBOX           (2)//点阵屏demo
#define STYLE_JL_CHARGE             (3)//点阵屏充电仓
#define STYLE_JL_LED7               (4)//led7
#define STYLE_UI_SIMPLE             (5)//没有ui框架




#ifdef CONFIG_SDFILE_ENABLE

#define SDFILE_DEV				"sdfile"
#define SDFILE_MOUNT_PATH     	"mnt/sdfile"

#if (USE_SDFILE_NEW)
#define SDFILE_APP_ROOT_PATH       	SDFILE_MOUNT_PATH"/app/"  //app分区
#define SDFILE_RES_ROOT_PATH       	SDFILE_MOUNT_PATH"/res/"  //资源文件分区
#else
#define SDFILE_RES_ROOT_PATH       	SDFILE_MOUNT_PATH"/C/"
#endif

#endif

#define RTC_CLK_RES_SEL             CLK_SEL_32K     //rtc时钟源选择:CLK_SEL_32K/CLK_SEL_12M/CLK_SEL_24M

//*********************************************************************************//
//                                 测试模式配置                                    //
//*********************************************************************************//
#if (CONFIG_BT_MODE != BT_NORMAL)
#undef  TCFG_BD_NUM
#define TCFG_BD_NUM						          1

#undef  TCFG_USER_TWS_ENABLE
#define TCFG_USER_TWS_ENABLE                      0     //tws功能使能

#undef  TCFG_USER_BLE_ENABLE
#define TCFG_USER_BLE_ENABLE                      1     //BLE功能使能

#undef  TCFG_AUTO_SHUT_DOWN_TIME
#define TCFG_AUTO_SHUT_DOWN_TIME		          0

#undef  TCFG_SYS_LVD_EN
#define TCFG_SYS_LVD_EN						      0

#undef  TCFG_LOWPOWER_LOWPOWER_SEL
#define TCFG_LOWPOWER_LOWPOWER_SEL                0

#undef TCFG_AUDIO_DAC_LDO_VOLT
#define TCFG_AUDIO_DAC_LDO_VOLT			    DUT_AUDIO_DAC_LDO_VOLT

#undef TCFG_LOWPOWER_POWER_SEL
#define TCFG_LOWPOWER_POWER_SEL				PWR_LDO15

#undef  TCFG_PWMLED_ENABLE
#define TCFG_PWMLED_ENABLE					DISABLE_THIS_MOUDLE

#undef  TCFG_ADKEY_ENABLE
#define TCFG_ADKEY_ENABLE                   DISABLE_THIS_MOUDLE

#undef  TCFG_IOKEY_ENABLE
#define TCFG_IOKEY_ENABLE					DISABLE_THIS_MOUDLE

#undef TCFG_TEST_BOX_ENABLE
#define TCFG_TEST_BOX_ENABLE			    0

#undef TCFG_AUTO_SHUT_DOWN_TIME
#define TCFG_AUTO_SHUT_DOWN_TIME		          0

#undef TCFG_POWER_ON_NEED_KEY
#define TCFG_POWER_ON_NEED_KEY				      0

#undef TCFG_SD0_ENABLE
#define TCFG_SD0_ENABLE				0

#undef TCFG_SD1_ENABLE
#define TCFG_SD1_ENABLE				0

#undef TCFG_APP_PC_EN
#define TCFG_APP_PC_EN					    0

#undef TCFG_PC_ENABLE
#define TCFG_PC_ENABLE                     0

#undef TCFG_UDISK_ENABLE
#define TCFG_UDISK_ENABLE				0

/* #undef TCFG_UART0_ENABLE
#define TCFG_UART0_ENABLE					DISABLE_THIS_MOUDLE */

#endif //(CONFIG_BT_MODE != BT_NORMAL)

/////要确保 上面 undef 后在include usb
#include "usb_common_def.h"


#if TCFG_USER_TWS_ENABLE

//*********************************************************************************//
//                                 对耳配置方式配置                                    //
//*********************************************************************************//
#define CONFIG_TWS_CONNECT_SIBLING_TIMEOUT    4    /* 开机或超时断开后对耳互连超时时间，单位s */
#define CONFIG_TWS_REMOVE_PAIR_ENABLE              /* 不连手机的情况下双击按键删除配对信息 */
#define CONFIG_TWS_POWEROFF_SAME_TIME         1    /*按键关机时两个耳机同时关机*/

#define ONE_KEY_CTL_DIFF_FUNC                 1    /*通过左右耳实现一个按键控制两个功能*/
#define CONFIG_TWS_SCO_ONLY_MASTER			  0	   /*通话的时候只有主机出声音*/

/* 配对方式选择 */
#define CONFIG_TWS_PAIR_BY_CLICK            0      /* 按键发起配对 */
#define CONFIG_TWS_PAIR_BY_AUTO             1      /* 开机自动配对 */
#define CONFIG_TWS_PAIR_BY_FAST_CONN        2      /* 开机快速连接,连接速度比自动配对快,不支持取消配对操作 */
#define CONFIG_TWS_PAIR_MODE                CONFIG_TWS_PAIR_BY_CLICK

#define CONFIG_TWS_USE_COMMMON_ADDR         1      /* tws 使用公共地址 */
#define CONFIG_TWS_PAIR_ALL_WAY             0      /* tws 任何时候 链接搜索  */

#define CONFIG_TWS_PAIR_BY_BOTH_SIDES       0      /* tws 要两边同时按下才能进行配对 */
#define CONFIG_TWS_DISCONN_NO_RECONN        0      /* 按键断开tws 不会回连 调用  tws_api_detach(TWS_DETACH_BY_REMOVE_NO_RECONN);*/


/* 声道确定方式选择 */
#define CONFIG_TWS_MASTER_AS_LEFT             0 //主机作为左耳
#define CONFIG_TWS_AS_LEFT_CHANNEL            1 //固定左耳
#define CONFIG_TWS_AS_RIGHT_CHANNEL           2 //固定右耳
#define CONFIG_TWS_LEFT_START_PAIR            3 //双击发起配对的耳机做左耳
#define CONFIG_TWS_RIGHT_START_PAIR           4 //双击发起配对的耳机做右耳
#define CONFIG_TWS_EXTERN_UP_AS_LEFT          5 //外部有上拉电阻作为左耳
#define CONFIG_TWS_EXTERN_DOWN_AS_LEFT        6 //外部有下拉电阻作为左耳
#define CONFIG_TWS_SECECT_BY_CHARGESTORE      7 //充电仓决定左右耳
#define CONFIG_TWS_CHANNEL_SELECT             CONFIG_TWS_LEFT_START_PAIR //配对方式选择

#define CONFIG_TWS_CHANNEL_CHECK_IO           IO_PORTA_07					//上下拉电阻检测引脚


#if CONFIG_TWS_PAIR_MODE != CONFIG_TWS_PAIR_BY_CLICK
#if (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_LEFT_START_PAIR) ||\
    (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_RIGHT_START_PAIR)
#undef CONFIG_TWS_CHANNEL_SELECT
#define CONFIG_TWS_CHANNEL_SELECT             CONFIG_TWS_MASTER_AS_LEFT
#endif

#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
#define CONFIG_TWS_AUTO_PAIR_WITHOUT_UNPAIR     /* 不取消配对也可以配对新的耳机 */
#endif

#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_FAST_CONN
#undef CONFIG_TWS_REMOVE_PAIR_ENABLE
#endif

#endif




#if TCFG_CHARGESTORE_ENABLE
#undef CONFIG_TWS_CHANNEL_SELECT
#define CONFIG_TWS_CHANNEL_SELECT             CONFIG_TWS_SECECT_BY_CHARGESTORE	//充电仓区分左右
#endif //TCFG_CHARGESTORE_ENABLE


#if TCFG_TEST_BOX_ENABLE && (!TCFG_CHARGESTORE_ENABLE)
#define CONFIG_TWS_SECECT_CHARGESTORE_PRIO    1 //测试盒配置左右耳优先
#else
#define CONFIG_TWS_SECECT_CHARGESTORE_PRIO    0
#endif //TCFG_TEST_BOX_ENABLE

//*********************************************************************************//
//                                 对耳电量显示方式                                //
//*********************************************************************************//

#if BT_SUPPORT_DISPLAY_BAT
#define CONFIG_DISPLAY_TWS_BAT_LOWER          1 //对耳手机端电量显示，显示低电量耳机的电量
#define CONFIG_DISPLAY_TWS_BAT_HIGHER         2 //对耳手机端电量显示，显示高电量耳机的电量
#define CONFIG_DISPLAY_TWS_BAT_LEFT           3 //对耳手机端电量显示，显示左耳的电量
#define CONFIG_DISPLAY_TWS_BAT_RIGHT          4 //对耳手机端电量显示，显示右耳的电量

#define CONFIG_DISPLAY_TWS_BAT_TYPE           CONFIG_DISPLAY_TWS_BAT_LOWER
#endif //BT_SUPPORT_DISPLAY_BAT

#define CONFIG_DISPLAY_DETAIL_BAT             0 //BLE广播显示具体的电量
#define CONFIG_NO_DISPLAY_BUTTON_ICON         1 //BLE广播不显示按键界面,智能充电仓置1

#endif //TCFG_USER_TWS_ENABLE

#if(CONFIG_CPU_BR25)
#if TCFG_USER_TWS_ENABLE
#define CONFIG_BT_RX_BUFF_SIZE  (12 * 1024)

#else
#if (TCFG_REVERB_ENABLE || TCFG_USER_BLE_ENABLE || RECORDER_MIX_EN)
#define CONFIG_BT_RX_BUFF_SIZE  (10 * 1024 + 512)
#else
#if (TCFG_BD_NUM == 2)
#define CONFIG_BT_RX_BUFF_SIZE  (14 * 1024)
#else
#define CONFIG_BT_RX_BUFF_SIZE  (12 * 1024)
#endif
#endif
#endif

#else
#define CONFIG_BT_RX_BUFF_SIZE  (12 * 1024)
#endif


#if(CONFIG_CPU_BR25)

#if TCFG_USER_TWS_ENABLE
#if TCFG_BT_SUPPORT_AAC
#define CONFIG_BT_TX_BUFF_SIZE  (4 * 1024)
#else
#define CONFIG_BT_TX_BUFF_SIZE  (3 * 1024)
#endif
#else
#if TCFG_BT_SUPPORT_AAC
#define CONFIG_BT_TX_BUFF_SIZE  (3 * 1024)
#else
#if (RECORDER_MIX_EN)
#define CONFIG_BT_TX_BUFF_SIZE  (2 * 1024 - 512)
#else
#if TCFG_USER_EMITTER_ENABLE
#define CONFIG_BT_TX_BUFF_SIZE  (8 * 1024)
#else
#if (TCFG_BD_NUM == 2)
#define CONFIG_BT_TX_BUFF_SIZE  (4 * 1024)
#else
#define CONFIG_BT_TX_BUFF_SIZE  (2 * 1024)
#endif
#endif
#endif

#endif
#endif

#else

#if TCFG_BT_SUPPORT_AAC
#define CONFIG_BT_TX_BUFF_SIZE  (4 * 1024)
#else
#define CONFIG_BT_TX_BUFF_SIZE  (3 * 1024)
#endif

#endif

#ifndef CONFIG_NEW_BREDR_ENABLE

#if TCFG_USER_TWS_ENABLE

#if CONFIG_LOCAL_TWS_ENABLE
#define CONFIG_TWS_BULK_POOL_SIZE  (4 * 1024)
#else
#define CONFIG_TWS_BULK_POOL_SIZE  (2 * 1024)
#endif
#endif
#endif

#if TCFG_APP_BT_EN

#else

#undef CONFIG_BT_RX_BUFF_SIZE
#define CONFIG_BT_RX_BUFF_SIZE  (0)

#undef CONFIG_BT_TX_BUFF_SIZE
#define CONFIG_BT_TX_BUFF_SIZE  (0)

#undef CONFIG_TWS_BULK_POOL_SIZE
#define CONFIG_TWS_BULK_POOL_SIZE  (0)

#endif

#if (CONFIG_BT_MODE != BT_NORMAL)
////bqb 如果测试3M tx buf 最好加大一点
#undef  CONFIG_BT_TX_BUFF_SIZE
#define CONFIG_BT_TX_BUFF_SIZE  (6 * 1024)

#endif

//*********************************************************************************//
//                                 电源切换配置                                    //
//*********************************************************************************//

#define PHONE_CALL_USE_LDO15	CONFIG_PHONE_CALL_USE_LDO15


#ifdef CONFIG_FPGA_ENABLE

#undef TCFG_CLOCK_OSC_HZ
#define TCFG_CLOCK_OSC_HZ		12000000

#endif

#if TCFG_APP_MUSIC_EN
#define CONFIG_SD_UPDATE_ENABLE
#define CONFIG_USB_UPDATE_ENABLE
#endif

//*********************************************************************************//
//                                  低功耗配置                                     //
//*********************************************************************************//
#if TCFG_IRKEY_ENABLE
#undef  TCFG_LOWPOWER_LOWPOWER_SEL
#define TCFG_LOWPOWER_LOWPOWER_SEL			0                     //开红外不进入低功耗
#endif  /* #if TCFG_IRKEY_ENABLE */

//*********************************************************************************//
//                                 升级配置                                        //
//*********************************************************************************//
#if (defined(CONFIG_CPU_BR30)) || defined(CONFIG_CPU_BR25) || defined(CONFIG_CPU_BR23)
//升级LED显示使能
#define UPDATE_LED_REMIND
//升级提示音使能
#define UPDATE_VOICE_REMIND
#endif

#if (defined(CONFIG_CPU_BR23) || defined(CONFIG_CPU_BR25))
//升级IO保持使能
//#define DEV_UPDATE_SUPPORT_JUMP           //目前只有br23\br25支持
#endif

#if (defined(CONFIG_CPU_BR23) || defined(CONFIG_CPU_BR25))
#define USER_UART_UPDATE_ENABLE           0//用于客户开发上位机或者多MCU串口升级方案

#define UART_UPDATE_SLAVE	0
#define UART_UPDATE_MASTER	1

//配置串口升级的角色
#define UART_UPDATE_ROLE	UART_UPDATE_SLAVE

#if USER_UART_UPDATE_ENABLE
#undef TCFG_CHARGESTORE_ENABLE
#undef TCFG_TEST_BOX_ENABLE
#define TCFG_CHARGESTORE_ENABLE				DISABLE_THIS_MOUDLE       //用户串口升级也使用了UART1
#endif

#endif  //USER_UART_UPDATE_ENABLE




//*********************************************************************************//
//                                 录音配置                                        //
//*********************************************************************************//
//录音文件夹名称定义，可以通过修改此处修改录音文件夹名称
#define REC_FOLDER_NAME				"JL_REC"

#endif

