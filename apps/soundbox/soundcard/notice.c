#include "soundcard/notice.h"
#include "tone_player.h"
#include "audio_dec.h"


#if SOUNDCARD_ENABLE
static const char *electric_notice_tab[] = {
    SDFILE_RES_ROOT_PATH"tone/AA.*",
    SDFILE_RES_ROOT_PATH"tone/SA.*",
    SDFILE_RES_ROOT_PATH"tone/BB.*",
    //SDFILE_RES_ROOT_PATH"tone/SB.*",
    SDFILE_RES_ROOT_PATH"tone/CC.*",
    SDFILE_RES_ROOT_PATH"tone/SC.*",
    SDFILE_RES_ROOT_PATH"tone/DD.*",
    SDFILE_RES_ROOT_PATH"tone/SD.*",
    SDFILE_RES_ROOT_PATH"tone/EE.*",
    //SDFILE_RES_ROOT_PATH"tone/SE.*",
    SDFILE_RES_ROOT_PATH"tone/FF.*",
    SDFILE_RES_ROOT_PATH"tone/SF.*",
    SDFILE_RES_ROOT_PATH"tone/GG.*",
    SDFILE_RES_ROOT_PATH"tone/SG.*",
};

static const char *noise_tab[] = {
    SDFILE_RES_ROOT_PATH"tone/huan_hu.*",
    SDFILE_RES_ROOT_PATH"tone/gan_ga.*",
    SDFILE_RES_ROOT_PATH"tone/qiang.*",
    SDFILE_RES_ROOT_PATH"tone/bi_shi.*",
    SDFILE_RES_ROOT_PATH"tone/kaichang.*",
    SDFILE_RES_ROOT_PATH"tone/FeiWen.*",
    SDFILE_RES_ROOT_PATH"tone/xiao.*",
    SDFILE_RES_ROOT_PATH"tone/zhangshen.*",
    SDFILE_RES_ROOT_PATH"tone/QiuFenXiang.*",
    SDFILE_RES_ROOT_PATH"tone/memeda.*",
    SDFILE_RES_ROOT_PATH"tone/zeilala.*",
    SDFILE_RES_ROOT_PATH"tone/feicheng.*",
};




struct tone_dec_handle *tone_hdl = NULL;
struct audio_stream_entry *tone_vol_entry = NULL;
static u8 soundcard_notice_mic_out_en = 1;


static void soundcard_notice_play_stream_resume(void *p)
{
    struct audio_dec_app_hdl *app_dec = p;
    audio_decoder_resume(&app_dec->decoder);
}
static void soundcard_notice_end(void *priv, int data)
{
    printf("soundcard_notice_end\n");
}

void soundcard_notice_play_stream_handler(void *priv, int event, struct audio_dec_app_hdl *app_dec)
{
    u8 entry_cnt = 0;
    switch (event) {
    case AUDIO_DEC_APP_EVENT_START_INIT_OK:
        entry_cnt = 0;
        if (soundcard_notice_mic_out_en) {
            audio_mixer_ch_set_aud_ch_out(&app_dec->mix_ch, 0, BIT(0) | BIT(2) | BIT(3));
            audio_mixer_ch_set_aud_ch_out(&app_dec->mix_ch, 1, BIT(1) | BIT(2) | BIT(3));
        } else {
            audio_mixer_ch_set_aud_ch_out(&app_dec->mix_ch, 0, BIT(0));
            audio_mixer_ch_set_aud_ch_out(&app_dec->mix_ch, 1, BIT(1));
        }

        tone_vol_entry = sys_digvol_group_ch_open("tone_tone", -1, NULL);

        struct audio_stream_entry *entries[8] = {NULL};
        entries[entry_cnt++] = &app_dec->decoder.entry;
#if SYS_DIGVOL_GROUP_EN
        entries[entry_cnt++] = tone_vol_entry;
#endif
        entries[entry_cnt++] = &app_dec->mix_ch.entry;
        app_dec->stream = audio_stream_open(app_dec, soundcard_notice_play_stream_resume);
        audio_stream_add_list(app_dec->stream, entries, entry_cnt);
        break;

    case AUDIO_DEC_APP_EVENT_DEC_CLOSE:
#if SYS_DIGVOL_GROUP_EN
        if (tone_vol_entry) {
            audio_stream_del_entry(tone_vol_entry);
            sys_digvol_group_ch_close("tone_tone");
        }
#endif
        break;

    }
}

int soundcard_notice_play_by_path(char *name, u8 preemption, void (*evt_handler)(void *priv, int flag), void *evt_priv)
{
    static char *single_file[2] = {NULL};
    single_file[0] = name;
    single_file[1] = NULL;
    tone_dec_stop(&tone_hdl, 1, TONE_DEC_STOP_BY_OTHER_PLAY);
    tone_hdl = tone_dec_create();
    if (tone_hdl == NULL) {
        return -1;
    }
    struct tone_dec_list_handle *dec_list = tone_dec_list_create(tone_hdl,
                                            (const char **) single_file,
                                            preemption,
                                            evt_handler,
                                            NULL,
                                            soundcard_notice_play_stream_handler,
                                            NULL);
    return tone_dec_list_add_play(tone_hdl, dec_list);
}

void soundcard_make_notice_electric(u8 mode)
{
    if (mode >= (sizeof(electric_notice_tab) / sizeof(electric_notice_tab[0]))) {
        return ;
    }
    soundcard_notice_mic_out_en = 0;
    soundcard_notice_play_by_path(electric_notice_tab[mode], 0, soundcard_notice_end, 0);
    /* tone_play_with_callback_by_name(electric_notice_tab[mode], 0, NULL, NULL); */
}

void soundcard_make_some_noise(u8 id)
{
    if (id >= (sizeof(noise_tab) / sizeof(noise_tab[0]))) {
        return ;
    }
    soundcard_notice_mic_out_en = 1;
    soundcard_notice_play_by_path(noise_tab[id], 0, soundcard_notice_end, 0);
    /* tone_play_with_callback_by_name(noise_tab[id], 0, NULL, NULL); */
}
#endif//SOUNDCARD_ENABLE

