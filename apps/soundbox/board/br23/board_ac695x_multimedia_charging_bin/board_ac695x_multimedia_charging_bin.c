#include "app_config.h"

#ifdef CONFIG_BOARD_AC695X_MULTIMEDIA_CHARGING_BIN

#include "system/includes.h"
#include "media/includes.h"
#include "asm/sdmmc.h"
#include "asm/chargestore.h"
#include "asm/charge.h"
#include "asm/pwm_led.h"
#include "asm/rtc.h"
#include "tone_player.h"
#include "audio_config.h"
#include "asm/iic_hw.h"
#include "asm/iic_soft.h"
#include "asm/spi.h"
#include "norflash.h"
#include "gSensor/gSensor_manage.h"
#include "key_event_deal.h"
#include "user_cfg.h"
#include "linein/linein_dev.h"
#include "usb/otg.h"
#include "ui/ui_api.h"
#include "dev_manager.h"
#include "asm/ctmu.h"
#include "asm/power/p33.h"
#include "ui_manage.h"
#include "app_power_manage.h"

#define LOG_TAG_CONST       BOARD
#define LOG_TAG             "[BOARD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

void board_power_init(void);

/*各个状态下默认的闪灯方式和提示音设置，如果USER_CFG中设置了USE_CONFIG_STATUS_SETTING为1，则会从配置文件读取对应的配置来填充改结构体*/
STATUS_CONFIG status_config = {
    //提示音设置
    .tone = {
        .charge_start  = IDEX_TONE_NONE,
        .charge_full   = IDEX_TONE_NONE,
        .power_on      = IDEX_TONE_POWER_ON,
        .power_off     = IDEX_TONE_POWER_OFF,
        .lowpower      = IDEX_TONE_LOW_POWER,
        .max_vol       = IDEX_TONE_MAX_VOL,
        .phone_in      = IDEX_TONE_NONE,
        .phone_out     = IDEX_TONE_NONE,
        .phone_activ   = IDEX_TONE_NONE,
        .bt_init_ok    = IDEX_TONE_BT_MODE,
        .bt_connect_ok = IDEX_TONE_BT_CONN,
        .bt_disconnect = IDEX_TONE_BT_DISCONN,
        .tws_connect_ok   = IDEX_TONE_TWS_CONN,
        .tws_disconnect   = IDEX_TONE_TWS_DISCONN,
    }
};

#define __this (&status_config)


// *INDENT-OFF*
/************************** UART config****************************/
#if TCFG_UART0_ENABLE
UART0_PLATFORM_DATA_BEGIN(uart0_data)
    .tx_pin = TCFG_UART0_TX_PORT,
    .rx_pin = TCFG_UART0_RX_PORT,
    .baudrate = TCFG_UART0_BAUDRATE,

    .flags = UART_DEBUG,
UART0_PLATFORM_DATA_END()
#endif //TCFG_UART0_ENABLE


/************************** SD config ****************************/
#if TCFG_SD0_ENABLE
/* int sdmmc_0_io_detect(const struct sdmmc_platform_data *data) */
/* { */
/*     return 1; */
/* } */

/* void sd_set_power(u8 enable) */
/* { */
/*     static u8 old_enable = 0xff; */
/*  */
/*     if(old_enable == enable){ */
/*         return; */
/*     } */
/*     if(enable){ */
/*         sdpg_config(1); */
/*     }else{ */
/*         sdpg_config(0); */
/*     } */
/*     old_enable = enable; */
/* } */

SD0_PLATFORM_DATA_BEGIN(sd0_data)
//	.port 					= 'B',  //sd0 support port 'A' and port 'B'
//	.data_width             = 1,
//	.speed                  = 3000000,
//	.detect_mode            = SD_CMD_DECT,
	.port 					= TCFG_SD0_PORTS,  //sd0 support port 'A' and port 'B'
	.data_width             = TCFG_SD0_DAT_MODE,
	.speed                  = TCFG_SD0_CLK,
	.detect_mode            = TCFG_SD0_DET_MODE,

	.priority				= 3,
#if (TCFG_SD0_DET_MODE == SD_IO_DECT)
	.detect_io              = TCFG_SD0_DET_IO,//当检测模式为io口检测是有效
	.detect_io_level        = TCFG_SD0_DET_IO_LEVEL,//0：低电平检测到卡。 1：高电平检测到卡
	.detect_func            = sdmmc_0_io_detect,//库函数，需要detect_io和detect_io_level两个元素作为参数。可以自行重写一个检测函数，在线返回1，不在线返回0，即可。
    .power                  = sd_set_power,//库函数，使用SDPG引脚。可以自行重写其他的SD卡电源控制函数，传入0：关电源。传入1，开电源。如果电源硬件已固定不可控，则该函数无效，可以填NULL
    /* .power                  = NULL, */
