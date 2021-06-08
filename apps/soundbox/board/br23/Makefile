

LIBS = \
--start-group \
   $(AR_DIR)/cpu.a \
   $(AR_DIR)/system.a \
   $(AR_DIR)/aec.a \
   $(AR_DIR)/media.a \
   $(AR_DIR)/media_app.a \
   $(AR_DIR)/libAptFilt_pi32v2_OnChip.a \
   $(AR_DIR)/libEchoSuppress_pi32v2_OnChip.a \
   $(AR_DIR)/libNoiseSuppress_pi32v2_OnChip.a \
   $(AR_DIR)/libSplittingFilter_pi32v2_OnChip.a \
   $(AR_DIR)/libDelayEstimate_pi32v2_OnChip.a \
   $(AR_DIR)/libSingleMicSystem_pi32v2_OnChip.a \
   $(AR_DIR)/libSMS_TDE_pi32v2_OnChip.a \
   $(AR_DIR)/lib_esco_repair.a \
   $(AR_DIR)/mp3_dec_lib.a \
   $(AR_DIR)/mp3tsy_dec_lib.a \
   $(AR_DIR)/wma_dec_lib.a \
   $(AR_DIR)/wtg_dec_lib.a \
   $(AR_DIR)/wtgv2_dec_lib.a \
   $(AR_DIR)/wav_dec_lib.a \
   $(AR_DIR)/flac_dec_lib.a \
   $(AR_DIR)/ape_dec_lib.a \
   $(AR_DIR)/m4a_tws_lib.a \
   $(AR_DIR)/amr_dec_lib.a \
   $(AR_DIR)/dts_dec_lib.a \
   $(AR_DIR)/mp3_decstream_lib.a \
   $(AR_DIR)/wma_decstream_lib.a \
   $(AR_DIR)/sbc_eng_lib.a \
   $(AR_DIR)/compressor.a \
   $(AR_DIR)/crossover_coff.a \
   $(AR_DIR)/limiter.a \
   $(AR_DIR)/lib_mp2_code.a \
   $(AR_DIR)/lib_adpcm_code.a \
   $(AR_DIR)/mp3_enc_lib.a \
   $(AR_DIR)/opus_enc_lib.a \
   $(AR_DIR)/speex_enc_lib.a \
   $(AR_DIR)/lib_pitchshifter.a \
   $(AR_DIR)/lib_pitch_speed.a \
   $(AR_DIR)/lib_reverb_cal.a \
   $(AR_DIR)/lib_rmono2stereo.a \
   $(AR_DIR)/bfilterfun_lib.a \
   $(AR_DIR)/libFFT_pi32v2_OnChip.a \
   $(AR_DIR)/libOpcore_maskrom_pi32v2_OnChip.a \
   $(AR_DIR)/lib_resample_cal.a \
   $(AR_DIR)/lib_resample_fast_cal.a \
   $(AR_DIR)/crypto_toolbox_Ospeed.a \
   $(AR_DIR)/lib_g726_decode.a \
   $(AR_DIR)/lib_g726_code.a \
   $(AR_DIR)/limiter_noiseGate.a \
   $(AR_DIR)/howling.a \
   $(AR_DIR)/noisegate.a \
   $(AR_DIR)/shiftPhase.a \
   $(AR_DIR)/pemafrow.a \
   $(AR_DIR)/lib_howlings_phf.a \
   $(AR_DIR)/math.a \
   $(AR_DIR)/bt_hash_enc.a \
   $(AR_DIR)/filtparam_cal_lib.a \
   $(AR_DIR)/agreement.a \
   $(AR_DIR)/lib_sur_cal.a \
   $(AR_DIR)/lib_vbass_cal.a \
   $(AR_DIR)/loudness.a \
   $(AR_DIR)/rcsp_stack.a \
   $(AR_DIR)/lib_usbaudio_plc.a \
   $(AR_DIR)/coeff_calculate.a \
   $(AR_DIR)/rms.a \
   $(AR_DIR)/SpectrumShow.a \

ifeq ($(MIDI_DEC_ADDR),y)
LIBS += \
   $(AR_DIR)/lib_midi_dec_addr.a \

else
LIBS += \
   $(AR_DIR)/lib_midi_dec.a \

endif

ifeq ($(CONFIG_BLUETOOTH_EN),y)

LIBS += \
   $(AR_DIR)/btstack$(LIB_SUFFIX).a \
   $(AR_DIR)/btctrler$(LIB_SUFFIX).a \

endif

LIBS += \
--end-group

#$(AR_DIR)/crypto_toolbox_Osize.a \
   $(AR_DIR)/libepmotion.a \


LIBS += \
   $(AR_DIR)/update.a \


ifeq ($(CONFIG_UI_ENABLE),y)
ifeq ($(CONFIG_DOT_UI_ENABLE),y)
LIBS += \
   $(AR_DIR)/res.a \
   $(AR_DIR)/ui_dot.a \
   $(AR_DIR)/font.a \

else
LIBS += \
   $(AR_DIR)/res.a \
   $(AR_DIR)/ui.a \
   $(AR_DIR)/font.a \

endif
endif




objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/irq_config.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_demo/board_ac695x_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_demo/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_demo/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_demo/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_demo/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_demo/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_lcd/board_ac695x_lcd.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_lcd/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_lcd/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_lcd/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_lcd/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_lcd/key_table/touch_key_table.o \


objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6954a_demo/board_ac6954a_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6954a_demo/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6954a_demo/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6954a_demo/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6954a_demo/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6954a_demo/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6952e_lighter/board_ac6952e_lighter.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6952e_lighter/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6952e_lighter/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6952e_lighter/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6952e_lighter/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6952e_lighter/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6955f_headset_mono/board_ac6955f_headset_mono.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6955f_headset_mono/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6955f_headset_mono/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6955f_headset_mono/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6955f_headset_mono/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6955f_headset_mono/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_charging_bin/board_ac695x_charging_bin.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_charging_bin/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_charging_bin/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_charging_bin/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_charging_bin/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_charging_bin/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_multimedia_charging_bin/board_ac695x_multimedia_charging_bin.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_multimedia_charging_bin/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_multimedia_charging_bin/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_multimedia_charging_bin/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_multimedia_charging_bin/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_multimedia_charging_bin/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_btemitter/board_ac695x_btemitter.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_btemitter/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_btemitter/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_btemitter/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_btemitter/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_btemitter/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws_box/board_ac695x_tws_box.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws_box/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws_box/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws_box/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws_box/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws_box/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws/board_ac695x_tws.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_tws/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_soundcard/board_ac695x_soundcard.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_soundcard/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_soundcard/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_soundcard/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_soundcard/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_soundcard/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_smartbox/board_ac695x_smartbox.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_smartbox/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_smartbox/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_smartbox/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_smartbox/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac695x_smartbox/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6951g/board_ac6951g.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6951g/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6951g/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6951g/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6951g/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6951g/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a/board_ac6083a.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a/key_table/touch_key_table.o \

objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a_iap/board_ac6083a_iap.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a_iap/key_table/adkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a_iap/key_table/iokey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a_iap/key_table/irkey_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a_iap/key_table/rdec_key_table.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6083a_iap/key_table/touch_key_table.o \

