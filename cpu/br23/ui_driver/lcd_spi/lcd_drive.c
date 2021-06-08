/* LCD 调试等级，
 * 0只打印错误，
 * 1打印错误和警告，
 * 2全部内容都调试内容打印 */
#define SPI_LCD_DEBUG_ENABLE    0

#include "includes.h"
#include "app_config.h"
#include "ui/ui_api.h"
#include "system/includes.h"
#include "system/timer.h"
#include "asm/spi.h"
#include "clock_cfg.h"
#include "asm/mcpwm.h"



#if (TCFG_UI_ENABLE && (TCFG_SPI_LCD_ENABLE || TCFG_SIMPLE_LCD_ENABLE))

#include "ui/ui_sys_param.h"

/* 选择SPIx模块作为推屏通道，0、1、2 */
/* 但SPI0通常作为外部falsh使用，一般不用SPI0 */
#define SPI_MODULE_CHOOSE   TCFG_TFT_LCD_DEV_SPI_HW_NUM

/* 中断使能，一般推屏不需要 */

#ifndef LCD_SPI_INTERRUPT_ENABLE
#define LCD_SPI_INTERRUPT_ENABLE 1
#endif


#if LCD_SPI_INTERRUPT_ENABLE
#if SPI_MODULE_CHOOSE == 0
#define IRQ_SPI_IDX     IRQ_SPI0_IDX
#elif SPI_MODULE_CHOOSE == 1
#define IRQ_SPI_IDX     IRQ_SPI1_IDX
#elif SPI_MODULE_CHOOSE == 2
#define IRQ_SPI_IDX     IRQ_SPI2_IDX
#else
#error  "error! SPI_MODULE_CHOOSE defien error!"
#endif
#endif

int clk_get(const char *name);
int clk_set(const char *name, int clk);
void spi_clear_pending(spi_dev spi);
u8 spi_get_pending(spi_dev spi);
void spi_dma_set_addr_for_isr(spi_dev spi, void *buf, u32 len, u8 rw);;
void spi_dma_wait_finish();
void wdt_clear();
static int __lcd_backlight_ctrl(u8 on);

/* 屏幕驱动的接口 */
extern struct spi_lcd_init dev_drive;
struct lcd_spi_platform_data *spi_dat = NULL;
static int spi_pnd = false;
static u8  backlight_status = 0;
static u8  lcd_sleep_in     = 0;
static volatile u8 is_lcd_busy = 0;
#define __this (&dev_drive)


#if LCD_SPI_INTERRUPT_ENABLE
/* SPI中断函数 */
// 注：dma模式在发送数据时内部已经清理中断pnd
__attribute__((interrupt("")))
static void spi_isr()
{
    if (spi_get_pending(spi_dat->spi_cfg)) {
        /* printf("spi_isr\n"); */
        /* spi_clear_pending(spi_dat->spi_cfg); */
        if (is_lcd_busy) {
            is_lcd_busy = 0;
        }
        spi_set_ie(spi_dat->spi_cfg, 0);
    }
}
#endif

static void clock_critical_enter()
{

}

static void clock_critical_exit()
{
    if (spi_dat && spi_dat->spi_cfg) {
        spi_set_baud(spi_dat->spi_cfg, spi_get_baud(spi_dat->spi_cfg));
    }
}
CLOCK_CRITICAL_HANDLE_REG(spi_lcd, clock_critical_enter, clock_critical_exit);

static void spi_init(int spi_cfg)
{
    int err;
    // spi gpio init

    err = spi_open(spi_cfg);
    if (err < 0) {
        lcd_e("open spi falid\n");
    }
#if LCD_SPI_INTERRUPT_ENABLE
    // 配置中断优先级，中断函数
    /* spi_set_ie(spi_cfg, 1); */
    request_irq(IRQ_SPI_IDX, 3, spi_isr, 0);
#endif
}

// io口操作
void lcd_reset_l()
{
    gpio_direction_output((u32)spi_dat->pin_reset, 0);
}
void lcd_reset_h()
{
    gpio_direction_output((u32)spi_dat->pin_reset, 1);
}
void lcd_cs_l()
{
    gpio_direction_output((u32)spi_dat->pin_cs, 0);
}
void lcd_cs_h()
{
    gpio_direction_output((u32)spi_dat->pin_cs, 1);
}
void lcd_rs_l()
{
    gpio_direction_output((u32)spi_dat->pin_rs, 0);
}
void lcd_rs_h()
{
    gpio_direction_output((u32)spi_dat->pin_rs, 1);
}

void lcd_bl_l()
{
    gpio_direction_output((u32)spi_dat->pin_bl, 0);
}

