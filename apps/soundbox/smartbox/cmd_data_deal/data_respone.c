#include "smartbox/config.h"
#include "smartbox/event.h"

////>>>>>>>>>>>>>>>设备接收到上报数据的回复
#if (SMART_BOX_EN)
void data_respone(void *priv, u8 status, u8 CMD_OpCode, u8 *data, u16 len)
{
    printf("data_respone, %x\n", CMD_OpCode);
}
#endif//SMART_BOX_EN

