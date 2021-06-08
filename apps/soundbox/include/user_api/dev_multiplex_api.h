#ifndef __DEV_MULTIPLEX_API_H__
#define __DEV_MULTIPLEX_API_H__

#include "generic/typedef.h"
#include "app_config.h"
#include "usb/host/usb_host.h"

extern usb_dev g_usb_id;

void mult_sdio_resume();
int mult_sdio_suspend();
int dev_sd_change_usb();
int dev_usb_change_sd();
int mult_sd_online_mount_before(int sd_dev, usb_dev usb_id);
int mult_sd_offline_before(void *logo, usb_dev usb_id);
int mult_sd_online_mount_after(int sd_dev, usb_dev usb_id, int err);
int mult_usb_online_mount_after(usb_dev usb_id, int err);
int mult_usb_mount_before(usb_dev usb_id);
int mult_usb_mount_offline(usb_dev usb_id);
void pc_dm_multiplex_init();
void pc_dm_multiplex_exit();


#endif//__DEV_MULTIPLEX_API_H__
