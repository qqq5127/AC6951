
#include "app_config.h"
#include "system/includes.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "qn8007.h"

#if(TCFG_FM_EMITTER_QN8007_ENABLE == ENABLE)


u8 qn8007_read(u8 reg_addr)
{
    u8 read_dat;
    iic_readn(QN8007_ADDR_READ, reg_addr, &read_dat, 1);
    return read_dat;
}

void qn8007_write(u8 reg_addr, u8 reg_data)
{
    iic_write(QN8007_ADDR_WRITE, reg_addr, &reg_data, 1);
}

void qn8007_write_Bit(u8 reg, u8 start, u8 len, u8 data)
{
    u8 temp;
    temp = qn8007_read(reg);
    SFR(temp, start, len, data);
    qn8007_write(reg, temp);
}

void qn8007_init(u16 fre)
{
    u8 chipid_minor = 0, chipid_major = 0;
    u8 retry = 0, temp = 0;

    /*qn8007有时会初始化失败没有发射，这里先判断写进去的与读出来的一不一致*/
    while ((temp != 0x29) && (retry < 5)) {
        qn8007_write(QN8007_SYSTEM2, 0x80); 	//reset all reg
        delay_n10ms(1);
        qn8007_write(QN8007_REG_XLT3, 0x04);
        qn8007_write(QN8007_ANACTL1, 0x29);		//select 24M
        temp = qn8007_read(QN8007_ANACTL1);
        retry ++;
        printf("temp = %x  retry = %d\n", temp, retry);
    }

    qn8007_write(QN8007_SYSTEM2, 0x48);	//recalibrate
    qn8007_write(QN8007_SYSTEM2, 0x08);

#if (FMTX_CHIP_CLK_SOURCE == CLK_BY_EXTERN_OSC)
    qn8007_write(QN8007_REG_XLT3, 0x04); 	//外挂晶振
#else
    qn8007_write(QN8007_REG_XLT3, 0x14); 	//外部时钟输入
#endif

#if (FMTX_CHIP_OSC_SELECT == OSC_12M)
    qn8007_write(QN8007_ANACTL1, 0x21);	//select 12M
#else
    qn8007_write(QN8007_ANACTL1, 0x29);	//select 24M
#endif
    delay_n10ms(40);
    qn8007_write(QN8007_PAC_TARGET, 0xff);	//发射功率默认最大

    chipid_minor = qn8007_read(QN8007_CID1);
    chipid_major = qn8007_read(QN8007_CID2);
    delay_n10ms(10);
    printf("\nchipid: %x, %x\n", chipid_minor, chipid_major);

    if (((chipid_minor & 0x03) == 0) && ((chipid_major & 0x0f) == 1)) {
        //B1
        puts("\nchipid->B1");

        qn8007_write(0x3C, 0x89);	//for RDS SYNC
        qn8007_write(0x4a, 0xba);
        qn8007_write(0x5c, 0x05);
        qn8007_write(0x52, 0x0c);	//mute
        qn8007_write(0x57, 0x80);
        qn8007_write(0x57, 0x00);
        delay_n10ms(10);
        qn8007_write(0x52, 0x00);

    } else if (((chipid_minor & 0x03) == 1) && ((chipid_major & 0x0f) == 0)) {
        //A1
        puts("\nchipid->A1");

        qn8007_write(0x55, 0x56);
        delay_n10ms(1);
        qn8007_write(0x4A, 0x3D); //0x4A
        delay_n10ms(1);
        qn8007_write(0x38, 0x74); //0x38
        delay_n10ms(1);
        qn8007_write(0x42, 0xb5); //0x42
        delay_n10ms(1);
        qn8007_write(0x40, 0x7f); //0x40
        delay_n10ms(1);
        qn8007_write(0x42, 0xbd); //0x42
        qn8007_write(0x23, 0x4f); //0x23	 //John remark: to avoid Qn8005/6 audio output clipping when FM deviation=75KHz & Mono mode
        qn8007_write(0x4f, 0x02); //0x4f	 //John remark: to avoid Qn8005/6 audio output clipping when FM deviation=75KHz & Mono mode

    } else if (((chipid_minor & 0x03) == 0) && ((chipid_major & 0x0f) == 0)) {
        //A0
        puts("\nchipid->A0");

        qn8007_write(QN8007_CCA, 0x00);
        delay_n10ms(1);
        qn8007_write(0x23, 0x6e);
        delay_n10ms(1);
        qn8007_write(0x4a, 0x3d);
        delay_n10ms(1);
        qn8007_write(0x4c, 0x0f);
        delay_n10ms(1);
        qn8007_write(0x4e, 0x9a);
        delay_n10ms(1);
        qn8007_write(0x4f, 0x11);
        delay_n10ms(1);
        qn8007_write(0x50, 0xe0);
        delay_n10ms(1);
        qn8007_write(0x57, 0x80);
        delay_n10ms(1);
        qn8007_write(0x57, 0x00);
        delay_n10ms(10);
    }

    qn8007_write(QN8007_SYSTEM1, 0x41);	//enter transmit
    delay_n10ms(10);
    qn8007_set_freq(fre);
}

