/*--------------------------------------------------------------------------*/
/**@file     bk1080.c
   @brief    BK1080收音底层驱动
   @details
   @author
   @date   2011-3-30
   @note
*/
/*----------------------------------------------------------------------------*/

#include "app_config.h"

#include "system/includes.h"
#include "fm/fm_manage.h"
#include "BK1080.h"

#if(TCFG_FM_BK1080_ENABLE == ENABLE)


extern void delay_2ms(int cnt);

#define delay_n10ms(x)      \
	delay_2ms(x*5)

static struct _fm_dev_info *fm_dev_info;
/*------------BK1080 Initialization Table-----------------*/
/*
const u16 HW_Reg[]=
{
    0x0008,
    0x1080,
#if XTAL_CLOCK	//reg2
    0x1201,	//for internal crystal clock
#else
    0x0201,	//for extern clock
#endif
    0x0000,
    0x40C0,
    0x0A1F,	  //reg5	RSSI[15:8] BAND[7:6] SPACE[5:4],Europe standar
    0x002E,
    0x02FF,
    0x5B11,
    0x0000,
    0x411E,
    0x0000,
    0xCE00,
    0x0000,
    0x0000,
    0x1000,		//reg15
    0x0010,
    0x0000,
    0x13FF,
    0x9852,
    0x0000,
    0x0000,
    0x0008,
    0x0000,
    0x51E1,
    0x28DC,
    0x2645,
    0x00E4,
    0x1CD8,
    0x3A50,
    0xEAF0,
    0x3000,	//reg31
    0x0200,
    0x0000,
};
*/

const u16 HW_Reg[] = {
    0x0800,
    0x8010,
#if XTAL_CLOCK	//reg2
    0x0112,	//for internal crystal clock
#else
    0x0142,	//for extern clock, system mute
#endif
    0x0000,
    0xC040,
    0x1F0A,	  //reg5	RSSI[15:8] BAND[7:6] SPACE[5:4],Europe standar
    0x2E00,
    0xFF02,
    0x115B,
    0x0000,
    0x1E41,
    0x0000,
    0x00CE,
    0x0000,
    0x0000,
    0x0010,		//reg15
    0x1000,
    0x0000,
    0xFF13,
    0x5298,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0xE151,
    0xDC38,
    0x4526,
    0xE400,
    0xD81C,
    0x503A,
    0xF0EA,
    0x0030,	//reg31
    0x0002,
    0x0000,
};


u8 bk1080_iic_write(u8 w_chip_id, u8 register_address, u8 *buf, u32 data_len)
{
    u8 ret = 1;
    iic_start(fm_dev_info->iic_hdl);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, w_chip_id)) {
        ret = 0;
        log_e("\n fm iic wr err 0\n");
        goto __gcend;
    }

    delay(fm_dev_info->iic_delay);

    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, register_address)) {
        ret = 0;
        log_e("\n fm iic wr err 1\n");
        goto __gcend;
    }
    u8 *pbuf = buf;

    while (data_len--) {
        delay(fm_dev_info->iic_delay);

        if (0 == iic_tx_byte(fm_dev_info->iic_hdl, *pbuf++)) {
            ret = 0;
            log_e("\n fm iic wr err 2\n");
            goto __gcend;
        }
    }

__gcend:
    iic_stop(fm_dev_info->iic_hdl);

    return ret;
}

u8 bk1080_iic_readn(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len)
{
    u8 read_len = 0;

    iic_start(fm_dev_info->iic_hdl);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, (r_chip_id & 0x01) ? (r_chip_id - 1) : (r_chip_id))) {
        log_e("\n fm iic rd err 0\n");
        read_len = 0;
        goto __gdend;
    }


    delay(fm_dev_info->iic_delay);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, register_address)) {
        log_e("\n fm iic rd err 1\n");
        read_len = 0;
        goto __gdend;
    }

    /* delay(fm_dev_info->iic_delay); */
    /* iic_start(fm_dev_info->iic_hdl); */
    /* if (0 == iic_tx_byte(fm_dev_info->iic_hdl, r_chip_id)) { */
    /* log_e("\n fm iic rd err 2\n"); */
    /* read_len = 0; */
    /* goto __gdend; */
    /* } */

    delay(fm_dev_info->iic_delay);

    for (; data_len > 1; data_len--) {
        *buf++ = iic_rx_byte(fm_dev_info->iic_hdl, 1);
        read_len ++;
    }

    *buf = iic_rx_byte(fm_dev_info->iic_hdl, 0);

