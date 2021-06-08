#ifndef __SMARTBOX_CMD_USER_H__
#define __SMARTBOX_CMD_USER_H__

#include "typedef.h"
#include "JL_rcsp_protocol.h"

//用户自定义数据接收
void smartbox_user_cmd_recieve(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len);
//用户自定义数据发送
JL_ERR smartbox_user_cmd_send(u8 *data, u16 len);

#endif//__SMARTBOX_CMD_USER_H__
