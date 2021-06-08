#include "generic/gpio.h"
#include "asm/includes.h"
#include "system/timer.h"
#include "board_config.h"
#include "record/record.h"
#include "dev_manager.h"
#include "media/audio_base.h"
#include "audio_enc.h"
#include "tone_player.h"
#include "audio_dec.h"
#include "clock_cfg.h"
#ifdef CONFIG_BOARD_AC696X_DEMO
#include "effect_reg.h"

extern void *file_eq_drc_open(u16 sample_rate, u8 ch_num);
extern void file_eq_drc_close(struct audio_eq_drc *eq_drc);

/*----------------------------------------------------------------------------*/
/**@brief   叠加播放需要添加的函数
   @param
   @return
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
#define OVERLAY_PLAY     0  //叠加播放
#define INTERRUPT_PLAY   1  //打断之前所有提示音再进行播放

struct tone_dec_handle *tone_dec_lev1 = NULL;
struct tone_dec_handle *tone_dec_lev2 = NULL;

struct audio_eq_drc     *record_eq_drc_lev1 = NULL;
struct audio_eq_drc     *record_eq_drc_lev2 = NULL;
ECHO_API_STRUCT 		*record_play_echo_hdl_lev1 = NULL;
ECHO_API_STRUCT 		*record_play_echo_hdl_lev2 = NULL;
s_pitch_hdl 			*record_play_pitch_hdl_lev1 = NULL;
s_pitch_hdl 			*record_play_pitch_hdl_lev2 = NULL;

/****************************************************************************************************************
效果配置说明：
1.EFFECT_PITCH_SHIFT  移频，变调不变速，init_parm.shiftv调节有效，init_parm.formant_shift调节无效
2.EFFECT_VOICECHANGE_KIN0  变声，可以调节不同的 init_parm.shiftv  跟 init_parm.formant_shift ，调出更多的声音效果
3.EFFECT_VOICECHANGE_KIN1  变声，同EFFECT_VOICECHANGE_KIN0类似的，但是2者由于运算的不同，会有区别。
4.EFFECT_ROBORT 机器音效果，类似 喜马拉雅那样的
5.EFFECT_AUTOTUNE  电音效果
****************************************************************************************************************/
const PITCH_SHIFT_PARM record_play_pitch_parm = {
    .sr = 16000L,               //input  audio samplerate
    .shiftv = 100,              //pitch rate:  <8192(pitch up), >8192(pitch down)
    .effect_v = EFFECT_AUTOTUNE,
    .formant_shift = 100,
};

const ECHO_PARM_SET record_echo_parm = {
    .delay = 200,				//回声的延时时间 0-300ms
    .decayval = 50,				// 0-70%
    .direct_sound_enable = 1,	//直达声使能  0/1
    .filt_enable = 1,			//发散滤波器使能
};

static void record_play_stream_resume(void *p)
{
    struct audio_dec_app_hdl *app_dec = p;
    audio_decoder_resume(&app_dec->decoder);
}

void record_play_stream_handler_lev1(void *priv, int event, struct audio_dec_app_hdl *app_dec)
{
    switch (event) {
    case AUDIO_DEC_APP_EVENT_START_INIT_OK: {
        u8 entry_cnt = 0;
        struct audio_stream_entry *entries[8] = {NULL};
        entries[entry_cnt++] = &app_dec->decoder.entry;
        {
            clock_add_set(REVERB_CLK);
            //打开pitch和添加处理入口
            //record_play_pitch_hdl_lev1 = open_pitch((PITCH_SHIFT_PARM *)&record_play_pitch_parm);
            //entries[entry_cnt++] = &record_play_pitch_hdl_lev1->entry;

            //打开echo和添加处理入口
            //record_play_echo_hdl_lev1 = open_echo((ECHO_PARM_SET *)&record_echo_parm, 16000);
            //entries[entry_cnt++] = &record_play_echo_hdl_lev1->entry;

            //打开EQ和添加处理入口
            //record_eq_drc_lev1 = file_eq_drc_open(16000, 1);
            //entries[entry_cnt++] = &record_eq_drc_lev1->entry;
        }
        entries[entry_cnt++] = &app_dec->mix_ch.entry;
        app_dec->stream = audio_stream_open(app_dec, record_play_stream_resume);
        audio_stream_add_list(app_dec->stream, entries, entry_cnt);
        break;
    }
    case AUDIO_DEC_APP_EVENT_DEC_CLOSE:
        //如果打开了pitch则删除pitch处理入口和关闭pitch
        if (record_play_pitch_hdl_lev1) {
            audio_stream_del_entry(&record_play_pitch_hdl_lev1->entry);
            close_pitch(record_play_pitch_hdl_lev1);
            record_play_pitch_hdl_lev1 = NULL;
        }
        //如果打开了echo则删除echo处理入口和关闭echo
        if (record_play_echo_hdl_lev1) {
            audio_stream_del_entry(&record_play_echo_hdl_lev1->entry);
            close_echo(record_play_echo_hdl_lev1);
            record_play_echo_hdl_lev1 = NULL;
        }
        //如果打开了EQ则删除EQ处理入口和关闭EQ
        if (record_eq_drc_lev1) {
            file_eq_drc_close(record_eq_drc_lev1);
            record_eq_drc_lev1 = NULL;
        }
        break;
    }
}

