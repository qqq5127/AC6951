// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _SMARTBOX_RCSP_MANAGE_H
#define _SMARTBOX_RCSP_MANAGE_H

void JL_rcsp_event_to_user(u32 type, u8 event, u8 *msg, u8 size);
u8 get_rcsp_connect_status(void);
void rcsp_user_event_handler(u16 opcode, u8 *data, int size);

#endif
