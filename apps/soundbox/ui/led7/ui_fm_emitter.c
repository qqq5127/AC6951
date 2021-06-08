#include "app_config.h"
#include "key_event_deal.h"
#include "common/fm_emitter_led7_ui.h"
#include "ui/ui_api.h"
#include "fm_emitter/fm_emitter_manage.h"

#if (TCFG_UI_ENABLE && TCFG_APP_FM_EMITTER_EN)

#define LOG_TAG_CONST       APP_ACTION
#define LOG_TAG             "[APP_ACTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"



void fm_emitter_enter_ui_menu(void)
{
    u16 tmp_fmtx_fre;
    tmp_fmtx_fre = fm_emitter_manage_get_fre();

    if (ui_get_app_menu(GRT_CUR_MENU) == MENU_FM_SET_FRE) {
        UI_REFLASH_WINDOW(true);//刷新主页
        printf("fm menu exit !\n");
    } else if (ui_get_app_menu(GRT_CUR_MENU) == MENU_MAIN) {
        printf("fm menu in !\n");
        UI_SHOW_MENU(MENU_FM_SET_FRE, 10000, tmp_fmtx_fre, NULL);
    } else {
        printf("fm menu ui not ready %d!\n", __LINE__);
    }
}

void fm_emitter_enter_ui_next_fre(void)
{
    u16 tmp_fmtx_fre;
    tmp_fmtx_fre = fm_emitter_manage_get_fre();
    if (tmp_fmtx_fre < 1080) {
        tmp_fmtx_fre++;
    } else {
        tmp_fmtx_fre = 875;
    }
    UI_SHOW_MENU(MENU_FM_SET_FRE,  10000, tmp_fmtx_fre, NULL);
    fm_emitter_manage_set_fre(tmp_fmtx_fre);
}

void fm_emitter_enter_ui_prev_fre(void)
{
    u16 tmp_fmtx_fre;
    tmp_fmtx_fre = fm_emitter_manage_get_fre();
    if (tmp_fmtx_fre > 875) {
        tmp_fmtx_fre--;
    } else {
        tmp_fmtx_fre = 1080;
    }
#if TCFG_UI_ENABLE
    UI_SHOW_MENU(MENU_FM_SET_FRE,  10000, tmp_fmtx_fre, NULL);
    fm_emitter_manage_set_fre(tmp_fmtx_fre);
#endif
}


static u16 fre_num_tmp = 0;
static void set_fm_fre_timeout(u8 menu)
{
    if (menu == MENU_IR_FM_SET_FRE) {
        fre_num_tmp = 0;
        /* printf("set_fm_fre_timeout\n"); */
    }
}

void fm_emitter_fre_set_by_number(u8 num)
{
    fre_num_tmp = fre_num_tmp * 10 + num;
#if TCFG_UI_ENABLE
    /* printf("fre_num_tmp =%d \n",fre_num_tmp); */
    UI_SHOW_MENU(MENU_IR_FM_SET_FRE, 4 * 1000, fre_num_tmp, set_fm_fre_timeout);
#endif
    if (fre_num_tmp > 875) {
        fre_num_tmp = 0;
    }
}

int ui_fm_emitter_common_key_msg(int key_event)
{
    u8 ret = 0;
    switch (key_event) {
    case KEY_IR_NUM_0:
    case KEY_IR_NUM_1:
    case KEY_IR_NUM_2:
    case KEY_IR_NUM_3:
    case KEY_IR_NUM_4:
    case KEY_IR_NUM_5:
    case KEY_IR_NUM_6:
    case KEY_IR_NUM_7:
    case KEY_IR_NUM_8:
    case KEY_IR_NUM_9:
        fm_emitter_fre_set_by_number(key_event - KEY_IR_NUM_0);
        break;

    case KEY_FM_EMITTER_MENU:
        fm_emitter_enter_ui_menu();
        break;
    case KEY_FM_EMITTER_NEXT_FREQ:
        fm_emitter_enter_ui_next_fre();
        break;
    case KEY_FM_EMITTER_PERV_FREQ:
        fm_emitter_enter_ui_prev_fre();
        break;

    /* case KEY_VOL_UP: */
    /* log_info("COMMON KEY_VOL_UP\n"); */
    /* if (ui_get_app_menu(GRT_CUR_MENU) == MENU_FM_SET_FRE) { */
    /* fm_emitter_enter_ui_next_fre(); */
    /* ret = 1; */
    /* } */
    /* break; */

    /* case KEY_VOL_DOWN: */
    /* log_info("COMMON KEY_VOL_DOWN\n"); */
    /* if (ui_get_app_menu(GRT_CUR_MENU) == MENU_FM_SET_FRE) { */
    /* fm_emitter_enter_ui_prev_fre(); */
    /* ret = 1; */
    /* } */
    /* break; */
    default:
        ret = 1;
        break;
    }
    return ret;
}

#endif //(TCFG_UI_ENABLE && TCFG_APP_FM_EMITTER_EN)



