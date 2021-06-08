#ifndef NOISEGATE_API_H
#define NOISEGATE_API_H


int noiseGate_buf();
void noiseGate_init(void *work_buf, int attackTime, int releaseTime, int threshold, int low_th_gain, int sampleRate, int channel);
int noiseGate_run(void *work_buf, short *in, short *out, int len);
#endif
