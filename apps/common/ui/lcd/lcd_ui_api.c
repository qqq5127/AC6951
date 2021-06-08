#include "app_config.h"
#include "includes.h"
#include "ui/ui_api.h"
#include "ui/ui.h"
#include "typedef.h"

#if (TCFG_UI_ENABLE &&TCFG_SPI_LCD_ENABLE && (!TCFG_SIMPLE_LCD_ENABLE))

#include "ui/ui_sys_param.h"
#include "ui/res_config.h"

#define CURR_WINDOW_MAIN (0)

#define UI_NO_ARG (-1)
#define UI_TASK_NAME "ui"

enum {
    UI_MSG_OTHER,
    UI_MSG_KEY,
    UI_MSG_TOUCH,
    UI_MSG_SHOW,
    UI_MSG_HIDE,
    UI_TIME_TOUCH_RESUME,
    UI_TIME_TOUCH_SUPEND,
    UI_MSG_EXIT,
};


struct ui_server_env {
    u8 init: 1;
    u8 key_lock : 1;
    OS_SEM start_sem;
    int (*touch_event_call)(void);
    int touch_event_interval;
    u16 timer;
};

int lcd_backlight_status();
int lcd_backlight_ctrl(u8 on);
void ui_backlight_open(void);
void ui_backlight_close(void);
int lcd_sleep_ctrl(u8 enter);

static struct ui_server_env __ui_display = {0};

int key_is_ui_takeover()
{
    return __ui_display.key_lock;
}

void key_ui_takeover(u8 on)
{
    __ui_display.key_lock = !!on;
}

static int post_ui_msg(int *msg, u8 len)
{
    int err = 0;
    int count = 0;
    if (!__ui_display.init) {
        return -1;
    }
__retry:
    err =  os_taskq_post_type(UI_TASK_NAME, msg[0], len - 1, &msg[1]);

    if (cpu_in_irq() || cpu_irq_disabled()) {
        return err;
    }

    if (err) {
        if (!strcmp(os_current_task(), UI_TASK_NAME)) {
            return err;
        }
        if (count > 20) {
            return -1;
        }
        count++;
        os_time_dly(1);
        goto __retry;
    }
    return err;
}


//=================================================================================//
// @brief: 显示主页 应用于非ui线程显示主页使用
//有针对ui线程进行处理，允许用于ui线程等同ui_show使用
//=================================================================================//
int ui_show_main(int id)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    printf("__func__ %s %x\n", __func__, rets);

    static u32 mem_id[8] = {0};
    static OS_SEM sem;// = zalloc(sizeof(OS_SEM));
    int msg[8];
    int i;


    if (id <= 0) {
        if (mem_id[7 + id]) {
            id = mem_id[7 + id];
        } else {
            printf("[ui_show_main] id %d is invalid.\n", id);
            return 0;
        }
    }
    //else {
    if (mem_id[7] != id) {
        for (i = 1; i <= 7; i++) {
            mem_id[i - 1] = mem_id[i];
        }
        mem_id[7] = id;
    }
    //}

    printf("[ui_show_main] id = 0x%x\n", id);

    /* if (!strcmp(os_current_task(), UI_TASK_NAME)) { */
    /*     ui_show(id); */
    /*     return 0; */
    /* } */
    u8 need_pend = 0;
    if (!(cpu_in_irq() || cpu_irq_disabled())) {
        if (strcmp(os_current_task(), UI_TASK_NAME)) {
            need_pend = 1;
        }
    }
    msg[0] = UI_MSG_SHOW;
    msg[1] = id;
    msg[2] = 0;

    if (need_pend) {
        os_sem_create(&sem, 0);
        msg[2] = (int)&sem;
    }

    if (!post_ui_msg(msg, 3)) {
        if (need_pend) {
            os_sem_pend(&sem, 0);
        }
    }


    if (!lcd_backlight_status()) {
        printf("backlight is close ... ui_show_main_now\n");
        ui_auto_shut_down_enable();
        ui_touch_timer_start();
    }


    return 0;
}


