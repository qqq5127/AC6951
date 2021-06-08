#include "spdif/hdmi_cec.h"
#include "spdif/hdmi_cec_api.h"
#include "app_config.h"
#include "asm/clock.h"
#include "app_config.h"
#include "gpio.h"
#include "system/timer.h"
#if ((TCFG_SPDIF_ENABLE) && (TCFG_HDMI_ARC_ENABLE))

//#pragma const_seg(".HDMI_const")
//#pragma code_seg(".HDMI_code")


static void cec_set_wait_time(u32 us);

static void cec_set_line_state(const u8 state)
{
    gpio_direction_output(TCFG_HDMI_CEC_PORT, state);
}

static u32 cec_get_line_state()
{
    return gpio_read(TCFG_HDMI_CEC_PORT);
}

static u8 CEC_Version_cmd[3] = {0x00, CEC_VERSION, 0x04}; //0x04 :1.3a ????
static u8 Report_Power_Status_Cmd[3] = {0x00, 0x90, 0x00}; //0x00:ON ; 0x01:OFF
static u8 Standby_Cmd[2] = {0x00, STANDBY};

static u8 PollingMessage_cmd[1] = {0x55};
static u8 Report_Physical_Add_Cmd1[7] = {0x5F, REPORT_PHYSICAL_ADDRESS, 0x20, 0x00, 0x05};
static u8 Report_Physical_Add_Cmd[7] = {0x5F, REPORT_PHYSICAL_ADDRESS, 0x10, 0x00, 0x05};

static u8 Device_Vendor_Id_Cmd[5] = {0x5f, DEVICE_VENDOR_ID, 0x45, 0x58, 0x50};
static u8 Set_System_Audio_Mode_cmd[] = {0x50, SET_SYSTEM_AUDIO_MODE, 0x01};
static u8 Broadcast_System_Audio_Mode_cmd[] = {0x5F, SET_SYSTEM_AUDIO_MODE, 0x01};
static u8 Set_Osd_Name_cmd[] = {0x50, CECOP_SET_OSD_NAME, 0x41, 0x75, 0x64, 0x69, 0x6f, 0x20, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6d}; //["Audio System"]

static u8 System_Audio_Mode_Status_cmd[] = {0x50, CECOP_SYSTEM_AUDIO_MODE_STATUS, 0x01};
//static u8 Report_Audio_Status_cmd[] = {0x50, CECOP_REPORT_AUDIO_STATUS, 0xB2};// bit7 off/on bit6-0 volume
static u8 Report_Audio_Status_cmd[] = {0x50, CECOP_REPORT_AUDIO_STATUS, 0x32};

static u8 Report_Power_Status_cmd[] = {0x50, CECOP_REPORT_POWER_STATUS, 0x00};// 0:on 1 standby 2: S2O 3: o2s

static u8 Terminate_Arc_cmd[2] = {0x50, CECOP_ARC_TERMINATE};
static u8 Initiate_Arc_cmd[2] = {0x50, INITIATE_ARC};
static u8 Report_Short_Audio_Desc_cmd[8] = {0x50, REPORT_SHORT_AUDIO_DESCRIPTOR, 0x15, 0x17, 0x50, 0x3E, 0x06, 0xC0}; //AC-3 6Ch (48 44 32k) 640kbps/ DTS 7Ch (48 44K)  1536kbps

static u8 Give_Tuner_Device_Status_cmd[3] = {0x50, CECOP_GIVE_TUNER_DEVICE_STATUS, 0x01};
static u8 Feature_Abort_cmd[3] = {0x50, CECOP_FEATURE_ABORT, 0x00};

#define     CEC_IDLE    0
#define     CEC_TX      1
#define     CEC_RX      2

static u8 cec_work_mode;
static s8 cec_state;
static u16 high_period_time;
enum {
    CEC_SUCC,
    NAK_HEAD,
    NAK_DATA,
};

struct cec_msg_frame {
    u8 *msg;
    u8 msg_len;
    u8 tx_count;
    u8 ack;
    u8 broadcast;
};

static struct cec_msg_frame cec_frame;

struct hdmi_arc_t {
    u8 msg_count;
    u8 state;
    u8 logic_addr;
    u8 rx_count;
    u8 rx_msg;
    u8 idle_time;
    u16 time_id;
    u8 cec_ok;

