#include "app_config.h"
#include "system/includes.h"
#include "chgbox_box.h"
#include "user_cfg.h"
#include "device/vm.h"
#include "app_task.h"
#include "app_main.h"
#include "chargeIc_manage.h"
#include "chgbox_ctrl.h"
#include "device/chargebox.h"
#include "asm/power/p33.h"
#include "chgbox_det.h"

#if(TCFG_CHARGE_BOX_ENABLE)

#define LOG_TAG_CONST       APP_CHGBOX
#define LOG_TAG             "[APP_CBOX]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#define CHGBOX_THR_NAME   "chgbox_n"
OS_MUTEX power_mutex;

//电池参数，电池不同，参数不同
//使用不同的电池要更新此表，不然数据可能会不准确
#define POWER_TOP_LVL   4200
#define POWER_BOOT_LVL  3100
#define POWER_LVL_MAX   11

const u16  voltage_table[2][POWER_LVL_MAX] = {
    {0,              10,   20,   30,   40,   50,   60,   70,   80,   90,   100},
    {POWER_BOOT_LVL, 3600, 3660, 3720, 3780, 3840, 3900, 3950, 4000, 4050, POWER_TOP_LVL},
};

/*------------------------------------------------------------------------------------*/
/**@brief    获取充电仓电量值
   @param    无
   @return   0~100 的电量值
   @note     电量根据电池电量表换算成0~100
*/
/*------------------------------------------------------------------------------------*/
u8 get_box_power_lvl()
{
    u16 max, min, power;
    u8 i;

    power = get_vbat_voltage();

    if (power <= POWER_BOOT_LVL) {
        return 0;
    }
    if (power >= POWER_TOP_LVL) {
        return 100;
    }

    for (i = 0; i < POWER_LVL_MAX; i++) {
        if (power < voltage_table[1][i]) {
            break;
        }
    }
    min = voltage_table[1][i - 1];
    max = voltage_table[1][i];
    return ((u8)(((power - min) * 10 / (max - min)) + voltage_table[0][i - 1]));
}

//通信的时候不能进入低功耗
static volatile u8 is_comm_active = 0;
static u8 comm_idle_query(void)
{
    return (!is_comm_active);
}
REGISTER_LP_TARGET(comm_lp_target) = {
    .name = "chgbox_comm",
    .is_idle = comm_idle_query,
};

/*------------------------------------------------------------------------------------*/
/**@brief    进入发送数据关处理
   @param    无
   @return   无
   @note     通信前要设置相关接口
*/
/*------------------------------------------------------------------------------------*/
void enter_hook(void)
{
    //进入发送数据前,先关闭升压输出
    os_mutex_pend(&power_mutex, 0);
    if (sys_info.charge) {
        chargeIc_pwr_ctrl(0);
    }
    chargebox_api_open_port(EAR_L);
    chargebox_api_open_port(EAR_R);
    is_comm_active = 1;
}

/*------------------------------------------------------------------------------------*/
/**@brief    发送数据后恢复处理
   @param    无
   @return   无
   @note     根据状态恢复通信口
*/
/*------------------------------------------------------------------------------------*/
void exit_hook(void)
{
    //退出发送数据后,是否需要打开升压输出
    if (sys_info.charge && (sys_info.temperature_limit == 0)) {
        chargebox_check_output_short();
        chargebox_api_shutdown_port(EAR_L);
        chargebox_api_shutdown_port(EAR_R);
        if (sys_info.current_limit == 0) {
            chargeIc_pwr_ctrl(1);
        }
    } else {
        chargebox_api_close_port(EAR_L);
        chargebox_api_close_port(EAR_R);
    }
    is_comm_active = 0;
    os_mutex_post(&power_mutex);
}

/*------------------------------------------------------------------------------------*/
/**@brief    发消息到通信处理线程
   @param    msg:待发送消息
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void app_chargebox_send_mag(int msg)
{
    //有数据需要发送,自动关机计时器复位
    sys_info.life_cnt = 0;
    os_taskq_post_msg(CHGBOX_THR_NAME, 1, msg);
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓事件发送函数
   @param    event:待发送事件
   @return   无
   @note
*/
/*------------------------------------------------------------------------------------*/
void app_chargebox_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_CHARGEBOX;
    e.u.dev.event = event;
    sys_event_notify(&e);
}

