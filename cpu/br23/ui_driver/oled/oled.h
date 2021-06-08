#ifndef __OLED_H
#define __OLED_H			  	 

#include "typedef.h"

//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127
//[1]0 1 2 3 ... 127
//[2]0 1 2 3 ... 127
//[3]0 1 2 3 ... 127
//[4]0 1 2 3 ... 127
//[5]0 1 2 3 ... 127
//[6]0 1 2 3 ... 127
//[7]0 1 2 3 ... 127

#define OLED_CMD  0
#define OLED_DATA 1

#define OLED_CS_Clr()  //gpio_direction_output(IO_PORTC_00, 1)
#define OLED_CS_Set()  //gpio_direction_output(IO_PORTC_00, 1)


#define OLED_RST_Clr() gpio_direction_output(IO_PORTA_00, 0)
#define OLED_RST_Set() gpio_direction_output(IO_PORTA_00, 1)

#define OLED_DC_Clr() //OLED_DC=0
#define OLED_DC_Set() //OLED_DC=1

#define OLED_SCLK_Clr() gpio_direction_output(IO_PORTA_01, 0)
#define OLED_SCLK_Set() gpio_direction_output(IO_PORTA_01, 1)


#define OLED_SDIN_Clr() gpio_direction_output(IO_PORTA_03, 0)
#define OLED_SDIN_Set() gpio_direction_output(IO_PORTA_03, 1)




#define SIZE 16
#define XLevelL     0x02
#define XLevelH     0x10
#define Max_Column  128
#define Max_Row     64
#define Brightness  0xFF
#define X_WIDTH     128
#define Y_WIDTH     64

void OLED_Display_On(void);
void OLED_Display_Off(void);	   							   		    
void OLED_Init(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_Fill(u8 x1,u8 y1,u8 x2,u8 y2,u8 dot);
void OLED_ShowChar(u8 x,u8 y,u8 chr);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size2);
void OLED_ShowString(u8 x,u8 y, u8 *p);	 
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowCHinese(u8 x,u8 y,u8 no);
void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[]);
#endif  



