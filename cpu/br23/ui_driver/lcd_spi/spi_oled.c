#include "includes.h"
#include "app_config.h"
#include "ui/ui_api.h"
#include "system/includes.h"
#include "system/timer.h"
#include "asm/spi.h"
#include "ui_driver/oled/oled.h"

#if TCFG_LCD_OLED_ENABLE

#define USED_SPI_4LINE 1


void lcd_spi_dma_send_wait(u8 *map, u32 size);

static void OLED_BackLightCtrl(u8 on)
{
    if (on) {
        lcd_bl_h();
    } else {
        lcd_bl_l();
    }
    printf("call %s \n", __FUNCTION__);
}

static void delay_ms(unsigned int ms)
{
    unsigned int a;
    while (ms) {
        a = 80;
        while (a--);
        ms--;
    }
    return;
}


static void OLED_WR_Cmd(u8 dat)
{
#if USED_SPI_4LINE
    lcd_cs_l();
    lcd_rs_l();
    spi_dma_send_byte(dat);
    lcd_cs_h();
#else
    u8 i;

    OLED_CS_Clr();
    OLED_SCLK_Clr();
    OLED_SDIN_Clr();
    OLED_SCLK_Set();

    for (i = 0; i < 8; i++) {
        OLED_SCLK_Clr();
        if (dat & 0x80) {
            OLED_SDIN_Set();
        } else {
            OLED_SDIN_Clr();
        }
        OLED_SCLK_Set();
        dat <<= 1;
    }
    OLED_CS_Set();
#endif
}

static void OLED_WR_Data(u8 dat)
{
#if USED_SPI_4LINE
    lcd_cs_l();
    lcd_rs_h();
    spi_dma_send_byte(dat);
    lcd_cs_h();
#else
    u8 i;
    OLED_CS_Clr();
    OLED_SCLK_Clr();
    OLED_SDIN_Set();
    OLED_SCLK_Set();

    for (i = 0; i < 8; i++) {
        OLED_SCLK_Clr();
        if (dat & 0x80) {
            OLED_SDIN_Set();
        } else {
            OLED_SDIN_Clr();
        }
        OLED_SCLK_Set();
        dat <<= 1;
    }
    OLED_CS_Set();
#endif
}

static void OLED_Set_Draw_Area(int xs, int xe, int ys, int ye)
{
    OLED_WR_Cmd(0xb0 + (ys / 8) % 8);   //设置页地址（0~7）
    OLED_WR_Cmd(0x00 + xs & 0x0f);      //设置显示位置—列低地址
    OLED_WR_Cmd(0x10 + (xs >> 4) & 0x0f); //设置显示位置—列高地址
}

static void OLED_Write_Cmd(u8 cmd)
{
    OLED_WR_Cmd(cmd);
}

static void OLED_Write_Data(u8 data)
{
    OLED_WR_Data(data);
}




static void OLED_Write_PAGE(char *map, u8 page_star, u8 page_len)
{
    u8 page;
    for (page = page_star; page < page_star + page_len; page++) {
        OLED_WR_Cmd(0xb0 + page);
        OLED_WR_Cmd(0x00);
        OLED_WR_Cmd(0x10);
        lcd_cs_l();
        lcd_rs_h();
        lcd_spi_dma_send_wait(map, 128);
        map += 128;
        lcd_cs_h();
    }
}

static void OLED_Write_Map(char *map, int size)
{
    u8 page;
    u8 col;
    for (page = 0; page < 8; page++) {
        OLED_WR_Cmd(0xb0 + page);
        OLED_WR_Cmd(0x00);
        OLED_WR_Cmd(0x10);
#if USED_SPI_4LINE
        lcd_cs_l();
        lcd_rs_h();
        lcd_spi_dma_send_wait(map, 128);
        map += 128;
        lcd_cs_h();
#else
        for (col = 0; col < 128; col++) {
            OLED_WR_Data(*map++);
        }
#endif
    }
}

//向SSD1306写入一个字节。
//dat:要写入的数据/命令
//cmd:数据/命令标志 0,表示命令;1,表示数据;
static void OLED_WR_Byte(u8 dat, u8 cmd)
{
#if USED_SPI_4LINE
    if (cmd == OLED_CMD) {
        OLED_WR_Cmd(dat);
    } else {
        OLED_WR_Data(dat);
    }
#else
    u8 i;
    OLED_CS_Clr();
    OLED_SCLK_Clr();

    if (cmd == OLED_CMD) {
        /* OLED_SDIN_Set(); */
        OLED_SDIN_Clr();
    } else {
        /* OLED_SDIN_Clr(); */
        OLED_SDIN_Set();
    }

    OLED_SCLK_Set();

    for (i = 0; i < 8; i++) {
        OLED_SCLK_Clr();
        if (dat & 0x80) {
            OLED_SDIN_Set();
        } else {
            OLED_SDIN_Clr();
        }
        OLED_SCLK_Set();
        dat <<= 1;
    }
    OLED_CS_Set();
#endif
}