    u8 msg[17];

};

static struct hdmi_arc_t arc_devcie;
#define __this  (&arc_devcie)

typedef enum {
    CEC_XMIT_STARTBIT,
    CEC_XMIT_BIT7,
    CEC_XMIT_BIT6,
    CEC_XMIT_BIT5,
    CEC_XMIT_BIT4,
    CEC_XMIT_BIT3,
    CEC_XMIT_BIT2,
    CEC_XMIT_BIT1,
    CEC_XMIT_BIT0,
    CEC_XMIT_BIT_EOM,
    CEC_XMIT_BIT_ACK,
    CEC_XMIT_ACK_PU,
    CEC_XMIT_CHECK_ACK,
    CEC_XMIT_ACK_END,

    CEC_WAIT_IDLE,
    CEC_RECEIVE,
    CEC_CONTINUE_BIT,
} CEC_XMIT_STATE ;

static void cec_start_bit()
{
    cec_state = CEC_XMIT_STARTBIT;
    gpio_direction_output(TCFG_HDMI_CEC_PORT, 0);
    gpio_set_pull_down(TCFG_HDMI_CEC_PORT, 0);

    const u16 low_time = 3700;
    const u16 high_time = 4500 - low_time;
    cec_set_wait_time(low_time);
    high_period_time = high_time;

}
static void cec_logic_bit0()
{
    const u16 low_time = 1500;
    const u16 high_time = 2400 - low_time;
    cec_set_wait_time(low_time);
    cec_set_line_state(0);
    high_period_time = high_time;
}
static void cec_logic_bit1()
{
    const u16 low_time = 600;
    const u16 high_time = 2400 - low_time;
    cec_set_wait_time(low_time);
    cec_set_line_state(0);
    high_period_time = high_time;
}
static void cec_end_bit()
{
    const u16 low_time = 600;
    cec_set_line_state(0);
    cec_set_wait_time(low_time);
}
static void cec_ack_bit()
{
    const u16 low_time = 1500;
    const u16 high_time = 1800 - low_time;
    cec_set_wait_time(low_time);
    cec_set_line_state(0);
    high_period_time = high_time;
}
static void transmit_complete(u32 state)
{
    gpio_set_direction(TCFG_HDMI_CEC_PORT, 1);
    cec_work_mode = CEC_IDLE;
    cec_frame.ack = state;
    cec_state = CEC_RECEIVE;
}
static void tx_bit_isr()
{
    if (high_period_time) {
        cec_set_line_state(1);
        cec_set_wait_time(high_period_time);
        high_period_time = 0;
        cec_state++;
        return;
    }
    switch (cec_state) {
    case CEC_WAIT_IDLE:
        cec_start_bit();
        break;

    case CEC_XMIT_STARTBIT:
        break;

    case CEC_XMIT_ACK_END:
        if (cec_frame.ack) {
            cec_set_line_state(1);
            if (cec_frame.tx_count == cec_frame.msg_len) {
                transmit_complete(CEC_SUCC);
                break;
            } else {
                cec_state = CEC_XMIT_BIT7;
            }
        } else {
            if (cec_frame.tx_count == 1) {
                transmit_complete(NAK_HEAD);
            } else {
                transmit_complete(NAK_DATA);
            }
            break;
        }
    case CEC_XMIT_BIT7 ... CEC_XMIT_BIT0:
        if (cec_frame.msg[cec_frame.tx_count] & BIT(7 - (cec_state - CEC_XMIT_BIT7))) {
            cec_logic_bit1();
        } else {
            cec_logic_bit0();
        }
        break;

    case CEC_XMIT_BIT_EOM:
        cec_frame.tx_count++;
        if (cec_frame.tx_count == cec_frame.msg_len) {
            cec_logic_bit1();
        } else {
            cec_logic_bit0();
        }
        break;

    case CEC_XMIT_BIT_ACK:
        cec_state++;
        cec_end_bit();
        break;

    case CEC_XMIT_ACK_PU:
        gpio_set_direction(TCFG_HDMI_CEC_PORT, 1);
        cec_set_wait_time(1000 - 600);
        cec_state++;
        break;

    case CEC_XMIT_CHECK_ACK:
        cec_frame.ack = cec_frame.broadcast ? 1 : !cec_get_line_state();
        cec_state++;
        cec_set_wait_time(2400 - 1000);
        break;
    }

}

