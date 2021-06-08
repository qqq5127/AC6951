#ifndef __IIS_H__
#define __IIS_H__

#include "generic/typedef.h"

#define ALINK_PORT_TO_MODULE(port)      ((port < 2) ? 0 : 1)

#define ALINK_SEL(idx, reg)             (((JL_ALNK_TypeDef    *)(((u8 *)JL_ALNK0) + idx*(JL_ALNK1_BASE - JL_ALNK0_BASE)))->reg)
//#define ALINK_SEL(idx, reg)             ((JL_ALNK0 + idx)->reg)

#define ALNK_CON_RESET(idx)	do {ALINK_SEL(idx, CON0) = 0; ALINK_SEL(idx, CON1) = 0; ALINK_SEL(idx, CON2) = 0; ALINK_SEL(idx, CON3) = 0;} while(0)

#define ALINK_DSPE(idx, x)		SFR(ALINK_SEL(idx, CON0), 6, 1, x)
#define ALINK_SOE(idx, x)		SFR(ALINK_SEL(idx, CON0), 7, 1, x)
#define ALINK_MOE(idx, x)		SFR(ALINK_SEL(idx, CON0), 8, 1, x)
#define F32_EN(idx, x)           SFR(ALINK_SEL(idx, CON0), 9, 1, x)
#define SCLKINV(idx, x)         	SFR(ALINK_SEL(idx, CON0),10, 1, x)
#define ALINK_EN(idx, x)         SFR(ALINK_SEL(idx, CON0),11, 1, x)
#define ALINK_24BIT_MODE(idx)	(ALINK_SEL(idx, CON1) |= (BIT(2) | BIT(6) | BIT(10) | BIT(14)))
#define ALINK_16BIT_MODE(idx)	(ALINK_SEL(idx, CON1) &= (~(BIT(2) | BIT(6) | BIT(10) | BIT(14))))

#define ALINK_IIS_MODE(idx)	(ALINK_SEL(idx, CON1) |= (BIT(2) | BIT(6) | BIT(10) | BIT(14)))

#define ALINK_CHx_DIR_TX_MODE(idx, ch)	(ALINK_SEL(idx, CON1) &= (~(1 << (3 + 4 * ch))))
#define ALINK_CHx_DIR_RX_MODE(idx, ch)	(ALINK_SEL(idx, CON1) |= (1 << (3 + 4 * ch)))

#define ALINK_CHx_MODE_SEL(idx, val, ch)		(ALINK_SEL(idx, CON1) |= (val << (0 + 4 * ch)))
#define ALINK_CHx_CLOSE(idx, val, ch)		(ALINK_SEL(idx, CON1) &= ~(val << (0 + 4 * ch)))


#define ALINK_CLR_CH0_PND(idx)		(ALINK_SEL(idx, CON2) |= BIT(0))
#define ALINK_CLR_CH1_PND(idx)		(ALINK_SEL(idx, CON2) |= BIT(1))
#define ALINK_CLR_CH2_PND(idx)		(ALINK_SEL(idx, CON2) |= BIT(2))
#define ALINK_CLR_CH3_PND(idx)		(ALINK_SEL(idx, CON2) |= BIT(3))

#define ALINK_MSRC(idx, x)		SFR(ALINK_SEL(idx, CON3), 0, 2, x)
#define ALINK_MDIV(idx, x)		SFR(ALINK_SEL(idx, CON3), 2, 3, x)
#define ALINK_LRDIV(idx, x)		SFR(ALINK_SEL(idx, CON3), 5, 3, x)

#endif
