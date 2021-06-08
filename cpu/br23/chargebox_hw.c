#include "board_config.h"
#include "generic/typedef.h"
#include "generic/gpio.h"
#include "asm/gpio.h"
#include "asm/clock.h"
#include "device/chargebox.h"

#if(TCFG_CHARGE_BOX_ENABLE)

#define BUF_LEN     (64)

struct chargebox_handle {
    const struct chargebox_platform_data *data;
    u32 uart2_baud;
    u32 uart1_baud;
    u8 init_flag;
};

static u8 uart2_buf[BUF_LEN] __attribute__((aligned(4)));
static u8 uart1_buf[BUF_LEN] __attribute__((aligned(4)));
#define __this  (&hdl)
static struct chargebox_handle hdl;

extern void chargebox_data_deal(u8 cmd, u8 l_r, u8 *data, u8 length);
void __attribute__((weak)) chargebox_data_deal(u8 cmd, u8 l_r, u8 *data, u8 len)
{

}

___interrupt
static void uart2_isr(void)
{
    u32 rx_len, timeout;

    if ((JL_UART2->CON0 & BIT(2)) && (JL_UART2->CON0 & BIT(15))) {//发送完成
        JL_UART2->CON0 |= BIT(13);
        chargebox_data_deal(CMD_COMPLETE, EAR_L, NULL, 0);
    }
    if ((JL_UART2->CON0 & BIT(3)) && (JL_UART2->CON0 & BIT(14))) {//接收中断
        JL_UART2->CON0 |= BIT(12);
        JL_UART2->RXSADR = (u32)uart2_buf;
        JL_UART2->RXEADR = (u32)(uart2_buf + BUF_LEN);
        JL_UART2->RXCNT = BUF_LEN;
        chargebox_data_deal(CMD_RECVDATA, EAR_L, uart2_buf, BUF_LEN);
    }
    if ((JL_UART2->CON0 & BIT(5)) && (JL_UART2->CON0 & BIT(11))) {
        //OTCNT PND
        JL_UART2->CON0 |= BIT(7);//DMA模式
        JL_UART2->CON0 |= BIT(10);//清OTCNT PND
        asm volatile("nop");
        rx_len = JL_UART2->HRXCNT;//读当前串口接收数据的个数
        JL_UART2->CON0 |= BIT(12);//清RX PND(这里的顺序不能改变，这里要清一次)
        JL_UART2->RXSADR = (u32)uart2_buf;
        JL_UART2->RXEADR = (u32)(uart2_buf + BUF_LEN);
        JL_UART2->RXCNT = BUF_LEN;
        timeout = 20 * 1000000 /  __this->uart2_baud;
        JL_UART2->OTCNT = timeout * (clk_get("uart") / 1000000);
        chargebox_data_deal(CMD_RECVDATA, EAR_L, uart2_buf, rx_len);
    }
}
___interrupt
static void uart1_isr(void)
{
    u32 rx_len, timeout;


    if ((JL_UART1->CON0 & BIT(2)) && (JL_UART1->CON0 & BIT(15))) {//发送完成
        JL_UART1->CON0 |= BIT(13);
        /* printf("uart1 sned isr\n"); */
        chargebox_data_deal(CMD_COMPLETE, EAR_R, NULL, 0);
    }
    if ((JL_UART1->CON0 & BIT(3)) && (JL_UART1->CON0 & BIT(14))) {//接收中断
        /* putchar('Z'); */
        JL_UART1->CON0 |= BIT(12);
        JL_UART1->RXSADR = (u32)uart1_buf;
        JL_UART1->RXEADR = (u32)(uart1_buf + BUF_LEN);
        JL_UART1->RXCNT = BUF_LEN;
        chargebox_data_deal(CMD_RECVDATA, EAR_R, uart1_buf, BUF_LEN);
    }
    if ((JL_UART1->CON0 & BIT(5)) && (JL_UART1->CON0 & BIT(11))) {
        //OTCNT PND
        /* putchar('W'); */
        JL_UART1->CON0 |= BIT(7);//DMA模式
        JL_UART1->CON0 |= BIT(10);//清OTCNT PND
        asm volatile("nop");
        JL_UART1->CON0 |= BIT(12);//清RX PND(这里的顺序不能改变，这里要清一次)
        rx_len = JL_UART1->HRXCNT;//读当前串口接收数据的个数
        JL_UART1->RXSADR = (u32)uart1_buf;
        JL_UART1->RXEADR = (u32)(uart1_buf + BUF_LEN);
        JL_UART1->RXCNT = BUF_LEN;
        timeout = 20 * 1000000 / __this->uart1_baud;
        JL_UART1->OTCNT = timeout * (clk_get("lsb") / 1000000);
        /* printf("uart1 dma recvdata isr:%x\n",uart1_buf[0]); */
        /* put_buf(uart1_buf,rx_len); */
        chargebox_data_deal(CMD_RECVDATA, EAR_R, uart1_buf, rx_len);
    }
}