#elif (TCFG_SD0_DET_MODE == SD_CLK_DECT)
	.detect_io_level        = TCFG_SD0_DET_IO_LEVEL,//0：低电平检测到卡。 1：高电平检测到卡
	.detect_func            = sdmmc_0_clk_detect,//库函数，需要detect_io_level元素作为参数。可以自行重写一个检测函数，在线返回1，不在线返回0，即可。
    /* .power                  = sd_set_power,//库函数，使用SDPG引脚。可以自行重写其他的SD卡电源控制函数，传入0：关电源。传入1，开电源。如果电源硬件已固定不可控，则该函数无效，可以填NULL */
    .power                  = NULL,
#else
	.detect_func            = sdmmc_cmd_detect,
    .power                  = NULL,//cmd检测需要全程供电，建议用硬件固定电源。当然，可以自行写其他的SD卡电源控制函数，传入0：关电源。传入1，开电源。
    /* .power                  = sd_set_power, */
#endif
SD0_PLATFORM_DATA_END()
#endif

#if TCFG_SD1_ENABLE
SD1_PLATFORM_DATA_BEGIN(sd1_data)
//	.port 					= 'A',  //sd1 only support port 'A'
//	.data_width             = 1,
//	.speed                  = 3000000,
//	.detect_mode            = SD_CMD_DECT,
	.port 					= TCFG_SD1_PORTS,  //sd1 only support port 'A'
	.data_width             = TCFG_SD1_DAT_MODE,
	.speed                  = TCFG_SD1_CLK,
	.detect_mode            = TCFG_SD1_DET_MODE,

	.priority				= 3,
#if (TCFG_SD1_DET_MODE == SD_IO_DECT)
	.detect_io              = TCFG_SD1_DET_IO,//当检测模式为io口检测是有效
	.detect_io_level        = TCFG_SD1_DET_IO_LEVEL,//0：低电平检测到卡。 1：高电平检测到卡
	.detect_func            = sdmmc_1_io_detect,//库函数，需要detect_io和detect_io_level两个元素作为参数。可以自行重写一个检测函数，在线返回1，不在线返回0，即可。
    .power                  = sd_set_power,//库函数，使用SDPG引脚。可以自行重写其他的SD卡电源控制函数，传入0：关电源。传入1，开电源。如果电源硬件已固定不可控，则该函数无效，可以填NULL
    /* .power                  = NULL, */
#elif (TCFG_SD1_DET_MODE == SD_CLK_DECT)
	.detect_io_level        = TCFG_SD1_DET_IO_LEVEL,//0：低电平检测到卡。 1：高电平检测到卡
	.detect_func            = sdmmc_1_clk_detect,//库函数，需要detect_io_level元素作为参数。可以自行重写一个检测函数，在线返回1，不在线返回0，即可。
    .power                  = sd_set_power,//库函数，使用SDPG引脚。可以自行重写其他的SD卡电源控制函数，传入0：关电源。传入1，开电源。如果电源硬件已固定不可控，则该函数无效，可以填NULL
    /* .power                  = NULL, */
#else
	.detect_func            = sdmmc_cmd_detect,
    .power                  = NULL,//cmd检测需要全程供电，建议用硬件固定电源。当然，可以自行写其他的SD卡电源控制函数，传入0：关电源。传入1，开电源。
    /* .power                  = sd_set_power, */
#endif
SD1_PLATFORM_DATA_END()
#endif


/************************** CHARGE config****************************/
#if TCFG_CHARGE_ENABLE
CHARGE_PLATFORM_DATA_BEGIN(charge_data)
    .charge_en              = TCFG_CHARGE_ENABLE,              //内置充电使能
    .charge_poweron_en      = TCFG_CHARGE_POWERON_ENABLE,      //是否支持充电开机
    .charge_full_V          = TCFG_CHARGE_FULL_V,              //充电截止电压
    .charge_full_mA			= TCFG_CHARGE_FULL_MA,             //充电截止电流
    .charge_mA				= TCFG_CHARGE_MA,                  //充电电流
/*ldo5v拔出过滤值，过滤时间 = (filter*2 + 20)ms,ldoin<0.6V且时间大于过滤时间才认为拔出
 对于充满直接从5V掉到0V的充电仓，该值必须设置成0，对于充满由5V先掉到0V之后再升压到xV的
 充电仓，需要根据实际情况设置该值大小*/
	.ldo5v_off_filter		= 100,
/*ldo5v的10k下拉电阻使能,若充电舱需要更大的负载才能检测到插入时，请将该变量置1,默认值设置为1
  对于充电舱需要按键升压,且维持电压是从充电舱经过上拉电阻到充电口的舱，请将该值改为0*/
	.ldo5v_pulldown_en		= 1,
CHARGE_PLATFORM_DATA_END()
#endif//TCFG_CHARGE_ENABLE

/************************** DAC ****************************/
#if TCFG_AUDIO_DAC_ENABLE
struct dac_platform_data dac_data = {
    .ldo_volt       = TCFG_AUDIO_DAC_LDO_VOLT,                   //DACVDD等级.需要根据具体硬件来设置（高低压）可选:1.2V/1.3V/2.35V/2.5V/2.65V/2.8V/2.95V/3.1V
#if ((TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_LR) || (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_DUAL_LR_DIFF))
    .vcmo_en        = 0,                                         //四声道与双声道差分关闭VCOMO
#else
    .vcmo_en        = 1,                                         //是否打开VCOMO
#endif
    .output         = TCFG_AUDIO_DAC_CONNECT_MODE,               //DAC输出配置，和具体硬件连接有关，需根据硬件来设置
    .ldo_isel       = 3,
    .ldo_fb_isel    = 2,
    .lpf_isel       = 0x8,
};
#endif

