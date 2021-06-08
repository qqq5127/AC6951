#ifndef _FM_SEVER__H_
#define _FM_SEVER__H_


void fm_volume_pp(void);//播放暂停
void fm_scan_down();//半自动收台
void fm_scan_up();//半自动收台
void fm_scan_all();//全自动收音
void fm_delete_freq();//删当前频点
void fm_prev_station();//上下台
void fm_next_station();//上下台
void fm_volume_up();//声音调节
void fm_volume_down();//声音调整
void fm_next_freq();//上下频点
void fm_prev_freq();//上下频点
void fm_scan_stop(void);//停止搜索，停止后会自动跳到最后一个台


void fm_api_init();//资源申请，读取vm
void fm_api_release();//释放







#endif
