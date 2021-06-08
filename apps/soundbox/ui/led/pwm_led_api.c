#include "ui_manage.h"
#include "asm/pwm_led.h"
#include "system/includes.h"
#include "app_config.h"
#include "user_cfg.h"

#if TCFG_PWMLED_ENABLE

#define LOG_TAG_CONST       UI
#define LOG_TAG             "[UI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define UI_MANAGE_BUF             8

static bool ui_status_read_alloc(void);
static u8 ui_status_pop(u8 *status);
static void ui_status_manager_init(void);
static u32 ui_status_push(u8 status);
static int ui_normal_status_deal(u8 *status, u8 *power_status, u8 ui_mg_para);
static int ui_charging_deal(u8 *status, u8 *power_status, u8 ui_mg_para);
static int ui_power_off_deal(u8 *status, u8 *power_status, u8 ui_mg_para);
static void ui_manage_scan(void *priv);
extern const pwm_led_breathe_para pwm_led_breathe_para_table[];
extern const pwm_led_double_flash_para pwm_led_double_flash_para_table[];
extern const pwm_led_one_flash_para pwm_led_one_flash_para_table[];
extern const pwm_led_on_para pwm_led_on_para_table[];
static int led_remap_status_get(u8 status);

typedef struct _ui_var {
    u8 ui_init_flag;
    u8 other_status;
    u8 power_status;
    u8 current_status;
    volatile u8 ui_flash_cnt;
    volatile u32 ui_flash_fre;
    cbuffer_t ui_cbuf;
    u8 ui_buf[UI_MANAGE_BUF];
    int sys_ui_timer;
} ui_var;

enum {
    UI_MG_CALL_USER 		= 1,
    UI_MG_CALL_LED_FLASH 	= 2,
};

static ui_var sys_ui_var = {.power_status =  STATUS_NORMAL_POWER};

/*
 * display:显示的模式，PWM_LED_ALL_OFF ~ PWM_LED_MODE_END.*注意显示的模式是系统固定的，不能用户自己增加模式
 *
 * para_sel:同一个模式下面，用户可以根据para_sel值来选择不同的参数值，进而实现不同的效果控制
 * 示例：
	if (para_sel == 1)
	{可以通过para_sel*来选择不同的参数
		para.on = on_para[0];
	}
	else
	{
		para.on = on_para[1];
	}
*/
void ui_mode_set(u8 display, u8 para_sel)
{
    pwm_led_para para = {0};

    if (display == PWM_LED_NULL) {
        return;
    }


    switch (display) {
    case PWM_LED_ALL_OFF:
    case PWM_LED0_OFF:
    case PWM_LED1_OFF:
        break;

//灯常亮
    case PWM_LED0_ON:
        if (para_sel == 1) {
            para.on = pwm_led_on_para_table[3];
        } else {
            para.on = pwm_led_on_para_table[0];
        }
        break;
    case PWM_LED1_ON:
        para.on = pwm_led_on_para_table[1];
        break;
    case PWM_LED_ALL_ON:
        para.on = pwm_led_on_para_table[2];
        break;

//单灯单闪
    case PWM_LED0_SLOW_FLASH:
        para.one_flash = pwm_led_one_flash_para_table[0];
        break;
    case PWM_LED1_SLOW_FLASH:
        para.one_flash = pwm_led_one_flash_para_table[1];
        break;
    case PWM_LED0_FAST_FLASH:
        para.one_flash = pwm_led_one_flash_para_table[2];
        break;
    case PWM_LED1_FAST_FLASH:
        para.one_flash = pwm_led_one_flash_para_table[3];
        break;
    case PWM_LED0_ONE_FLASH_5S:
        para.one_flash = pwm_led_one_flash_para_table[4];
        break;
    case PWM_LED1_ONE_FLASH_5S:
        para.one_flash = pwm_led_one_flash_para_table[5];
        break;
//双灯互闪
    case PWM_LED0_LED1_FAST_FLASH:
        para.one_flash = pwm_led_one_flash_para_table[6];
        break;
    case PWM_LED0_LED1_SLOW_FLASH:
        para.one_flash = pwm_led_one_flash_para_table[7];
        break;

//单灯双闪
    case PWM_LED0_DOUBLE_FLASH_5S:
        para.double_flash = pwm_led_double_flash_para_table[0];
        break;
    case PWM_LED1_DOUBLE_FLASH_5S:
        para.double_flash = pwm_led_double_flash_para_table[1];
        break;

//呼吸模式
    case PWM_LED0_BREATHE:
        para.breathe = pwm_led_breathe_para_table[0];
        break;
    case PWM_LED1_BREATHE:
        para.breathe = pwm_led_breathe_para_table[1];
        break;
    case PWM_LED0_LED1_BREATHE:
        para.breathe = pwm_led_breathe_para_table[2];
        break;

    default:
        log_error("pwm led para not match:%d \n", display);
        return;
        break;
    }

    pwm_led_mode_set_with_para(display, para);
}

