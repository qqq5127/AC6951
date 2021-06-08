#include "app_config.h"
#include "circular_buf.h"
#include "system/includes.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "ac3433/ac3433.h"
#include "qn8007/qn8007.h"
#include "qn8027/qn8027.h"
#include "fm_inside/fm_emitter_inside.h"
#include "audio_config.h"
#include "system/includes.h"

#if TCFG_APP_FM_EMITTER_EN

static FM_EMITTER_INTERFACE *fm_emitter_hdl = NULL;

/*************************************************
 *
 *      Fmtx fre save vm
 *
 *************************************************/
static int fm_emitter_manage_timer = 0;
static u8 fm_emitter_manage_cnt = 0;
static u16 cur_fmtx_freq = 0;

static void fm_emitter_manage_save_fre(void)
{
    printf("fm_emitter_manage_save_fre %d\n", cur_fmtx_freq);
    if (fm_emitter_hdl) {
        u8 tbuf[2];
        tbuf[0] = (cur_fmtx_freq >> 8) & 0xFF;
        tbuf[1] = cur_fmtx_freq & 0xFF;
        int ret = syscfg_write(VM_FM_EMITTER_FREQ, tbuf, 2);
        if (ret != 2) {
            printf("fm last fre write err!\n");
        }
    } else {
        printf("%s %d no hdl\n", __func__, __LINE__);
    }
}

static void fm_emitter_manage_fre_save_do(void *priv)
{
    //printf("fm_emitter_manage_fre_save_do %d\n", fm_emitter_manage_cnt);
    local_irq_disable();
    if (++fm_emitter_manage_cnt >= 5) {
        sys_hi_timer_del(fm_emitter_manage_timer);
        fm_emitter_manage_timer = 0;
        fm_emitter_manage_cnt = 0;
        local_irq_enable();
        fm_emitter_manage_save_fre();
        return;
    }
    local_irq_enable();
}

static void fm_emitter_manage_fre_change(void)
{
    local_irq_disable();
    fm_emitter_manage_cnt = 0;
    if (fm_emitter_manage_timer == 0) {
        fm_emitter_manage_timer = sys_hi_timer_add(NULL, fm_emitter_manage_fre_save_do, 1000);
    }
    local_irq_enable();
}


void fm_emitter_manage_init(u16 fre, void (*fmtx_isr_cb)(s16 *, u32))
{
    printf("fm_emitter_manage_init \n");
    int found = 0;
    u8 tbuf[2];
    int ret = 0;
    list_for_each_fm_emitter(fm_emitter_hdl) {
        printf("fm_emitter_hdl %x\n", fm_emitter_hdl);
        if (!memcmp(fm_emitter_hdl->name, "fm_emitter_inside", strlen(fm_emitter_hdl->name))) {
            printf("fm fine dev %s \n", fm_emitter_hdl->name);
            found = 1;
            break;
        }
    }

    if (found) {
        if (!fre) {
            ret = syscfg_read(VM_FM_EMITTER_FREQ, tbuf, 2);
            if (ret == 2) {
                fre = (tbuf[0] << 8) | tbuf[1];
                if (fre < 875) {
                    fre = 875;
                } else if (fre > 1080) {
                    fre = 1080;
                }
                printf("fm last fre: %d\n", fre);
            } else {
                fre = 875;
                printf("fm last fre read err!\n");
            }
        }
        fm_emitter_hdl->init(fre);
        cur_fmtx_freq = fre;
        fm_emitter_hdl->data_cb(fmtx_isr_cb);

        tbuf[0] = (fre >> 8) & 0xFF;
        tbuf[1] = fre & 0xFF;
        /* ret = syscfg_write(VM_FM_EMITTER_FREQ, tbuf, 2); */
        if (ret != 2) {
            printf("fm last fre init write err!\n");
        }
    }
    fm_emitter_manage_fre_change();
}


void fm_emitter_manage_start(void)
{
    if (fm_emitter_hdl) {
        fm_emitter_hdl->start();
    }
}

void fm_emitter_manage_stop(void)
{
    if (fm_emitter_hdl) {
        fm_emitter_hdl->stop();
    }
}

u16 fm_emitter_manage_get_fre()
{
    return cur_fmtx_freq;
}



void fm_emitter_manage_set_fre(u16 fre)
{
    if (fm_emitter_hdl) {
        if (fre < 875) {
            fre = 875;
        } else if (fre > 1080) {
            fre = 1080;
        }
        fm_emitter_hdl->set_fre(fre);
        cur_fmtx_freq = fre;
        fm_emitter_manage_fre_change();
    } else {
        printf("%s %d no hdl\n", __func__, __LINE__);
    }
}


void fm_emitter_manage_set_power(u8 power, u16 fre)
{
    if (fm_emitter_hdl) {
        fm_emitter_hdl->set_power(power, fre);
    }
}

#endif