/************************** ADC ****************************/
#if TCFG_AUDIO_ADC_ENABLE
struct adc_platform_data adc_data = {
/*MIC LDO电流档位设置：
    0:0.625ua    1:1.25ua    2:1.875ua    3:2.5ua*/
	.mic_ldo_isel   = TCFG_AUDIO_ADC_LDO_SEL,
/*MIC 是否省隔直电容：
    0: 不省电容  1: 省电容 */
#if ((TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_LR) || (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_DUAL_LR_DIFF))
	.mic_capless    = 0,//四声道与双声道差分使用，不省电容接法
#else
	.mic_capless    = TCFG_MIC_CAPLESS_ENABLE,
#endif
/*MIC免电容方案需要设置，影响MIC的偏置电压
    21:1.18K	20:1.42K 	19:1.55K 	18:1.99K 	17:2.2K 	16:2.4K 	15:2.6K		14:2.91K	13:3.05K 	12:3.5K 	11:3.73K
	10:3.91K  	9:4.41K 	8:5.0K  	7:5.6K		6:6K		5:6.5K		4:7K		3:7.6K		2:8.0K		1:8.5K				*/
    .mic_bias_res   = 16,
/*MIC LDO电压档位设置,也会影响MIC的偏置电压
    0:2.3v  1:2.5v  2:2.7v  3:3.0v */
	.mic_ldo_vsel  = 2,
/*MIC电容隔直模式使用内部mic偏置(PC7)*/
	.mic_bias_inside = 1,
/*保持内部mic偏置输出*/
	.mic_bias_keep = 0,
};
#endif/*TCFG_AUDIO_ADC_ENABLE*/

/************************** IO KEY ****************************/
#if TCFG_IOKEY_ENABLE
const struct iokey_port iokey_list[] = {
    {
        .connect_way = TCFG_IOKEY_POWER_CONNECT_WAY,          //IO按键的连接方式
        .key_type.one_io.port = TCFG_IOKEY_POWER_ONE_PORT,    //IO按键对应的引脚
        .key_value = 0,                                       //按键值
    },

    {
        .connect_way = TCFG_IOKEY_PREV_CONNECT_WAY,
        .key_type.one_io.port = TCFG_IOKEY_PREV_ONE_PORT,
        .key_value = 1,
    },

    {
        .connect_way = TCFG_IOKEY_NEXT_CONNECT_WAY,
        .key_type.one_io.port = TCFG_IOKEY_NEXT_ONE_PORT,
        .key_value = 2,
    },
};
const struct iokey_platform_data iokey_data = {
    .enable = TCFG_IOKEY_ENABLE,                              //是否使能IO按键
    .num = ARRAY_SIZE(iokey_list),                            //IO按键的个数
    .port = iokey_list,                                       //IO按键参数表
};

#if MULT_KEY_ENABLE
//组合按键消息映射表
//配置注意事项:单个按键按键值需要按照顺序编号,如power:0, prev:1, next:2
//bit_value = BIT(0) | BIT(1) 指按键值为0和按键值为1的两个按键被同时按下,
//remap_value = 3指当这两个按键被同时按下后重新映射的按键值;
const struct key_remap iokey_remap_table[] = {
	{.bit_value = BIT(0) | BIT(1), .remap_value = 3},
	{.bit_value = BIT(0) | BIT(2), .remap_value = 4},
	{.bit_value = BIT(1) | BIT(2), .remap_value = 5},
};

const struct key_remap_data iokey_remap_data = {
	.remap_num = ARRAY_SIZE(iokey_remap_table),
	.table = iokey_remap_table,
};
#endif

#endif

/************************** AD KEY ****************************/
#if TCFG_ADKEY_ENABLE
const struct adkey_platform_data adkey_data = {
    .enable = TCFG_ADKEY_ENABLE,                              //AD按键使能
    .adkey_pin = TCFG_ADKEY_PORT,                             //AD按键对应引脚
    .ad_channel = TCFG_ADKEY_AD_CHANNEL,                      //AD通道值
    .extern_up_en = TCFG_ADKEY_EXTERN_UP_ENABLE,              //是否使用外接上拉电阻
    .ad_value = {                                             //根据电阻算出来的电压值
        TCFG_ADKEY_VOLTAGE0,
        TCFG_ADKEY_VOLTAGE1,
        TCFG_ADKEY_VOLTAGE2,
        TCFG_ADKEY_VOLTAGE3,
        TCFG_ADKEY_VOLTAGE4,
        TCFG_ADKEY_VOLTAGE5,
        TCFG_ADKEY_VOLTAGE6,
        TCFG_ADKEY_VOLTAGE7,
        TCFG_ADKEY_VOLTAGE8,
        TCFG_ADKEY_VOLTAGE9,
    },
    .key_value = {                                             //AD按键各个按键的键值
        TCFG_ADKEY_VALUE0,
        TCFG_ADKEY_VALUE1,
        TCFG_ADKEY_VALUE2,
        TCFG_ADKEY_VALUE3,
        TCFG_ADKEY_VALUE4,
        TCFG_ADKEY_VALUE5,
        TCFG_ADKEY_VALUE6,
        TCFG_ADKEY_VALUE7,
        TCFG_ADKEY_VALUE8,
        TCFG_ADKEY_VALUE9,
    },
};
#endif


