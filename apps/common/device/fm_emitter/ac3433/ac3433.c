#include "app_config.h"
#include "system/includes.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "ac3433.h"


#if(TCFG_FM_EMITTER_AC3433_ENABLE == ENABLE)


#if IIC_SD_MULT_EN
extern bool iic_sd_mult_sd_suspend(void);
extern void iic_sd_mult_sd_resume(void);
#endif



volatile u32 ac3433_iic_delay_cnt = 0;

void ac3433_set_iic_delay(u32 delay_cnt)
{
    ac3433_iic_delay_cnt = delay_cnt;
}
void ac3433_iic_delay(void)
{
    delay(ac3433_iic_delay_cnt);
}

void ac3433_mult_detect(void)
{
#if ADKEY_IIC_MULT_EN
    while (g_adkey_busy) {
        clear_wdt();
        delay(5);
    }
#endif

#if AUX_SD_MULT_EN
    while (g_ad_aux_busy) {
        clear_wdt();
        delay(5);
    }
#endif

#if IIC_SD_MULT_EN
    while (1) {
        if (iic_sd_mult_sd_suspend()) {
            JL_PORTB->PU |= (BIT(3) | BIT(4) | BIT(5));
            break;
        }
        clear_wdt();
        delay(5);
    }
    iic_data_h();
    iic_data_in();
    iic_clk_h();
    iic_clk_in();

    iic_start();
    ac3433_iic_delay();
    ac3433_iic_delay();
    ac3433_iic_delay();
    ac3433_iic_delay();
    ac3433_iic_delay();
    iic_stop();
#endif
}

void ac3433_mult_release()
{
#if IIC_SD_MULT_EN
    iic_sd_mult_sd_resume();
#endif
}

extern void uart_tx_dac_api(void);
extern void uart_tx_dac(void);

void ac3433_uart_isr_callback(u8 uto_buf, void *p, u8 isr_flag)
{
    printf("ac3433_uart_isr_callback\n");

    if (isr_flag == UART_ISR_TYPE_WRITE_OVER) {
        JL_UART0->CON0 &= ~BIT(2);     //disable tx isr
        uart_tx_dac();
    }
}

s32 ac3433_uart_init(u32 baud)
{
    puts("--------ac3433_uart_init\n");

    JL_IOMAP->CON3 &= ~BIT(3);
    JL_IOMAP->CON1 &= ~(BIT(8) | BIT(9) | BIT(10) | BIT(11));	//output channel0 ->UT0_TX

    //任意IO口输出,所选则的IO口需要设置为输出, 且开上下拉.
    AC3433_UART_DATA_PORT->DIR &= ~AC3433_UART_DATA_BIT;
    AC3433_UART_DATA_PORT->DIE &= ~AC3433_UART_DATA_BIT;
    AC3433_UART_DATA_PORT->OUT &= ~AC3433_UART_DATA_BIT;
    AC3433_UART_DATA_PORT->PD |= AC3433_UART_DATA_BIT;
    AC3433_UART_DATA_PORT->PU |= AC3433_UART_DATA_BIT;

    set_uart_isr_callback(0, ac3433_uart_isr_callback);
    JL_UART0->BAUD = (UART_CLK / baud) / 3 - 1;
    JL_UART0->CON0 = BIT(13) | BIT(12) | BIT(10) | BIT(6) | BIT(4) | BIT(0);

    return 0;

}

void ac3433_tx_dac_data(void)
{
    uart_tx_dac_api();
}

void ac3433_iic_init(void)
{
    iic_init();
    ac3433_set_iic_delay(5000);
}

void ac3433_write_data(u8 chip_id, u8 iic_addr, u8 iic_dat)
{
    g_iic_busy  = 1;
    ac3433_mult_detect();
    iic_start();                //I2C启动
    iic_sendbyte(chip_id);         //写命令

    if (0xff != iic_addr) {
        iic_sendbyte(iic_addr);   //写地址
    }

    iic_sendbyte(iic_dat);      //写数据

    iic_stop();                 //I2C停止时序

    if (iic_dat == FT33_ADDR_READ) {
        iic_start();
        iic_sendbyte(FT33_ADDR_READ);
        iic_revbyte(0);
        iic_revbyte(1);
        iic_stop();
    }
    ac3433_mult_release();
    g_iic_busy = 0;
}

void ac3433_write(u8 chip_id, u8 iic_addr, u8 *iic_dat, u8 n)
{
    g_iic_busy  = 1;
    ac3433_mult_detect();
    iic_start();                //I2C启动
    iic_sendbyte(chip_id);         //写命令

    if (0xff != iic_addr) {
        iic_sendbyte(iic_addr);   //写地址
    }
    for (; n > 0; n--) {
        iic_sendbyte(*iic_dat++);      //写数据
    }
    iic_stop();                 //I2C停止时序
    ac3433_mult_release();
    g_iic_busy = 0;
}