#define     CAP_DIV         64
#define     CAP_CLK         24000000
#define     CEC_TMR         JL_TIMER2
#define     CEC_TMR_IDX     IRQ_TIME2_IDX

#define     CAP_RISING_EDGE    0b10
#define     CAP_FALLING_EDGE   0b11
static void change_capture_mode(u32 mode)
{
    CEC_TMR->PRD = 0;
    CEC_TMR->CNT = 0;
    CEC_TMR->CON = ((0b0011 << 4) | BIT(3));
    CEC_TMR->CON |= mode;
}
static u16 get_captrue_time()
{
    return CEC_TMR->PRD;
}
//New initiator wants to send a frame wait time
#define     _ms *1//000
#define     CEC_NEW_FRAME_WAIT_TIME            	(5*(2.4 _ms))//(5*(2.4 _ms))
#define     CEC_RETRANSMIT_FRAME                (3*(2.4 _ms))

static void cec_set_wait_time(u32 us)
{
    /* us = 3700; */
    CEC_TMR->CON = BIT(14);
    u32 clk = clk_get("timer") / 320000;
    /* printf("clk %d %d %d", clk, us, us / 100 * clk); */
    CEC_TMR->CON = ((0b0110 << 4) | BIT(3));
    CEC_TMR->CNT = 0;
    CEC_TMR->PRD = us / 100 * clk;
    CEC_TMR->CON |= BIT(0);
}
static u32 cec_bit_time_check(u16 t, u16 min, u16 max)
{
    const u16 _min = min * (CAP_CLK / 1000000) / CAP_DIV;
    const u16 _max = max * (CAP_CLK / 1000000) / CAP_DIV;
    if ((t > _min) && (t < _max)) {
        return 1;
    } else {
        return 0;
    }
}
static s8 cec_get_bit_value(const u16 low_time, const u16 high_time)
{
    if (cec_bit_time_check(low_time + high_time, 2050, 2750)) {
        if (cec_bit_time_check(low_time, 400, 800)) {
            return 1;
        } else if (cec_bit_time_check(low_time, 1300, 1700)) {
            return 0;
        }
    }
    return -1;
}