u8 chargebox_write(u8 l_r, u8 *data, u8 len)
{
    if ((len == 0) || (len > BUF_LEN)) {
        return 0;
    }
    if (l_r == EAR_L) {
        memcpy(uart2_buf, data, len);
        JL_UART2->TXADR = (u32)uart2_buf;
        JL_UART2->TXCNT = (u16)len;
    } else {
        memcpy(uart1_buf, data, len);
        JL_UART1->TXADR = (u32)uart1_buf;
        JL_UART1->TXCNT = (u16)len;
    }
    return len;
}

void chargebox_open(u8 l_r, u8 mode)
{
    if (mode == MODE_RECVDATA) {
        if (l_r == EAR_L) {
            JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
            gpio_uart_rx_input(__this->data->L_port, 2, INPUT_CH0);
            gpio_set_pull_up(__this->data->L_port, 1);
            JL_UART2->RXSADR = (u32)uart2_buf;
            JL_UART2->RXEADR = (u32)(uart2_buf + BUF_LEN);
            JL_UART2->RXCNT = BUF_LEN;
            JL_UART2->CON0 |= BIT(6) | BIT(5) | BIT(3);
            JL_UART2->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(0);
        } else {
            JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
            gpio_uart_rx_input(__this->data->R_port, 1, INPUT_CH3);
            gpio_set_pull_up(__this->data->R_port, 1);
            JL_UART1->RXSADR = (u32)uart1_buf;
            JL_UART1->RXEADR = (u32)(uart1_buf + BUF_LEN);
            JL_UART1->RXCNT = BUF_LEN;
            JL_UART1->CON0 |= BIT(6) | BIT(5) | BIT(3);
            JL_UART1->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(0);
        }
    } else {
        if (l_r == EAR_L) {
            JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
            gpio_output_channle(__this->data->L_port, CH1_UT2_TX);
            gpio_set_hd(__this->data->L_port, 0);
            gpio_set_hd0(__this->data->L_port, 0);
            JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10) | BIT(2) | BIT(0);
        } else {
            JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
            gpio_output_channle(__this->data->R_port, CH2_UT1_TX);
            gpio_set_hd(__this->data->R_port, 0);
            gpio_set_hd0(__this->data->R_port, 0);
            JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10) | BIT(2) | BIT(0);
        }
    }
}


void chargebox_close(u8 l_r)
{
    if (l_r == EAR_L) {
        JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
        gpio_set_pull_down(__this->data->L_port, 0);
        gpio_set_pull_up(__this->data->L_port, 0);
        gpio_set_die(__this->data->L_port, 1);
        gpio_set_hd(__this->data->L_port, 0);
        gpio_direction_output(__this->data->L_port, 1);
    } else {
        JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
        gpio_set_pull_down(__this->data->R_port, 0);
        gpio_set_pull_up(__this->data->R_port, 0);
        gpio_set_die(__this->data->R_port, 1);
        gpio_set_hd(__this->data->R_port, 0);
        gpio_direction_output(__this->data->R_port, 1);
    }
}

