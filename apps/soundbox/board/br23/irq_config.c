#include "system/includes.h"

static const u8 irq_pro_list[MAX_IRQ_ENTRY_NUM] = {
    [IRQ_EMUEXCPT_IDX  ] = 0,
    [IRQ_EXCEPTION_IDX ] = 7,
    [IRQ_SYSCALL_IDX   ] = 0,
    [IRQ_TICK_TMR_IDX  ] = 1,
    [IRQ_TIME0_IDX     ] = 5,//ir
    [IRQ_TIME1_IDX     ] = 3,

#if TCFG_LED7_RUN_RAM
    [IRQ_TIME2_IDX     ] = 6,//总中断关闭，不关闭优先级6的中断、扫屏led7
#else
    [IRQ_TIME2_IDX     ] = 5,//总中断关闭，不关闭优先级5的中断、扫屏led7
#endif

    [IRQ_TIME3_IDX     ] = 3,
    [IRQ_USB_SOF_IDX   ] = 1,
    [IRQ_USB_CTRL_IDX  ] = 3,
    [IRQ_RTC_WDT_IDX   ] = 0,
    [IRQ_ALINK0_IDX    ] = 3,
    [IRQ_AUDIO_IDX     ] = 2,
    [IRQ_PORT_IDX      ] = 0,
    [IRQ_SPI0_IDX      ] = 0,
    [IRQ_SPI1_IDX      ] = 0,
    [IRQ_SD0_IDX       ] = 3,
    [IRQ_SD1_IDX       ] = 3,
    [IRQ_UART0_IDX     ] = 3,
    [IRQ_UART1_IDX     ] = 3,
    [IRQ_UART2_IDX     ] = 0,
    [IRQ_PAP_IDX       ] = 0,
    [IRQ_IIC_IDX       ] = 0,
    [IRQ_SARADC_IDX    ] = 0,
    [IRQ_PDM_LINK_IDX  ] = 1,
    [IRQ_RDEC0_IDX     ] = 1,
    [IRQ_LRCT_IDX      ] = 2,
    [IRQ_BREDR_IDX     ] = 2,
    [IRQ_BT_CLKN_IDX   ] = 2,
    [IRQ_BT_DBG_IDX    ] = 0,
    [IRQ_WL_LOFC_IDX   ] = 2,
    [IRQ_SRC_IDX       ] = 1,
    [IRQ_FFT_IDX       ] = 1,
    [IRQ_EQ_IDX        ] = 1,
    [IRQ_LP_TIMER0_IDX ] = 3,
    [IRQ_LP_TIMER1_IDX ] = 3,
    [IRQ_ALINK1_IDX    ] = 3,
    [IRQ_OSA_IDX       ] = 0,
    [IRQ_BLE_RX_IDX    ] = 2,
    [IRQ_BLE_EVENT_IDX ] = 2,
    [IRQ_AES_IDX       ] = 0,
    [IRQ_MCTMRX_IDX    ] = 0,
    [IRQ_CHX_PWM_IDX   ] = 0,
    [IRQ_FMRX_IDX      ] = 0,
    [IRQ_SPI2_IDX      ] = 0,
    [IRQ_SBC_IDX	   ] = 1,
    [IRQ_GPC_IDX	   ] = 0,
    [IRQ_FMTX_IDX	   ] = 4,
    [IRQ_DCP_IDX	   ] = 1,
    [IRQ_RDEC1_IDX     ] = 1,
    [IRQ_RDEC2_IDX     ] = 1,
    [IRQ_SPDIF_IDX     ] = 2,
    [IRQ_PWM_LED_IDX   ] = 1,
    [IRQ_CTM_IDX       ] = 1,
    [IRQ_SOFT0_IDX     ] = 0,
    [IRQ_SOFT1_IDX     ] = 0,
    [IRQ_SOFT2_IDX     ] = 0,
    [IRQ_SOFT3_IDX     ] = 0,
};

void irq_disable_t()
{
    u8 i;
    for (i = 0; i < MAX_IRQ_ENTRY_NUM; i++) {
        irq_disable(i);
    }
}

void irq_enable_t()
{
    u8 i;
    for (i = 0; i < MAX_IRQ_ENTRY_NUM; i++) {
        irq_enable(i);
    }
}



u8 irq_priority_get(u8 index)
{
    /* printf("irq index = %d\n", index); */
    ///测试如果有问题， 请先直接该函数返回0xff,即可以还原之前中断优先级配置
    if (index >= MAX_IRQ_ENTRY_NUM) {
        return 0xff;
    }
    return irq_pro_list[index];
}

