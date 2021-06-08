#include "smartbox/cmd_user.h"

//*----------------------------------------------------------------------------*/
/**@brief    smartbox自定义命令数据接收处理
   @param    priv:全局smartbox结构体， OpCode:当前命令， OpCode_SN:当前的SN值， data:数据， len:数据长度
   @return
   @note	 二次开发需要增加自定义命令，通过JL_OPCODE_CUSTOMER_USER进行扩展，
  			 不要定义这个命令以外的命令，避免后续SDK更新导致命令冲突
*/
/*----------------------------------------------------------------------------*/
void smartbox_user_cmd_recieve(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    //自定义数据接收
    printf("%s:", __FUNCTION__);
    put_buf(data, len);
#if 0
    ///以下是发送测试代码
    u8 test_send_buf[] = {0x04, 0x05, 0x06};
    smartbox_user_cmd_send(test_send_buf, sizeof(test_send_buf));
#endif

    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);

}


//*----------------------------------------------------------------------------*/
/**@brief    smartbox自定义命令数据发送接口
   @param    data:数据， len:数据长度
   @return
   @note	 二次开发需要增加自定义命令，通过JL_OPCODE_CUSTOMER_USER进行扩展，
  			 不要定义这个命令以外的命令，避免后续SDK更新导致命令冲突
*/
/*----------------------------------------------------------------------------*/
JL_ERR smartbox_user_cmd_send(u8 *data, u16 len)
{
    //自定义数据接收
    printf("%s:", __FUNCTION__);
    put_buf(data, len);
    return JL_CMD_send(JL_OPCODE_CUSTOMER_USER, data, len, 1);
}


