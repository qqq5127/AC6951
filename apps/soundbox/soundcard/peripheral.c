#include "soundcard/peripheral.h"
#include "soundcard/soundcard.h"
#include "soundcard/lamp.h"
#include "asm/audio_linein.h"
#include "audio_dec.h"
#include "common/user_msg.h"
#include "os/os_api.h"
#include "app_task.h"
#include "asm/rtc.h"
#include "asm/power/p33.h"
#include "asm/gpio.h"
#include "key_event_deal.h"
#include "application/audio_output_dac.h"
#include "linein/linein.h"


#if SOUNDCARD_ENABLE
typedef enum __VDET_STATUS {
    VDET_OFF = 0,
    VDET_ON,
    NULL_VDET,
} VDET_STATUS;


typedef struct __DEVICE_DET_VAR {
    u8  pre_status;
    u8  cur_status;
    u8  status_cnt;
    u8  original_status;
} DEVICE_DET_VAR;

static DEVICE_DET_VAR g_mic1_var = {0};
static DEVICE_DET_VAR g_mic2_var = {0};
static DEVICE_DET_VAR g_aux0_var = {0};


void soundcard_pa_init(void)
{
    gpio_set_direction(IO_PORTC_07, 0);
    gpio_set_pull_down(IO_PORTC_07, 0);
    gpio_set_pull_up(IO_PORTC_07, 1);
    gpio_set_output_value(IO_PORTC_07, 0);
}

void soundcard_pa_umute(void)
{
    printf("soundcard_pa_umute\n");
    gpio_set_output_value(IO_PORTC_07, 1);
    audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(0) | BIT(1) | BIT(2) | BIT(3), 30, 1);//模拟音量范围0~30
}

void soundcard_pa_mute(void)
{
    gpio_set_output_value(IO_PORTC_07, 0);
}

static VDET_STATUS soundcard_device_check(struct __DEVICE_DET_VAR *var, u8 st, u8 cnt)
{
    var->cur_status = st;
    if (var->cur_status != var->pre_status) {
        var->original_status = var->pre_status;
        var->pre_status = var->cur_status;
        var->status_cnt = 0;
    } else {
        if (var->status_cnt < cnt) {
            var->status_cnt++;
            if (var->status_cnt == cnt) {
                if ((VDET_ON == var->cur_status) && (!var->original_status)) {
                    return VDET_ON;
                } else if ((VDET_OFF == var->cur_status) && var->original_status) {
                    return VDET_OFF;
                }
            }
        }
    }
    return NULL_VDET;
}

static void soundcard_mic_check_detect(void)
{
    VDET_STATUS res;

    u32 check_value_cur = 0;
    u8 st;

    check_value_cur = adc_get_value(AD_CH_PA6);
    //y_printf("check_value_cur:%d\n",check_value_cur);

    if (check_value_cur > 923) { //2.975V
        st = 0;
    } else {
        st = 1;
    }
    res = soundcard_device_check(&g_mic1_var, st, 20);
    if (VDET_ON == res) {
        y_printf("MIC1=============ON\n");
        app_task_put_key_msg(KEY_SOUNDCARD_EAR_MIC_STATUS_UPDATE, res);
    } else if (VDET_OFF == res) {
        y_printf("MIC1=============OFF\n");
        app_task_put_key_msg(KEY_SOUNDCARD_EAR_MIC_STATUS_UPDATE, res);
    }
    //y_printf(":%d %d %d\n",g_mic1_var.cur_status,g_mic1_var.pre_status,g_mic1_var.original_status);

    st = !rtc_port_pr_read(0);
    res = soundcard_device_check(&g_mic2_var, st, 20);
    if (VDET_ON == res) {
        y_printf("MIC2=============ON\n");
        app_task_put_key_msg(KEY_SOUNDCARD_NORMAL_MIC_STATUS_UPDATE, res);
    } else if (VDET_OFF == res) {
        y_printf("MIC2=============OFF\n");
        app_task_put_key_msg(KEY_SOUNDCARD_NORMAL_MIC_STATUS_UPDATE, res);
    }

    st = gpio_read(IO_PORTA_02);
    res = soundcard_device_check(&g_aux0_var, st, 20);
    if (VDET_ON == res) {
        y_printf("aux0=============ON\n");
        app_task_put_key_msg(KEY_SOUNDCARD_AUX_STATUS_UPDATE, res);
    } else if (VDET_OFF == res) {
        y_printf("aux0=============OFF\n");
        app_task_put_key_msg(KEY_SOUNDCARD_AUX_STATUS_UPDATE, res);
    }
}
static void soundcard_mic_detect_init(void)
{
    y_printf("%s", __func__);
    adc_add_sample_ch(AD_CH_PA6);
    gpio_set_die(IO_PORTA_06, 0);
    gpio_set_pull_down(IO_PORTA_06, 0);
    gpio_set_pull_up(IO_PORTA_06, 0);
    gpio_direction_input(IO_PORTA_06);

    p33_tx_1byte(R3_OSL_CON, 0);
    rtc_port_pr_die(0, 1);
    rtc_port_pr_pu(0, 1);
    rtc_port_pr_pd(0, 0);
    rtc_port_pr_in(0);

    gpio_set_die(IO_PORTA_02, 1);
    gpio_set_pull_down(IO_PORTA_02, 0);
    gpio_set_pull_up(IO_PORTA_02, 1);
    gpio_direction_input(IO_PORTA_02);

    rtc_port_pr_die(1, 1);
    rtc_port_pr_out(1, 1);

    sys_hi_timer_add(NULL, soundcard_mic_check_detect, 10);
}

void soundcard_ear_mic_mute(u8 mute)
{
    if (mute) {
        rtc_port_pr_out(1, 1);
    } else {
        rtc_port_pr_out(1, 0);
    }
}



extern void slidekey_update(void);
static void soundcard_slider_key_update(void *p)
{
    slidekey_update();
}

void soundcard_peripheral_init(void)
{
    printf("%s", __func__);
    soundcard_pa_init();
    soundcard_led_init();
    soundcard_mic_detect_init();
    soundcard_start();
    sys_timeout_add(NULL, soundcard_pa_umute, 2000);
    sys_timeout_add(NULL, soundcard_slider_key_update, 5000);//防止一开始按键消息更新事件丢失导致的音量不正常
}

#endif//SOUNDCARD_ENABLE