void ui_user_mode_set(u8 status, u8 display, u8 para_sel)
{
    int ret = led_remap_status_get(status);
    if (ret != (-1)) {
        display = (u8)ret;
    }

    ui_mode_set(display, para_sel);
}

static bool ui_status_fetch(u8 *status, u8 *power_status)
{
    bool ret = false;
    if (ui_status_pop(&sys_ui_var.current_status)) {
        if (sys_ui_var.current_status >= STATUS_CHARGE_START && sys_ui_var.current_status <= STATUS_NORMAL_POWER) {
            ///充电模式优先处理
            *power_status = sys_ui_var.current_status;
        } else {
            *status = sys_ui_var.current_status;
        }

        ret = true;
    }

    return ret;
}

/***********************led flash three ******************************/
#define IS_LED_FLASH_PROCESSING(ui_mg_para)  (ui_mg_para == UI_MG_CALL_LED_FLASH)
static void ui_led_flash_register_next_process(void)
{
    sys_ui_var.sys_ui_timer = 0;
    if (sys_ui_var.ui_flash_cnt) {
        sys_ui_var.ui_flash_cnt --;
        sys_ui_var.sys_ui_timer = sys_hi_timeout_add((void *)UI_MG_CALL_LED_FLASH, ui_manage_scan, sys_ui_var.ui_flash_fre);
    }
}

/***********************ui process manage ******************************/
static void ui_manage_scan(void *priv)
{
    u8 ui_mg_para = (u8)priv;
    log_info("ui_flash_cnt:%d cur_ui_status:%d ui_mg_para:%d \n", \
             sys_ui_var.ui_flash_cnt, sys_ui_var.current_status, ui_mg_para);

    if (!IS_LED_FLASH_PROCESSING(ui_mg_para)) {        //有特殊的闪烁状态等当前状态执行完再进入下一个状态
        ui_status_fetch(&(sys_ui_var.other_status), &(sys_ui_var.power_status));
    }

    ///step one处理STATUS_POWEROFF
    if (ui_power_off_deal(&(sys_ui_var.other_status), &(sys_ui_var.power_status), ui_mg_para) != (-1)) {
        log_info("*STATUS_POWEROFF*\n");
        goto _ui_status_deal_end;
    }

    //step two处理power status
    if (ui_charging_deal(&(sys_ui_var.other_status), &(sys_ui_var.power_status), ui_mg_para) != (-1)) {
        log_info("*STATUS_CHARGING*\n");
        goto _ui_status_deal_end;
    }

    //last step 处理normal status
    if (ui_normal_status_deal(&(sys_ui_var.other_status), &(sys_ui_var.power_status), ui_mg_para) != (-1)) {
        goto _ui_status_deal_end;
    }

_ui_status_deal_end:

    if (!IS_LED_FLASH_PROCESSING(ui_mg_para)) {        //有特殊的闪烁状态等当前状态执行完再进入下一个状态
        if (ui_status_read_alloc() == true) {
            ///cmd pool还有命令，启动一下命令处理
            log_info("*FETCH NEXT CMD*\n");
            sys_ui_var.sys_ui_timer = sys_hi_timeout_add((void *)UI_MG_CALL_USER, ui_manage_scan, 10);
        }
    }

    ui_led_flash_register_next_process();
    return;

}

static int ui_power_off_deal(u8 *status, u8 *power_status, u8 ui_mg_para)
{
    int ret = -1;

    switch (*status) {
    case STATUS_POWEROFF:
        log_info("[STATUS_POWEROFF]\n");
        ///init ui_flash_cnt
        if (ui_mg_para == UI_MG_CALL_USER) {
            sys_ui_var.ui_flash_cnt = 7;
            sys_ui_var.ui_flash_fre = 300;
        }

        if (sys_ui_var.ui_flash_cnt) {
            if (sys_ui_var.ui_flash_cnt % 2) {
                ui_user_mode_set(STATUS_POWEROFF, PWM_LED1_OFF, 0);
            } else {
                ui_user_mode_set(STATUS_POWEROFF, PWM_LED1_ON, 0);
            }
        }
        ret = 0;
        break;
    default:
        break;
    }

    return ret;
}

