#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "tone_player.h"
#include "app_main.h"
#include "ui_manage.h"
#include "vm.h"
#include "key_event_deal.h"
#include "asm/pwm_led.h"
#include "user_cfg.h"
#include "fm/fm.h"
#include "fm/fm_manage.h"
#include "fm/fm_rw.h"
#include "ui/ui_api.h"
#include "clock_cfg.h"
#include "includes.h"
#include "fm/fm_api.h"
#include "bt/bt_tws.h"
#include "ui/ui_style.h"
#include "audio_dec/audio_dec_fm.h"
#include "fm/fm_inside/fm_inside.h"

#define LOG_TAG_CONST       APP_FM
#define LOG_TAG             "[APP_FM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"



#if TCFG_APP_FM_EN

#define TCFG_FM_SC_REVERB_ENABLE 1//搜索过程关闭混响

/*************************************************************
   此文件函数主要是fm模式 实现函数



   任务内初始化：
   fm_api_init 主要申请空间、读取vm信息操作

   释放资源：
   fm_api_release

   注意：在逻辑操作上，sdk大部分是使用了虚拟频点,即(1,、2、3、4)代替了8750、8760、8770等真实的频率
   转换规则：真实 = REAL_FREQ(虚拟)
             虚拟 = VIRTUAL_FREQ(真实)
**************************************************************/


struct fm_opr {
    void *dev;
    u8 volume: 7;
    u8 fm_dev_mute : 1;
    u8 scan_flag;//搜索标志位,客户增加了自己了搜索标志位也要对应增加
    u16 fm_freq_cur;		// 这是虚拟频率,从1计算  real_freq = fm_freq_cur + 874
    u16 fm_freq_channel_cur;//当前的台号,从1计算
    u16 fm_total_channel;//总共台数
    s16 scan_fre;//搜索过程的虚拟频率,因为--会少于0，使用带符号
    u16 fm_freq_temp;		// 这是虚拟频率,从1计算  real_freq = fm_freq_cur + 874，用来做记录
};

#define  SCANE_ALL         (0x01)
#define  SEMI_SCANE_DOWN   (0x02)//半自动搜索标志位
#define  SEMI_SCANE_UP     (0x03)


static struct fm_opr *fm_hdl = NULL;
#define __this 	(fm_hdl)
extern void fm_dec_pause_out(u8 pause);
/*----------------------------------------------------------------------------*/
/**@brief    fm 的 mute 操作
   @param    1: mute  0:解mute
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void fm_app_mute(u8 mute)
{
    if (!__this) {
        return;
    }
    if (mute) {
        if (!__this->fm_dev_mute) {
            fm_manage_mute(1);
            __this->fm_dev_mute = 1;
        }
    } else {
        if (__this->fm_dev_mute) {
            fm_manage_mute(0);
            __this->fm_dev_mute = 0;
        }
    }
    /* fm_dec_pause_out(mute); */
}


/*----------------------------------------------------------------------------*/
/**@brief    fm 获取vm 保存信息
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/

static void fm_read_info_init(void)
{
    FM_INFO fm_info = {0};

    fm_vm_check();//校验mask,不符合mask进行清0
    fm_read_info(&fm_info);//获取vm信息

    __this->fm_freq_cur = fm_info.curFreq;//上一次的保存的频率(虚拟频率)
    printf("__this->fm_freq_cur = 0x%x\n", __this->fm_freq_cur);
    __this->fm_freq_channel_cur	= fm_info.curChanel;//上一次保存的频道
    printf("__this->fm_freq_channel_cur = 0x%x\n", __this->fm_freq_channel_cur);
    __this->fm_total_channel = fm_info.total_chanel;//总台数
    printf("__this->fm_total_channel = 0x%x\n", __this->fm_total_channel);

    if (__this->fm_freq_cur == 0 && __this->fm_freq_channel_cur && __this->fm_total_channel) {
        __this->fm_freq_cur = get_fre_via_channel(__this->fm_freq_channel_cur);//上次记录操作是频道，则获取频道对应的频率
        fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
    } else {
        fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));//上次记录操作的是频率
    }

}



/*----------------------------------------------------------------------------*/
/**@brief    fm ui 更新
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static  void __fm_ui_reflash_main()//刷新主页
{
    UI_REFLASH_WINDOW(true);
    UI_MSG_POST("fm_fre", NULL);
}

static  void __fm_ui_show_station()//显示台号
{
    UI_SHOW_MENU(MENU_FM_STATION, 1000, __this->fm_total_channel, NULL);
    /* ui_menu_reflash(true); */
    UI_MSG_POST("fm_fre", NULL);
}

