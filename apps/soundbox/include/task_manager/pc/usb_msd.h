#ifndef __USB_MSD_H__
#define __USB_MSD_H__

#include "system/includes.h"
#include "usb/device/msd.h"

void usb_msd_unreg_disk(const usb_dev usb_id, const u32 lun);
void usb_msd_run_notify(void);
void usb_msd_init(void);
void usb_msd_uninit(void);

#endif//__USB_MSD_H__
