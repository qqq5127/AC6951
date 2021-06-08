#ifndef _AC3433_H_
#define _AC3433_H_

/******************************************************/
#define AC3433_TX_POWER_MIN			0
#define AC3433_TX_POWER_MAX			7
/******************************************************/

/******************************************************/
#define AC3433_UART_SEL          UART0_OUTPUT_CHAL
#define AC3433_UART_RATE         2000000

#define AC3433_UART_DATA_PORT	JL_PORTB
#define AC3433_UART_DATA_BIT	BIT(2)
/******************************************************/

void ac3433_iic_delay(void);
void ac3433_init(u16 fre);
void ac3433_set_freq(u16 fre);
void ac3433_set_power(u8 power, u16 freq);

#endif