void lcd_bl_h()
{
    gpio_direction_output((u32)spi_dat->pin_bl, 1);
}


u8 lcd_bl_io()
{
    return spi_dat->pin_bl;
}

void spi_dma_wait_finish()
{
    if (spi_pnd) {
        while (!spi_get_pending(spi_dat->spi_cfg)) {
            wdt_clear();
        }
        spi_clear_pending(spi_dat->spi_cfg);
        spi_pnd = false;
    }
}

int __spi_dma_send(spi_dev spi, void *buf, u32 len, u8 wait)
{
    int err = 0;

    if (!wait || spi_pnd) {
        spi_dma_wait_finish();
    }
    spi_dma_set_addr_for_isr(spi_dat->spi_cfg, buf, len, 0);
    spi_pnd = true;
    asm("csync");

    if (wait) {
        spi_dma_wait_finish();
    }

    return err;
}

void spi_dma_send_byte(u8 dat)
{
    int err = 0;
    u32 _dat __attribute__((aligned(4))) = 0;

    ((u8 *)(&_dat))[0] = dat;

    if (spi_dat) {
        err = __spi_dma_send(spi_dat->spi_cfg, &_dat, 1, 1);
    }

    if (err < 0) {
        lcd_e("spi dma send byte timeout\n");
    }
}


u8 lcd_spi_recv_byte()
{
    int err;
    int ret;

#if 0
    ret = spi_recv_byte_for_isr(spi_dat->spi_cfg);
    asm("csync");
    spi_pnd = true;
    spi_dma_wait_finish();
#else
    ret = spi_recv_byte(spi_dat->spi_cfg, &err);
    spi_pnd = false;
#endif

    return ret;
}

int lcd_spi_send_byte(u8 byte)
{
    int ret;

#if 0
    spi_send_byte_for_isr(spi_dat->spi_cfg, byte);
    asm("csync");
    spi_pnd = true;
    spi_dma_wait_finish();
#else
    ret = spi_send_byte(spi_dat->spi_cfg, byte);
    spi_pnd = false;
#endif

    return 0;
}

void lcd_spi_set_bit_mode(int mode)
{
    spi_set_bit_mode(spi_dat->spi_cfg, mode);
}

void lcd_spi_dma_send_wait(u8 *map, u32 size)
{
    int err = 0;

    if (spi_dat) {
        err = __spi_dma_send(spi_dat->spi_cfg, map, size, 1);
    }

    if (err < 0) {
        lcd_e("spi dma send map timeout\n");
    }

}

void spi_dma_send_map(u8 *map, u32 size)
{
    int err = 0;

    if (spi_dat) {
        err = __spi_dma_send(spi_dat->spi_cfg, map, size, 0);
    }

    if (err < 0) {
        lcd_e("spi dma send map timeout\n");
    }

}

void spi_dma_recv_data(u8 *buf, u32 size)
{
    int err = 0;

    if (spi_dat) {
        err = spi_dma_recv(spi_dat->spi_cfg, buf, size);
    }

    if (err < 0) {
        lcd_e("spi dma recv timeout\n");
    }
}


static void spi_init_code(const InitCode *code, u8 cnt)
{
    u8 i, j;

    for (i = 0; i < cnt; i++) {
        if (code[i].cmd == REGFLAG_DELAY) {
            extern void wdt_clear(void);
            wdt_clear();
            delay2ms(code[i].cnt / 2);
        } else {
            __this->WriteComm(code[i].cmd);
            lcd_d("cmd:%x ", code[i].cmd);
            for (j = 0; j < code[i].cnt; j++) {
                __this->WriteData(code[i].dat[j]);
                lcd_d("%02x ", code[i].dat[j]);
            }
            lcd_d("\n");
        }
    }
}

static void lcd_dev_init(void *p)
{
    struct ui_devices_cfg *cfg = (struct ui_devices_cfg *)p;
    int err = 0;
    spi_dat = (struct lcd_spi_platform_data *)cfg->private_data;
    ASSERT(spi_dat, "Error! spi io not config");
    lcd_d("spi pin rest:%d, cs:%d, rs:%d, spi:%d\n", spi_dat->pin_reset, spi_dat->pin_cs, spi_dat->pin_rs, spi_dat->spi_cfg);


    if (spi_dat->pin_reset != -1) {
        gpio_direction_output((u32)spi_dat->pin_reset, 1);
    }
    gpio_direction_output((u32)spi_dat->pin_cs, 1);
    gpio_direction_output((u32)spi_dat->pin_rs, 1);

    if (!__this->soft_spi) {
        spi_init(spi_dat->spi_cfg);
    }

    if (__this->Reset) { // 如果有硬件复位
        __this->Reset();
    }

    if (__this->initcode && __this->initcode_cnt) {
        spi_init_code(__this->initcode, __this->initcode_cnt);  // 初始化屏幕
    } else if (__this->Init) {
        __this->Init();
    }

}