//=================================================================================//
// @brief: 关闭主页 应用于非ui线程关闭使用
//有针对ui线程进行处理，允许用于ui线程等同ui_hide使用
//=================================================================================//
int ui_hide_main(int id)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    printf("__func__ %s %x\n", __func__, rets);


    static OS_SEM sem;// = zalloc(sizeof(OS_SEM));
    int msg[8];

    u8 need_pend = 0;;
    if (!(cpu_in_irq() || cpu_irq_disabled())) {
        if (strcmp(os_current_task(), UI_TASK_NAME)) {
            need_pend = 1;
        }
    }
    /* if (!strcmp(os_current_task(), UI_TASK_NAME)) { */
    /*     if (CURR_WINDOW_MAIN == id) { */
    /*         id = ui_get_current_window_id(); */
    /*     } */
    /*     ui_hide(id); */
    /*     return 0; */
    /* } */
    msg[0] = UI_MSG_HIDE;
    msg[1] = id;
    msg[2] = 0;

    if (need_pend) {
        os_sem_create(&sem, 0);
        msg[2] = (int)&sem;
    }

    if (!post_ui_msg(msg, 3)) {
        if (need_pend) {
            os_sem_pend(&sem, 0);
        }
    }
    return 0;
}

//=================================================================================//
// @brief: 关闭当前主页 灵活使用自动判断当前主页进行关闭
//用户可以不用关心当前打开的主页,特别适用于一个任务使用了多个主页的场景
//=================================================================================//
int ui_hide_curr_main()
{
    return ui_hide_main(CURR_WINDOW_MAIN);
}



//=================================================================================//
// @brief: 应用往ui发送消息，ui主页需要注册回调函数关闭当前主页
// //消息发送方法demo： UI_MSG_POST("test1:a=%4,test2:bcd=%4,test3:efgh=%4,test4:hijkl=%4", 1,2,3,4);
// 往test1、test2、test3、test4发送了字符为a、bcd、efgh、hijkl，附带变量为1、2、3、4
// 每次可以只发一个消息，也可以不带数据例如:UI_MSG_POST("test1")
//=================================================================================//
int ui_server_msg_post(const char *msg, ...)
{
    int ret = 0;
    int argv[9];
    argv[0] = UI_MSG_OTHER;
    argv[1] = (int)msg;
    va_list argptr;
    va_start(argptr, msg);
    for (int i = 2; i < 7; i++) {
        argv[i] = va_arg(argptr, int);
    }
    va_end(argptr);
    return post_ui_msg(argv, 9);
}

//=================================================================================//
// @brief: 应用往ui发送key消息，由ui控件分配
//=================================================================================//
int ui_key_msg_post(int key)
{
    u8 count = 0;
    int msg[8];

    if (key >= 0x80) {
        return -1;
    }

    msg[0] = UI_MSG_KEY;
    msg[1] = key;
    return post_ui_msg(msg, 2);
}


//=================================================================================//
// @brief: 应用往ui发送触摸消息，由ui控件分配
//=================================================================================//
int ui_touch_msg_post(struct touch_event *event)
{
    int msg[8];
    int i = 0;
    ui_auto_shut_down_modify();
    if (!lcd_backlight_status()) {
        ui_backlight_open();
        return 0;
    }
    msg[0] = UI_MSG_TOUCH;
    msg[1] = (int)NULL;
    memcpy(&msg[2], event, sizeof(struct touch_event));
    return post_ui_msg(msg, sizeof(struct touch_event) / 4 + 2);
}

//=================================================================================//
// @brief: 应用往ui发送触摸消息，由ui控件分配
//=================================================================================//
int ui_touch_msg_post_withcallback(struct touch_event *event, void (*cb)(u8 finish))
{
    int msg[8];
    int i = 0;
    msg[0] = UI_MSG_TOUCH;
    msg[1] = (int)cb;
    memcpy(&msg[2], event, sizeof(struct touch_event));
    return post_ui_msg(msg, sizeof(struct touch_event) / 4 + 2);
}



const char *str_substr_iter(const char *str, char delim, int *iter)
{
    const char *substr;
    ASSERT(str != NULL);
    substr = str + *iter;
    if (*substr == '\0') {
        return NULL;
    }
    for (str = substr; *str != '\0'; str++) {
        (*iter)++;
        if (*str == delim) {
            break;
        }
    }
    return substr;
}


static int do_msg_handler(const char *msg, va_list *pargptr, int (*handler)(const char *, u32))
{
    int ret = 0;
    int width;
    int step = 0;
    u32 arg = 0x5678;
    int m[16];
    char *t = (char *)&m[3];
    va_list argptr = *pargptr;

    if (*msg == '\0') {
        handler((const char *)' ', 0);
        return 0;
    }

    while (*msg && *msg != ',') {
        switch (step) {
        case 0:
            if (*msg == ':') {
                step = 1;
            }
            break;
        case 1:
            switch (*msg) {
            case '%':
                msg++;
                if (*msg >= '0' && *msg <= '9') {
                    if (*msg == '1') {
                        arg = va_arg(argptr, int) & 0xff;
                    } else if (*msg == '2') {
                        arg = va_arg(argptr, int) & 0xffff;
                    } else if (*msg == '4') {
                        arg = va_arg(argptr, int);
                    }
                } else if (*msg == 'p') {
                    arg = va_arg(argptr, int);
                }
                m[2] = arg;
                handler((char *)&m[3], m[2]);
                t = (char *)&m[3];
                step = 0;
                break;
            case '=':
                *t = '\0';
                break;
            case ' ':
                break;
            default:
                *t++ = *msg;
                break;
            }
            break;
        }
        msg++;
    }
    *pargptr = argptr;
    return ret;
}