static  void __fm_ui_cur_station()//显示当前台号
{
    UI_SHOW_MENU(MENU_FM_STATION, 1000, __this->fm_freq_channel_cur, NULL);
    /* ui_menu_reflash(true); */
    UI_MSG_POST("fm_fre", NULL);
}
/*----------------------------------------------------------------------------*/
/**@brief    fm 搜台完毕开混响
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void __fm_reverb_resume()
{
#if ((TCFG_MIC_EFFECT_ENABLE) &&(TCFG_FM_SC_REVERB_ENABLE))
    mic_effect_pause(0);
#endif
}
/*----------------------------------------------------------------------------*/
/**@brief    fm 搜台前关混响
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void __fm_reverb_pause()
{
#if ((TCFG_MIC_EFFECT_ENABLE) &&(TCFG_FM_SC_REVERB_ENABLE))
    mic_effect_pause(1);
#endif
}


/*----------------------------------------------------------------------------*/
/**@brief    fm 下一个台
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void __set_fm_station()
{
    fm_app_mute(1);
    __this->fm_freq_cur = get_fre_via_channel(__this->fm_freq_channel_cur);
    fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
    fm_last_ch_save(__this->fm_freq_channel_cur);
    fm_app_mute(0);
}

static void __set_fm_frq()
{
    fm_app_mute(1);
    fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
    fm_last_freq_save(REAL_FREQ(__this->fm_freq_cur));
    fm_app_mute(0);
}



static void __fm_scan_all(void)
{
    if ((!__this) || (__this->scan_flag != SCANE_ALL)) {
        return;
    }

    if (__this->scan_fre > VIRTUAL_FREQ(REAL_FREQ_MAX)) {
        __this->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MIN);
        //搜索完毕跑这里
        fm_app_mute(1);

        /////////////////////////////////////
        //这里设置搜索完毕的默认台号
        if (__this->fm_freq_channel_cur) { //搜索到台就自动跳转1第一台
            __this->fm_freq_cur = get_fre_via_channel(1);
            __this->fm_freq_channel_cur = 1;
        } else {
            __this->fm_freq_cur = 1;    //搜索不了就默认第一个
        }
        ///////////////////////////
        fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
        __this->scan_flag = 0;
        fm_app_mute(0);
        __fm_ui_reflash_main();
        __fm_reverb_resume();
        return;
    }

    fm_app_mute(1);

    if (fm_manage_set_fre(REAL_FREQ(__this->scan_fre))) {
        __this->fm_freq_cur  = __this->scan_fre;
        __this->fm_total_channel++;
        __this->fm_freq_channel_cur = __this->fm_total_channel;//++;
        save_fm_point(REAL_FREQ(__this->scan_fre));
        sys_timeout_add(NULL, __fm_scan_all, 1500); //播放一秒
        __fm_ui_show_station();
        fm_app_mute(0);
    } else {
        __this->fm_freq_cur  = __this->scan_fre;
        sys_timeout_add(NULL, __fm_scan_all, 20);

        __fm_ui_reflash_main();
    }
    __this->scan_fre++;

}


static void __fm_semi_scan(void)//半自动收台
{
    if ((!__this) || (__this->scan_flag != SEMI_SCANE_UP && __this->scan_flag != SEMI_SCANE_DOWN)) {
        return;
    }


    if (__this->scan_flag == SEMI_SCANE_DOWN) {
        __this->scan_fre++;
    } else {
        __this->scan_fre--;
    }

    if (__this->scan_fre > VIRTUAL_FREQ(REAL_FREQ_MAX)) {
        __this->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MIN);
#if TCFG_FM_INSIDE_ENABLE
        save_scan_freq_org(REAL_FREQ(__this->scan_fre) * 10);
#endif
    }

    if (__this->scan_fre <  VIRTUAL_FREQ(REAL_FREQ_MIN)) {
        __this->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MAX);
#if TCFG_FM_INSIDE_ENABLE
        save_scan_freq_org(REAL_FREQ(__this->scan_fre) * 10);
#endif
    }


    if (__this->scan_fre == __this->fm_freq_temp) {
        fm_app_mute(1);
        __this->fm_freq_cur = __this->fm_freq_temp;
        fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
        fm_app_mute(0);
        __this->scan_flag = 0;
        __fm_ui_reflash_main();
        __fm_reverb_resume();
        return;
    }

    fm_app_mute(1);

    if (fm_manage_set_fre(REAL_FREQ(__this->scan_fre))) {
        __this->fm_freq_cur  = __this->scan_fre;
        save_fm_point(REAL_FREQ(__this->scan_fre));//保存当前频点
        __this->fm_freq_channel_cur = get_channel_via_fre(REAL_FREQ(__this->scan_fre));//获取当前台号
        __this->fm_total_channel = get_total_mem_channel();//获取新的总台数
        fm_app_mute(0);
        __fm_ui_show_station();
        __fm_reverb_resume();
        __this->scan_flag = 0;
        return;
    } else {
        __fm_ui_reflash_main();
        __this->fm_freq_cur  = __this->scan_fre; //影响半自动搜台结束条件,先进行注释,app问题要重新解决
        sys_timeout_add(NULL, __fm_semi_scan, 20);
    }

}





void fm_delete_freq()
{
    if (!__this || __this->scan_flag) {
        return;
    }
    delete_fm_point(__this->fm_freq_cur);
}





void fm_scan_up()//半自动收台
{
    log_info("KEY_FM_SCAN_UP\n");
    if (!__this || __this->scan_flag) {
        return;
    }
    __this->scan_fre =  __this->fm_freq_cur;
    __this->scan_flag = SEMI_SCANE_UP;
    __this->fm_freq_temp = __this->fm_freq_cur;
    __fm_reverb_pause();
    fm_app_mute(1);

#if TCFG_FM_INSIDE_ENABLE
    fm_inside_trim(REAL_FREQ(__this->scan_fre) * 10);
#endif
    sys_timeout_add(NULL, __fm_semi_scan, 20);
}



void fm_scan_down()//半自动收台
{
    log_info("KEY_FM_SCAN_DOWN\n");
    if (!__this || __this->scan_flag) {
        return;
    }
    __this->scan_fre = __this->fm_freq_cur;
    __this->scan_flag = SEMI_SCANE_DOWN;
    __this->fm_freq_temp = __this->fm_freq_cur;

    __fm_reverb_pause();
    fm_app_mute(1);

#if TCFG_FM_INSIDE_ENABLE
    fm_inside_trim(REAL_FREQ(__this->scan_fre) * 10);
#endif

    sys_timeout_add(NULL, __fm_semi_scan, 20);

}

void fm_scan_stop(void)
{
    if (__this && __this->scan_flag) {
        __this->scan_flag = 0;
        os_time_dly(1);
        __set_fm_station();
        __fm_reverb_resume();
    }
}

void fm_scan_all()
{
    log_info("KEY_FM_SCAN_ALL\n");
    if (!__this) {
        return;
    }

    if (__this->scan_flag) {
        fm_scan_stop();
        return;
    }

    clear_all_fm_point();

    __this->fm_freq_cur  = 1;;
    __this->fm_total_channel = 0;
    __this->fm_freq_channel_cur = 0;

    __this->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MIN);
    __this->scan_flag = SCANE_ALL;

    __fm_reverb_pause();
    fm_app_mute(1);

#if TCFG_FM_INSIDE_ENABLE
    fm_inside_trim(REAL_FREQ(__this->scan_fre) * 10);
#endif
    sys_timeout_add(NULL, __fm_scan_all, 20);
}

void fm_volume_pp(void)
{
    log_info("KEY_MUSIC_PP\n");
    if (!__this || __this->scan_flag) {
        return ;
    }
    if (__this->fm_dev_mute == 0) {
        fm_app_mute(1);
    } else {
        fm_app_mute(0);
    }
}


void fm_prev_freq()
{
    log_info("KEY_FM_PREV_FREQ\n");
    if (!__this || __this->scan_flag) {
        return;
    }

    if (__this->fm_freq_cur <= VIRTUAL_FREQ(REAL_FREQ_MIN)) {
        __this->fm_freq_cur = VIRTUAL_FREQ(REAL_FREQ_MAX);
    } else {
        __this->fm_freq_cur -= 1;
    }
    __set_fm_frq();
    __fm_ui_reflash_main();
}

void fm_next_freq()
{
    log_info("KEY_FM_NEXT_FREQ\n");
    if (!__this || __this->scan_flag) {
        return;
    }
    if (__this->fm_freq_cur >= VIRTUAL_FREQ(REAL_FREQ_MAX)) {
        __this->fm_freq_cur = VIRTUAL_FREQ(REAL_FREQ_MIN);
    } else {
        __this->fm_freq_cur += 1;
    }
    __set_fm_frq();
    __fm_ui_reflash_main();

}

void fm_volume_up()
{
    u8 vol = 0;
    log_info("KEY_VOL_UP\n");
    if (!__this || __this->scan_flag) {
        return;
    }

    app_audio_volume_up(1);
    log_info("fm vol+: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));

#if (TCFG_DEC2TWS_ENABLE)
    bt_tws_sync_volume();
#endif

    vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
    UI_SHOW_MENU(MENU_MAIN_VOL, 1000, vol, NULL);

}

void fm_volume_down()
{
    u8 vol = 0;
    log_info("KEY_VOL_DOWN\n");
    if (!__this || __this->scan_flag) {
        return;
    }
    app_audio_volume_down(1);
    log_info("fm vol-: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));

#if (TCFG_DEC2TWS_ENABLE)
    bt_tws_sync_volume();
#endif

    vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
    UI_SHOW_MENU(MENU_MAIN_VOL, 1000, vol, NULL);

}


void fm_prev_station()
{
    log_info("KEY_FM_PREV_STATION\n");

    if (!__this || __this->scan_flag || (!__this->fm_total_channel)) {
        return;
    }

    if (__this->fm_freq_channel_cur <= 1) {
        __this->fm_freq_channel_cur = __this->fm_total_channel;
    } else {
        __this->fm_freq_channel_cur -= 1;
    }
    __set_fm_station();
    __fm_ui_cur_station();
}


void fm_next_station()
{
    log_info("KEY_FM_NEXT_STATION\n");
    if (!__this || __this->scan_flag || (!__this->fm_total_channel)) {
        return;
    }


    if (__this->fm_freq_channel_cur >= __this->fm_total_channel) {
        __this->fm_freq_channel_cur = 1;
    } else {
        __this->fm_freq_channel_cur += 1;
    }

    __set_fm_station();
    __fm_ui_cur_station();
}

/*----------------------------------------------------------------------------*/
/**@brief    fm 入口初始化
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void fm_api_init()
{
    fm_hdl = (struct fm_opr *)malloc(sizeof(struct fm_opr));
    memset(fm_hdl, 0x00, sizeof(struct fm_opr));
    if (fm_hdl == NULL) {
        puts("fm_state_machine fm_hdl malloc err !\n");
    }
    __this->fm_dev_mute = 0;
    fm_app_mute(1);
    fm_read_info_init();
    os_time_dly(1);
    fm_app_mute(0);
    UI_SHOW_WINDOW(ID_WINDOW_FM);
}


/*----------------------------------------------------------------------------*/
/**@brief    fm 任务资源释放
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/


void fm_api_release()
{
    int err = 0;
    if (!__this) {
        return;
    }

    if (__this->scan_flag) {
        __this->scan_flag = 0;
        __fm_reverb_resume();
    }

    if (__this != NULL) {
        free(__this);
        __this = NULL;
    }
    UI_HIDE_CURR_WINDOW();
}

u8 fm_get_scan_flag(void)
{
    return __this->scan_flag;
}

u8 fm_get_fm_dev_mute(void)
{
    return __this->fm_dev_mute;
}

u8 fm_get_cur_channel(void)
{
    if (!__this) {
        return 0;
    }

    return (u8)__this->fm_freq_channel_cur;
}

u16 fm_get_cur_fre(void)
{
    if (!__this) {
        return 0;
    }
    if (__this->fm_freq_cur > 1080) {
        __this->fm_freq_cur /= 10;
    }
    return	(__this->fm_freq_cur % 874) + 874;
}

u8 fm_get_mode(void)
{
    u32 freq_min = REAL_FREQ_MIN / VIRTUAL_FREQ_STEP;
    if (freq_min < 875) {
        return 1;
    } else {
        return 0;
    }
}

void fm_sel_station(u8 channel)
{
    if (!__this) {
        return;
    }

    if (channel > __this->fm_total_channel)	{
        printf("channel sel err!\n");
        return;
    }
    __this->fm_freq_channel_cur = channel;
    __set_fm_station();
}

u8 fm_set_fre(u16 fre)
{
    if (!__this) {
        return -1;
    }

    if ((fre < REAL_FREQ_MIN) || (fre > REAL_FREQ_MAX)) {
        return -1;
    }
    __this->fm_freq_cur = VIRTUAL_FREQ(fre);
    __set_fm_frq();
    return 0;
}

u8 get_fm_scan_status(void)
{
    return __this->scan_flag;
}

//FM发射模式下扫描强台，避开强台发射
//在最低优先级的线程运行、关闭解调、关闭FM解码
void txmode_fm_inside_freq_scan(void)
{
    u16 scan_cnt = 0;
    fm_vm_check();
    fm_inside_init(NULL);//初始化FM模块
    save_scan_freq_org(REAL_FREQ_MIN * 10); //搜台的起始频点
    for (scan_cnt = 1; scan_cnt <= MAX_CHANNEL; scan_cnt++) {
        wdt_clear();//搜台时间较长，每次切频点要清看门狗
        if (fm_inside_set_fre(NULL, REAL_FREQ(scan_cnt))) {  //真 判为有台
            save_fm_point(REAL_FREQ(scan_cnt));//保存频点到VM

            //REAL_FREQ(scan_cnt) * 10 ---->87.5M对应87500
            printf("get_freq = %d\n", REAL_FREQ(scan_cnt) * 10);
        }
    }
    fm_inside_powerdown(NULL);//关闭FM模块
}

//FM发射模式下获取扫描到的强台
//REAL_FREQ(get_fre_via_channel(freq_channel)) * 10 ---->87.5M对应87500
void txmode_fm_inside_freq_get(void)
{
    u16 freq_channel = 0;
    FM_INFO info;
    fm_vm_check();
    fm_read_info(&info);//读取扫完台的信息
    for (freq_channel = 1; freq_channel < info.total_chanel; freq_channel++) {
        printf("freq_channel_%d : %dKHz\n", freq_channel,
               REAL_FREQ(get_fre_via_channel(freq_channel)) * 10);
    }
}
#endif