static void cec_cmd_response();
static void rx_bit_isr()
{
    static u16 low_time = 0;
    static u16 high_time = 0;
    static u16 eom = 0;
    static u8 follower = 0;
    u32 mode = CEC_TMR->CON & 0x3;
    CEC_TMR->CON = 0;

    if (high_period_time) {
        cec_set_line_state(1);
        cec_set_wait_time(high_period_time);
        high_period_time = 0;
        cec_state = CEC_XMIT_ACK_END;
        return;
    }

    if (cec_state == CEC_XMIT_ACK_END) {
        gpio_set_direction(TCFG_HDMI_CEC_PORT, 1);
        __this->rx_count ++;
        if (eom) {
            __this->rx_msg = 1;
            u16 ttmp;
            ttmp = usr_timeout_add(NULL, cec_cmd_response, CEC_NEW_FRAME_WAIT_TIME, 0);
//			printf("****ttmp = 0x%x\n", ttmp);
        } else {
            cec_state = CEC_CONTINUE_BIT;
        }

        change_capture_mode(CAP_FALLING_EDGE);
        return;
    }

    u16 time = get_captrue_time();

    if (mode == CAP_RISING_EDGE) {
        low_time = time;
        time = 0;
        mode = CAP_FALLING_EDGE;
    } else {
        mode = CAP_RISING_EDGE;
    }

    change_capture_mode(mode);

    if (time == 0) {
        return;
    }

    if (cec_state == CEC_RECEIVE) {
        cec_state = CEC_XMIT_STARTBIT;
        low_time = 0;
        high_time = 0;
        return;
    }

    s8 bit;

    high_time = time;

    switch (cec_state) {
    case CEC_XMIT_STARTBIT:
        if (low_time && high_time) {
            if (cec_bit_time_check(low_time, 3500, 3900) &&
                cec_bit_time_check(low_time + high_time, 4300, 4700)) {
                cec_state ++;
            }
        }

        follower = 0;
        __this->rx_count = 0;
        low_time = 0;
        high_time = 0;
        break;

    case CEC_CONTINUE_BIT:
        cec_state = CEC_XMIT_BIT7;
        break;

    case CEC_XMIT_BIT7 ... CEC_XMIT_BIT0:
        bit = cec_get_bit_value(low_time, high_time);
        if (bit == 0) {
            __this->msg[__this->rx_count] &= ~BIT(7 - (cec_state - CEC_XMIT_BIT7));
//            cec_state++;
        } else if (bit == 1) {
            __this->msg[__this->rx_count] |= BIT(7 - (cec_state - CEC_XMIT_BIT7));
//            cec_state++;
        } else if (bit == -1) { //error bit
            cec_state = CEC_XMIT_STARTBIT;
        }
        cec_state++;
        break;

    case CEC_XMIT_BIT_EOM:
        cec_state++;
        bit = cec_get_bit_value(low_time, high_time);
        if (bit == 0) {
            eom = 0;
        } else if (bit == 1) {
            eom = 1;
        } else if (bit == -1) { //error bit
            cec_state = CEC_XMIT_STARTBIT;
        }
        if (__this->rx_count == 0) { //head block
            follower = __this->msg[0] & 0xf;
        }

        if (follower == __this->logic_addr) {
            cec_ack_bit();//send ack
        } else {
            /* printf(" %d %d %d", __this->rx_count, follower, __this->logic_addr); */
            __this->rx_count = 0;
        }
        break;

    case CEC_XMIT_BIT_ACK:
        cec_state = CEC_XMIT_BIT7;
        break;
    }

}
#define     TIME_PRD    30
___interrupt
static void cec_timer_isr()
{
    CEC_TMR->CON |= BIT(14);

    if (cec_work_mode == CEC_TX) {
        tx_bit_isr();
        if (cec_work_mode == CEC_IDLE) {
            change_capture_mode(CAP_FALLING_EDGE);
        }
    } else {

        if (__this->idle_time < (150 / TIME_PRD)) {
            __this->idle_time = 150 / TIME_PRD;
        }

        rx_bit_isr();
    }

}


u32 cec_transmit_frame(unsigned char *buffer, int count)
{
    if (cec_work_mode != CEC_IDLE) {
        return 1;
    }
    __this->idle_time = 1;
    cec_work_mode = CEC_TX;
    CEC_TMR->CON = BIT(14);
    cec_frame.ack = -1;
    cec_frame.msg = buffer;
    cec_frame.msg_len = count;
    cec_frame.tx_count = 0;
    cec_frame.broadcast = 0;

    if ((buffer[0] & 0xf) == CEC_LOGADDR_UNREGORBC) {
        cec_frame.broadcast = 1;
    }

    if ((buffer[0] & 0xf) == __this->logic_addr) {
        cec_frame.broadcast = 1;
    }

    gpio_set_direction(TCFG_HDMI_CEC_PORT, 1);
    if (cec_get_line_state()) {
        /* g_printf("start"); */
        cec_start_bit();
    } else {
        /* g_printf("wait idle"); */
        high_period_time = 0;
        cec_state = CEC_WAIT_IDLE;
        change_capture_mode(CAP_RISING_EDGE);
    }

    return 0;
}
typedef enum {
    ARC_STATUS_REQ_ADD,
    ARC_STATUS_WAIT_ONLINE,
    ARC_STATUS_REPORT_ADD,
    ARC_STATUS_STB,
    ARC_STATUS_INIT,
    ARC_STATUS_REQ_INIT,
    ARC_STATUS_INIT_SUCCESS,
    ARC_STATUS_TERMINATE,
    ARC_STATUS_REQ_TERMINATE,
    ARC_STATUS_REPORT_TERMINATE,
} ARC_STATUS;