void chargebox_set_baud(u8 l_r, u32 baudrate)
{
    u32 uart_timeout;
    if (l_r == EAR_L) {
        __this->uart2_baud = baudrate;
        uart_timeout = 20 * 1000000 / __this->uart2_baud;
        JL_UART2->OTCNT = uart_timeout * (clk_get("lsb") / 1000000);
        JL_UART2->BAUD = (clk_get("uart") / __this->uart2_baud) / 4 - 1;
        JL_UART2->CON1 = (((clk_get("uart") / __this->uart2_baud) % 4) << 4);
    } else {
        __this->uart1_baud = baudrate;
        uart_timeout = 20 * 1000000 / __this->uart1_baud;
        JL_UART1->OTCNT = uart_timeout * (clk_get("lsb") / 1000000);
        JL_UART1->BAUD = (clk_get("uart") / __this->uart1_baud) / 4 - 1;
        JL_UART1->CON1 = (((clk_get("uart") / __this->uart1_baud) % 4) << 4);
    }
}

void chargebox_init(const struct chargebox_platform_data *data)
{
    u32 uart_timeout;
    __this->data = (struct chargebox_platform_data *)data;
    ASSERT(data);
    ASSERT(!(JL_UART2->CON0 & BIT(0)), "uart2 already used!\n");
    ASSERT(!(JL_UART1->CON0 & BIT(0)), "uart1 already used!\n");

    /////////////////////////////////////////////////////////////////////////////////////////////
    /* JL_CLOCK->CLK_CON1 |= BIT(11); */
    /* JL_CLOCK->CLK_CON1 &= ~BIT(10); */
    /////////////////////////////////////////////////////////////////////////////////////////////

    uart_timeout = 20 * 1000000 / __this->data->baudrate;
    __this->uart2_baud = __this->data->baudrate;
    __this->uart1_baud = __this->data->baudrate;
    //init uart2
    JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
    JL_UART2->OTCNT = uart_timeout * (clk_get("lsb") / 1000000);
    JL_UART2->BAUD = (clk_get("uart") / __this->data->baudrate) / 4 - 1;
    JL_UART2->CON1 = (((clk_get("uart") / __this->data->baudrate) % 4) << 4);
    gpio_set_pull_down(__this->data->L_port, 0);
    gpio_set_pull_up(__this->data->L_port, 1);
    gpio_set_die(__this->data->L_port, 1);
    gpio_direction_input(__this->data->L_port);
    request_irq(IRQ_UART2_IDX, 3, uart2_isr, 0);
    //used output_channel0 input_ch0
    JL_IOMAP->CON3 &= ~BIT(11);//不占用IO

    //init uart1
    JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
    JL_UART1->OTCNT = uart_timeout * (clk_get("lsb") / 1000000);
    JL_UART1->BAUD = (clk_get("uart") / __this->data->baudrate) / 4 - 1;
    JL_UART1->CON1 = (((clk_get("uart") / __this->data->baudrate) % 4) << 4);
    gpio_set_pull_down(__this->data->R_port, 0);
    gpio_set_pull_up(__this->data->R_port, 1);
    gpio_set_die(__this->data->R_port, 1);
    gpio_direction_input(__this->data->R_port);
    request_irq(IRQ_UART1_IDX, 3, uart1_isr, 0);
    //used output_channel1, input_ch3
    JL_IOMAP->CON3 &= ~BIT(7);//不占用IO

    __this->init_flag = 1;
}

static void clock_critical_enter(void)
{

}
static void clock_critical_exit(void)
{
    u32 uart_timeout;
    if (!__this->init_flag) {
        return;
    }
    JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
    JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
    //set uart2
    uart_timeout = 20 * 1000000 / __this->uart2_baud;
    JL_UART2->OTCNT = uart_timeout * (clk_get("lsb") / 1000000);
    JL_UART2->BAUD = (clk_get("uart") / __this->uart2_baud) / 4 - 1;
    JL_UART2->CON1 = (((clk_get("uart") / __this->uart2_baud) % 4) << 4);
    //set uart1
    uart_timeout = 20 * 1000000 / __this->uart1_baud;
    JL_UART1->OTCNT = uart_timeout * (clk_get("lsb") / 1000000);
    JL_UART1->BAUD = (clk_get("uart") / __this->uart1_baud) / 4 - 1;
    JL_UART1->CON1 = (((clk_get("uart") / __this->uart1_baud) % 4) << 4);
}
CLOCK_CRITICAL_HANDLE_REG(chargebox, clock_critical_enter, clock_critical_exit)

#endif//end if TCFG_CHARGE_BOX_ENABLE
