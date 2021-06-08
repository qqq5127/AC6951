
includes += \
	-I$(ROOT)/apps/$(APP_CASE)/include/ \
	-I$(ROOT)/apps/$(APP_CASE)/include/task_manager/ \
	-I$(ROOT)/apps/$(APP_CASE)/include/task_manager/bt \
	-I$(ROOT)/apps/$(APP_CASE)/include/user_api/ \
	-I$(ROOT)/apps/common/ \
	-I$(ROOT)/apps/common/device/ \
	-I$(ROOT)/apps/common/audio/ \
	-I$(ROOT)/apps/common/audio/stream/ \
	-I$(ROOT)/apps/common/power_manage/ \
	-I$(ROOT)/apps/common/charge_box/ \
	-I$(ROOT)/apps/common/third_party_profile/common/ \
	-I$(ROOT)/apps/common/third_party_profile/jieli/ \
	-I$(ROOT)/apps/common/third_party_profile/jieli/trans_data_demo/ \
	-I$(ROOT)/apps/common/third_party_profile/jieli/online_db/ \
	-I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/ \
    -I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/ \
    -I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/bt_trans_data/ \
    -I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/adv_rcsp_protocol/ \
    -I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/ \
    -I$(ROOT)/apps/common/dev_manager/ \
    -I$(ROOT)/apps/common/file_operate/ \
    -I$(ROOT)/apps/common/music/ \
	-I$(ROOT)/apps/common/include/ \
	-I$(ROOT)/apps/common/config/include/ \
	-I$(ROOT)/apps/$(APP_CASE)/board/$(CPU)/ \
	-I$(ROOT)/cpu/$(CPU)/ \
	-I$(ROOT)/cpu/$(CPU)/audio_common \
	-I$(ROOT)/cpu/$(CPU)/audio_dec \
	-I$(ROOT)/cpu/$(CPU)/audio_enc \
	-I$(ROOT)/cpu/$(CPU)/audio_effect \
	-I$(ROOT)/cpu/$(CPU)/audio_mic \
	-I$(ROOT)/cpu/$(CPU)/localtws \
	-I$(ROOT)/include_lib/btstack/third_party/baidu/ \
	-I$(ROOT)/include_lib/btstack/ \

includes += \
	-I$(ROOT)/apps/common/third_party_profile/tm_gma_protocol/ \

includes += \
	-I$(ROOT)/apps/common/third_party_profile/dma_deal/ \


# LIBS = \
--start-group \
   $(AR_DIR)/cpu.a \
   $(AR_DIR)/system.a \
--whole-archive \
   $(AR_DIR)/btstack.a \
--no-whole-archive \
   $(AR_DIR)/btctrler.a \
   $(AR_DIR)/media.a \
   $(AR_DIR)/aes_lib_pi32.a \
--end-group


#     $(AR_DIR)/libc.a \


objs += \
	$(ROOT)/apps/common/device/norflash/norflash.o \
	$(ROOT)/apps/common/device/nandflash/nandflash.o \