void qn8007_transmit_start(void)
{
#if 0
    u8 value = qn8007_read(QN8007_SYSTEM1);
    value &= ~QN8007_RXREQ;
    value |=  QN8007_TXREQ;
    value &= ~QN8007_STNBY;
    qn8007_write(QN8007_SYSTEM1, value);
#endif
    qn8007_write_Bit(QN8007_SYSTEM1, 6, 1, 1);
    qn8007_write_Bit(QN8007_SYSTEM1, 4, 1, 0);
}

void qn8007_transmit_stop(void)
{
#if 0
    u8 value = qn8007_read(QN8007_SYSTEM1);
    value &= ~QN8007_RXREQ;
    value &= ~QN8007_TXREQ;
    value |=  QN8007_STNBY;
    qn8007_write(QN8007_SYSTEM1, value);
#endif
    qn8007_write_Bit(QN8007_SYSTEM1, 6, 1, 0);
    qn8007_write_Bit(QN8007_SYSTEM1, 4, 1, 1);
}

void qn8007_set_freq(u16 freq)
{
    u8  tStep;

    freq = freq * 10;
    freq = ((freq - 7600) / 5);
    qn8007_write(QN8007_CH, (u8)freq);
    tStep = qn8007_read(QN8007_CH_STEP);
    tStep &= ~0x03;
    tStep |= (((u8)(freq >> 8)) & 0x03);
    qn8007_write(QN8007_CH_STEP, tStep);
}

void qn8007_set_power(u8 power, u16 freq)
{
    if (power >= QN8007_TX_POWER_MAX || power < QN8007_TX_POWER_MIN) {
        power = QN8007_TX_POWER_MAX;
    }
    qn8007_write(QN8007_PAC_TARGET, power);
    qn8007_transmit_stop();
    qn8007_transmit_start();
    qn8007_set_freq(freq);
}
void qn8007_mute(u8 mute)
{
    if (mute) {
        qn8007_write_Bit(QN8007_ANACTL1, 7, 1, 1);
    } else {
        qn8007_write_Bit(QN8007_ANACTL1, 7, 1, 0);
    }
}


/*----------------------------------------------------------------------------*/
/*	qn8007自动扫台功能
 	ch_start:扫描的起始频点
	ch_stop:扫描的停止频点
	return:扫描结束后得到的频点
*/
/*----------------------------------------------------------------------------*/
u16 qn8007_channel_scan(u16 ch_start, u16 ch_stop)
{
    u8 temp = 0;
    u8 fre_low = 0, fre_high = 0;
    u16 fre = 0;
    u16 ch_stp = 0, ch_sta = 0;

    ch_sta = (ch_start - 760) * 2;
    ch_stp = (ch_stop - 760) * 2;

    qn8007_write(QN8007_CH_START, (u8)(ch_sta & 0xff));	//Lower 8 bits
    qn8007_write(QN8007_CH_STOP, (u8)(ch_stp & 0xff));	//Lower 8 bits

    /* temp = qn8007_read(QN8007_CH_START); */
    /* printf("QN8007_CH_START = %x\n",temp); */
    /* temp = qn8007_read(QN8007_CH_STOP); */
    /* printf("QN8007_CH_STOP = %x\n",temp); */

    qn8007_write_Bit(QN8007_CH_STEP, 2, 2, (u8)((ch_sta >> 8) & 0x03));	//Highest 2 bits
    qn8007_write_Bit(QN8007_CH_STEP, 4, 2, (u8)((ch_stp >> 8) & 0x03));	//Highest 2 bits

    /* temp = qn8007_read(QN8007_CH_STEP); */
    /* temp &= ~(0x30); */
    /* temp |= 0x20; */
    /* qn8007_write(QN8007_CH_STEP,temp); */

    /* temp = qn8007_read(QN8007_CH_STEP); */
    /* temp &= ~(0x0C); */
    /* temp |= 0x00; */
    /* qn8007_write(QN8007_CH_STEP,temp); */

    /* temp = qn8007_read(QN8007_CH_STEP); */
    /* printf("QN8007_CH_STEP = %x\n",temp); */

    qn8007_write_Bit(QN8007_SYSTEM1, 5, 1, 1);	//Channel Scan mode
    qn8007_write_Bit(QN8007_SYSTEM1, 0, 1, 0);	//CH is determined by channel scan

    /* temp = qn8007_read(QN8007_SYSTEM1); */
    /* temp |= BIT(5); */
    /* temp &= ~BIT(0); */
    /* printf("SYSTEM1=%x\n",temp); */
    /* qn8007_write(QN8007_SYSTEM1,temp); */

    while (qn8007_read(QN8007_SYSTEM1)&BIT(5));	//等待扫描完毕

    fre_low = qn8007_read(QN8007_CH);
    fre_high = qn8007_read(QN8007_CH_STEP);
    fre_high &= 0x03;
    fre = fre_low | (((u16)fre_high) << 8);
    fre = (fre * 5 + 7600) / 10;
    printf("auto scan finish,fre=%d\n", fre);
    return fre;

}

REGISTER_FM_EMITTER(qn8007) = {
    .name = "qn8007",
    .init      = qn8007_init,
    .start     = qn8007_transmit_start,
    .stop      = qn8007_transmit_stop,
    .set_fre   = qn8007_set_freq,
    .set_power = qn8007_set_power,
};

#endif //QN8007