void record_play_stream_handler_lev2(void *priv, int event, struct audio_dec_app_hdl *app_dec)
{
    switch (event) {
    case AUDIO_DEC_APP_EVENT_START_INIT_OK:
        printf("AUDIO_DEC_APP_EVENT_START_INIT_OK\n");
        struct audio_stream_entry *entries[8] = {NULL};
        u8 entry_cnt = 0;
        entries[entry_cnt++] = &app_dec->decoder.entry;
        {
            clock_add_set(REVERB_CLK);
            //打开pitch和添加处理入口
            //record_play_pitch_hdl_lev2 = open_pitch((PITCH_SHIFT_PARM *)&record_play_pitch_parm);
            //entries[entry_cnt++] = &record_play_pitch_hdl_lev2->entry;

            //打开echo和添加处理入口
            //record_play_echo_hdl_lev2 = open_echo((ECHO_PARM_SET *)&record_echo_parm, 16000);
            //entries[entry_cnt++] = &record_play_echo_hdl_lev2->entry;

        }
        entries[entry_cnt++] = &app_dec->mix_ch.entry;
        app_dec->stream = audio_stream_open(app_dec, record_play_stream_resume);
        audio_stream_add_list(app_dec->stream, entries, entry_cnt);
        break;
    case AUDIO_DEC_APP_EVENT_DEC_CLOSE:
        //如果打开了pitch则删除pitch处理入口和关闭pitch
        if (record_play_pitch_hdl_lev2) {
            audio_stream_del_entry(&record_play_pitch_hdl_lev2->entry);
            close_pitch(record_play_pitch_hdl_lev2);
            record_play_pitch_hdl_lev2 = NULL;
        }
        //如果打开了echo则删除echo处理入口和关闭echo
        if (record_play_echo_hdl_lev2) {
            audio_stream_del_entry(&record_play_echo_hdl_lev2->entry);
            close_echo(record_play_echo_hdl_lev2);
            record_play_echo_hdl_lev2 = NULL;
        }
        //如果打开了EQ则删除EQ处理入口和关闭EQ
        if (record_eq_drc_lev2) {
            file_eq_drc_close(record_eq_drc_lev2);
            record_eq_drc_lev2 = NULL;
        }

        break;
    }
}

int record_play_with_callback_by_path_lev1(char *name, u8 preemption, void (*evt_handler)(void *priv, int flag), void *evt_priv)
{
    static char *single_file[2] = {NULL};
    single_file[0] = name;
    single_file[1] = NULL;
    printf("record_play_with_callback_by_path_lev1\n");
    tone_dec_stop(&tone_dec_lev1, 1, TONE_DEC_STOP_BY_OTHER_PLAY);
    tone_dec_lev1 = tone_dec_create();
    if (tone_dec_lev1 == NULL) {
        return -1;
    }
    struct tone_dec_list_handle *dec_list = tone_dec_list_create(tone_dec_lev1, (const char **) single_file, preemption, evt_handler, NULL, record_play_stream_handler_lev1, NULL);
    return tone_dec_list_add_play(tone_dec_lev1, dec_list);
}

int record_play_with_callback_by_path_lev2(char *name, u8 preemption, void (*evt_handler)(void *priv, int flag), void *evt_priv)
{
    static char *single_file[2] = {NULL};
    single_file[0] = name;
    single_file[1] = NULL;
    printf("record_play_with_callback_by_path_lev2 \n");
    tone_dec_stop(&tone_dec_lev2, 1, TONE_DEC_STOP_BY_OTHER_PLAY);
    tone_dec_lev2 = tone_dec_create();
    if (tone_dec_lev2 == NULL) {
        return -1;
    }
    struct tone_dec_list_handle *dec_list = tone_dec_list_create(tone_dec_lev2, (const char **)single_file, preemption, evt_handler, NULL, record_play_stream_handler_lev2, NULL);
    return tone_dec_list_add_play(tone_dec_lev2, dec_list);
}

void record_play_end_callback_lev1(void *priv, int data)
{
    printf("record_play_end_callback_lev1\n");
}
void record_play_end_callback_lev2(void *priv, int data)
{
    printf("record_play_end_callback_lev2\n");
}
void record_file_replay_lev1()
{
    char path[64] = {0};
    char filepath[] = {"/JL_REC/REC_TEST.MP3"};
    sprintf(path, "%s%s%s%s", "storage/", "sd0", "/C", filepath);
    record_play_with_callback_by_path_lev1(path, INTERRUPT_PLAY, record_play_end_callback_lev1, 0);
}

void record_file_replay_lev2()
{
    char path[64] = {0};
    char filepath[] = {"/JL_REC/REC_TEST.WAV"};
    sprintf(path, "%s%s%s%s", "storage/", "sd0", "/C", filepath);
    record_play_with_callback_by_path_lev2(path, OVERLAY_PLAY, record_play_end_callback_lev2, 0);
}

#endif