__gdend:

    iic_stop(fm_dev_info->iic_hdl);
    delay(fm_dev_info->iic_delay);

    return read_len;
}

#define app_IIC_write bk1080_iic_write
#define app_IIC_readn bk1080_iic_readn

/*----------------------------------------------------------------------------*/
/**@brief    BK1080读寄存器函数
   @param    num 需要的数目
   @return   无
   @note     void BEKEN_I2c_Read(u8 reg,u8 *pBuf,u8 num)
*/
/*----------------------------------------------------------------------------*/
static void BEKEN_I2c_Read(u8 reg, u8 *pBuf, u8 num)
{
    reg = reg << 1;
    reg |= 0x01;
    app_IIC_readn(CHIP_DEV_ID, reg, pBuf, num);
}

/*----------------------------------------------------------------------------*/
/**@brief    BK1080读寄存器函数
   @param    num 需要的数目
   @return   无
   @note     void BEKEN_I2c_Write(u8 reg,u8 *pBuf,u8 num)
*/
/*----------------------------------------------------------------------------*/
static void BEKEN_I2c_Write(u8 reg, u8 *pBuf, u8 num)
{
    reg = reg << 1;
    app_IIC_write(CHIP_DEV_ID, reg, pBuf, num);
}

/*----------------------------------------------------------------------------*/
/**@brief    BK1080 初始化
   @param    无
   @return   无
   @note     void init_BK1080(void)
*/
/*----------------------------------------------------------------------------*/
void bk1080_init(void *priv)
{
    if (priv != NULL) {
        fm_dev_info = priv;
    }
    u8 temp[2];
    //delay_n10ms(100);	//delay 1s,please revise delay time according to your MCU

    BEKEN_I2c_Write(0, (u8 *)HW_Reg, 68);	//start from reg2,total 60 byte

    delay_n10ms(25);			//delay 250ms
    temp[0] = HW_Reg[25];
    temp[1] = HW_Reg[25] >> 8;
    temp[1] &= 0x7f;
    BEKEN_I2c_Write(25, temp, 2);

    temp[1] |= 0x80;
    BEKEN_I2c_Write(25, temp, 2);

    /* dac_channel_on(FM_IIC_CHANNEL, FADE_ON); */

    delay_n10ms(5);			//delay 50ms
}


/*----------------------------------------------------------------------------*/
/**@brief    设置一个频点 BK1080
   @param    curFreq：设置频点
   @return   无
   @note     void bk1080_setfreq(u16 curFreq)
*/
/*----------------------------------------------------------------------------*/
void bk1080_setfreq(u16 curFreq)
{
    u8 curChan;
    u16 temp;

    curChan = curFreq - MIN_FRE;

    temp = curChan << 8;
    BEKEN_I2c_Write(0x03, (u8 *)&temp, 2); //write reg3,with 2 bytes

    temp |= 0x80;
    BEKEN_I2c_Write(0x03, (u8 *)&temp, 2);
}