static int ui_charging_deal(u8 *status, u8 *power_status, u8 ui_mg_para)
{
#define IS_CHARGING_MODE_STATUS() ((*power_status) != STATUS_NORMAL_POWER)
    int ret = -1;

    if (IS_CHARGING_MODE_STATUS()) {
        switch (*power_status) {
        case STATUS_LOWPOWER:
            log_info("[STATUS_LOWPOWER]\n");
            ui_user_mode_set(STATUS_LOWPOWER, PWM_LED1_SLOW_FLASH, 0);
            break;

        case STATUS_CHARGE_START:
            log_info("[STATUS_CHARGE_START]\n");
            ui_user_mode_set(STATUS_CHARGE_START, PWM_LED1_ON, 0);
            break;

        case STATUS_CHARGE_FULL:
            log_info("[STATUS_CHARGE_FULL]\n");
            ui_user_mode_set(STATUS_CHARGE_FULL, PWM_LED0_ON, 0);
            break;

        case STATUS_CHARGE_ERR:
            log_info("[STATUS_CHARGE_ERR]\n");
            ui_user_mode_set(STATUS_CHARGE_ERR, PWM_LED0_ON, 0);
            ui_user_mode_set(STATUS_CHARGE_ERR, PWM_LED1_ON, 0);
            break;

        case STATUS_CHARGE_CLOSE:
            log_info("[STATUS_CHARGE_CLOSE]\n");
            ui_user_mode_set(STATUS_CHARGE_CLOSE, PWM_LED0_OFF, 0);
            ui_user_mode_set(STATUS_CHARGE_CLOSE, PWM_LED1_OFF, 0);
            break;

        case STATUS_CHARGE_LDO5V_OFF:
            log_info("[STATUS_CHARGE_LDO5V_OFF]\n");
        case STATUS_EXIT_LOWPOWER:
        case STATUS_NORMAL_POWER:
            //ui_user_mode_set(STATUS_NORMAL_POWER,PWM_LED0_OFF,0);
            //ui_user_mode_set(STATUS_NORMAL_POWER,PWM_LED1_OFF,0);
            *power_status = STATUS_NORMAL_POWER;
            break;

        default:
            break;
        }

        ret = 0;
    }

    return ret;
}

