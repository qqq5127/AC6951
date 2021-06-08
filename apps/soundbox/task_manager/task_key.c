#include "key_event_deal.h"
#include "key_driver.h"
#include "app_config.h"
#include "board_config.h"
#include "app_task.h"

//ad key
extern const u16 bt_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];
extern const u16 music_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];
extern const u16 fm_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];
extern const u16 record_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];
extern const u16 linein_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];
extern const u16 rtc_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];
extern const u16 pc_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];
extern const u16 spdif_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];
extern const u16 idle_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];
/***********************************************************
 *				adkey table 映射管理
 ***********************************************************/
typedef const u16(*type_key_ad_table)[KEY_EVENT_MAX];
static const type_key_ad_table ad_table[APP_TASK_MAX_INDEX] = {
#if TCFG_APP_BT_EN
    [APP_BT_TASK] 		= bt_key_ad_table,
#endif
#if TCFG_APP_MUSIC_EN
    [APP_MUSIC_TASK] 	= music_key_ad_table,
#endif
#if TCFG_APP_FM_EN
    [APP_FM_TASK] 		= fm_key_ad_table,
#endif
#if TCFG_APP_RECORD_EN
    [APP_RECORD_TASK] 	= record_key_ad_table,
#endif
#if TCFG_APP_LINEIN_EN
    [APP_LINEIN_TASK] 	= linein_key_ad_table,
#endif
#if TCFG_APP_RTC_EN
    [APP_RTC_TASK] 		= rtc_key_ad_table,
#endif
#if TCFG_APP_PC_EN
    [APP_PC_TASK] 		= pc_key_ad_table,
#endif
#if TCFG_APP_SPDIF_EN
    [APP_SPDIF_TASK] 	= spdif_key_ad_table,
#endif
    [APP_IDLE_TASK]     = idle_key_ad_table,
};

u16 adkey_event_to_msg(u8 cur_task, struct key_event *key)
{
    if (ad_table[cur_task] == NULL) {
        return KEY_NULL;
    }

    type_key_ad_table cur_task_ad_table = ad_table[cur_task];
    return	cur_task_ad_table[key->value][key->event];
}

//io key
extern const u16 bt_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX];
extern const u16 music_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX];
extern const u16 fm_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX];
extern const u16 record_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX];
extern const u16 linein_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX];
extern const u16 rtc_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX];
extern const u16 pc_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX];
extern const u16 spdif_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX];
extern const u16 idle_key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX];
/***********************************************************
 *				iokey table 映射管理
 ***********************************************************/
typedef const u16(*type_key_io_table)[KEY_EVENT_MAX];
static const type_key_io_table io_table[APP_TASK_MAX_INDEX] = {
#if TCFG_APP_BT_EN
    [APP_BT_TASK] 		= bt_key_io_table,
#endif
#if TCFG_APP_MUSIC_EN
    [APP_MUSIC_TASK] 	= music_key_io_table,
#endif
#if TCFG_APP_FM_EN
    [APP_FM_TASK] 		= fm_key_io_table,
#endif
#if TCFG_APP_RECORD_EN
    [APP_RECORD_TASK] 	= record_key_io_table,
#endif
#if TCFG_APP_LINEIN_EN
    [APP_LINEIN_TASK] 	= linein_key_io_table,
#endif
#if TCFG_APP_RTC_EN
    [APP_RTC_TASK] 		= rtc_key_io_table,
#endif
#if TCFG_APP_PC_EN
    [APP_PC_TASK] 		= pc_key_io_table,
#endif
#if TCFG_APP_SPDIF_EN
    [APP_SPDIF_TASK] 	= spdif_key_io_table,
#endif
    [APP_IDLE_TASK] 	= idle_key_io_table,
};

u16 iokey_event_to_msg(u8 cur_task, struct key_event *key)
{
    if (io_table[cur_task] == NULL) {
        return KEY_NULL;
    }

    type_key_io_table cur_task_io_table = io_table[cur_task];
    return	cur_task_io_table[key->value][key->event];
}

//ir key
extern const u16 bt_key_ir_table[KEY_IR_NUM_MAX][KEY_EVENT_MAX];
extern const u16 music_key_ir_table[KEY_IR_NUM_MAX][KEY_EVENT_MAX];
extern const u16 fm_key_ir_table[KEY_IR_NUM_MAX][KEY_EVENT_MAX];
extern const u16 record_key_ir_table[KEY_IR_NUM_MAX][KEY_EVENT_MAX];
extern const u16 linein_key_ir_table[KEY_IR_NUM_MAX][KEY_EVENT_MAX];
extern const u16 rtc_key_ir_table[KEY_IR_NUM_MAX][KEY_EVENT_MAX];
extern const u16 pc_key_ir_table[KEY_IR_NUM_MAX][KEY_EVENT_MAX];
extern const u16 spdif_key_ir_table[KEY_IR_NUM_MAX][KEY_EVENT_MAX];
extern const u16 idle_key_ir_table[KEY_IR_NUM_MAX][KEY_EVENT_MAX];
/***********************************************************
 *				irkey table 映射管理
 ***********************************************************/