/*----------------------------------------------------------------------------*/
/**@brief    设置一个频点BK1080
   @param    fre 频点  875~1080
   @return   1：当前频点有台，0：当前频点无台
   @note     bool bk1080_set_fre(u16 freq)
*/
/*----------------------------------------------------------------------------*/
bool bk1080_set_fre(void *priv, u16 freq)
{
    static u16 last_tuned_freq;
    static u16 g_last_freq_deviation_value;
    u8 temp[4];
    u16 cur_freq_deviation;

    bk1080_setfreq(freq);
    delay_n10ms(4);		//延时时间>=30ms

///////////////////////////////////////////////////////////////////
//new added 2009-05-30

    BEKEN_I2c_Read(0x07, temp, 2);	//start from reg 0x7,with 2bytes

    cur_freq_deviation = (temp[0] << 8) | temp[1];
    cur_freq_deviation = cur_freq_deviation >> 4;

    BEKEN_I2c_Read(0x0a, &temp[2], 2);

/////////////////////////////////////////////////////////////////////
    if (temp[2] & 0x10) {  			//check AFCRL bit12
        last_tuned_freq = freq;			//save last tuned freqency
        g_last_freq_deviation_value = cur_freq_deviation;
        return 0;
    }

    if (temp[3] < 8) { //RSSI<10   //搜台较少可减少该值，假台较多可增大该值    //8
        last_tuned_freq = freq;			//save last tuned freqency
        g_last_freq_deviation_value = cur_freq_deviation;
        return 0;
    }

    if ((temp[1] & 0x0f) < 2) { //SNR<2 //搜台较少可减少该值，假台较多可增大该值
        last_tuned_freq = freq;			//save last tuned freqency
        g_last_freq_deviation_value = cur_freq_deviation;
        return 0;
    }
/////////////////////////////////////////////////////////////////////////
//add frequency devation check
    if ((cur_freq_deviation >= 192) && (cur_freq_deviation <= (0xfff - 192))) { //0x64 //搜台较少可减少该值，假台较多可增大该值
        last_tuned_freq = freq;			//save last tuned freqency
        g_last_freq_deviation_value = cur_freq_deviation;

        return 0;
    }

///////////////////////////////////////////////////////////////////////////
//new added 2009-05-30
    if ((freq > 875) && ((freq - last_tuned_freq) == 1)) { //start_freq)&&( (freq-last_tuned_freq)==1) )
        if (g_last_freq_deviation_value & 0x800) {
            last_tuned_freq = freq;		//save last tuned freqency
            g_last_freq_deviation_value = cur_freq_deviation;
            return 0;
        }

        if (g_last_freq_deviation_value < 150) { //搜台较少可减少该值，假台较多可增大该值
            last_tuned_freq = freq;		//save last tuned freqency
            g_last_freq_deviation_value = cur_freq_deviation;
            return 0;
        }
    }

    if ((freq >= 875) && ((last_tuned_freq - freq) == 1)) { //start_freq)&&( (last_tuned_freq - freq)==1) )
        if ((g_last_freq_deviation_value & 0x800) == 0) {
            last_tuned_freq = freq;		//save last tuned freqency
            g_last_freq_deviation_value = cur_freq_deviation;
            return 0;
        }

        if (g_last_freq_deviation_value > (0xfff - 150)) { //搜台较少可减少该值，假台较多可增大该值
            last_tuned_freq = freq;		//save last tuned freqency
            g_last_freq_deviation_value = cur_freq_deviation;
            return 0;
        }
    }

    last_tuned_freq = freq;				//save last tuned freqency
    g_last_freq_deviation_value = cur_freq_deviation;

    return 1;
}


/*----------------------------------------------------------------------------*/
/**@brief    关闭 BK1080的电源
   @param    无
   @return   无
   @note     void BK1080_PowerDown(void)
*/
/*----------------------------------------------------------------------------*/
void bk1080_powerdown(void *priv)
{
    u16 temp = 0x4102;
    //temp[0] = 0x02;			  //write 0x0241 into reg2
    //temp[1] = 0x41;
    BEKEN_I2c_Write(0x02, (u8 *)&temp, 2);

    /* dac_channel_off(FM_IIC_CHANNEL, FADE_ON); */
    delay_n10ms(5);				  //delay 50ms
}

/*----------------------------------------------------------------------------*/
/**@brief    FM 模块静音控制
   @param    dir：1-mute 0-unmute
   @return   无
   @note     void BK1080_mute(dir)
*/
/*----------------------------------------------------------------------------*/
void bk1080_mute(void *priv, u8 flag)
{
    u8 TmpData8[2];

    BEKEN_I2c_Read(2, TmpData8, 2);
    if (flag) {
        TmpData8[0] |= 0x40;    //mute
    } else {
        TmpData8[0] &= 0xbf;
    }

    BEKEN_I2c_Write(2, TmpData8, 2);
}

/*----------------------------------------------------------------------------*/
/**@brief   FM模块检测，获取BK1080 模块ID
   @param   无
   @return  检测到BK1080模块返回1，否则返回0
   @note    bool BK1080_Read_ID(void)
*/
/*----------------------------------------------------------------------------*/
bool bk1080_read_id(void *priv)
{
    u16 bk_id;

    BEKEN_I2c_Read(0x01, (u8 *)&bk_id, 2); 	//read reg3,with 2 bytes

    printf("bk1080_read_id %x", bk_id);
    if (0x8010 == bk_id) {
        return TRUE;
    }
    return FALSE;
}

void bk1080_test(void)
{
    printf("bk1080 test\n");
}

REGISTER_FM(bk1080) = {
    .logo    = "bk1080",
    .init    = bk1080_init,
    .close   = bk1080_powerdown,
    .set_fre = bk1080_set_fre,
    .mute    = bk1080_mute,
    .read_id = bk1080_read_id,
};


#endif