/************************** IR KEY ****************************/
#if TCFG_IRKEY_ENABLE
const struct irkey_platform_data irkey_data = {
	    .enable = TCFG_IRKEY_ENABLE,                              //IR按键使能
	    .port = TCFG_IRKEY_PORT,                                       //IR按键口
};
#endif
/************************** RDEC_KEY ****************************/
#if TCFG_RDEC_KEY_ENABLE
const struct rdec_device rdeckey_list[] = {
	{
		.index = RDEC0 ,
		.sin_port0 = TCFG_RDEC0_ECODE1_PORT,
		.sin_port1 = TCFG_RDEC0_ECODE2_PORT,
		.key_value0 = TCFG_RDEC0_KEY0_VALUE | BIT(7),
		.key_value1 = TCFG_RDEC0_KEY1_VALUE | BIT(7),
	},

	{
		.index = RDEC1 ,
		.sin_port0 = TCFG_RDEC1_ECODE1_PORT,
		.sin_port1 = TCFG_RDEC1_ECODE2_PORT,
		.key_value0 = TCFG_RDEC1_KEY0_VALUE | BIT(7),
		.key_value1 = TCFG_RDEC1_KEY1_VALUE | BIT(7),
	},

	{
		.index = RDEC2 ,
		.sin_port0 = TCFG_RDEC2_ECODE1_PORT,
		.sin_port1 = TCFG_RDEC2_ECODE2_PORT,
		.key_value0 = TCFG_RDEC2_KEY0_VALUE | BIT(7),
		.key_value1 = TCFG_RDEC2_KEY1_VALUE | BIT(7),
	},


};
const struct rdec_platform_data  rdec_key_data = {
	.enable = TCFG_RDEC_KEY_ENABLE,                              //是否使能RDEC按键
	.num = ARRAY_SIZE(rdeckey_list),                            //RDEC按键的个数
	.rdec = rdeckey_list,                                       //RDEC按键参数表
};
#endif

/**************************CTMU_TOUCH_KEY ****************************/
#if TCFG_CTMU_TOUCH_KEY_ENABLE
static const struct ctmu_key_port touch_ctmu_port[] = {
    {
        .port = TCFG_CTMU_TOUCH_KEY0_PORT,
        .key_value = TCFG_CTMU_TOUCH_KEY0_VALUE,
    },
    {
		.port = TCFG_CTMU_TOUCH_KEY1_PORT,
        .key_value = TCFG_CTMU_TOUCH_KEY1_VALUE,
    },
};

const struct ctmu_touch_key_platform_data ctmu_touch_key_data = {
    .num = ARRAY_SIZE(touch_ctmu_port),
	.press_cfg		= TCFG_CTMU_TOUCH_KEY_PRESS_CFG,
	.release_cfg0 	= TCFG_CTMU_TOUCH_KEY_RELEASE_CFG0,
	.release_cfg1 	= TCFG_CTMU_TOUCH_KEY_RELEASE_CFG1,
    .port_list = touch_ctmu_port,
};
#endif  /* #if TCFG_CTMU_TOUCH_KEY_ENABLE */

/************************** linein KEY ****************************/
#if TCFG_LINEIN_ENABLE
struct linein_dev_data linein_data = {
	.enable = TCFG_LINEIN_ENABLE,
	.port = TCFG_LINEIN_CHECK_PORT,
	.up = TCFG_LINEIN_PORT_UP_ENABLE,
	.down = TCFG_LINEIN_PORT_DOWN_ENABLE,
	.ad_channel = TCFG_LINEIN_AD_CHANNEL,
	.ad_vol = TCFG_LINEIN_VOLTAGE,
};
#endif

/************************** otg data****************************/
#if TCFG_OTG_MODE
struct otg_dev_data otg_data = {
    .usb_dev_en = TCFG_OTG_USB_DEV_EN,
	.slave_online_cnt = TCFG_OTG_SLAVE_ONLINE_CNT,
	.slave_offline_cnt = TCFG_OTG_SLAVE_OFFLINE_CNT,
	.host_online_cnt = TCFG_OTG_HOST_ONLINE_CNT,
	.host_offline_cnt = TCFG_OTG_HOST_OFFLINE_CNT,
	.detect_mode = TCFG_OTG_MODE,
	.detect_time_interval = TCFG_OTG_DET_INTERVAL,
};
#endif


/************************** rtc ****************************/