static void decode_cec_command()
{
    printf("CEC command ");
    put_buf(__this->msg, __this->rx_count);
    u8 intiator = __this->msg[0] >> 4;
    switch (__this->msg[1]) {
    case CECOP_FEATURE_ABORT:
        printf("__this->state < ARC_STATUS_INIT=0x%x\n", __this->state);
        if (__this->state <= ARC_STATUS_INIT) {
//		 	putchar('2');
            __this->cec_ok = 0;
            __this->state = ARC_STATUS_REQ_ADD;
        }
        break;
    case CECOP_GIVE_PHYSICAL_ADDRESS:
        cec_transmit_frame(Report_Physical_Add_Cmd, 5);
        break;
    case CECOP_GIVE_DEVICE_VENDOR_ID:
        cec_transmit_frame(Device_Vendor_Id_Cmd, 5);
        break;
    case GET_CEC_VERSION:
        CEC_Version_cmd[0] = intiator;
        cec_transmit_frame(CEC_Version_cmd, 3);
        break;
    case CECOP_ARC_REPORT_INITIATED:
        __this->idle_time = 500 / TIME_PRD;
        break;

    case CECOP_SET_STREAM_PATH:
        break;
    case CECOP_GIVE_OSD_NAME:
        cec_transmit_frame(Set_Osd_Name_cmd, 14);
        break;

    case CECOP_REQUEST_SHORT_AUDIO:
        __this->cec_ok = 1;
        cec_transmit_frame(Report_Short_Audio_Desc_cmd, sizeof(Report_Short_Audio_Desc_cmd));
        __this->idle_time = 3500 / TIME_PRD;
        break;

    case CECOP_SYSTEM_AUDIO_MODE_REQUEST:
//        cec_transmit_frame(Set_System_Audio_Mode_cmd, sizeof(Set_System_Audio_Mode_cmd));
        cec_transmit_frame(Broadcast_System_Audio_Mode_cmd, sizeof(Set_System_Audio_Mode_cmd));
        break;
    case CECOP_GIVE_AUDIO_STATUS:
        cec_transmit_frame(Report_Audio_Status_cmd, sizeof(Report_Audio_Status_cmd));
        break;

    case CECOP_GIVE_SYSTEM_AUDIO_MODE_STATUS:
        cec_transmit_frame(System_Audio_Mode_Status_cmd, sizeof(System_Audio_Mode_Status_cmd));
        break;
    case CECOP_TUNER_DEVICE_STATUS:
        cec_transmit_frame(Give_Tuner_Device_Status_cmd, sizeof(Give_Tuner_Device_Status_cmd) / sizeof(Give_Tuner_Device_Status_cmd[0]));
        break;
    case CECOP_GIVE_DEVICE_POWER_STATUS:
        cec_transmit_frame(Report_Power_Status_cmd, sizeof(Report_Power_Status_cmd));
        break;
    case CECOP_ARC_REQUEST_INITIATION:
        cec_transmit_frame(Initiate_Arc_cmd, 2);
        __this->state = ARC_STATUS_INIT;
        __this->idle_time = 3500 / TIME_PRD;
        break;

    default:
        printf("CEC command ");
        put_buf(__this->msg, __this->rx_count);
        break;
    }
}
static void cec_cmd_response()
{
    if (__this->rx_count > 1) {
        decode_cec_command();
    }

//    putchar('1');
    if (__this->idle_time < (150 / TIME_PRD)) {
        __this->idle_time = 150 / TIME_PRD;
    }

    if (cec_work_mode == CEC_IDLE) {
        cec_state = CEC_RECEIVE;
    }
    if (__this->msg_count != 0) {
        __this->msg_count = 0xff;
    }
    __this->rx_msg = 0;
}
static void cec_timer_loop()
{
    if (__this->rx_msg) {
        return;
    }

    if (cec_work_mode) {
        return;
    }

    if (__this->idle_time) {
        __this->idle_time--;
        return;
    }

    if (__this->msg_count && cec_frame.ack != CEC_SUCC) {
        __this->msg_count = 0;
        r_printf("send cec msg nak %d", cec_frame.ack);
    } else {
    }

    printf("[%d]", __this->state);
    switch (__this->state) {

    case ARC_STATUS_REQ_ADD:
        if (__this->msg_count == 0xff) {
            __this->state = ARC_STATUS_REPORT_ADD;
            __this->msg_count = 0;
            break;
        }
        __this->cec_ok = 0;
        cec_transmit_frame(PollingMessage_cmd, 1);
        __this->msg_count ++;

        if (__this->msg_count == 2) {
            __this->idle_time = 50 / TIME_PRD;
        }
        if (__this->msg_count > 3) {
            __this->state = ARC_STATUS_REPORT_ADD;
            __this->idle_time = 50 / TIME_PRD;
            __this->msg_count = 0;
        }
        break;

    case ARC_STATUS_REPORT_ADD:
        __this->logic_addr = 5;
        if (__this->msg_count == 0xff) {
            __this->state = ARC_STATUS_WAIT_ONLINE;
            __this->msg_count = 0;
            break;
        }
//		__this->cec_ok=1;
        cec_transmit_frame(Report_Physical_Add_Cmd1, 5);
        __this->msg_count ++;
        if (__this->msg_count > 1) {
            __this->state = ARC_STATUS_WAIT_ONLINE;
            __this->msg_count = 0;
        }
        break;

    case ARC_STATUS_WAIT_ONLINE:
        if (__this->msg_count == 0xff) {
            __this->state = ARC_STATUS_STB;
            __this->msg_count = 0;
            break;
        }
        cec_transmit_frame(Device_Vendor_Id_Cmd, 5);
        __this->msg_count ++;
        if (__this->msg_count > 1) {
            __this->state = ARC_STATUS_STB;
            __this->msg_count = 0;
        }
        break;

    case ARC_STATUS_STB:
        if (__this->msg_count == 0xff) {
            __this->state = ARC_STATUS_INIT;
            __this->msg_count = 0;
            break;
        }
        __this->cec_ok = 1;
//        cec_transmit_frame(Set_System_Audio_Mode_cmd, 3);
        cec_transmit_frame(Broadcast_System_Audio_Mode_cmd, 3);
        __this->msg_count ++;
        __this->idle_time = 300 / TIME_PRD;
        if (__this->msg_count > 1) {
            __this->state = ARC_STATUS_INIT;
        }
        break;

    case ARC_STATUS_INIT:
        if (__this->cec_ok) {
            break;
        }
        cec_transmit_frame(Initiate_Arc_cmd, 2);
        __this->idle_time = 3500 / TIME_PRD;
        break;
    }
}