int ui_message_handler(int id, const char *msg, va_list argptr)
{
    int iter = 0;
    const char *str;
    const struct uimsg_handl *handler;
    struct window *window = (struct window *)ui_core_get_element_by_id(id);
    if (!window || !window->private_data) {
        return -EINVAL;
    }
    handler = (const struct uimsg_handl *)window->private_data;
    while ((str = str_substr_iter(msg, ',', &iter)) != NULL) {
        for (; handler->msg != NULL; handler++) {
            if (!memcmp(str, handler->msg, strlen(handler->msg))) {
                do_msg_handler(str + strlen(handler->msg), &argptr, handler->handler);
                break;
            }
        }
    }

    return 0;
}




extern void sys_param_init(void);

void ui_touch_timer_delete()
{
    if (!__ui_display.init) {
        return;
    }

#if UI_WATCH_RES_ENABLE
    local_irq_disable();
    if (!__ui_display.timer) {
        local_irq_enable();
        return;
    }
    local_irq_enable();
#endif
    int msg[8];
    msg[0] = UI_TIME_TOUCH_SUPEND;
    post_ui_msg(msg, 1);

}


void ui_touch_timer_start()
{
    if (!__ui_display.init) {
        return;
    }

    local_irq_disable();
    if (__ui_display.timer) {
        local_irq_enable();
        return;
    }
    local_irq_enable();
    int msg[8];
    msg[0] = UI_TIME_TOUCH_RESUME;
    post_ui_msg(msg, 1);
}

void ui_set_touch_event(int (*touch_event)(void), int interval)
{
    __ui_display.touch_event_call = touch_event;
    __ui_display.touch_event_interval = interval;
}


void ui_auto_shut_down_enable(void);
void ui_auto_shut_down_disable(void);
static int ui_auto_shut_down_timer = 0;
void ui_backlight_open(void)
{

    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));

    if (!__ui_display.init) {
        return;
    }

    printf("__func__ %s %x\n", __func__, rets);
    if (!lcd_backlight_status()) {


        /* UI_MSG_POST("ui_lp_cb:exit=%4", 1); */
        /* sys_key_event_enable();//关闭按键 */
        ui_auto_shut_down_enable();
        //lcd_sleep_ctrl(0);//屏幕退出低功耗
#if UI_WATCH_RES_ENABLE
        ui_show_main(0);//恢复当前ui
#endif
        ui_touch_timer_start();
    }
}

void ui_backlight_close(void)
{

    if (!__ui_display.init) {
        return;
    }

    if (lcd_backlight_status()) {
        /* UI_MSG_POST("ui_lp_cb:enter=%4", 0); */
        /* sys_key_event_disable();//关闭按键 */
#if UI_WATCH_RES_ENABLE
        ui_hide_curr_main();//关闭当前页面
#endif
        ui_auto_shut_down_disable();
        ui_touch_timer_delete();
    }
}


void ui_auto_shut_down_enable(void)
{
#if TCFG_UI_SHUT_DOWN_TIME
    if (!__ui_display.init) {
        return;
    }
    if (ui_auto_shut_down_timer == 0) {
        ui_auto_shut_down_timer = sys_timeout_add(NULL, ui_backlight_close, TCFG_UI_SHUT_DOWN_TIME * 1000);
    }
#endif
}

void ui_auto_shut_down_disable(void)
{
#if TCFG_UI_SHUT_DOWN_TIME
    if (ui_auto_shut_down_timer) {
        sys_timeout_del(ui_auto_shut_down_timer);
        ui_auto_shut_down_timer = 0;
    }
#endif
}

void ui_auto_shut_down_modify(void)
{
#if TCFG_UI_SHUT_DOWN_TIME
    if (ui_auto_shut_down_timer) {
        sys_timer_modify(ui_auto_shut_down_timer, TCFG_UI_SHUT_DOWN_TIME * 1000);
    }
#endif
}

