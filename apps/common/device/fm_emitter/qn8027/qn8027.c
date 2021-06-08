
#include "app_config.h"
#include "system/includes.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "qn8027.h"


#if(TCFG_FM_EMITTER_QN8027_ENABLE == ENABLE)






/*----------------------------------------------------------------------------*/
/**@brief    QN8027 写寄存器函数
   @param    addr：寄存器地址   data：写入数据
   @return   无
   @note     void qn8027_write(u8 iic_addr,u8 iic_data)
*/
/*----------------------------------------------------------------------------*/
void qn8027_write(u8 reg_addr, u8 reg_data)
{
    iic_write(QN8027_ADDR_WRITE, reg_addr, &reg_data, 1);
}

/*----------------------------------------------------------------------------*/
/**@brief    QN8027 读寄存器函数
   @param    addr：要读寄存器地址
   @return   读取到的寄存器的值
   @note     u8 qn8027_read(u8 iic_addr)
*/
/*----------------------------------------------------------------------------*/
u8 qn8027_read(u8 reg_addr)
{
    u8 Byte_RecivData;
    iic_start();                    //I2C启动
    iic_sendbyte(QN8027_ADDR_WRITE);             //写命令
    iic_sendbyte(reg_addr);             //写地址
    iic_start();                    //写转为读命令，需要再次启动I2C
    iic_sendbyte(QN8027_ADDR_READ);            //读命令
    Byte_RecivData = iic_revbyte(1);
    iic_stop();                     //I2C停止
    return Byte_RecivData;
}


void qn8027_write_Bit(u8 reg, u8 start, u8 len, u8 data)
{
    u8 temp;
    temp = qn8027_read(reg);
    SFR(temp, start, len, data);
    qn8027_write(reg, temp);
}


/*----------------------------------------------------------------------------*/
/**@brief    QN8027 初始化函数
   @param    fre：初始化时设置的频点
   @return   无
   @note     void qn8027_init(u16 fre)
*/
/*----------------------------------------------------------------------------*/

void qn8027_init(u16 fre)
{
    u8 read_data;

    puts("qn8027_init\n");
    printf("qn8027_id:%x:%x\n", qn8027_read(QN8027_CID1), qn8027_read(QN8027_CID2));

    qn8027_write(QN8027_SYSTEM, 0x81); 		//reset all reg
    delay_n10ms(2);

#if (FMTX_CHIP_CLK_SOURCE == CLK_BY_EXTERN_OSC)
    qn8027_write(QN8027_REG_XTL, 0x3f);		//外挂晶振
#else
    qn8027_write(QN8027_REG_XTL, 0xff);		//外部时钟输入
#endif

#if (FMTX_CHIP_OSC_SELECT == OSC_24M)
    qn8027_write(QN8027_REG_VGA, 0x91);     //24M
#else
    qn8027_write(QN8027_REG_VGA, 0x21);		//12M
#endif
    qn8027_write(QN8027_SYSTEM, 0x41);		//reset FSM
    qn8027_write(QN8027_SYSTEM, 0x01);
    delay_n10ms(2);

    qn8027_write(0x18, 0xe4);           //SNR improve xx1x x1xx
    qn8027_write(0x1b, 0xf0);			//最大发射功率
    qn8027_write(QN8027_CH1, 0x7e);		//设置发射频率

#if PA_OFF_WHEN_NO_AUDIO
    qn8027_write(QN8027_GPLT, 0xA9);    //当没有音频信号输入时,一分钟后关闭发射
#else
    qn8027_write(QN8027_GPLT, 0xB9);    //放弃QN8027 PA关闭功能当没有音频信号输入时
#endif
    qn8027_write(QN8027_FDEV, 64);

    qn8027_write(QN8027_SYSTEM, 0x22);			//发射

    delay_n10ms(10);
    qn8027_set_freq(fre);

}
/*
void qn8027_set_freq(u16 fre)
fre 是频率值
例子：90MHZ应该传入900
*/
/*----------------------------------------------------------------------------*/
/**@brief    QN8027 频点设置函数
   @param    fre：设置频点
   @return   无
   @note     void qn8027_set_freq(u16 fre)
*/
/*----------------------------------------------------------------------------*/
void qn8027_set_freq(u16 fre)
{
    u8  ch_temp0;
    u16 ch_temp1;
    ch_temp1 = (fre - 760) * 2;
    ch_temp0 = (ch_temp1 >> 8) & 0x3;
    ch_temp0 = (0x20 | ch_temp0);
    qn8027_write(QN8027_CH1, ch_temp1 & 0xff);
    qn8027_write(QN8027_SYSTEM, ch_temp0);
}

void qn8027_set_power(u8 power, u16 freq)
{
    if (power > QN8027_TX_POWER_MAX || power < QN8027_TX_POWER_MIN) {
        power = QN8027_TX_POWER_MAX;
    }
    qn8027_write(QN8027_PAC, power | BIT(7));
    delay_n10ms(1);
    qn8027_write(QN8027_SYSTEM, 0x01);	//enter IDLE mode
    delay_n10ms(1);
    qn8027_write(QN8027_SYSTEM, 0x22);	//enter TX mode
    delay_n10ms(1);
    qn8027_set_freq(freq);
}

void qn8027_transmit_start(void)
{
    qn8027_write_Bit(QN8027_SYSTEM, 5, 1, 1);
}

void qn8027_transmit_stop(void)
{
    qn8027_write_Bit(QN8027_SYSTEM, 5, 1, 0);
}

void qn8027_mute(u8 mute)
{
    if (mute) {
        qn8027_write_Bit(QN8027_SYSTEM, 3, 1, 1);

    } else {
        qn8027_write_Bit(QN8027_SYSTEM, 3, 1, 0);
    }
}


REGISTER_FM_EMITTER(qn8027) = {
    .name = "qn8027",
    .init      = qn8027_init,
    .start     = qn8027_transmit_start,
    .stop      = qn8027_transmit_stop,
    .set_fre   = qn8027_set_freq,
    .set_power = qn8027_set_power,
};
#endif //QN8027

