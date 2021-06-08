
#ifndef _AUDIO_SPEAKER_API_H_
#define _AUDIO_SPEAKER_API_H_

void stop_loud_speaker(void);
void start_loud_speaker(struct audio_fmt *fmt);
int speaker_if_working(void);
void set_speaker_gain_up(u8 value);
void set_speaker_gain_down(u8 value);
void switch_holwing_en(void);
void switch_echo_en(void);
void loud_speaker_pause(void);
void loud_speaker_resume(void);
#endif