static int lcd_init(void *p)
{
    printf("lcd_init ...\n");
    lcd_dev_init(p);
    return 0;
}

static int lcd_get_screen_info(struct lcd_info *info)
{
    info->width = __this->lcd_width;
    info->height = __this->lcd_height;
    info->color_format = __this->color_format;
    info->interface = __this->interface;
    info->col_align = __this->column_addr_align;
    info->row_align = __this->row_addr_align;
    info->bl_status = backlight_status;
    ASSERT(info->col_align, " = 0, lcd driver column address align error, default value is 1");
    ASSERT(info->row_align, " = 0, lcd driver row address align error, default value is 1");

    return 0;
}


static int lcd_buffer_malloc(u8 **buf, u32 *size)
{
    *buf = __this->dispbuf;
    *size = __this->bufsize;
    return 0;
}

static int lcd_buffer_free(u8 *buf)
{
    return 0;
}

static int lcd_set_draw_area(u16 xs, u16 xe, u16 ys, u16 ye)
{
    if ((is_lcd_busy == 0x11) || lcd_sleep_in) {
        return 0;
    }
    is_lcd_busy = 1;
    spi_set_ie(spi_dat->spi_cfg, 0);
    spi_dma_wait_finish();
    __this->SetDrawArea(xs, xe, ys, ye);
    /* is_lcd_busy = 0; */
    return 0;
}

static int lcd_draw(u8 *buf, u32 len, u8 wait)
{
    if ((is_lcd_busy == 0x11) || lcd_sleep_in) {
        return 0;
    }
    /* is_lcd_busy = 1; */
    /* int clk = clk_get("sys"); */
    /* if (clk != 192000000) { */
    /* clk_set("sys", 192000000L); */
    /* } */

    if (wait) {
        __this->WriteMap(buf, len);
#if LCD_SPI_INTERRUPT_ENABLE
        spi_set_ie(spi_dat->spi_cfg, 1);
#else
        is_lcd_busy = 0;
#endif
    } else {
        __this->WriteMap(buf, len);
        is_lcd_busy = 0;
    }
    return 0;
}

static int lcd_draw_gage(u8 *buf, u8 page_star, u8 page_len)
{
    if ((is_lcd_busy == 0x11) || lcd_sleep_in) {
        return 0;
    }
    __this->WritePAGE(buf, page_star, page_len);
    is_lcd_busy = 0;
    return 0;
}




static int lcd_clear_screen(u16 color)
{
    int i;
    int buffer_lines;
    int remain;
    int draw_line;
    int y;

    if (__this->color_format == LCD_COLOR_RGB565) {
        buffer_lines = __this->bufsize / __this->lcd_width / 2;
        for (i = 0; i < buffer_lines * __this->lcd_width; i++) {
            __this->dispbuf[2 * i] = color >> 8;
            __this->dispbuf[2 * i + 1] = color;
        }

        y = 0;
        remain = __this->lcd_height;
        while (remain) {
            draw_line = buffer_lines > remain ? remain : buffer_lines;
            lcd_set_draw_area(0, __this->lcd_width - 1, y, y + draw_line - 1);
            lcd_draw(__this->dispbuf, draw_line * __this->lcd_width * 2, 0);
            remain -= draw_line;
            y += draw_line;
        }
        spi_dma_wait_finish();
    } else if (__this->color_format == LCD_COLOR_MONO) {
        __this->SetDrawArea(0, -1, 0, -1);
        memset(__this->dispbuf, 0x00, __this->lcd_width * __this->lcd_height / 8);
        __this->WriteMap(__this->dispbuf, __this->lcd_width * __this->lcd_height / 8);
    } else {
        ASSERT(0, "the color_format %d not support yet!", __this->color_format);
    }

    return 0;
}

int lcd_backlight_ctrl(u8 on)
{
    while (is_lcd_busy);

    is_lcd_busy = 0x11;
    __lcd_backlight_ctrl(on);
    is_lcd_busy = 0;

    return 0;
}

int lcd_sleep_ctrl(u8 enter)
{
    if ((!!enter) == lcd_sleep_in) {
        return -1;
    }
    while (is_lcd_busy);
    is_lcd_busy = 0x11;

    if (enter) {
        if (__this->EnterSleep) {
            __this->EnterSleep();
        }
        lcd_sleep_in = true;
    } else {
        if (__this->ExitSleep) {
            __this->ExitSleep();
        }
        clock_add_set(LCD_UI_CLK);
        lcd_sleep_in = false;
    }

    is_lcd_busy = 0;
    return 0;
}

