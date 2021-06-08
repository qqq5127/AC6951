#ifndef __FM_EMITTER_LED7_UI_H__
#define __FM_EMITTER_LED7_UI_H__

#include "generic/typedef.h"
#include "app_config.h"

void fm_emitter_enter_ui_menu(void);
void fm_emitter_enter_ui_next_fre(void);
void fm_emitter_enter_ui_prev_fre(void);
void fm_emitter_fre_set_by_number(u8 num);
int ui_fm_emitter_common_key_msg(int key_event);

#endif//__FM_EMITTER_LED7_UI_H__