#if TCFG_RTC_ENABLE
const struct rtc_dev_data rtc_data = {
    .port = (u8)-1,
    .edge = 0,    //0 leading edge, 1 falling edge
    .port_en = false,
    .rtc_ldo = false,//使用内部ldo 供电
    .clk_res = RTC_CLK_RES_SEL,
};
#endif

/************************** PWM_LED ****************************/
#if TCFG_PWMLED_ENABLE
LED_PLATFORM_DATA_BEGIN(pwm_led_data)
	.io_mode = TCFG_PWMLED_IOMODE,              //推灯模式设置:支持单个IO推两个灯和两个IO推两个灯
	.io_cfg.one_io.pin = TCFG_PWMLED_PIN,       //单个IO推两个灯的IO口配置
LED_PLATFORM_DATA_END()
#endif


#if TCFG_UI_LED7_ENABLE
LED7_PLATFORM_DATA_BEGIN(led7_data)
	.pin_type = LED7_PIN7,
    .pin_cfg.pin7.pin[0] = IO_PORTC_00,
    .pin_cfg.pin7.pin[1] = IO_PORTC_01,
    .pin_cfg.pin7.pin[2] = IO_PORTC_02,
    .pin_cfg.pin7.pin[3] = IO_PORTC_03,
    .pin_cfg.pin7.pin[4] = IO_PORTC_04,
    .pin_cfg.pin7.pin[5] = IO_PORTC_05,
    .pin_cfg.pin7.pin[6] = IO_PORTB_02,
LED7_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = LED_7,
	.private_data = (void *)&led7_data,
};
#endif /* #if TCFG_UI_LED7_ENABLE */

#if TCFG_UI_LED1888_ENABLE
LED7_PLATFORM_DATA_BEGIN(led7_data)
	.pin_type = LED7_PIN7,
	.pin_cfg.pin7.pin[0] = IO_PORTB_00,
	.pin_cfg.pin7.pin[1] = IO_PORTB_01,
	.pin_cfg.pin7.pin[2] = IO_PORTB_02,
	.pin_cfg.pin7.pin[3] = IO_PORTB_03,
	.pin_cfg.pin7.pin[4] = IO_PORTB_04,
	.pin_cfg.pin7.pin[5] = IO_PORTB_05,
	.pin_cfg.pin7.pin[6] = (u8)-1,//IO_PORTB_06,
LED7_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = LED_7,
	.private_data = (void *)&led7_data,
};
#endif /* #if TCFG_UI_LED7_ENABLE */


#if TCFG_UI_LCD_SEG3X9_ENABLE
LCD_SEG3X9_PLATFORM_DATA_BEGIN(lcd_seg3x9_data)
	.vlcd = LCD_SEG3X9_VOLTAGE_3_3V,
	.bias = LCD_SEG3X9_BIAS_1_3,
	.pin_cfg.pin_com[0] = IO_PORTC_05,
	.pin_cfg.pin_com[1] = IO_PORTC_04,
	.pin_cfg.pin_com[2] = IO_PORTC_03,

	.pin_cfg.pin_seg[0] = IO_PORTA_00,
	.pin_cfg.pin_seg[1] = IO_PORTA_01,
	.pin_cfg.pin_seg[2] = IO_PORTA_02,
	.pin_cfg.pin_seg[3] = IO_PORTA_03,
	.pin_cfg.pin_seg[4] = IO_PORTA_04,
	.pin_cfg.pin_seg[5] = IO_PORTA_07,
	.pin_cfg.pin_seg[6] = IO_PORTA_12,
	.pin_cfg.pin_seg[7] = IO_PORTA_10,
	.pin_cfg.pin_seg[8] = IO_PORTA_09,
LCD_SEG3X9_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = LCD_SEG3X9,
	.private_data = (void *)&lcd_seg3x9_data,
};

#endif /* #if TCFG_UI_LCD_SEG3X9_ENABLE */

#if 0
const struct soft_iic_config soft_iic_cfg[] = {
    //iic0 data
    {
        .scl = TCFG_SW_I2C0_CLK_PORT,                   //IIC CLK脚
        .sda = TCFG_SW_I2C0_DAT_PORT,                   //IIC DAT脚
        .delay = TCFG_SW_I2C0_DELAY_CNT,                //软件IIC延时参数，影响通讯时钟频率
        .io_pu = 1,                                     //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
    },
#if 0
    //iic1 data
    {
        .scl = IO_PORTA_05,
        .sda = IO_PORTA_06,
        .delay = 50,
        .io_pu = 1,
    },
#endif
};


const struct hw_iic_config hw_iic_cfg[] = {
    //iic0 data
    {
        /*硬件IIC端口下选择
                     SCL         SDA
            'A': IO_PORT_DP   IO_PORT_DM
            'B': IO_PORTA_09  IO_PORTA_10
            'C': IO_PORTA_07  IO_PORTA_08
            'D': IO_PORTA_05  IO_PORTA_06
         */
        .port = TCFG_HW_I2C0_PORTS,
        .baudrate = TCFG_HW_I2C0_CLK,      //IIC通讯波特率
        .hdrive = 0,                       //是否打开IO口强驱
        .io_filter = 1,                    //是否打开滤波器（去纹波）
        .io_pu = 1,                        //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
    },
};
#endif