void ac3433_read(u8 chip_id, u8 iic_addr, u8 *iic_dat)
{
    g_iic_busy = 1;
    ac3433_mult_detect();
    iic_start();                //I2C启动
    iic_sendbyte(chip_id);         //写命令
    if (0xff != iic_addr) {
        iic_sendbyte(iic_addr);   //写地址
    }
    *iic_dat = iic_revbyte(1);
    if (*iic_dat == FT33_ADDR_READ) {
        iic_start();
        iic_sendbyte(FT33_ADDR_READ);
        iic_revbyte(0);
        iic_revbyte(1);
        iic_stop();
    }
    iic_stop();
    ac3433_mult_release();
    g_iic_busy = 0;
}

void ac3433_readn(u8 chip_id, u8 iic_addr, u8 *iic_dat, u8 n)
{
    g_iic_busy = 1;
    ac3433_mult_detect();
    iic_start();                //I2C启动
    iic_sendbyte(chip_id);         //写命令
    if (0xff != iic_addr) {
        iic_sendbyte(iic_addr);   //写地址
    }
    for (; n > 1; n--) {
        *iic_dat++ = iic_revbyte(0);      //读数据
    }
    *iic_dat++ = iic_revbyte(1);
    iic_stop();                 //I2C停止时序
    ac3433_mult_release();
    g_iic_busy = 0;
}

void ac3433_write_and(u8 chip_id, u8 iic_addr, u8 n)
{
    u8 read_data;
    ac3433_read(FT33_ADDR_READ, iic_addr, &read_data);
    read_data = read_data & n;
    ac3433_write_data(chip_id, iic_addr, read_data);
}

void ac3433_write_or(u8 chip_id, u8 iic_addr, u8 n)
{
    u8 read_data;
    ac3433_read(FT33_ADDR_READ, iic_addr, &read_data);
    read_data = read_data | n;
    ac3433_write_data(chip_id, iic_addr, read_data);
}

void ac3433_interface_register(void)
{
    ft33_init_interface_register(ac3433_iic_init);
    ft33_write_data_interface_register(ac3433_write_data);
    ft33_write_datan_interface_register(ac3433_write);
    ft33_write_and_interface_register(ac3433_write_and);
    ft33_write_or_interface_register(ac3433_write_or);
    ft33_read_data_interface_register(ac3433_read);
    ft33_read_datan_interface_register(ac3433_readn);
    ft33_set_iic_delay_interface_register(ac3433_set_iic_delay);
}

/*----------------------------------------------------------------------------*/
/**@brief    AC3433 初始化函数
   @param    fre：初始化时设置的频点
   @return   无
   @note     void ac3433_init(u16 fre)
*/
/*----------------------------------------------------------------------------*/

void ac3433_init(u16 fre)
{

#if (FMTX_CHIP_OSC_SELECT == OSC_24M)
    ft33_set_clk(24000000L);
#else
    ft33_set_clk(12000000L);
#endif
    ac3433_interface_register();
    ft33_set_data_mode(0);
    ft33_set_channel(2);	//1:MONO 2:STEREO
    ft33_set_prem_cof(3);
    if (ft33_init(fre) == false) {
        printf("warning !!! ac3433 init err\n");
    }
    ft33_set_icp(1);
    ft33_set_cpib(7);

    ac3433_uart_init(AC3433_UART_RATE);
    timer3_init(ac3433_tx_dac_data);
}

/*
void ac3433_set_freq(u16 fre)
fre 是频率值
例子：90MHZ应该传入900
*/
/*----------------------------------------------------------------------------*/
/**@brief    AC3433 频点设置函数
   @param    fre：设置频点
   @return   无
   @note     void ac3433_set_freq(u16 fre)
*/
/*----------------------------------------------------------------------------*/
void irq_enable(u32 idx);
void irq_disable(u32 idx);
void ac3433_set_freq(u16 fre)
{
    irq_disable(63);
    ft33_set_freq(fre);
    irq_enable(63);
}

void ac3433_set_power(u8 power, u16 freq)
{
    if (power > AC3433_TX_POWER_MAX || power < AC3433_TX_POWER_MIN) {
        power = AC3433_TX_POWER_MAX;
    }
    ft33_set_power(power);
}

REGISTER_FM_EMITTER(ac3433) = {
    .name      = "ac3433",
    .init      = ac3433_init,
    .start     = NULL,
    .stop      = NULL,
    .set_fre   = ac3433_set_freq,
    .set_power = ac3433_set_power,
};

#endif //AC3433

