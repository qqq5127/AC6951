#include "asm/includes.h"
#include "media/includes.h"
#include "overlay_code.h"


struct code_type {
    u32 type;
    u32 dst;
    u32 src;
    u32 size;
};

////用于区分 overlay code 和 bss
/*此处不使用这种用法，直接在overlay段中使用LONG(0xFFFFFFFF),避免印代码被优化掉，导致的坑*/
/* u32 aec_bss_id sec(.aec_bss_id) = 0xffffffff; */
/* u32 fm_bss_id sec(.fm_bss_id) = 0xffffffff; */
/* u32 mp3_bss_id sec(.mp3_bss_id) = 0xffffffff; */
/* u32 wma_bss_id sec(.wma_bss_id) = 0xffffffff; */
/* u32 wav_bss_id sec(.wav_bss_id) = 0xffffffff; */

/* u32 wav_bss_id sec(.wav_bss_id) = 0xffffffff;
u32 ape_bss_id sec(.ape_bss_id) = 0xffffffff;
u32 flac_bss_id sec(.flac_bss_id) = 0xffffffff;
u32 m4a_bss_id sec(.m4a_bss_id) = 0xffffffff;
u32 amr_bss_id sec(.amr_bss_id) = 0xffffffff;
u32 dts_bss_id sec(.dts_bss_id) = 0xffffffff;
 */
extern int aec_addr, aec_begin, aec_size;
extern int wav_addr, wav_begin, wav_size;
extern int ape_addr, ape_begin, ape_size;
extern int flac_addr, flac_begin, flac_size;
extern int m4a_addr, m4a_begin, m4a_size;
extern int amr_addr, amr_begin, amr_size;
extern int dts_addr, dts_begin, dts_size;
extern int fm_addr,  fm_begin,  fm_size;

#ifdef CONFIG_MP3_WMA_LIB_SPECIAL
extern int mp3_addr, mp3_begin, mp3_size;
extern int wma_addr, wma_begin, wma_size;
#endif

const struct code_type  ctype[] = {
    {OVERLAY_AEC, (u32) &aec_addr, (u32) &aec_begin, (u32) &aec_size},
    {OVERLAY_WAV, (u32) &wav_addr, (u32) &wav_begin, (u32) &wav_size },
    {OVERLAY_APE, (u32) &ape_addr, (u32) &ape_begin, (u32) &ape_size },
    {OVERLAY_FLAC, (u32) &flac_addr, (u32) &flac_begin, (u32) &flac_size},
    {OVERLAY_M4A, (u32) &m4a_addr, (u32) &m4a_begin, (u32) &m4a_size },
    {OVERLAY_AMR, (u32) &amr_addr, (u32) &amr_begin, (u32) &amr_size },
    {OVERLAY_DTS, (u32) &dts_addr, (u32) &dts_begin, (u32) &dts_size },
    {OVERLAY_FM, (u32) &fm_addr, (u32) &fm_begin, (u32) &fm_size },

#ifdef CONFIG_MP3_WMA_LIB_SPECIAL
    {OVERLAY_MP3, (u32) &mp3_addr, (u32) &mp3_begin, (u32) &mp3_size },
    {OVERLAY_WMA, (u32) &wma_addr, (u32) &wma_begin, (u32) &wma_size },
#endif

};

struct audio_overlay_type {
    u32 atype;
    u32 otype;
};

const struct audio_overlay_type  aotype[] = {

    {AUDIO_CODING_MSBC, OVERLAY_AEC },
    {AUDIO_CODING_CVSD, OVERLAY_AEC },
    {AUDIO_CODING_WAV, OVERLAY_WAV},
    {AUDIO_CODING_APE, OVERLAY_APE },
    {AUDIO_CODING_FLAC, OVERLAY_FLAC},
    {AUDIO_CODING_M4A, OVERLAY_M4A },
    {AUDIO_CODING_ALAC, OVERLAY_M4A },
    {AUDIO_CODING_AMR, OVERLAY_AMR },
    {AUDIO_CODING_DTS, OVERLAY_DTS },
    {AUDIO_CODING_AAC, OVERLAY_M4A },
#ifdef CONFIG_MP3_WMA_LIB_SPECIAL
    {AUDIO_CODING_MP3, OVERLAY_MP3 },
    {AUDIO_CODING_WMA, OVERLAY_WMA },
#endif

};


void overlay_load_code(u32 type)
{
    int i = 0;
    for (i = 0; i < ARRAY_SIZE(ctype); i++) {
        if (type == ctype[i].type) {
            if (ctype[i].dst != 0) {
                memcpy((void *)ctype[i].dst, (void *)ctype[i].src, (int)ctype[i].size);
            }
            break;
        }
    }
}

void audio_overlay_load_code(u32 type)
{
    int i = 0;
    for (i = 0; i < ARRAY_SIZE(aotype); i++) {
        if (type == aotype[i].atype) {
            overlay_load_code(aotype[i].otype);
        }
    }
}