static int ui_normal_status_deal(u8 *status, u8 *power_status, u8 ui_mg_para)
{
    int ret = -1;

    ret = 0;
    switch (*status) {
    case STATUS_DEMO_MODE:
        log_info("[STATUS_DEMO_MODE]\n");
        ui_user_mode_set(STATUS_DEMO_MODE, PWM_LED0_ON, 1);
        break;

    case STATUS_POWERON:
        log_info("[STATUS_POWERON]\n");
        ui_user_mode_set(STATUS_POWERON, PWM_LED0_ON, 0);
        break;
    case STATUS_POWEROFF:
        log_info("[STATUS_POWEROFF]\n");
        ///init ui_flash_cnt
        if (ui_mg_para == UI_MG_CALL_USER) {
            sys_ui_var.ui_flash_cnt = 7;
            sys_ui_var.ui_flash_fre = 300;
        }

        if (sys_ui_var.ui_flash_cnt) {
            if (sys_ui_var.ui_flash_cnt % 2) {
                ui_user_mode_set(STATUS_POWEROFF, PWM_LED1_OFF, 0);
            } else {
                ui_user_mode_set(STATUS_POWEROFF, PWM_LED1_ON, 0);
            }
        }
        break;

    case STATUS_BT_INIT_OK:
        log_info("[STATUS_BT_INIT_OK]\n");
        ui_user_mode_set(STATUS_BT_INIT_OK, PWM_LED0_LED1_SLOW_FLASH, 0);
        break;

    case STATUS_BT_SLAVE_CONN_MASTER:
        ui_user_mode_set(STATUS_BT_SLAVE_CONN_MASTER, PWM_LED1_SLOW_FLASH, 0);
        break;

    case STATUS_BT_CONN:
        log_info("[STATUS_BT_CONN]\n");
        ui_user_mode_set(STATUS_BT_CONN, PWM_LED0_ONE_FLASH_5S, 0);
        break;

    case STATUS_BT_MASTER_CONN_ONE:
        log_info("[STATUS_BT_MASTER_CONN_ONE]\n");
        ui_user_mode_set(STATUS_BT_MASTER_CONN_ONE, PWM_LED0_LED1_SLOW_FLASH, 0);
        break;

    case STATUS_BT_DISCONN:
        log_info("[STATUS_BT_DISCONN]\n");
        ui_user_mode_set(STATUS_BT_DISCONN, PWM_LED0_LED1_FAST_FLASH, 0);
        break;

    case STATUS_PHONE_INCOME:
        log_info("[STATUS_PHONE_INCOME]\n");
        ui_user_mode_set(STATUS_PHONE_INCOME, PWM_LED_NULL, 0);
        break;

    case STATUS_PHONE_OUT:
        log_info("[STATUS_PHONE_OUT]\n");
        ui_user_mode_set(STATUS_PHONE_OUT, PWM_LED_NULL, 0);
        break;

    case STATUS_PHONE_ACTIV:
        log_info("[STATUS_PHONE_ACTIV]\n");
        ui_user_mode_set(STATUS_PHONE_ACTIV, PWM_LED_NULL, 0);
        break;

    case STATUS_POWERON_LOWPOWER:
        log_info("[STATUS_POWERON_LOWPOWER]\n");
        ui_user_mode_set(STATUS_POWERON_LOWPOWER, PWM_LED1_SLOW_FLASH, 0);
        break;

    case STATUS_BT_TWS_CONN:
        log_info("[STATUS_BT_TWS_CONN]\n");
        ui_user_mode_set(STATUS_BT_TWS_CONN, PWM_LED0_LED1_FAST_FLASH, 0);
        break;
    case STATUS_BT_TWS_DISCONN:
        log_info("[STATUS_BT_TWS_DISCONN]\n");
        ui_user_mode_set(STATUS_BT_TWS_DISCONN, PWM_LED0_LED1_SLOW_FLASH, 0);
        break;
    case STATUS_MUSIC_MODE:
        log_info("[STATUS_MUSIC_MODE]\n");
        ui_user_mode_set(STATUS_MUSIC_MODE, PWM_LED0_SLOW_FLASH, 0);
        break;
    case STATUS_MUSIC_PLAY:
        log_info("[STATUS_MUSIC_PLAY]\n");
        ui_user_mode_set(STATUS_MUSIC_PLAY, PWM_LED0_SLOW_FLASH, 0);
        break;
    case STATUS_MUSIC_PAUSE:
        log_info("[STATUS_MUSIC_PAUSE]\n");
        ui_user_mode_set(STATUS_MUSIC_PAUSE, PWM_LED0_ON, 0);
        break;
    case STATUS_LINEIN_MODE:
        log_info("[STATUS_LINEIN_MODE]\n");
        ui_user_mode_set(STATUS_LINEIN_MODE, PWM_LED0_SLOW_FLASH, 0);
        break;
    case STATUS_LINEIN_PLAY:
        log_info("[STATUS_LINEIN_PLAY]\n");
        ui_user_mode_set(STATUS_LINEIN_PLAY, PWM_LED0_SLOW_FLASH, 0);
        break;
    case STATUS_LINEIN_PAUSE:
        log_info("[STATUS_LINEIN_PAUSE]\n");
        ui_user_mode_set(STATUS_LINEIN_PAUSE, PWM_LED0_ON, 0);
        break;
    case STATUS_PC_MODE:
        log_info("[STATUS_PC_MODE]\n");
        ui_user_mode_set(STATUS_PC_MODE, PWM_LED0_SLOW_FLASH, 0);
        break;
    case STATUS_PC_PLAY:
        log_info("[STATUS_PC_PLAY]\n");
        ui_user_mode_set(STATUS_PC_PLAY, PWM_LED0_SLOW_FLASH, 0);
        break;
    case STATUS_PC_PAUSE:
        log_info("[STATUS_PC_PAUSE]\n");
        ui_user_mode_set(STATUS_PC_PAUSE, PWM_LED0_ON, 0);
        break;
    case STATUS_FM_MODE:
        log_info("[STATUS_FM_MODE]\n");
        ui_user_mode_set(STATUS_FM_MODE, PWM_LED0_SLOW_FLASH, 0);
        break;
    case STATUS_RECORD_MODE:
        log_info("[STATUS_RECORD_MODE]\n");
        ui_user_mode_set(STATUS_RECORD_MODE, PWM_LED0_SLOW_FLASH, 0);
        break;
    case STATUS_SPDIF_MODE:
        log_info("[STATUS_SPDIF_MODE]\n");
        ui_user_mode_set(STATUS_SPDIF_MODE, PWM_LED0_SLOW_FLASH, 0);
        break;
    case STATUS_RTC_MODE:
        log_info("[STATUS_RTC_MODE]\n");
        ui_user_mode_set(STATUS_RTC_MODE, PWM_LED0_SLOW_FLASH, 0);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

int ui_manage_init(void)
{
    if (!sys_ui_var.ui_init_flag) {
        ui_status_manager_init();
        sys_ui_var.ui_init_flag = 1;
    }
    return 0;
}

void ui_update_status(u8 status)
{
    int ui_mg_para = UI_MG_CALL_USER;
    if (!sys_ui_var.ui_init_flag) {    //更新UI状态之前需先初始化ui_cbuf
        ui_manage_init();
    }
    log_info("update ui status :%d", status);

    ui_status_push(status);

    ui_manage_scan((void *)ui_mg_para);
}

/***********************ui flag******************************/
u8 get_ui_busy_status()
{
    return sys_ui_var.ui_flash_cnt;
}

u8 adv_get_led_status(void)
{
    return sys_ui_var.other_status;
}

/***********************ui remap led status******************************/
static LED_REMAP_STATUS led_remap_t = {PWM_LED_NULL};
void *led_get_remap_t(void)
{
    return &led_remap_t;
}

static void led_remap_init(void)
{
    memset(&led_remap_t, PWM_LED_NULL, sizeof(LED_REMAP_STATUS));
}

static int led_remap_status_get(u8 status)
{
    LED_REMAP_STATUS *p_led = led_get_remap_t();
    u8 led_user_status = PWM_LED_NULL;
    switch (status) {
    case STATUS_LOWPOWER:
        led_user_status = p_led->lowpower;
        break;
    case STATUS_CHARGE_START:
        led_user_status = p_led->charge_start;
        break;
    case STATUS_CHARGE_FULL:
        led_user_status = p_led->charge_full;
        break;
    case STATUS_POWERON:
        led_user_status = p_led->power_on;
        break;
    case STATUS_POWEROFF:
        led_user_status = p_led->power_off;
        break;
    case STATUS_BT_INIT_OK:
        led_user_status = p_led->bt_init_ok;
        break;
    case STATUS_BT_CONN:
        led_user_status = p_led->bt_connect_ok;
        break;
    case STATUS_BT_DISCONN:
        led_user_status = p_led->bt_disconnect;
        break;
    case STATUS_PHONE_INCOME:
        led_user_status = p_led->phone_in;
        break;
    case STATUS_PHONE_OUT:
        led_user_status = p_led->phone_out;
        break;
    case STATUS_PHONE_ACTIV:
        led_user_status = p_led->phone_activ;
        break;
    case STATUS_POWERON_LOWPOWER:
        led_user_status = p_led->lowpower;
        break;
    case STATUS_BT_TWS_CONN:
        led_user_status = p_led->tws_connect_ok;
        break;
    case STATUS_BT_TWS_DISCONN:
        led_user_status = p_led->tws_disconnect;
        break;
    case STATUS_MUSIC_MODE:
    case STATUS_MUSIC_PLAY:
        led_user_status = p_led->music_play;
        break;
    case STATUS_MUSIC_PAUSE:
        led_user_status = p_led->music_pause;
        break;
    case STATUS_LINEIN_MODE:
    case STATUS_LINEIN_PLAY:
        led_user_status = p_led->linein_play;
        break;
    case STATUS_LINEIN_PAUSE:
        led_user_status = p_led->linein_mute;
        break;
    }

    if (led_user_status == PWM_LED_NULL) {
        return (-1);
    }

    log_info("led remap index:%d \n", led_user_status);

    return led_user_status;
}

/***********************ui command manage******************************/
static void ui_status_manager_init(void)
{
    cbuf_init(&(sys_ui_var.ui_cbuf), &(sys_ui_var.ui_buf), UI_MANAGE_BUF);
}

static u8 ui_status_pop(u8 *status)
{
    return cbuf_read(&(sys_ui_var.ui_cbuf), status, 1);
}

static u32 ui_status_push(u8 status)
{
    return cbuf_write(&(sys_ui_var.ui_cbuf), &status, 1);
}

static bool ui_status_read_alloc(void)
{
    u32 len = 0;
    cbuf_read_alloc(&(sys_ui_var.ui_cbuf), &len);
    return (len > 0);
}

void ui_pwm_led_init(const struct led_platform_data *user_data)
{
    led_remap_init();
    ui_manage_init();
    pwm_led_init(user_data);
}
#else
int ui_manage_init(void)
{
    return 0;
}

void ui_update_status(u8 status)
{
    u8 status_para = status;
}
u8 get_ui_busy_status()
{
    return 0;
}

u8 adv_get_led_status(void)
{
    return 0;
}

void ui_pwm_led_init(const struct led_platform_data *user_data)
{
    const struct led_platform_data *user_data_para = user_data;
}

static LED_REMAP_STATUS led_remap_t = {PWM_LED_NULL};
void *led_get_remap_t(void)
{
    return &led_remap_t;
}

#endif