objs += \
	$(ROOT)/apps/common/key/iokey.o \
	$(ROOT)/apps/common/key/irkey.o \
	$(ROOT)/apps/common/key/adkey.o \
	$(ROOT)/apps/common/key/slidekey.o \
	$(ROOT)/apps/common/key/touch_key.o \
	$(ROOT)/apps/common/key/ctmu_touch_key.o \
	$(ROOT)/apps/common/key/adkey_rtcvdd.o \
	$(ROOT)/apps/common/key/rdec_key.o \
	$(ROOT)/apps/common/key/key_driver.o \
	$(ROOT)/apps/common/debug/debug_lite.o \
	$(ROOT)/apps/common/update/update.o \
	$(ROOT)/apps/common/update/uart_update.o \
	$(ROOT)/apps/common/update/uart_update_master.o \
    $(ROOT)/apps/common/update/norflash_update.o \
    $(ROOT)/apps/common/update/testbox_update.o \
    $(ROOT)/apps/common/update/norflash_ufw_update.o \
	$(ROOT)/apps/common/audio/sine_make.o \
	$(ROOT)/apps/common/audio/uartPcmSender.o \
	$(ROOT)/apps/common/audio/audio_digital_vol.o \
	$(ROOT)/apps/common/audio/stream/stream_entry.o \
	$(ROOT)/apps/common/audio/stream/stream_sync.o \
	$(ROOT)/apps/common/audio/stream/stream_src.o \
	$(ROOT)/apps/common/device/detection.o \
	$(ROOT)/apps/common/device/gSensor/da230.o \
    $(ROOT)/apps/common/device/gSensor/SC7A20.o \
	$(ROOT)/apps/common/device/gSensor/gSensor_manage.o \
	$(ROOT)/apps/common/third_party_profile/jieli/trans_data_demo/spp_trans_data.o \
	$(ROOT)/apps/common/third_party_profile/jieli/hid_user.o \
	$(ROOT)/apps/common/third_party_profile/jieli/trans_data_demo/le_trans_data.o \
	$(ROOT)/apps/common/third_party_profile/jieli/le_client_demo.o \
    $(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/rcsp_adv_user_update.o \
	$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_bt_name_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_key_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_led_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_mic_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/adv_rcsp_protocol/rcsp_adv_tws_sync.o \
	$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_time_stamp_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting/adv_work_setting.o \
	$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/adv_rcsp_protocol/rcsp_adv_opt.o \
	$(ROOT)/apps/common/third_party_profile/common/3th_profile_api.o \
	$(ROOT)/apps/common/third_party_profile/common/custom_cfg.o \
	$(ROOT)/apps/common/third_party_profile/jieli/online_db/spp_online_db.o \
	$(ROOT)/apps/common/third_party_profile/jieli/online_db/online_db_deal.o \
	$(ROOT)/apps/common/audio/decode/decode.o \
	$(ROOT)/apps/common/audio/decode/audio_key_tone.o \
	$(ROOT)/apps/$(APP_CASE)/app_main.o \
	$(ROOT)/apps/$(APP_CASE)/version.o \
	$(ROOT)/apps/$(APP_CASE)/power_manage/app_charge.o \
	$(ROOT)/apps/$(APP_CASE)/power_manage/app_chargestore.o \
	$(ROOT)/apps/$(APP_CASE)/power_manage/app_power_manage.o \

objs += \
	$(ROOT)/apps/common/third_party_profile/tm_gma_protocol/gma_license.o \
	$(ROOT)/apps/common/third_party_profile/tm_gma_protocol/gma_spp_user.o \
	$(ROOT)/apps/common/third_party_profile/tm_gma_protocol/le_gma.o \
	$(ROOT)/apps/common/third_party_profile/tm_gma_protocol/tm_gma_deal.o \

objs += \
	$(ROOT)/apps/common/third_party_profile/common/mic_rec.o \


objs += \
	$(ROOT)/apps/common/third_party_profile/dma_deal/dma_deal.o \
	$(ROOT)/apps/common/third_party_profile/dma_deal/dueros_spp_user.o \
	$(ROOT)/apps/common/third_party_profile/dma_deal/le_dueros_dma.o \

objs += \
	$(ROOT)/apps/$(APP_CASE)/log_config/lib_btctrler_config.o \
	$(ROOT)/apps/$(APP_CASE)/log_config/lib_driver_config.o \
	$(ROOT)/apps/$(APP_CASE)/log_config/lib_system_config.o \
	$(ROOT)/apps/$(APP_CASE)/log_config/lib_update_config.o \
	$(ROOT)/apps/$(APP_CASE)/log_config/lib_media_config.o \
	$(ROOT)/apps/$(APP_CASE)/log_config/lib_btstack_config.o \
	$(ROOT)/apps/$(APP_CASE)/log_config/app_config.o \


-include $(ROOT)/apps/common/usb/Makefile.inc

#------------- EQ config-----------------#
# objs += \
	$(ROOT)/apps/$(APP_CASE)/app_eq.o \

include $(ROOT)/apps/$(APP_CASE)/soundcard/Makefile
include $(ROOT)/apps/$(APP_CASE)/smartbox/Makefile
include $(ROOT)/apps/$(APP_CASE)/ui/lcd/Makefile
include $(ROOT)/apps/$(APP_CASE)/third_party_profile/Makefile
include $(ROOT)/apps/$(APP_CASE)/ui/color_led/Makefile

objs += \
	$(ROOT)/apps/$(APP_CASE)/common/task_table.o \
	$(ROOT)/apps/$(APP_CASE)/common/init.o \
	$(ROOT)/apps/$(APP_CASE)/common/user_cfg.o \
	$(ROOT)/apps/$(APP_CASE)/common/dev_status.o \
	$(ROOT)/apps/$(APP_CASE)/common/tone_table.o \

objs += \
	$(ROOT)/apps/common/config/app_config.o \
	$(ROOT)/apps/common/config/ci_transport_uart.o \
	$(ROOT)/apps/common/config/bt_profile_config.o \

objs += \
	$(ROOT)/apps/common/file_operate/file_bs_deal.o \
	$(ROOT)/apps/common/file_operate/file_api.o \
	$(ROOT)/apps/common/file_operate/file_manager.o \
	$(ROOT)/apps/common/music/music_decrypt.o \
	$(ROOT)/apps/common/music/music_id3.o \
	$(ROOT)/apps/common/music/music_player.o \
	$(ROOT)/apps/common/music/breakpoint.o \
	$(ROOT)/apps/common/music/general_player.o \
	$(ROOT)/apps/common/audio/encode/encode_write_file.o \

objs += \
	$(ROOT)/apps/common/device/fm/fm_inside/fm_inside.o \
	$(ROOT)/apps/common/device/fm/bk1080/Bk1080.o \
	$(ROOT)/apps/common/device/fm/qn8035/QN8035.o \
	$(ROOT)/apps/common/device/fm/rda5807/RDA5807.o \
	$(ROOT)/apps/common/device/fm/fm_manage.o \

objs += \
	$(ROOT)/apps/common/device/fm_emitter/fm_inside/fm_emitter_inside.o \
	$(ROOT)/apps/common/device/fm_emitter/ac3433/ac3433.o \
	$(ROOT)/apps/common/device/fm_emitter/qn8007/qn8007.o \
	$(ROOT)/apps/common/device/fm_emitter/qn8027/qn8027.o \
	$(ROOT)/apps/common/device/fm_emitter/fm_emitter_manage.o \

objs += \
	$(ROOT)/apps/common/ui/led7/led7_ui_api.o \
	$(ROOT)/apps/common/ui/lcd/lcd_ui_api.o \

objs += \
	$(ROOT)/apps/common/dev_manager/dev_manager.o \
	$(ROOT)/apps/common/dev_manager/dev_update.o \
	$(ROOT)/apps/common/dev_manager/dev_reg.o \

objs += \
    $(ROOT)/apps/common/charge_box/chargeIc_manage.o \
    $(ROOT)/apps/common/charge_box/chgbox_box.o \
    $(ROOT)/apps/common/charge_box/chgbox_ctrl.o \
    $(ROOT)/apps/common/charge_box/chgbox_det.o \
    $(ROOT)/apps/common/charge_box/chgbox_wireless.o \
    $(ROOT)/apps/common/charge_box/chgbox_ui.o \
    $(ROOT)/apps/common/charge_box/chgbox_ui_drv_timer.o \
    $(ROOT)/apps/common/charge_box/chgbox_ui_drv_pwmled.o \
    $(ROOT)/apps/common/charge_box/chgbox_handshake.o \

objs += \
	$(ROOT)/apps/common/fat_nor/nor_fs.o \
	$(ROOT)/apps/common/fat_nor/phone_rec_fs.o \
	$(ROOT)/apps/common/fat_nor/virfat_flash.o \

objs += \
	$(ROOT)/apps/common/rec_nor/nor_interface.o \
	$(ROOT)/apps/common/rec_nor/nor_rec_fs.o \

#bt user api
objs += \
    $(ROOT)/apps/common/bt_common/bt_test_api.o \

objs += \
	$(ROOT)/apps/$(APP_CASE)/ui/led/pwm_led_api.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led/pwm_led_para_table.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led7/ui_bt.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led7/ui_fm.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led7/ui_fm_emitter.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led7/ui_record.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led7/ui_rtc.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led7/ui_pc.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led7/ui_linein.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led7/ui_music.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led7/ui_common.o \
	$(ROOT)/apps/$(APP_CASE)/ui/led7/ui_idle.o \

objs += \
	$(ROOT)/apps/$(APP_CASE)/user_api/dev_multiplex_api.o \
	$(ROOT)/apps/$(APP_CASE)/user_api/app_pwmled_api.o \
	$(ROOT)/apps/$(APP_CASE)/user_api/app_record_api.o \
	$(ROOT)/apps/$(APP_CASE)/user_api/app_special_play_api.o \
	$(ROOT)/apps/$(APP_CASE)/user_api/app_status_api.o \
	$(ROOT)/apps/$(APP_CASE)/user_api/product_info_api.o \

objs += \
	$(ROOT)/apps/$(APP_CASE)/font/fontinit.o \

objs += \
	$(ROOT)/apps/$(APP_CASE)/task_manager/fm/fm.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/fm/fm_rw.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/fm/fm_api.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/music/music.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/pc/pc.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/linein/linein_dev.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/linein/linein.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/linein/linein_api.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/record/record.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/rtc/rtc.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/rtc/virtual_rtc.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/rtc/alarm_api.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/rtc/alarm_user.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/spdif/spdif.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/spdif/hdmi_cec_drv.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/bt/bt.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/bt/bt_event_fun.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/bt/bt_switch_fun.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/bt/bt_key_fun.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/bt/bt_product_test.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/bt/bt_ble.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/bt/bt_tws.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/bt/vol_sync.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/bt/bt_emitter.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/idle/idle.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/app_task_switch.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/power_on/power_on.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/power_off/power_off.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/task_key.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/app_common.o \
	$(ROOT)/apps/$(APP_CASE)/task_manager/sleep/sleep.o \

#ifeq ($(AC608N),y)
objs += \
	$(ROOT)/apps/common/iap/iAP_des.o \
	$(ROOT)/apps/common/iap/iAP_device.o \
	$(ROOT)/apps/common/iap/iAP_iic.o \
	$(ROOT)/apps/common/usb/host/apple_mfi.o \

#endif

