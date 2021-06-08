#include "dev_format.h"
#include "smartbox/smartbox.h"
#include "system/fs/fs.h"
#include "dev_manager.h"

#if (SMART_BOX_EN && TCFG_DEV_MANAGER_ENABLE)
enum {
    DEV_FORMAT_ERR_NONE = 0,
    DEV_FORMAT_ERR_OFFLINE,
    DEV_FORMAT_ERR_FAIL,
};

void (*end_cbk)(void) = NULL;

//*----------------------------------------------------------------------------*/
/**@brief    设备格式化初始化(手机客户端发指令)
   @param
   @return   格式化完成回调
   @note
*/
/*----------------------------------------------------------------------------*/
void dev_format_init(void (*end_callback)(void))
{
    end_cbk = 	end_callback;
}

//*----------------------------------------------------------------------------*/
/**@brief    设备格式化退出
   @param
   @return   格式化完成回调
   @note
*/
/*----------------------------------------------------------------------------*/
static void dev_format_close(void)
{
    if (end_cbk) {
        end_cbk();
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    设备格式化处理响应(手机客户端发指令)
   @param
   @return   格式化完成回调
   @note
*/
/*----------------------------------------------------------------------------*/
void dev_format_start(u8 OpCode_SN, u8 *data, u16 len)
{
    u8 reason = DEV_FORMAT_ERR_NONE;
    u32 dev_handle = READ_BIG_U32(data);
    char *logo = smartbox_browser_dev_remap(dev_handle);
    if (logo == NULL) {
        reason = DEV_FORMAT_ERR_FAIL ;
        JL_CMD_response_send(JL_OPCODE_DEVICE_FORMAT, JL_PRO_STATUS_FAIL, OpCode_SN, &reason, 1);
        dev_format_close();
        return ;
    }

    char *root_path = dev_manager_get_root_path_by_logo(logo);
    if (root_path) {

        wdt_disable();
        int err = f_format(root_path, "fat", 32 * 1024);
        wdt_enable();
        if (err) {
            reason = DEV_FORMAT_ERR_FAIL ;
            JL_CMD_response_send(JL_OPCODE_DEVICE_FORMAT, JL_PRO_STATUS_FAIL, OpCode_SN, &reason, 1);
        } else {
            printf("dev format ok\n");
            JL_CMD_response_send(JL_OPCODE_DEVICE_FORMAT, JL_PRO_STATUS_SUCCESS, OpCode_SN, &reason, 1);
            dev_format_close();
            return ;
        }
    } else {
        reason = DEV_FORMAT_ERR_OFFLINE ;
        JL_CMD_response_send(JL_OPCODE_DEVICE_FORMAT, JL_PRO_STATUS_FAIL, OpCode_SN, &reason, 1);
    }
    dev_format_close();
    printf("dev format fail\n");
}
#else
void dev_format_init(void (*end_callback)(void))
{
}
void dev_format_start(u8 OpCode_SN, u8 *data, u16 len)
{
}
#endif

