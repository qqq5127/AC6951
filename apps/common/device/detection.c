#include "system/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "gpio.h"
#include "app_main.h"
#include "update.h"
#include "user_cfg.h"




u16 VDET_SAM_VALUE = 0;
/* #define VDET_FIX_VALUE  372 */
#define VDET_FIX_VALUE  330

u16 vdet_timer = 0;

typedef enum __VDET_STATUS {
    VDET_OFF = 0,
    VDET_ON,
    NULL_VDET,
} VDET_STATUS;

typedef struct __VDET_VAR {
    u8  pre_status;
    u8  cur_status;
    u8  status_cnt;
    u8  original_status;
} VDET_VAR;


VDET_VAR g_VDET_VAR = {0};

VDET_STATUS vdet_check(u8 cnt)
{
    VDET_SAM_VALUE = adc_get_value(AD_CH_PB1);
    printf("aaaaaaaaaaaaaaaaaaaaaa=%d\n", VDET_SAM_VALUE);
    if (VDET_SAM_VALUE > VDET_FIX_VALUE) {
        g_VDET_VAR.cur_status = 1;
    } else {
        g_VDET_VAR.cur_status = 0;
    }
    if (g_VDET_VAR.cur_status != g_VDET_VAR.pre_status) {
        g_VDET_VAR.original_status = g_VDET_VAR.pre_status;
        g_VDET_VAR.pre_status = g_VDET_VAR.cur_status;
        g_VDET_VAR.status_cnt = 0;
    } else {
        g_VDET_VAR.status_cnt++;
    }
    if (g_VDET_VAR.status_cnt < cnt) {
        ///检测到插入//消抖
        return NULL_VDET;
    }
    g_VDET_VAR.status_cnt = 0;

    ///检测到插入
    if ((VDET_ON == g_VDET_VAR.cur_status) && (!g_VDET_VAR.original_status)) {
        return VDET_ON;
    } else if ((VDET_OFF == g_VDET_VAR.cur_status) && g_VDET_VAR.original_status) {
        return VDET_OFF;
    }
    return NULL_VDET;
}

void vdet_detect(void)
{
    VDET_STATUS res;
    res = vdet_check(20);
    if (VDET_ON == res) {
        printf("VDET=============ON\n");
    } else if (VDET_OFF == res) {
        printf("VDET=============OFF\n");
    }
}

void  vdet_init(void)
{
    adc_add_sample_ch(AD_CH_PB1);
    gpio_set_die(IO_PORTB_01, 0);
    gpio_set_direction(IO_PORTB_01, 1);
    gpio_set_pull_down(IO_PORTB_01, 0);
    gpio_set_pull_up(IO_PORTB_01, 0);
}

void vdet_check_init(void)
{
    if (vdet_timer == 0) {
        vdet_timer = sys_timer_add(NULL, vdet_detect, 10);
    }
    vdet_init();
}