static int __lcd_backlight_ctrl(u8 on)
{
    static u8 first_power_on = true;

    if (on) {
#if (TCFG_SPI_LCD_ENABLE)
        on = get_backlight_brightness() & 0xff;
#else
        on = 100;
#endif
    }

    if (first_power_on) {
        backlight_status = true;
        lcd_clear_screen(0x0000);
        backlight_status = false;
        //os_time_dly(6);
        first_power_on = false;
    }

    if (__this->BackLightCtrl) {
        if (on) {
            clock_add_set(LCD_UI_CLK);
            if (__this->ExitSleep) {
                __this->ExitSleep();
            }
            __this->BackLightCtrl(on);
        } else {
            __this->BackLightCtrl(false);
            if (__this->EnterSleep) {
                __this->EnterSleep();
            }
            clock_remove_set(LCD_UI_CLK);
        }
        lcd_sleep_in = !!!on;
        backlight_status = on;
    }

    return 0;
}

int lcd_backlight_status()
{
    return backlight_status;
}

struct lcd_interface *lcd_get_hdl()
{
    struct lcd_interface *p;

    ASSERT(lcd_interface_begin != lcd_interface_end, "don't find lcd interface!");
    for (p = lcd_interface_begin; p < lcd_interface_end; p++) {
        return p;
    }
    return NULL;
}

/*
 * 注意，MCPWM的输出IO可以是固定的硬件IO或者是映射到任何一个IO，但由于映射到非硬件IO需要占用一个OUTPUT_CHANNEL,
 * 因此建议设计是尽量使用带MCPWM输出的硬件IO。硬件IO一共有六组，分别是
 * (IO_PORTA_00, IO_PORTA_01)
 * (IO_PORTB_00, IO_PORTB_02)
 * (IO_PORTB_04, IO_PORTB_06)
 * (IO_PORTB_09, IO_PORTB_10)
 * (IO_PORTA_09, IO_PORTA_10)
 * (IO_PORTC_04, IO_PORTC_05)
 * 上述五组分别对应ch0、ch1、ch2、ch3、ch4、ch5。使用对应硬件IO时，pwm_p_data.pwm_ch_num和pwm_p_data.pwm_timer_num需要改成对应的值，
 * 详细请参考mcpwm.c中的mcpwm_test
 */
struct pwm_platform_data lcd_pwm_p_data;
void lcd_mcpwm_init()
{
    extern void mcpwm_init(struct pwm_platform_data * arg);
    lcd_pwm_p_data.pwm_aligned_mode = pwm_edge_aligned;         //边沿对齐
    lcd_pwm_p_data.frequency = 10000;                           //KHz

    lcd_pwm_p_data.pwm_ch_num = pwm_ch0;                        //通道0
    lcd_pwm_p_data.pwm_timer_num = pwm_timer0;                  //时基选择通道0
    lcd_pwm_p_data.duty = 0;                                    //占空比50%
    //hw
    lcd_pwm_p_data.h_pin = -1;//IO_PORTA_00;                    //没有则填 -1。h_pin_output_ch_num无效，可不配置
    lcd_pwm_p_data.l_pin = lcd_bl_io();//TCFG_BACKLIGHT_PWM_IO;               //硬件引脚，l_pin_output_ch_num无效，可不配置
    //output_channel
    /* pwm_p_data.h_pin_output_ch_num = 0;                      //output channel0 非硬件PWM_IO时需要打开*/
    lcd_pwm_p_data.l_pin_output_ch_num = 1;                     //output channel1 非硬件PWM_IO时需要打开
    lcd_pwm_p_data.complementary_en = 1;                        //两个引脚的波形, 1: 互补, 0: 同步;

    mcpwm_init(&lcd_pwm_p_data);
}

REGISTER_LCD_INTERFACE(lcd) = {
    .init = lcd_init,
    .get_screen_info = lcd_get_screen_info,
    .buffer_malloc = lcd_buffer_malloc,
    .buffer_free = lcd_buffer_free,
    .draw = lcd_draw,
    .set_draw_area = lcd_set_draw_area,
    .clear_screen = lcd_clear_screen,
    .backlight_ctrl = __lcd_backlight_ctrl,
    .draw_page = lcd_draw_gage,
};

static u8 lcd_idle_query(void)
{
    return !is_lcd_busy;
}

REGISTER_LP_TARGET(lcd_lp_target) = {
    .name = "lcd",
    .is_idle = lcd_idle_query,
};

#endif