#if	TCFG_HW_SPI1_ENABLE
const struct spi_platform_data spi1_p_data = {
	.port = TCFG_HW_SPI1_PORT,
	.mode = TCFG_HW_SPI1_MODE,
	.clk  = TCFG_HW_SPI1_BAUD,
	.role = TCFG_HW_SPI1_ROLE,
};
#endif

#if	TCFG_HW_SPI2_ENABLE
const struct spi_platform_data spi2_p_data = {
	.port = TCFG_HW_SPI2_PORT,
	.mode = TCFG_HW_SPI2_MODE,
	.clk  = TCFG_HW_SPI2_BAUD,
	.role = TCFG_HW_SPI2_ROLE,
};
#endif

#if TCFG_NORFLASH_DEV_ENABLE
NORFLASH_DEV_PLATFORM_DATA_BEGIN(norflash_dev_data)
    .spi_hw_num     = TCFG_FLASH_DEV_SPI_HW_NUM,
    .spi_cs_port    = TCFG_FLASH_DEV_SPI_CS_PORT,
#if (TCFG_FLASH_DEV_SPI_HW_NUM == 1)
    .spi_pdata      = &spi1_p_data,
#elif (TCFG_FLASH_DEV_SPI_HW_NUM == 2)
    .spi_pdata      = &spi2_p_data,
#endif
NORFLASH_DEV_PLATFORM_DATA_END()
#endif


#if TCFG_SPI_LCD_ENABLE

LCD_SPI_PLATFORM_DATA_BEGIN(lcd_spi_data)

#if TCFG_LCD_OLED_ENABLE
    .pin_reset	= IO_PORTC_01,
    .pin_cs		= IO_PORTC_03,
    .pin_rs		= IO_PORTC_02,
    .pin_bl     = IO_PORTC_00,
#else
    .pin_reset	= -1,
    .pin_cs		= IO_PORTC_03,
    .pin_rs		= IO_PORTC_02,
    .pin_bl     = IO_PORTC_01,
#endif

#if (TCFG_TFT_LCD_DEV_SPI_HW_NUM == 1)
    .spi_cfg	= SPI1,
    .spi_pdata  = &spi1_p_data,
#elif (TCFG_TFT_LCD_DEV_SPI_HW_NUM == 2)
    .spi_cfg	= SPI2,
    .spi_pdata  = &spi2_p_data,
#endif
LED7_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = TFT_LCD,
	.private_data = (void *)&lcd_spi_data,
};
#endif


#if TCFG_GSENSOR_ENABLE
#if TCFG_DA230_EN

GSENSOR_PLATFORM_DATA_BEGIN(gSensor_data)
    .iic = 0,
    .gSensor_name = "da230",
    .gSensor_int_io = IO_PORTB_08,
GSENSOR_PLATFORM_DATA_END();
#endif      //end if DA230_EN
#endif      //end if CONFIG_GSENSOR_ENABLE


extern const struct device_operations mass_storage_ops;
REGISTER_DEVICES(device_table) = {
    /* { "audio", &audio_dev_ops, (void *) &audio_data }, */

/* #if TCFG_CHARGE_ENABLE */
/*     { "charge", &charge_dev_ops, (void *)&charge_data }, */
/* #endif */


#if TCFG_SD0_ENABLE
	{ "sd0", 	&sd_dev_ops, 	(void *) &sd0_data},
#endif
#if TCFG_SD1_ENABLE
	{ "sd1", 	&sd_dev_ops, 	(void *) &sd1_data},
#endif

#if TCFG_LINEIN_ENABLE
    { "linein",  &linein_dev_ops, (void *)&linein_data},
#endif
#if TCFG_OTG_MODE
    { "otg",     &usb_dev_ops, (void *) &otg_data},
#endif
#if TCFG_UDISK_ENABLE
    { "udisk0",   &mass_storage_ops , NULL},
#endif
#if TCFG_RTC_ENABLE
#if TCFG_USE_VIRTUAL_RTC
    { "rtc",   &rtc_simulate_ops , (void *)&rtc_data},
#else
    { "rtc",   &rtc_dev_ops , (void *)&rtc_data},
#endif
#endif
#if TCFG_NORFLASH_DEV_ENABLE
    { "norflash",   &norflash_dev_ops , (void *)&norflash_dev_data},
#endif
};

/************************** LOW POWER config ****************************/
const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,          //0：sniff时芯片不进入低功耗  1：sniff时芯片进入powerdown
    .btosc_hz         = TCFG_CLOCK_OSC_HZ,                   //外接晶振频率
    .delay_us       = TCFG_CLOCK_SYS_HZ / 1000000L,        //提供给低功耗模块的延时(不需要需修改)
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.0V  2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.6V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
	.osc_type       = OSC_TYPE_BT_OSC ,
#if TCFG_USE_VIRTUAL_RTC
	.virtual_rtc  = 1,
#endif
};