typedef const u16(*type_key_ir_table)[KEY_EVENT_MAX];
static const type_key_ir_table ir_table[APP_TASK_MAX_INDEX] = {
#if TCFG_APP_BT_EN
    [APP_BT_TASK] 		= bt_key_ir_table,
#endif
#if TCFG_APP_MUSIC_EN
    [APP_MUSIC_TASK] 	= music_key_ir_table,
#endif
#if TCFG_APP_FM_EN
    [APP_FM_TASK] 		= fm_key_ir_table,
#endif
#if TCFG_APP_RECORD_EN
    [APP_RECORD_TASK] 	= record_key_ir_table,
#endif
#if TCFG_APP_LINEIN_EN
    [APP_LINEIN_TASK] 	= linein_key_ir_table,
#endif
#if TCFG_APP_RTC_EN
    [APP_RTC_TASK] 		= rtc_key_ir_table,
#endif
#if TCFG_APP_PC_EN
    [APP_PC_TASK] 		= pc_key_ir_table,
#endif
#if TCFG_APP_SPDIF_EN
    [APP_SPDIF_TASK] 	= spdif_key_ir_table,
#endif
    [APP_IDLE_TASK] 	= idle_key_ir_table,
};

u16 irkey_event_to_msg(u8 cur_task, struct key_event *key)
{
    if (ir_table[cur_task] == NULL) {
        return KEY_NULL;
    }

    type_key_ir_table cur_task_ir_table = ir_table[cur_task];
    return	cur_task_ir_table[key->value][key->event];
}

//rdec key
extern const u16 bt_key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX];
extern const u16 music_key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX];
extern const u16 fm_key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX];
extern const u16 record_key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX];
extern const u16 linein_key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX];
extern const u16 rtc_key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX];
extern const u16 pc_key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX];
extern const u16 spdif_key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX];
extern const u16 idle_key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX];

/***********************************************************
 *				rdec_key table 映射管理
 ***********************************************************/
typedef const u16(*type_key_rdec_table)[KEY_EVENT_MAX];
static const type_key_rdec_table rdec_table[APP_TASK_MAX_INDEX] = {
#if TCFG_APP_BT_EN
    [APP_BT_TASK] 		= bt_key_rdec_table,
#endif
#if TCFG_APP_MUSIC_EN
    [APP_MUSIC_TASK] 	= music_key_rdec_table,
#endif
#if TCFG_APP_FM_EN
    [APP_FM_TASK] 		= fm_key_rdec_table,
#endif
#if TCFG_APP_RECORD_EN
    [APP_RECORD_TASK] 	= record_key_rdec_table,
#endif
#if TCFG_APP_LINEIN_EN
    [APP_LINEIN_TASK] 	= linein_key_rdec_table,
#endif
#if TCFG_APP_RTC_EN
    [APP_RTC_TASK] 		= rtc_key_rdec_table,
#endif
#if TCFG_APP_PC_EN
    [APP_PC_TASK] 		= pc_key_rdec_table,
#endif
#if TCFG_APP_SPDIF_EN
    [APP_SPDIF_TASK] 	= spdif_key_rdec_table,
#endif
    [APP_IDLE_TASK] 	= idle_key_rdec_table,
};

u16 rdec_key_event_to_msg(u8 cur_task, struct key_event *key)
{
    if (rdec_table[cur_task] == NULL) {
        return KEY_NULL;
    }

    type_key_rdec_table cur_task_rdec_table = rdec_table[cur_task];
    return	cur_task_rdec_table[key->value][key->event];
}

//touch key
extern const u16 bt_key_touch_table[KEY_TOUCH_NUM_MAX][KEY_EVENT_MAX];
extern const u16 music_key_touch_table[KEY_TOUCH_NUM_MAX][KEY_EVENT_MAX];
extern const u16 fm_key_touch_table[KEY_TOUCH_NUM_MAX][KEY_EVENT_MAX];
extern const u16 record_key_touch_table[KEY_TOUCH_NUM_MAX][KEY_EVENT_MAX];
extern const u16 linein_key_touch_table[KEY_TOUCH_NUM_MAX][KEY_EVENT_MAX];
extern const u16 rtc_key_touch_table[KEY_TOUCH_NUM_MAX][KEY_EVENT_MAX];
extern const u16 pc_key_touch_table[KEY_TOUCH_NUM_MAX][KEY_EVENT_MAX];
extern const u16 spdif_key_touch_table[KEY_TOUCH_NUM_MAX][KEY_EVENT_MAX];
extern const u16 idle_key_touch_table[KEY_TOUCH_NUM_MAX][KEY_EVENT_MAX];
/***********************************************************
 *				touch_key table 映射管理
 ***********************************************************/
typedef const u16(*type_key_touch_table)[KEY_EVENT_MAX];
static const type_key_touch_table touch_table[APP_TASK_MAX_INDEX] = {
#if TCFG_APP_BT_EN
    [APP_BT_TASK] 		= bt_key_touch_table,
#endif
#if TCFG_APP_MUSIC_EN
    [APP_MUSIC_TASK] 	= music_key_touch_table,
#endif
#if TCFG_APP_FM_EN
    [APP_FM_TASK] 		= fm_key_touch_table,
#endif
#if TCFG_APP_RECORD_EN
    [APP_RECORD_TASK] 	= record_key_touch_table,
#endif
#if TCFG_APP_LINEIN_EN
    [APP_LINEIN_TASK] 	= linein_key_touch_table,
#endif
#if TCFG_APP_RTC_EN
    [APP_RTC_TASK] 		= rtc_key_touch_table,
#endif
#if TCFG_APP_PC_EN
    [APP_PC_TASK] 		= pc_key_touch_table,
#endif
#if TCFG_APP_SPDIF_EN
    [APP_SPDIF_TASK] 	= spdif_key_touch_table,
#endif
    [APP_IDLE_TASK] 	= idle_key_touch_table,
};

u16 touch_key_event_to_msg(u8 cur_task, struct key_event *key)
{
    if (touch_table[cur_task] == NULL) {
        return KEY_NULL;
    }

    type_key_touch_table cur_task_touch_table = touch_table[cur_task];
    return	cur_task_touch_table[key->value][key->event];
}
