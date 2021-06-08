#ifndef __LAMP_H__
#define __LAMP_H__

#include "generic/typedef.h"
#include "app_config.h"


#define UI_LED_OFF              0
#define UI_LED_ELECTRIC_SOUND   BIT(0)
#define UI_LED_PITCH_SOUND      BIT(1)
#define UI_LED_MAGIC_SOUND      BIT(2)
#define UI_LED_BOOM_SOUND       BIT(3)
#define UI_LED_MIC_SOUND        BIT(4)

#define UI_LED_SELF_DODGE_SOUND      BIT(0)
#define UI_LED_SELF_RED              BIT(1)
#define UI_LED_SELF_BLUE_FLASH       BIT(2)


#define TCFG_MY_LED_RED_PORT     IO_PORTA_07
#define TCFG_MY_LED_BLUE_PORT    IO_PORTA_04
#define TCFG_MY_LED1_PORT    IO_PORTB_06
#define TCFG_MY_LED2_PORT    IO_PORTB_05
#define TCFG_MY_LED3_PORT    IO_PORTB_02
#define TCFG_MY_LED4_PORT    IO_PORTB_00

void soundcard_led_all_on(void);
void soundcard_led_all_off(void);
void soundcard_low_power_led(u8 onoff);
void soundcard_led_set(u8 led, u8 sw);
void soundcard_led_mode(u8 mode, u8 sw);
void soundcard_led_self_set(u8 led, u8 sw);
void soundcard_led_init(void);

#endif//__LAMP_H__