void hdmi_cec_init(void)
{
    memset(__this, 0, sizeof(*__this));

    __this->logic_addr = 5;
    cec_state = CEC_RECEIVE;
    cec_work_mode = CEC_IDLE;
    __this->cec_ok = 0;

    //port
    gpio_set_die(TCFG_HDMI_CEC_PORT, 1);
    gpio_set_direction(TCFG_HDMI_CEC_PORT, 1);
    gpio_set_pull_up(TCFG_HDMI_CEC_PORT, 1);
    gpio_set_pull_down(TCFG_HDMI_CEC_PORT, 0);

    u32 clk = clk_get("timer");
    printf("clk %d %d", clk, CAP_CLK);

#if CEC_TMR_IDX  == IRQ_TIME2_IDX
    //capture input channel
    printf("TCFG_HDMI_CEC_PORT:[%d]", TCFG_HDMI_CEC_PORT);
    JL_IOMAP->CON1 |= BIT(30);
    JL_IOMAP->CON2 &= ~(0x7F << 16);
    JL_IOMAP->CON2 |= (TCFG_HDMI_CEC_PORT << 16);

    CEC_TMR->CON = BIT(14);
#else
#error "please config time input channle"
#endif
    change_capture_mode(CAP_FALLING_EDGE);
    request_irq(CEC_TMR_IDX, 2, cec_timer_isr, 0);
    __this->time_id = sys_timer_add(NULL, cec_timer_loop, TIME_PRD);
    g_printf("add timer %x", __this->time_id);
}

void hdmi_cec_close(void)
{
    CEC_TMR->CON = BIT(14);
    __this->cec_ok = 0;

    gpio_set_direction(TCFG_HDMI_CEC_PORT, 1);
    gpio_set_pull_up(TCFG_HDMI_CEC_PORT, 0);
    gpio_set_die(TCFG_HDMI_CEC_PORT, 0);

    g_printf("del timer %x", __this->time_id);
    if (__this->time_id) {
        sys_timer_del(__this->time_id);
        __this->time_id = 0;
    }

}

#endif

