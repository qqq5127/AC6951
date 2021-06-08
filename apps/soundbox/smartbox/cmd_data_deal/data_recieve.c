#include "smartbox/config.h"
#include "smartbox/event.h"

///>>>>>>>>设备接收到APP下发需要设备回复的数据
#if (SMART_BOX_EN)

void data_recieve(void *priv, u8 OpCode_SN, u8 CMD_OpCode, u8 *data, u16 len)
{
    printf("data_recieve, %x\n", CMD_OpCode);
}

#endif//SMART_BOX_EN