//清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!
void OLED_Clear(void)
{
    u8 i, n;
    for (i = 0; i < 8; i++) {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);  //设置页地址（0~7）
        OLED_WR_Byte(0x00, OLED_CMD);      //设置显示位置—列低地址
        OLED_WR_Byte(0x10, OLED_CMD);      //设置显示位置—列高地址
        for (n = 0; n < 128; n++) {
            OLED_WR_Byte(0x00, OLED_DATA);
        }
    } //更新显示
}

void OLED_Set_Pos(unsigned char x, unsigned char y)
{
    OLED_WR_Byte(0xb0 + y, OLED_CMD);
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
    OLED_WR_Byte((x & 0x0f) | 0x01, OLED_CMD);
}

//初始化SSD1306
static void SPI_OLED_Init(void)
{

    printf("OLED_Init...\n");

#if USED_SPI_4LINE
    lcd_reset_h();
#else
    OLED_RST_Set();
#endif

    delay_ms(1000);

#if USED_SPI_4LINE
    lcd_reset_l();
#else
    OLED_RST_Clr();

#endif

    delay_ms(1000);
    delay_ms(1000);

#if USED_SPI_4LINE
    lcd_reset_h();
    delay_ms(1000);
#else
    OLED_RST_Set();

#endif

    OLED_WR_Byte(0xAE, OLED_CMD); //--turn off oled panel
    OLED_WR_Byte(0x00, OLED_CMD); //---set low column address
    OLED_WR_Byte(0x10, OLED_CMD); //---set high column address
    OLED_WR_Byte(0x40, OLED_CMD); //--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
    OLED_WR_Byte(0x81, OLED_CMD); //--set contrast control register
    OLED_WR_Byte(0xCF, OLED_CMD); // Set SEG Output Current Brightness
    OLED_WR_Byte(0xA1, OLED_CMD); //--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
    OLED_WR_Byte(0xC8, OLED_CMD); //Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
    OLED_WR_Byte(0xA6, OLED_CMD); //--set normal display
    OLED_WR_Byte(0xA8, OLED_CMD); //--set multiplex ratio(1 to 64)
    OLED_WR_Byte(0x3f, OLED_CMD); //--1/64 duty
    OLED_WR_Byte(0xD3, OLED_CMD); //-set display offset   Shift Mapping RAM Counter (0x00~0x3F)
    OLED_WR_Byte(0x00, OLED_CMD); //-not offset
    OLED_WR_Byte(0xd5, OLED_CMD); //--set display clock divide ratio/oscillator frequency
    OLED_WR_Byte(0x80, OLED_CMD); //--set divide ratio, Set Clock as 100 Frames/Sec
    OLED_WR_Byte(0xD9, OLED_CMD); //--set pre-charge period
    OLED_WR_Byte(0xF1, OLED_CMD); //Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    OLED_WR_Byte(0xDA, OLED_CMD); //--set com pins hardware configuration
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD); //--set vcomh
    OLED_WR_Byte(0x40, OLED_CMD); //Set VCOM Deselect Level
    OLED_WR_Byte(0x20, OLED_CMD); //-Set Page Addressing Mode (0x00/0x01/0x02)
    OLED_WR_Byte(0x02, OLED_CMD); //
    OLED_WR_Byte(0x8D, OLED_CMD); //--set Charge Pump enable/disable
    OLED_WR_Byte(0x14, OLED_CMD); //--set(0x10) disable
    OLED_WR_Byte(0xA4, OLED_CMD); // Disable Entire Display On (0xa4/0xa5)
    OLED_WR_Byte(0xA6, OLED_CMD); // Disable Inverse Display On (0xa6/a7)
    OLED_WR_Byte(0xAF, OLED_CMD); //--turn on oled panel
    OLED_WR_Byte(0xAF, OLED_CMD); /*display ON*/

    OLED_Clear();
    OLED_Set_Pos(0, 0);
}

#define LCD_BUFFER_SIZE (128*64/8)

u8 LCDBuff[8][128];



REGISTER_LCD_DRIVE() = {
    .name = "oled",
    .lcd_width = 128,
    .lcd_height = 64,
    .color_format = LCD_COLOR_MONO,
    .interface = LCD_SPI,
    .column_addr_align = 1,
    .row_addr_align = 1,
    .dispbuf = LCDBuff,
    .bufsize = LCD_BUFFER_SIZE,
    .initcode = NULL,
    .initcode_cnt = 0,
    .Init = SPI_OLED_Init,
    .WriteComm = OLED_Write_Cmd,
    .WriteData = OLED_Write_Data,
    .WriteMap = OLED_Write_Map,
    .SetDrawArea = OLED_Set_Draw_Area,
    .Reset = NULL,
    .BackLightCtrl = OLED_BackLightCtrl,
    .WritePAGE = OLED_Write_PAGE,

};

#endif