/************************** PWR config ****************************/
struct port_wakeup port0 = {
    .pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
    .edge       = FALLING_EDGE,                            //唤醒方式选择,可选：上升沿\下降沿
    .attribute  = BLUETOOTH_RESUME,                        //保留参数
    .iomap      = IO_PORTB_01,                             //唤醒口选择
};

const struct sub_wakeup sub_wkup = {
    .attribute  = BLUETOOTH_RESUME,
};

const struct charge_wakeup charge_wkup = {
    .attribute  = BLUETOOTH_RESUME,
};

const struct wakeup_param wk_param = {
    .port[1] = &port0,
    .sub = &sub_wkup,
    .charge = &charge_wkup,
};

void gSensor_wkupup_disable(void)
{
    log_info("gSensor wkup disable\n");
    power_wakeup_index_disable(1);
}

void gSensor_wkupup_enable(void)
{
    log_info("gSensor wkup enable\n");
    power_wakeup_index_enable(1);
}

void debug_uart_init(const struct uart_platform_data *data)
{
#if TCFG_UART0_ENABLE
    if (data) {
        uart_init(data);
    } else {
        uart_init(&uart0_data);
    }
#endif
}


STATUS *get_led_config(void)
{
    return &(__this->led);
}

STATUS *get_tone_config(void)
{
    return &(__this->tone);
}

u8 get_sys_default_vol(void)
{
    return 21;
}

u8 get_power_on_status(void)
{
#if TCFG_IOKEY_ENABLE
    struct iokey_port *power_io_list = NULL;
    power_io_list = iokey_data.port;

    if (iokey_data.enable) {
        if (gpio_read(power_io_list->key_type.one_io.port) == power_io_list->connect_way){
            return 1;
        }
    }
#endif

#if TCFG_ADKEY_ENABLE
    if (adkey_data.enable) {
        if (adc_get_value(adkey_data.ad_channel) < 10) {
            return 1;
        }
    }
#endif

    return 0;
}

static void board_devices_init(void)
{
#if TCFG_PWMLED_ENABLE
    ui_pwm_led_init(&pwm_led_data);
#endif

#if (TCFG_IOKEY_ENABLE || TCFG_ADKEY_ENABLE || TCFG_IRKEY_ENABLE || TCFG_TOUCH_KEY_ENABLE ||  TCFG_CTMU_TOUCH_KEY_ENABLE)
	key_driver_init();
#endif

#if TCFG_GSENSOR_ENABLE
    gravity_sensor_init(&gSensor_data);
#endif      //end if CONFIG_GSENSOR_ENABLE

#if TCFG_UI_ENABLE
	UI_INIT(&ui_cfg_data);
#endif /* #if TCFG_UI_ENABLE */

	return;
}

void uartSendInit();
extern void alarm_init();
extern void cfg_file_parse(u8 idx);
void board_init()
{
    board_power_init();
    adc_vbg_init();
    adc_init();
    cfg_file_parse(0);

#if TCFG_CHARGE_ENABLE
    extern int charge_init(const struct dev_node *node, void *arg);
    charge_init(NULL, (void *)&charge_data);
#endif

    if (!get_charge_online_flag()) {
        check_power_on_voltage();
    }

/* #if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE) */
	sdpg_config(1);
/* #endif */

#if 0	/* SPI 屏幕驱动测试 */
	log_info("test tft>>>>>>>\n");
	extern void SPI_LcdTest();
	SPI_LcdTest();
	log_info("test tft over\n");
#endif

	dev_manager_init();

	board_devices_init();

	if(get_charge_online_flag()){
    	power_set_mode(PWR_LDO15);
	}else{
    	power_set_mode(TCFG_LOWPOWER_POWER_SEL);
	}

    /* void bt_menu_list_init(); */
    /* bt_menu_list_init(); */

    //针对硅mic要输出1给mic供电
    if(!adc_data.mic_capless){
        gpio_set_pull_up(IO_PORTA_04, 0);
        gpio_set_pull_down(IO_PORTA_04, 0);
        gpio_set_direction(IO_PORTA_04, 0);
        gpio_set_output_value(IO_PORTA_04,1);
    }

    gpio_set_direction(IO_PORTB_06, 1);
    gpio_set_pull_up(IO_PORTB_06, 0);
    gpio_set_pull_down(IO_PORTB_06, 0);
    gpio_set_die(IO_PORTB_06, 0);


 #if TCFG_UART0_ENABLE
    if (uart0_data.rx_pin < IO_MAX_NUM) {
        gpio_set_die(uart0_data.rx_pin, 1);
    }
#endif

#ifdef AUDIO_PCM_DEBUG
	uartSendInit();
#endif

#if TCFG_RTC_ENABLE
    alarm_init();
#endif

}

