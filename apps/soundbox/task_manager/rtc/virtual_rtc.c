#include "app_config.h"
#include "rtc/alarm.h"
#include "app_action.h"
#include "system/includes.h"
#include "clock_cfg.h"
#include "ioctl_cmds.h"
#include "system/sys_time.h"

#if (TCFG_APP_RTC_EN && TCFG_USE_VIRTUAL_RTC)


/*************************************************************
   此文件函数主要是虚拟rtc设备的实现
  应用于不外挂晶振的场景,用户可以忽略实现,
  最后的应用的接口与挂晶振的接口一致
**************************************************************/


#define WRITE_ALARM     BIT(1)
#define READ_ALARM      BIT(5)

#define WRITE_RTC       BIT(0)
#define READ_RTC        BIT(4)

#define MAX_YEAR          2099
#define MIN_YEAR          2000


typedef struct _RTC_SIMULATE_VAR {
    struct device device;
    struct sys_time sys_simulate_time;
    struct sys_time sys_alarm_time;
    /* OS_MUTEX mutex; */
} RTC_SIMULATE_VAR;

#define __this	(&rtc_smulate_var)
static RTC_SIMULATE_VAR rtc_smulate_var = {0};

static u8 g_alarm_flag = 0;


__attribute__((weak))void  alm_wakeup_isr(void);
__attribute__((weak)) void set_rtc_default_time(struct sys_time *t);

static u8 is_the_same_time(struct sys_time *time1, struct sys_time *time2)
{
    return (time1->year == time2->year &&
            time1->month == time2->month &&
            time1->day == time2->day &&
            time1->hour == time2->hour &&
            time1->min == time2->min &&
            time1->sec == time2->sec);
}


static int rtc_simulate_init(const struct dev_node *node, void *arg)
{
    /* os_mutex_create(&__this->mutex); */

    if (set_rtc_default_time) {
        set_rtc_default_time(&__this->sys_simulate_time);
    } else {
        __this->sys_simulate_time.year = 2020;
        __this->sys_simulate_time.month = 1;
        __this->sys_simulate_time.day = 1;
        __this->sys_simulate_time.hour = 0;
        __this->sys_simulate_time.min = 0;
        __this->sys_simulate_time.sec = 0;
    }
    printf("rtc_simulate_init: %d-%d-%d %d:%d:%d\n",
           __this->sys_simulate_time.year,
           __this->sys_simulate_time.month,
           __this->sys_simulate_time.day,
           __this->sys_simulate_time.hour,
           __this->sys_simulate_time.min,
           __this->sys_simulate_time.sec);
    return 0;
}

static int rtc_simulate_open(const char *name, struct device **device, void *arg)
{
    *device = &__this->device;

    return 0;
}

static void write_alarm(struct sys_time *alarm_time)
{
    local_irq_disable();
    memcpy(&__this->sys_alarm_time, alarm_time, sizeof(struct sys_time));
    local_irq_enable();
}

static void write_sys_time(struct sys_time *curr_time)
{
    local_irq_disable();
    memcpy(&__this->sys_simulate_time, curr_time, sizeof(struct sys_time));
    local_irq_enable();
}

static void read_sys_time(struct sys_time *curr_time)
{
    local_irq_disable();
    memcpy(curr_time, &__this->sys_simulate_time, sizeof(struct sys_time));
    local_irq_enable();
}

static void set_alarm_ctrl(u8 set_alarm)
{
    g_alarm_flag = set_alarm;
}

static int rtc_simulate_ioctl(struct device *device, u32 cmd, u32 arg)
{
    int err = 0;
    switch (cmd) {
    case IOCTL_SET_ALARM:
        write_alarm((struct sys_time *)arg);
        break;
    case IOCTL_SET_ALARM_ENABLE:
        if (arg) {
            set_alarm_ctrl(1);
        } else {
            set_alarm_ctrl(0);
        }
        break;
    case IOCTL_GET_SYS_TIME:
        read_sys_time((struct sys_time *)arg);
        break;
    case IOCTL_SET_SYS_TIME:
        write_sys_time((struct sys_time *)arg);
        break;
    }
    return err;
}

void set_virtual_rtc_tick(u64 actual_ns)
{
#if (defined(TCFG_USE_VIRTUAL_RTC) && (TCFG_USE_VIRTUAL_RTC))
    struct sys_time temp_time;
    static u64 sum_nsec = 0;
    static u8 cal_flag = 0;

    sum_nsec += actual_ns;
    if (sum_nsec >= 1000000000) {
        sum_nsec -= 1000000000;
        cal_flag = 1;
    }

    if (cal_flag) {
        cal_flag = 0;
        read_sys_time(&temp_time);
        if (++temp_time.sec >= 60) {
            temp_time.sec = 0;
            if (++temp_time.min >= 60) {
                temp_time.min = 0;
                if (++temp_time.hour >= 24) {
                    temp_time.hour = 0;
                    if (++temp_time.day > month_for_day(temp_time.month, temp_time.year)) {
                        temp_time.day = 1;
                        if (++temp_time.month > 12) {
                            temp_time.month = 1;
                            if (++temp_time.year > MAX_YEAR) {
                                temp_time.year = MIN_YEAR;
                            }
                        }
                    }
                }
            }
        }
        write_sys_time(&temp_time);
        /*
                printf("rtc_sys_time: %d-%d-%d %d:%d:%d\n",
                       temp_time.year,
                       temp_time.month,
                       temp_time.day,
                       temp_time.hour,
                       temp_time.min,
                       temp_time.sec);
        */

        local_irq_disable();
        if (g_alarm_flag && is_the_same_time(&__this->sys_alarm_time, &__this->sys_simulate_time)) {
            local_irq_enable();
            if (alm_wakeup_isr) {
                alm_wakeup_isr();
            }
        } else {
            local_irq_enable();
        }

    }
#endif
}


const struct device_operations rtc_simulate_ops = {
    .init   = rtc_simulate_init,
    .open   = rtc_simulate_open,
    .ioctl  = rtc_simulate_ioctl,
    .read   = NULL,
    .write  = NULL,
};

#endif