/*------------------------------------------------------------------------------------*/
/**@brief    耳机在线检测
   @param    ret_l:左耳命令回应标志
             ret_r:右耳命令回应标志
   @return   无
   @note     检测并发相应的出入仓事件
*/
/*------------------------------------------------------------------------------------*/
void app_chargebox_api_check_online(bool ret_l, bool ret_r)
{
    if (ret_l == TRUE) {
        if (ear_info.online[EAR_L] == 0) {
            //发事件,耳机入舱
            app_chargebox_event_to_user(CHGBOX_EVENT_EAR_L_ONLINE);
        }
        ear_info.online[EAR_L] = TCFG_EAR_OFFLINE_MAX;
    } else {
        if (ear_info.online[EAR_L]) {
            ear_info.online[EAR_L]--;
            if (ear_info.online[EAR_L] == 0) {
                ear_info.power[EAR_L] = 0xff;
                //发事件,耳机离舱
                app_chargebox_event_to_user(CHGBOX_EVENT_EAR_L_OFFLINE);
            }
        }
    }
    if (ret_r == TRUE) {
        if (ear_info.online[EAR_R] == 0) {
            //发事件,耳机入舱
            app_chargebox_event_to_user(CHGBOX_EVENT_EAR_R_ONLINE);
        }
        ear_info.online[EAR_R] = TCFG_EAR_OFFLINE_MAX;
    } else {
        if (ear_info.online[EAR_R]) {
            ear_info.online[EAR_R]--;
            if (ear_info.online[EAR_R] == 0) {
                ear_info.power[EAR_R] = 0xff;
                //发事件,耳机离舱
                app_chargebox_event_to_user(CHGBOX_EVENT_EAR_R_OFFLINE);
            }
        }
    }

    if ((ear_info.online[EAR_L] == 0) && (ear_info.online[EAR_R] == 0)) {
        chargebox_api_reset();
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    发送 耳机关机 指令
   @param    无
   @return   TRUE:发送成功  FALSE:发送失败
   @note
*/
/*------------------------------------------------------------------------------------*/
u8 app_chargebox_api_send_shutdown(void)
{
    u8 ret0, ret1;
    enter_hook();
    ret0 = chargebox_send_shut_down(EAR_L);
    ret1 = chargebox_send_shut_down(EAR_R);
    exit_hook();
    if ((ret0 == TRUE) && (ret1 == TRUE)) {
        log_debug("send shutdown ok!\n");
        return TRUE;
    } else {
        log_error("shut down, L:%d,R:%d\n", ret0, ret1);
    }
    return FALSE;
}

/*------------------------------------------------------------------------------------*/
/**@brief    发送合盖指令
   @param    无
   @return   无
   @note     发指令并检测在线
*/
/*------------------------------------------------------------------------------------*/
u8 app_chargebox_api_send_close_cid(void)
{
    u8 online_cnt = 0;;
    u8 ret0, ret1;
    if (ear_info.online[EAR_L]) {
        online_cnt += 1;
    }
    if (ear_info.online[EAR_R]) {
        online_cnt += 1;
    }
    enter_hook();
    ret0 = chargebox_send_close_cid(EAR_L, online_cnt);
    ret1 = chargebox_send_close_cid(EAR_R, online_cnt);
    exit_hook();
    app_chargebox_api_check_online(ret0, ret1);
    if ((ret0 == TRUE) && (ret1 == TRUE)) {
        log_debug("send close CID ok\n");
        return TRUE;
    } else {
        log_error("LID close, L:%d,R:%d\n", ret0, ret1);
    }
    return FALSE;
}

//存放左、右、公共地址
static u8 adv_addr_tmp_buf[3][6];
/*------------------------------------------------------------------------------------*/
/**@brief    记录左/右耳机地址
   @param    lr:1--左耳机  0--右耳机
             inbuf:输入地址的buf指针
   @return   无
   @note     记录地址到对应的buf
*/
/*------------------------------------------------------------------------------------*/
void get_lr_adr_cb(u8 lr, u8 *inbuf)
{
    if (lr) {
        memcpy(&adv_addr_tmp_buf[1][0], inbuf, 6);
    } else {
        memcpy(&adv_addr_tmp_buf[0][0], inbuf, 6);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    修改广播地址回调
   @param
   @return   无
   @note     修改广播地址，并把广播地址更新到对应的buf
*/
/*------------------------------------------------------------------------------------*/
void exchange_addr_succ_cb(void)
{
    u8 i;
    for (i = 0; i < 6; i++) {
        adv_addr_tmp_buf[2][i] = adv_addr_tmp_buf[0][i] + adv_addr_tmp_buf[1][i];
    }
    sys_info.chg_addr_ok = 1;
}

/*------------------------------------------------------------------------------------*/
/**@brief    获取广播地址
   @param
   @return   NULL:无广播地址  其他：返回数组地址
   @note
*/
/*------------------------------------------------------------------------------------*/
u8 *get_chargebox_adv_addr(void)
{
    if (sys_info.chg_addr_ok) {
        return &adv_addr_tmp_buf[2][0];
    } else {
        return NULL;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    从vm读取广播地址
   @param    无
   @return   TRUE:读取成功  0:读取不到地址
   @note     读取成功就记录在对应的数组中
*/
/*------------------------------------------------------------------------------------*/
u8 chgbox_addr_read_from_vm(void)
{
    if (6 == syscfg_read(CFG_CHGBOX_ADDR, &adv_addr_tmp_buf[2][0], 6)) {
        sys_info.chg_addr_ok = 1;
        log_info("Read adv addr OK:");
        put_buf(&adv_addr_tmp_buf[2][0], 6);
        return TRUE;
    } else {
        sys_info.chg_addr_ok = 0;
        log_error("Read adv addr error\n");
        return FALSE;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓获取两耳机地址扫描
   @param    无
   @return   1:无操作  0:配对中
   @note     没有检测到广播地址（chg_addr_ok==0）做配对扫描，获取广播地址(左右耳在线且开盖)
*/
/*------------------------------------------------------------------------------------*/
u8 chgbox_addr_save_to_vm(void)
{
    if (6 == syscfg_write(CFG_CHGBOX_ADDR, &adv_addr_tmp_buf[2][0], 6)) {
        log_info("Write adv addr OK!\n");
        return TRUE;
    } else {
        log_error("Write adv addr error!\n");
        return FALSE;
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓获取两耳机地址扫描
   @param    无
   @return   1:无操作  0:配对中
   @note     没有检测到广播地址（chg_addr_ok==0）做配对扫描，获取广播地址(左右耳在线且开盖)
*/
/*------------------------------------------------------------------------------------*/
u8 chgbox_adv_addr_scan(void)
{
    //注意：部分耳机要收到open_power指令后才跑蓝牙流程,才能拿到地址
    static u8 caa_cnt = 0;
    if (!sys_info.chg_addr_ok) {
        if (ear_info.online[EAR_L] && ear_info.online[EAR_R] && sys_info.chgbox_status == CHG_STATUS_COMM) {
            caa_cnt++;
            if (caa_cnt < 8) {
                //0~4是直接返回1，不做操作，留给外面的200Ms操作发open_power指令
                if (caa_cnt == 5) { //交换地址(配对)
                    log_debug("ss-0\n");
                    app_chargebox_send_mag(CHGBOX_MSG_SEND_PAIR);
                }
                if (caa_cnt > 4) {
                    log_debug("ss-1\n");
                    return 0;//发配对指令后不发其他命令,避免干扰
                }
            } else {
                caa_cnt = 0; //如果拿不到地址(chg_addr_ok==0)，清0循环
            }
        }
    }
    log_debug("ss-2\n");
    return 1;
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓交换地址app
   @param    无
   @return   交换地址是否成功
   @note     如果左右耳机在线，则交换地址，若交换成功就记录下地址
*/
/*------------------------------------------------------------------------------------*/
u8 app_chargebox_api_exchange_addr(void)
{
    u8 ret = FALSE;
    if (ear_info.online[EAR_L] && ear_info.online[EAR_R]) {
        enter_hook();
        ret = chargebox_exchange_addr(get_lr_adr_cb, exchange_addr_succ_cb);
        exit_hook();
    }

    if (ret) {
        //交换地址成功后记录地址
        chgbox_addr_save_to_vm();
    }
    return ret;
}

/*------------------------------------------------------------------------------------*/
/**@brief    私有命令例子
 * @param    无
 * @return   无
 * @note     1、自定义命令必须在0xC0和0xFE之间
 *           2、发送长度必须小于32字节
 *           3、读回来的数据在lr_buf 长度在lr_len
 */
/*------------------------------------------------------------------------------------*/
void app_chargebox_api_send_cmd_demo(void)
{
    extern u8 lr_buf[2][32];
    extern u8 lr_len[2];
    u8 buf[3];
    buf[0] = CMD_USER;
    buf[1] = 0x12;
    buf[2] = 0x34;
    enter_hook();
    if (chargebox_api_write_read(EAR_L, buf, 3, 4) == TRUE) {
        //耳机有回复数据
        log_dump(lr_buf[EAR_L], lr_len[EAR_L]);
    }
    if (chargebox_api_write_read(EAR_R, buf, 3, 4) == TRUE) {
        //耳机有回复数据
        log_dump(lr_buf[EAR_R], lr_len[EAR_R]);
    }
    exit_hook();
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓发送电量命令获取电量
   @param    flag:1 合盖   0 开盖
   @return   无
   @note     发送仓电量、充电态、A耳机的电量 给B耳机，获取B耳机的电量，根据回应判断B是否在线 (两个耳机都进行)
*/
/*------------------------------------------------------------------------------------*/
void app_chargebox_api_send_power(u8 flag)
{
    u8 power;
    u8 ret0, ret1, is_charge = 0;
    power = get_box_power_lvl();//获取仓的电量
    if (sys_info.status[USB_DET] == STATUS_ONLINE) {
        is_charge = 1;
    }
    enter_hook();
    if (flag == 0) {
        ret0 = chargebox_send_power_open(EAR_L, power, is_charge, ear_info.power[EAR_R]);
        ret1 = chargebox_send_power_open(EAR_R, power, is_charge, ear_info.power[EAR_L]);
    } else {
        ret0 = chargebox_send_power_close(EAR_L, power, is_charge, ear_info.power[EAR_R]);
        ret1 = chargebox_send_power_close(EAR_R, power, is_charge, ear_info.power[EAR_L]);
    }
    exit_hook();
    app_chargebox_api_check_online(ret0, ret1);
    if (ret0 == TRUE) {
        ear_info.power[EAR_L] = chargebox_get_power(EAR_L);
        log_info("Ear_L:%d_%d", ear_info.power[EAR_L]&BIT(7) ? 1 : 0, ear_info.power[EAR_L] & (~BIT(7)));
    } else {
        /* log_error("Can't got L\n"); */
    }
    if (ret1 == TRUE) {
        ear_info.power[EAR_R] = chargebox_get_power(EAR_R);
        log_info("Ear_R:%d_%d", ear_info.power[EAR_R]&BIT(7) ? 1 : 0, ear_info.power[EAR_R] & (~BIT(7)));
    } else {
        /* log_error("Can't got R\n"); */
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓通信线程
   @param    priv:扩展参数
   @return   无
   @note     该线程负责给左右耳发送数据
*/
/*------------------------------------------------------------------------------------*/
void app_chargebox_task_handler(void *priv)
{
    int msg[32];
    log_info("data thread running! \n");

    while (1) {
        if (os_task_pend("taskq", msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }
        switch (msg[1]) {
        case CHGBOX_MSG_SEND_POWER_OPEN:
            app_chargebox_api_send_power(0);
            break;
        case CHGBOX_MSG_SEND_POWER_CLOSE:
            app_chargebox_api_send_power(1);
            break;
        case CHGBOX_MSG_SEND_CLOSE_LID:
            app_chargebox_api_send_close_cid();
            break;
        case CHGBOX_MSG_SEND_SHUTDOWN:
            app_chargebox_api_send_shutdown();
            break;
        case CHGBOX_MSG_SEND_PAIR:
            log_info("CHANGE ear ADDR\n");
            if (app_chargebox_api_exchange_addr() == TRUE) {
                sys_info.pair_succ = 1;
            } else {
                log_error("pair_fail\n");
            }
            break;
        default:
            log_info("default msg: %d\n", msg[1]);
            break;
        }
    }
}

CHARGEBOX_PLATFORM_DATA_BEGIN(chargebox_data)
.L_port = TCFG_CHARGEBOX_L_PORT,
 .R_port = TCFG_CHARGEBOX_R_PORT,
  CHARGEBOX_PLATFORM_DATA_END()

  /*------------------------------------------------------------------------------------*/
  /**@brief    充电仓提前初始化内容
     @param    priv:扩展参数
     @return   无
     @note
  */
  /*------------------------------------------------------------------------------------*/
  void app_chargebox_timer_handle(void *priv)
{
    static u8 ms200_cnt = 0;
    static u8 ms500_cnt = 0;

    ms200_cnt++;
    if (ms200_cnt >= 2) {
        ms200_cnt = 0;
        app_chargebox_event_to_user(CHGBOX_EVENT_200MS);
    }
    ms500_cnt++;
    if (ms500_cnt >= 5) {
        ms500_cnt = 0;
        app_chargebox_event_to_user(CHGBOX_EVENT_500MS);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    充电仓提前初始化内容
   @param    无
   @return   无
   @note     读取广播地址、初始化模块、创建通信线程、注册相关函数到timer
*/
/*------------------------------------------------------------------------------------*/
int chargebox_advanced_init(void)
{
    chgbox_addr_read_from_vm();
    chargebox_api_init(&chargebox_data);
    task_create(app_chargebox_task_handler, NULL, CHGBOX_THR_NAME);
    os_mutex_create(&power_mutex);
    sys_timer_add(NULL, app_chargebox_timer_handle, 100);//推事件处理
    return 0;
}

__initcall(chargebox_advanced_init);

#endif