static void ui_task(void *p)
{
    int msg[32];
    int ret;
    struct element_key_event e = {0};

    sys_param_init();
    /* struct ui_style style; */
    /* style.file = NULL; */

    ui_framework_init(p);
    /* ret =  ui_set_style_file(&style); */
    /* if (ret) { */
    /* printf("ui task fail!\n"); */
    /* return; */
    /* } */
    __ui_display.init = 1;
    os_sem_post(&(__ui_display.start_sem));
    struct touch_event *touch;
    struct element_touch_event t;

    if (__ui_display.touch_event_call && __ui_display.touch_event_interval) {
        __ui_display.timer = sys_timer_add((void *)NULL, __ui_display.touch_event_call, __ui_display.touch_event_interval); //注册按键扫描定时器
    }

    while (1) {
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)); //500ms_reflash
        if (ret != OS_TASKQ) {
            continue;
        }

        switch (msg[0]) { //action
        case UI_MSG_EXIT:
            os_sem_post((OS_SEM *)msg[1]);
            os_time_dly(10000);
            break;
        case UI_MSG_OTHER:
            ui_message_handler(ui_get_current_window_id(), (const char *)msg[1], (void *)&msg[2]);
            break;
        case UI_MSG_KEY:
            e.value = msg[1];
            ui_event_onkey(&e);
            break;
        case UI_MSG_TOUCH:
            touch = (struct touch_event *)&msg[2];
            t.event = touch->event;
            t.pos.x = touch->x;
            t.pos.y = touch->y;
            /* printf("event = %d %d %d \n", t.event, t.pos.x, t.pos.y); */
            if (msg[1]) {
                ((void(*)(u8))msg[1])(0);
            }
            ui_event_ontouch(&t);
            if (msg[1]) {
                ((void(*)(u8))msg[1])(1);
            }

            break;
        case UI_MSG_SHOW:
            if (!lcd_backlight_status()) {
                lcd_sleep_ctrl(0);//退出低功耗
            }

            if (msg[1] >> 24 == CTRL_TYPE_WINDOW && ui_get_current_window_id() > 0) {
                printf("ui = %x show repeat.....\n", ui_get_current_window_id());
                ui_hide(ui_get_current_window_id());
            }

            ui_show(msg[1]);
            if (msg[2]) {
                os_sem_post((OS_SEM *)msg[2]);
            }
            break;
        case UI_MSG_HIDE:
            if (CURR_WINDOW_MAIN == msg[1]) {
                ui_hide(ui_get_current_window_id());
            } else {
                ui_hide(msg[1]);
            }

            if (msg[2]) {
                os_sem_post((OS_SEM *)msg[2]);
            }
            break;
        case UI_TIME_TOUCH_RESUME:
            lcd_backlight_ctrl(true);
            if (__ui_display.timer) {
                break;
            }
            if (__ui_display.touch_event_call && __ui_display.touch_event_interval) {
                __ui_display.timer = sys_timer_add((void *)NULL, __ui_display.touch_event_call, __ui_display.touch_event_interval); //注册按键扫描定时器
            }
            break;
        case UI_TIME_TOUCH_SUPEND:
            if (__ui_display.timer) {
                sys_timer_del(__ui_display.timer);
                __ui_display.timer = 0;
            }
            lcd_backlight_ctrl(false);
            break;
        default:
            break;
        }
    }
}


extern int ui_file_check_valid();
extern int ui_upgrade_file_check_valid();

int lcd_ui_init(void *arg)
{
    int err = 0;

    printf("open_resouece_start...\n");
    if (ui_file_check_valid()) {
        printf("ui_file_check_valid fail!!!\n");
        if (ui_upgrade_file_check_valid()) {
            printf("ui_upgrade_file_check_valid fail!!!\n");
            return -1;
        }
    }

    os_sem_create(&(__ui_display.start_sem), 0);
    err = task_create(ui_task, arg, UI_TASK_NAME);
    os_sem_pend(&(__ui_display.start_sem), 0);
    return err;
}

#else

int key_is_ui_takeover()
{
    return 0;
}

void key_ui_takeover(u8 on)
{

}

int ui_key_msg_post(int key)
{
    return 0;
}


int ui_touch_msg_post(struct touch_event *event)
{
    return 0;
}

int ui_touch_msg_post_withcallback(struct touch_event *event, void (*cb)(u8 finish))
{
    return 0;
}

void ui_touch_timer_delete()
{

}

void ui_touch_timer_start()
{

}

void ui_backlight_ctrl(u8 lcd_open_flag)
{

}
#endif

