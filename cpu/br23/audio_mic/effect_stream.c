#include "effect_stream.h"
#include "audio_dec.h"
#include "audio_splicing.h"

#if 0
extern struct audio_dac_hdl dac_hdl;
extern struct audio_dac_channel default_dac;

static u32 soundcard_mix_audio_mixer_check_sr(struct audio_mixer *mixer, u32 sr)
{
    return TCFG_REVERB_SAMPLERATE_DEFUAL;
}

static int mix_prob_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in)
{
    pcm_dual_mix_to_dual(in->data, in->data, in->data_len);
    return 0;
}

void soundcard_mix_init(struct audio_mixer *mix, s16 *mix_buf, u16 buf_size, struct audio_vocal_tract_ch *tract_ch, u8 flag)
{
    if (mix == NULL) {
        return ;
    }
    audio_mixer_open(mix);
    audio_mixer_set_event_handler(mix, NULL);
    audio_mixer_set_check_sr_handler(mix, soundcard_mix_audio_mixer_check_sr);
    /*初始化mix_buf的长度*/
    audio_mixer_set_output_buf(mix, mix_buf, buf_size);
    u8 ch_num = audio_output_channel_num();
    audio_mixer_set_channel_num(mix, ch_num);
    // 固定采样率输出
    audio_mixer_set_sample_rate(mix, MIXER_SR_SPEC, TCFG_REVERB_SAMPLERATE_DEFUAL);

    if (flag == 0) {
        //rrrl后置dac， 声道左右合成后再输出
        mix->entry.prob_handler = mix_prob_handler;
    }

    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &mix->entry;
    entries[entry_cnt++] = &tract_ch->entry;
    if (flag) {
        audio_dac_new_channel(&dac_hdl, &default_dac);
        struct audio_dac_channel_attr attr;
        audio_dac_channel_get_attr(&default_dac, &attr);
        attr.delay_time = 50;
        attr.write_mode = WRITE_MODE_BLOCK;
        audio_dac_channel_set_attr(&default_dac, &attr);
        entries[entry_cnt++] = &default_dac.entry;
        /*entries[entry_cnt++] = &dac_hdl.entry;*/
    }

    mix->stream = audio_stream_open(mix, audio_mixer_stream_resume);
    audio_stream_add_list(mix->stream, entries, entry_cnt);
}
#endif

