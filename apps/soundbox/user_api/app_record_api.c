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


//*----------------------------------------------------------------------------*/
/**@brief  用户自定义录音输入例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u16 rec_test_timer = 0;
FILE *pcm_file = NULL;
/*---调用recorder_userdata_to_enc将数据输入到编码器-----------------------------*/
void rec_userinput_sun()
{
    char data2[512];
    fread(pcm_file, data2, 512);
    recorder_userdata_to_enc((s16 *)data2, 512);
}
/*--关闭pcm文件，关闭定时器---------------------------------------------------*/
void rec_userinput_del()
{
    if (pcm_file) {
        fclose(pcm_file);
        pcm_file = NULL;
    }
    if (rec_test_timer) {
        sys_timer_del(rec_test_timer);
        rec_test_timer = 0;
    }
}
/*---打开pcm文件，启动一个定时器将pcm数据发给编码器---------------------------*/
void rec_userinput_init()
{
    pcm_file = fopen(SDFILE_RES_ROOT_PATH"sine.pcm", "r");
    if (pcm_file == NULL) {
        printf("sine.pcm open err\n");
        return;
    }
    rec_test_timer = sys_timer_add(NULL, rec_userinput_sun, 100);
}
//*----------------------------------------------------------------------------*/
/**@brief  文件录音接口
   @param
   @return
   @note   1.
*/
/*----------------------------------------------------------------------------*/
void record_file_start(void)
{
    struct record_file_fmt fmt = {0};
    //char *logo = dev_manager_get_logo(dev_manager_find_active(0));//录音设备
    char logo[] = {"sd0"};
    char folder[] = {"JL_REC"};        //录音文件夹名称
    char filename[] = {"REC_TEST"};     //录音文件名，不需要加后缀，录音接口会根据编码格式添加后缀

    fmt.dev = logo;
    fmt.folder = folder;
    fmt.filename = filename;
    fmt.coding_type = AUDIO_CODING_MP3; //编码格式：AUDIO_CODING_WAV, AUDIO_CODING_MP3
    fmt.channel = 1;                    //声道数： 1：单声道 2：双声道
    fmt.sample_rate = 16000;            //采样率：8000，16000，32000，44100
    fmt.cut_head_time = 300;            //录音文件去头时间,单位ms
    fmt.cut_tail_time = 300;            //录音文件去尾时间,单位ms
    fmt.limit_size = 3000;              //录音文件大小最小限制， 单位byte
    fmt.gain = 8;
    fmt.source = ENCODE_SOURCE_MIC;     //录音输入源：ENCODE_SOURCE_MIC, ENCODE_SOURCE_LINE0_LR, ENCODE_SOURCE_USER

    int ret = recorder_encode_start(&fmt);

    if (fmt.source == ENCODE_SOURCE_USER) {
        rec_userinput_init();//初始化自定义录音源
    }
}

#endif
