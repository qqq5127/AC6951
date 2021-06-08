#include "includes.h"
#include "app_config.h"
#include "ui/ui_api.h"
#include "system/includes.h"
#include "system/timer.h"
#include "asm/spi.h"
#include "asm/mcpwm.h"

#if TCFG_LCD_ST7789VW_ENABLE

#define USED_OLD 0


/* 初始化代码 */
static const InitCode LcdInit_code[] = {
    {0x01, 0},				// soft reset
    {REGFLAG_DELAY, 120},	// delay 120ms
    {0x11, 0},				// sleep out
    {REGFLAG_DELAY, 120},
#if USED_OLD
    {0x36, 1, {BIT(6) | BIT(3)}},
#else
    {0x36, 1, {0x00}},
#endif
    {0x3A, 1, {0x05}},
    {0xB2, 5, {0x0c, 0x0c, 0x00, 0x33, 0x33}},
    {0xB7, 1, {0x35}},
    {0xBB, 1, {0x19}},
    {0xC0, 1, {0x2C}},
    {0xC2, 1, {0x01}},
    {0xC3, 1, {0x12}},
    {0xC4, 1, {0x20}},
    {0xC6, 1, {0x0F}},
    {0xD0, 2, {0xA4, 0xA1}},
    {0xE0, 14, {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23}},
    {0xE1, 14, {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23}},

#if USED_OLD
    /* {0X21, 0}, */
#else
    {0X21, 0},
#endif
    {0X2A, 4, {0x00, 0x00, 0x00, 0xEF}},
    {0X2B, 4, {0x00, 0x00, 0x00, 0xEF}},
    {0X29, 0},
    {REGFLAG_DELAY, 20},
    {0X2C, 0},
    {REGFLAG_DELAY, 20},
};

void TFT_Write_Cmd(u8 data)
{
    lcd_cs_l();
    lcd_rs_l();
    spi_dma_send_byte(data);
    lcd_cs_h();
}

void TFT_Write_Data(u8 data)
{
    lcd_cs_l();
    lcd_rs_h();
    spi_dma_send_byte(data);
    lcd_cs_h();
}

void TFT_Write_Map(char *map, int size)
{
    spi_dma_send_map(map, size);
}

void TFT_Set_Draw_Area(int xs, int xe, int ys, int ye)
{
    TFT_Write_Cmd(0x2A);
    TFT_Write_Data(xs >> 8);
    TFT_Write_Data(xs);
    TFT_Write_Data(xe >> 8);
    TFT_Write_Data(xe);

    TFT_Write_Cmd(0x2B);
    TFT_Write_Data(ys >> 8);
    TFT_Write_Data(ys);
    TFT_Write_Data(ye >> 8);
    TFT_Write_Data(ye);

    TFT_Write_Cmd(0x2C);

    lcd_cs_l();
    lcd_rs_h();
}

static void TFT_BackLightCtrl(u8 on)
{
#if (TCFG_BACKLIGHT_PWM_MODE == 1)
    //注意：duty不能大于prd，并且prd和duty是非标准非线性的，建议用示波器看着来调
    extern int pwm_led_output_clk(u8 gpio, u8 prd, u8 duty);
    if (on) {
        pwm_led_output_clk(TCFG_BACKLIGHT_PWM_IO, 10, 10);
    } else {
        pwm_led_output_clk(TCFG_BACKLIGHT_PWM_IO, 10, 0);
    }
#elif (TCFG_BACKLIGHT_PWM_MODE == 2)
    static u8 first_flag = 1;
    extern void mcpwm_set_duty(pwm_ch_num_type pwm_ch, pwm_timer_num_type timer_ch, u16 duty);
    if (first_flag) {
        first_flag = 0;
        lcd_mcpwm_init();
    }
    if (on) {
        mcpwm_set_duty(lcd_pwm_p_data.pwm_ch_num, lcd_pwm_p_data.pwm_timer_num, 0);//duty 占空比：0 ~ 10000 对应 100% ~ 0%
    } else {
        mcpwm_set_duty(lcd_pwm_p_data.pwm_ch_num, lcd_pwm_p_data.pwm_timer_num, 10000);//duty 占空比：0 ~ 10000 对应 100% ~ 0%
    }
#else
    if (on) {
        lcd_bl_h();
    } else {
        lcd_bl_l();
    }
#endif
}

#define LCD_WIDTH 240
#define LCD_HIGHT 240
#define LINE_BUFF_SIZE  (10*2*LCD_WIDTH*2)
static u8 line_buffer[LINE_BUFF_SIZE] __attribute__((aligned(4)));

REGISTER_LCD_DRIVE() = {
    .name = "st7789vw",
    .lcd_width = LCD_WIDTH,
    .lcd_height = LCD_HIGHT,
    .color_format = LCD_COLOR_RGB565,
    .interface = LCD_SPI,
    .column_addr_align = 1,
    .row_addr_align = 1,
    .dispbuf = line_buffer,
    .bufsize = LINE_BUFF_SIZE,
    .initcode = LcdInit_code,
    .initcode_cnt = sizeof(LcdInit_code) / sizeof(LcdInit_code[0]),
    .WriteComm = TFT_Write_Cmd,
    .WriteData = TFT_Write_Data,
    .WriteMap = TFT_Write_Map,
    .SetDrawArea = TFT_Set_Draw_Area,
    .Reset = NULL,
    .BackLightCtrl = TFT_BackLightCtrl,
};

#endif