/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
extern void dac_power_off(void);
extern void dac_sniff_power_off(void);
void board_set_soft_poweroff(void)
{
    u8 mode = (JL_SFC->CON >> 8) & 0x0f;

    u32 porta_value = 0xffff;
    u32 portb_value = 0xffff;
    u32 portc_value = 0xffff;

    extern u32 spi_get_port(void);
    if (spi_get_port() != 0) {
        if ((mode == 0b0101) || (mode == 0b0111)) {
            porta_value = 0x1fff;
        } else {
            porta_value = 0x9fff;
        }
    }

    gpio_write(MIC_HW_IO, 0);

    gpio_dir(GPIOA, 0, 16, porta_value, GPIO_OR);
    gpio_set_pu(GPIOA, 0, 16, ~porta_value, GPIO_AND);
    gpio_set_pd(GPIOA, 0, 16, ~porta_value, GPIO_AND);
    gpio_die(GPIOA, 0, 16, ~porta_value, GPIO_AND);
    gpio_dieh(GPIOA, 0, 16, ~porta_value, GPIO_AND);

    //保留长按Reset Pin - PB1
    portb_value &= ~BIT(1);
    gpio_dir(GPIOB, 0, 16, portb_value, GPIO_OR);
    gpio_set_pu(GPIOB, 0, 16, ~portb_value, GPIO_AND);
    gpio_set_pd(GPIOB, 0, 16, ~portb_value, GPIO_AND);
    gpio_die(GPIOB, 0, 16, ~portb_value, GPIO_AND);
    gpio_dieh(GPIOB, 0, 16, ~portb_value, GPIO_AND);

    gpio_dir(GPIOC, 0, 16, portc_value, GPIO_OR);
    gpio_set_pu(GPIOC, 0, 16, ~portc_value, GPIO_AND);
    gpio_set_pd(GPIOC, 0, 16, ~portc_value, GPIO_AND);
    gpio_die(GPIOC, 0, 16, ~portc_value, GPIO_AND);
    gpio_dieh(GPIOC, 0, 16, ~portc_value, GPIO_AND);

    gpio_set_pull_up(IO_PORT_DP, 0);
    gpio_set_pull_down(IO_PORT_DP, 0);
    gpio_set_direction(IO_PORT_DP, 1);
    gpio_set_die(IO_PORT_DP, 0);
    gpio_set_dieh(IO_PORT_DP, 0);

    gpio_set_pull_up(IO_PORT_DM, 0);
    gpio_set_pull_down(IO_PORT_DM, 0);
    gpio_set_direction(IO_PORT_DM, 1);
    gpio_set_die(IO_PORT_DM, 0);
    gpio_set_dieh(IO_PORT_DM, 0);

    VDDIOW_VOL_SEL(power_param.vddiow_lev);
#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
	sdpg_config(0);
#endif

    //dac_power_off();
}

#define     APP_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     APP_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

void sleep_exit_callback(u32 usec)
{
    /* putchar('>'); */
    APP_IO_DEBUG_1(A, 6);
}

void sleep_enter_callback(u8  step)
{
    /* 此函数禁止添加打印 */
    if (step == 1) {
        /* putchar('<'); */
        APP_IO_DEBUG_0(A, 6);
        dac_sniff_power_off();
		/* dac_power_off(); */
    } else {
        gpio_set_pull_up(IO_PORTA_03, 0);
        gpio_set_pull_down(IO_PORTA_03, 0);
        gpio_set_direction(IO_PORTA_03, 1);

        usb_iomode(1);

        gpio_set_pull_up(IO_PORT_DP, 0);
        gpio_set_pull_down(IO_PORT_DP, 0);
        gpio_set_direction(IO_PORT_DP, 1);
        gpio_set_die(IO_PORT_DP, 0);

        gpio_set_pull_up(IO_PORT_DM, 0);
        gpio_set_pull_down(IO_PORT_DM, 0);
        gpio_set_direction(IO_PORT_DM, 1);
        gpio_set_die(IO_PORT_DM, 0);
    }
}

void board_power_init(void)
{
    log_info("Power init : %s", __FILE__);

    power_init(&power_param);

    power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);

    power_keep_dacvdd_en(0);

    /* power_wakeup_init(&wk_param); */

#if (!TCFG_IOKEY_ENABLE && !TCFG_ADKEY_ENABLE)
    charge_check_and_set_pinr(0);
#endif
}

static void board_power_wakeup_init(void)
{
    power_wakeup_init(&wk_param);
#if TCFG_POWER_ON_NEED_KEY
    extern u8 power_reset_src;
    if ((power_reset_src & BIT(0)) || (power_reset_src & BIT(1))) {
#if TCFG_CHARGE_ENABLE
        log_info("is ldo5v wakeup:%d\n",is_ldo5v_wakeup());
        if (is_ldo5v_wakeup()) {
            return;
        }
        if (get_ldo5v_online_hw()) {
            return;
        }
        /*LDO5V,检测上升沿，用于检测ldoin插入*/
        LDO5V_EN(1);
        LDO5V_EDGE_SEL(0);
        LDO5V_PND_CLR();
        LDO5V_EDGE_WKUP_EN(1);
#endif
        power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);
        power_set_soft_poweroff();
    }
#endif
}
early_initcall(board_power_wakeup_init);
#endif//CONFIG_BOARD_AC695X_MULTIMEDIA_CHARGING_BIN
