



















RAM1_LIMIT_L = 0x2C000;
RAM1_LIMIT_H = 0x30000;





UPDATA_SIZE = 0x80;
UPDATA_BEG = RAM1_LIMIT_H - UPDATA_SIZE;

RAM1_BEGIN = RAM1_LIMIT_L;
RAM1_END = RAM1_LIMIT_H - UPDATA_SIZE;
RAM1_SIZE = RAM1_END - RAM1_BEGIN;





RAM_LIMIT_L = 0x00000;


RAM_LIMIT_H = 0x2C000;




ISR_SIZE = 0x100;
ISR_BASE = RAM_LIMIT_H - ISR_SIZE;




RAM_BEGIN = RAM_LIMIT_L;
RAM_END = RAM_LIMIT_H - ISR_SIZE;
RAM_SIZE = RAM_END - RAM_BEGIN;


BANK_SIZE = 5*1024;


PSRAM_BEG = 0x800000;
PSRAM_SIZE = 2M;


CODE_BEG = 0X1E000C0;

UPDATA_BREDR_BASE_BEG = 0xF9000;




MIC_EFFECT_EQ_SECTION = 3;



EQ_SECTION_NUM = 20 +MIC_EFFECT_EQ_SECTION;




MEMORY
{
    psram(rwx) : ORIGIN = PSRAM_BEG , LENGTH = PSRAM_SIZE

 code0(rx) : ORIGIN = 0x1E00120, LENGTH = (1024 * 1024)



 ram0(rwx) : ORIGIN = RAM_BEGIN , LENGTH = RAM_SIZE
    ram1(rwx) : ORIGIN = RAM1_BEGIN , LENGTH = RAM1_SIZE
}


  memmem = ABSOLUTE(0x1127a4);
  memcpy = ABSOLUTE(0x1127a8);
  memmove = ABSOLUTE(0x1127ac);
  memcmp = ABSOLUTE(0x1127b0);
  memset = ABSOLUTE(0x1127b4);
  strcmp = ABSOLUTE(0x1127b8);
  strcpy = ABSOLUTE(0x1127bc);
  strlen = ABSOLUTE(0x1127c0);
  strncmp = ABSOLUTE(0x1127c4);
  strstr = ABSOLUTE(0x1127c8);
  flush_dcache = ABSOLUTE(0x1127cc);
  flushinv_dcache = ABSOLUTE(0x1127d0);
  sfc_suspend = ABSOLUTE(0x1127d4);
  sfc_resume = ABSOLUTE(0x1127d8);
  chip_crc16 = ABSOLUTE(0x1127e0);
  nvram_set_boot_state = ABSOLUTE(0x1127f0);
  _IRQ_MEM_ADDR = ABSOLUTE(0x2bf00);
  _MASK_MEM_BEGIN = ABSOLUTE(0x2bdc0);
  _MASK_MEM_SIZE = ABSOLUTE(0x12c);


EXTERN(
_start

fat_vfs_ops



sdfile_vfs_ops
sbc_decoder
msbc_decoder
sbc_hwaccel



cvsd_decoder



pcm_decoder







mp3_decoder






wma_decoder






flac_decoder



ape_decoder



m4a_decoder






alac_decoder



amr_decoder



dts_decoder



g729_decoder
wav_decoder
cvsd_encoder



msbc_encoder



mp3_encoder



g726_encoder



adpcm_encoder



pcm_encoder
sbc_encoder

);

ENTRY(_start)

SECTIONS
{

    . =ORIGIN(psram);
    .psram_text ALIGN(4):
    {





    } > psram

    .psram ALIGN(32):
    {




    } > psram

    . = ORIGIN(code0);
    .text ALIGN(4):
    {
        text_code_begin = .;

        PROVIDE(text_rodata_begin = .);

        *(.startup.text)

        bank_stub_start = .;
  *(.bank.stub.*)
  bank_stub_size = . - bank_stub_start;

  *(.aac_const)
  *(.aac_code)

  *(.alac_const)
  *(.alac_code)
  *(.alac_dec_code)

  *(.bt_aac_dec_eng_const)
  *(.bt_aac_dec_eng_code)
  *(.bt_aac_dec_core_code)
  *(.bt_aac_dec_core_sparse_code)

  *(.dts_dec_const)
  *(.cvsd_const)
  *(.cvsd_code)

  . = ALIGN(4);
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

  . = ALIGN(4);
        hrsensor_dev_begin = .;
        KEEP(*(.hrsensor_dev))
        hrsensor_dev_end = .;

  . = ALIGN(4);
        fm_dev_begin = .;
        KEEP(*(.fm_dev))
        fm_dev_end = .;

  . = ALIGN(4);
        fm_emitter_dev_begin = .;
        KEEP(*(.fm_emitter_dev))
        fm_emitter_dev_end = .;
        chargeIc_dev_begin = .;
        KEEP(*(.chargeIc_dev))
            chargeIc_dev_end = .;
        . = ALIGN(4);


        *(.opcore_table_maskrom)
        *(.bfilt_table_maskroom)
        *(.opcore_maskrom)
        *(.bfilt_code)
        *(.bfilt_const)

  . = ALIGN(4);



        btctler_code_start = .;

        BTCTLER_CONTROLLER_CODE_START = .;
  *(.bt_rf_const)
  *(.bt_rf_code)
  *(.vendor_manager_const)
  *(.vendor_manager_code)
  *(.device_manager_const)
  *(.device_manager_code)
  *(.hci_controller_const)
  *(.hci_controller_code)
  *(.hci_interface_const)
  *(.hci_interface_code)
        BTCTLER_CONTROLLER_CODE_SIZE = ABSOLUTE(. - BTCTLER_CONTROLLER_CODE_START);

        BTCTLER_LE_CONTROLLER_CODE_START = .;
        *(.ble_rf_const)
        *(.ble_rf_code)

     *(.ble_ll_const)
     *(.ble_ll_code)
  *(.ble_hci_const)
  *(.ble_hci_code)
        BTCTLER_LE_CONTROLLER_CODE_SIZE = ABSOLUTE(. - BTCTLER_LE_CONTROLLER_CODE_START);

        BTCTLER_CL_CODE_START = .;
        *(.bredr_irq)
        *(.bredr_irq_code)
        *(.bredr_irq_const)

        *(.classic_lmp_const)
        *(.classic_lmp_linkbulk_const)
        *(.classic_lmp_code)
        *(.classic_lmp_linkbulk_code)
  *(.classic_hci_const)
  *(.classic_hci_code)

        LMP_ENC_CODE_START = .;
        *(.classic_lmp_auth_const)
        *(.classic_lmp_bigint_const)
        *(.classic_lmp_crypt_const)
        *(.classic_lmp_ecdh_const)
        *(.classic_lmp_hmac_const)
        *(.classic_lmp_auth_code)
        *(.classic_lmp_bigint_code)
        *(.classic_lmp_crypt_code)
        *(.classic_lmp_ecdh_code)
        *(.classic_lmp_hmac_code)
        LMP_ENC_CODE_SIZE = ABSOLUTE(. - LMP_ENC_CODE_START);

        *(.classic_rf_const)
        *(.classic_rf_code)
        *(.classic_bb_const)
        *(.classic_bb_code)
        BTCTLER_CL_CODE_SIZE = ABSOLUTE(. - BTCTLER_CL_CODE_START);

  . = ALIGN(4);
        *(.classic_tws_const)
        *(.classic_tws_code)
        *(.classic_tws_code.esco)

        *(.tws_media_sync_code)
        *(.tws_media_sync_const)

        *(.tws_data_forward_code)
        *(.tws_data_forward_const)

  . = ALIGN(4);
        tws_func_stub_begin = .;
        KEEP(*(.tws_func_stub))
        tws_func_stub_end = .;

        *(.lmp_irq_code)
        *(.link_bulk_code)
        *(.frame_irq_code)

  . = ALIGN(4);
        *(.link_task_const)
        *(.link_task_code)

  . = ALIGN(4);
        *(.classic_irq_const)
        *(.classic_irq_code)

  . = ALIGN(4);
        *(.tws_irq_code)

  . = ALIGN(4);
        tws_sync_call_begin = .;
        KEEP(*(.tws_sync_call))
        tws_sync_call_end = .;


  . = ALIGN(4);
        tws_sync_channel_begin = .;
        KEEP(*(.tws_sync_channel.0))
        KEEP(*(.tws_sync_channel.1))
        KEEP(*(.tws_sync_channel.2))
        KEEP(*(.tws_sync_channel.3))
        KEEP(*(.tws_sync_channel.4))
        KEEP(*(.tws_sync_channel.5))
        KEEP(*(.tws_sync_channel.6))
        KEEP(*(.tws_sync_channel.7))
        KEEP(*(.tws_sync_channel.8))
        KEEP(*(.tws_sync_channel.9))
        KEEP(*(.tws_sync_channel.10))
        KEEP(*(.tws_sync_channel.11))
        KEEP(*(.tws_sync_channel.12))
        KEEP(*(.tws_sync_channel.13))
        tws_sync_channel_end = .;

        btctler_code_end = .;

  . = ALIGN(4);

  . = ALIGN(4);



        btstack_code_start = .;

        . = ALIGN(4);

        a2dp_source_media_codec_begin = .;
        KEEP(*(.a2dp_source_media_codec))
            a2dp_source_media_codec_end = .;
        a2dp_sink_media_probe_begin = .;
        KEEP(*(.a2dp_sink_media_probe))
            a2dp_sink_media_probe_end = .;

        a2dp_sink_media_codec_begin = .;
        KEEP(*(.a2dp_sink_media_codec))
            a2dp_sink_media_codec_end = .;

        a2dp_event_handler_begin = .;
        KEEP(*(.a2dp_event_handler))
            a2dp_event_handler_end = .;

        sdp_record_item_begin = .;
        KEEP(*(.sdp_record_item))
            sdp_record_item_end = .;

        bt_sleep_begin = .;
        KEEP(*(.bt_sleep))
            bt_sleep_end = .;

  *(.bt_stack_const)
  *(.bt_stack_code)
        *(.ble_db_const)
        *(.ble_db_code)
        *(.ble_sm_const)
        *(.ble_sm_code)
        *(.ble_att_const)
        *(.ble_att_code)
        *(.ble_gatt_const)
        *(.ble_gatt_code)


        BTSTACK_LE_HOST_MESH_CODE_START = .;
        *(.ble_mesh_code)
        *(.ble_mesh_tinycrypt_code)

        *(.ble_mesh_const)
        *(.ble_mesh_tinycrypt_const)
        BTSTACK_LE_HOST_MESH_CODE_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_CODE_START);

        btstack_code_end = .;
  . = ALIGN(4);


        BTSTACK_CODE_TOTAL_SIZE = btstack_code_end - btstack_code_start;

  . = ALIGN(4);


  . = ALIGN(4);
        system_text_start = .;

   _device_node_begin = .;
     PROVIDE(device_node_begin = .);
     KEEP(*(.device))
     _device_node_end = .;
     PROVIDE(device_node_end = .);

  config_target_begin = .;
     PROVIDE(config_target_begin = .);
     KEEP(*(.config_target))
  config_target_end = .;
     PROVIDE(config_target_end = .);

     system_code_begin = .;
     KEEP(*(.system.*.text))
     system_code_end = .;
  . = ALIGN(4);
  system_code_size = system_code_end - system_code_begin;

  vfs_ops_begin = .;
  KEEP(*(.vfs_operations))
  vfs_ops_end = .;

     _lib_version_begin = .;
     PROVIDE(lib_version_begin = .);
     KEEP(*(.lib_version))
     _lib_version_end = .;
     PROVIDE(lib_version_end = .);

  _initcall_begin = .;
  PROVIDE(initcall_begin = .);
     KEEP(*(.initcall))
  _initcall_end = .;
  PROVIDE(initcall_end = .);

  _early_initcall_begin = .;
  PROVIDE(early_initcall_begin = .);
  KEEP(*(.early.initcall))
  _early_initcall_end = .;
  PROVIDE(early_initcall_end = .);

  _late_initcall_begin = .;
  PROVIDE(late_initcall_begin = .);
  KEEP(*(.late.initcall))
  _late_initcall_end = .;
  PROVIDE(late_initcall_end = .);

  _platform_initcall_begin = .;
  PROVIDE(platform_initcall_begin = .);
  KEEP(*(.platform.initcall))
  _platform_initcall_end = .;
  PROVIDE(platform_initcall_end = .);

  _module_initcall_begin = .;
  PROVIDE(module_initcall_begin = .);
  KEEP(*(.module.initcall))
  _module_initcall_end = .;
  PROVIDE(module_initcall_end = .);

  _sys_event_handler_begin = .;
  PROVIDE(sys_event_handler_begin = .);
  KEEP(*(.sys_event.4.handler))
  KEEP(*(.sys_event.3.handler))
  KEEP(*(.sys_event.2.handler))
  KEEP(*(.sys_event.1.handler))
  KEEP(*(.sys_event.0.handler))
  _sys_event_handler_end = .;
  PROVIDE(sys_event_handler_end = .);

  _syscfg_arg_begin = .;
  PROVIDE(syscfg_arg_begin = .);
  KEEP(*(.syscfg.arg))
  _syscfg_arg_end = .;
  PROVIDE(syscfg_arg_end = .);

  _syscfg_handler_begin = .;
  PROVIDE(syscfg_handler_begin = .);
  KEEP(*(.syscfg.handler))
  _syscfg_handler_end = .;
  PROVIDE(syscfg_handler_end = .);

  _syscfg_ops_begin = .;
  PROVIDE(syscfg_ops_begin = .);
  KEEP(*(.syscfg.2.ops))
  KEEP(*(.syscfg.1.ops))
  KEEP(*(.syscfg.0.ops))
  _syscfg_ops_end = .;
  PROVIDE(syscfg_ops_end = .);

  _server_info_begin = .;
  PROVIDE(server_info_begin = .);
  KEEP(*(.server_info))
  _server_info_end = .;
  PROVIDE(server_info_end = .);

  _bus_device_begin = .;
  PROVIDE(bus_device_begin = .);
  KEEP(*(.bus_device))
  _bus_device_end = .;
  PROVIDE(bus_device_end = .);

     _sys_power_hal_ops_begin = .;
     PROVIDE(sys_power_hal_ops_begin = .);
     KEEP(*(.sys_power_hal_ops))
     _sys_power_hal_ops_end = .;
     PROVIDE(sys_power_hal_ops_end = .);


  . = ALIGN(4);
     lp_target_begin = .;
     PROVIDE(lp_target_begin = .);
     KEEP(*(.lp_target))
     lp_target_end = .;
     PROVIDE(lp_target_end = .);

  . = ALIGN(4);
        deepsleep_target_begin = .;
        PROVIDE(deepsleep_target_begin = .);
        KEEP(*(.deepsleep_target))
        deepsleep_target_end = .;
        PROVIDE(deepsleep_target_end = .);

     crypto_begin = .;
     *(.crypto_ecdh_code)
     *(.crypto_ecdh_const)

     *(.crypto_bigint_code)
     *(.crypto_bigint_const)

     *(.crypto_code)
     *(.crypto_const)

     *(.ECDH_sample_code)
     *(.ECDH_sample_const)

     *(.uECC_code)
     *(.uECC_const)

     *(.hmac_code)
     *(.hmac_const)

     *(.hash_sample_code)
     *(.hash_sample_const)

     *(.aes_cmac_sample_code)
     *(.aes_cmac_sample_const)
     crypto_end = .;
     crypto_size = . - crypto_begin;

        *(.mem_code)
        *(.mem_const)

        *(.os_port_code)
        *(.os_port_const)

        __movable_function_start = .;
        *(movable.text.*);
        *(movable.stub.*);
        *(movable.region.*);

        __movable_function_end = .;
        __movable_function_size = __movable_function_end - __movable_function_start;

        system_text_end = .;



        system_code_total_size = system_text_end - system_text_start;

        . = ALIGN(4);

        lcd_interface_begin = .;
        KEEP(*(.lcd_if_info))
        lcd_interface_end = .;

  ui_style_begin = .;
  KEEP(*(.ui_style))
  ui_style_end = .;


        elm_event_handler_begin_JL = .;
        KEEP(*(.elm_event_handler_JL))
  elm_event_handler_end_JL = .;

        elm_event_handler_begin_UPGRADE = .;
        KEEP(*(.elm_event_handler_UPGRADE))
        elm_event_handler_end_UPGRADE = .;



  elm_event_handler_begin_DIAL = .;
        KEEP(*(.elm_event_handler_DIAL))
  elm_event_handler_end_DIAL = .;


  control_event_handler_begin = .;
        KEEP(*(.control_event_handler))
  control_event_handler_end = .;

        control_ops_begin = .;
        KEEP(*(.control_ops))
        control_ops_end = .;

        battery_notify_begin = .;
        *(.battery_notify)
        battery_notify_end = .;

        . = ALIGN(4);


        media_text_start = .;

  *(.aec_code)
  *(.aec_const)
  *(.der_code)
  *(.der_const)
  *(.nlp_code)
  *(.nlp_const)
  *(.qmf_code)
  *(.qmf_const)
  *(.bt_audioplc_sparse_code)
  *(.bt_audioplc_code)
  *(.bt_audioplc_const)

  . = ALIGN(4);
  *(.res_code)
  *(.res_const)

  . = ALIGN(4);
  *(.ns_code)
  *(.ns_const)
  *(.opcore_maskrom)

  . = ALIGN(4);






  *(.g729_code)
  *(.g729_const)
  *(.g726_code)
  *(.g726_const)

  *(.wtg_dec_code)
  *(.wtg_dec_const)
  *(.wtg_dec_sparse_code)
  *(.wtg_dec_sparse_const)
  *(.wtgv2_code)
  *(.wtgv2_const)
  *(.wtgv2dec_code)
  *(.wtgv2dec_const)
  *(.wtgv2dec_str)
  *(.wtg_decv2_sparse_code)
        *(.bfilt_code)
        . = ALIGN(4);

  *(.mp3_decstream_const)
   *(.mp3_decstream_code)
   *(.mp3_decstream_sparse_code)
   *(.mp3_decstream_sparse_const)

   *(.mp3_dec_sparse_code)
   *(.mp3_dec_sparse_const)

   *(.mp3_dec_code)
   *(.mp3_dec_const)





   *(.mp3_const)



   *(.mp3_code)



  *(.msbc_code)
  *(.msbc_const)
  *(.mty_code)
  *(.mty_const)
  *(.mp3tsy_dec_code)
  *(.mp3tsy_dec_const)
  *(.mp3tsy_dec_sparse_code)
  *(.mp3tsy_dec_sparse_const)
  *(.sbc_code)
  *(.sbc_const)
  *(.sine_code)
  *(.sine_const)




  *(.wma_code)
  *(.wma_const)
  *(.wma_dec_code)
  *(.wma_dec_const)

  *(.wma_dec_sparse_code)
  *(.wma_dec_sparse_const)
  *(.wma_decstream_const)
  *(.wma_decstream_code)
  *(.wma_decstream_sparse_code)
  *(.wma_decstream_sparse_const)


  *(.amr_code)
  *(.amr_const)
  *(.midi_code)
  *(.midi_const)



  *(.audio_decoder_code)
  *(.audio_decoder_const)
        *(.mp3_encode_code)
        *(.mp3_encode_const)
  *(.media_device_code)
  *(.media_device_const)
  *(.audio_encoder_code)
  *(.audio_encoder_const)
  *(.mixer_code)
  *(.mixer_const)
  *(.stream_code)
  *(.stream_const)
  *(.dec_server_code)
  *(.dec_server_const)
  *(.rec_server_code)
  *(.rec_server_const)
  *(.auto_mute_code)
  *(.auto_mute_const)
  *(.plc_code)
  *(.plc_const)
  *(.wireless_sync_code)
  *(.wireless_sync_const)
  *(.fft_code)
  *(.fft_const)


  *(.mp3_enc_code)
  *(.mp3_enc_const)
  *(.mp3_enc_sparse_code)
  *(.mp3_enc_sparse_const)
  . = ALIGN(4);
  *(.mp2_encode_code)
  *(.mp2_encode_const)
  *(.mp2_encode_sparse_code)
  *(.mp2_encode_sparse_const)
  . = ALIGN(4);
  *(.adpcm_encode_code)
  *(.adpcm_encode_const)
  *(.adpcm_encode_sparse_code)
  *(.adpcm_encode_sparse_const)
  . = ALIGN(4);


  *(.m4a_dec_sparse_code)
  *(.m4a_dec_sparse_const)

   *(.aac_dec_sparse_code)
  *(.aac_dec_sparse_const)
  *(.bt_aac_dec_core_const)

  *(.amr_dec_const)


  *(.dts_dec_ff_const)
  *(.dts_dec_sparse_code)
  *(.dts_dec_sparse_const)

  *(.res_sparse_code)
  *(.ns_sparse_code )
  *(.aec_sparse_code)
  *(.nlp_sparse_code)
  *(.der_sparse_code)
  *(.qmf_sparse_code)

  *(.sms_const);
  *(.sms_sparse_code);
  *(.sms_code);


  *(.resample_cal_code)
  *(.resample_cal_sparse_code)
  *(.resample_cal_const)

  *(.bt_compressor_sparse_const)
  *(.bt_compressor_sparse_code)
  *(.compressor_sparse_code)
  *(.compressor_sparse_const)

  *(.bt_limiter_sparse_const)
  *(.bt_limiter_sparse_code)
  *(.limiter_sparse_code)
  *(.limiter_sparse_const)

  *(.bt_crossOver_sparse_const)
  *(.bt_crossOver_sparse_code)
  *(.crossOver_sparse_code)
  *(.crossOver_sparse_const)

  *(.iap_code)
  *(.iap_const)
  *(.audio_bfilt_code)
  *(.audio_bfilt_const)
  *(.audio_buf_sync_code)
  *(.audio_buf_sync_const)
  *(.audio_dec_app_code)
  *(.audio_dec_app_const)
  *(.audio_dec_app_sync_code)
  *(.audio_dec_app_sync_const)
  *(.audio_dec_app_event_callback)
  *(.audio_dec_app_dec_run)
  *(.audio_dec_app_normal_run)
  *(.audio_dec_tone_event_callback)
  *(.audio_dig_vol_code)
  *(.audio_dig_vol_const)
  *(.audio_echo_reverb_code)
  *(.audio_echo_reverb_const)
  *(.audio_echo_src_code)
  *(.audio_echo_src_const)
  *(.audio_energy_detect_code)
  *(.audio_energy_detect_const)
  *(.audio_eq_code)
  *(.audio_eq_const)
  *(.audio_drc_code)
  *(.audio_drc_const)
  *(.audio_eq_drc_apply_code)
  *(.audio_eq_drc_apply_const)
  *(.audio_equalloudness_code)
  *(.audio_equalloudness_const)
  *(.audio_howling_code)
  *(.audio_howling_const)
  *(.audio_noisegate_code)
  *(.audio_noisegate_const)
  *(.audio_output_dac_code)
  *(.audio_output_dac_const)
  *(.audio_pemafrow_code)
  *(.audio_pemafrow_const)
  *(.audio_pitch_code)
  *(.audio_pitch_const)
  *(.audio_pitchspeed_code)
  *(.audio_pitchspeed_const)
  *(.audio_surround_code)
  *(.audio_surround_const)
  *(.audio_vbass_code)
  *(.audio_vbass_const)
  *(.audio_vocal_remove_code)
  *(.audio_vocal_remove_const)
  *(.audio_vocal_tract_synthesis_code)
  *(.audio_vocal_tract_synthesis_const)
  *(.eq_config_code)
  *(.eq_config_const)
  *(.eq_config_protocol_code)
  *(.eq_config_protocol_const)
  *(.spectrum_eq_code)
  *(.spectrum_eq_const)
  *(.spectrum_fft_code)
  *(.spectrum_fft_const)
  *(.audio_localtws_code)
  *(.audio_localtws_const)
  *(.audio_dec_localtws_code)
  *(.audio_dec_localtws_const)
  *(.audio_dec_a2dp_code)
  *(.audio_dec_a2dp_const)
  *(.audio_dec_esco_code)
  *(.audio_dec_esco_const)
  *(.audio_dec_file_code)
  *(.audio_dec_file_const)




  *(.audio_resample_code)
  *(.audio_resample_const)
  *(.audio_splicing_code)
  *(.audio_splicing_const)
  *(.channel_switch_code)
  *(.channel_switch_const)
  *(.sw_drc_code)
  *(.sw_drc_const)
  *(.aptx_decoder_code)
  *(.aptx_decoder_const)
  *(.ldac_decoder_code)
  *(.ldac_decoder_const)
  *(.speex_encoder_code)
  *(.speex_encoder_const)
  *(.sbc_encoder_code)
  *(.sbc_encoder_const)
  *(.opus_encoder_code)
  *(.opus_encoder_const)
  *(.prevent_task_fill_code)
  *(.prevent_task_fill_const)
  *(.audio_cfifo_code)
  *(.audio_cfifo_const)




  *(.audio_adc_code)
  *(.audio_adc_const)
  *(.audio_dac_code)
  *(.audio_dac_const)
  *(.audio_linein_code)
  *(.audio_linein_const)
  *(.audio_link_code)
  *(.audio_link_const)
  *(.audio_sample_sync_code)
  *(.audio_sample_sync_const)
  *(.audio_wireless_sync_code)
  *(.audio_wireless_sync_const)
  *(.audio_spdif_code)
  *(.audio_spdif_const)
  *(.audio_src_code)
  *(.audio_src_const)
  *(.audio_src_base_code)
  *(.audio_src_base_const)
  *(.effectrs_sync_code)
  *(.effectrs_sync_const)
  *(.eq_design_code)
  *(.eq_design_const)
  *(.hw_eq_code)
  *(.hw_eq_const)
  *(.iic_code)
  *(.iic_const)
  *(.mic_cap_code)
  *(.mic_cap_const)
  *(.sbc_hwaccel_code)
  *(.sbc_hwaccel_const)

  . = ALIGN(4);
     _audio_decoder_begin = .;
     PROVIDE(audio_decoder_begin = .);
         KEEP(*(.audio_decoder))
     _audio_decoder_end = .;
     PROVIDE(audio_decoder_end = .);

     _audio_encoder_begin = .;
     PROVIDE(audio_encoder_begin = .);
         KEEP(*(.audio_encoder))
     _audio_encoder_end = .;
     PROVIDE(audio_encoder_end = .);

     _audio_package_begin = .;
     PROVIDE(audio_package_begin = .);
         KEEP(*(.audio_package))
     _audio_package_end = .;
     PROVIDE(audio_package_end = .);

     _audio_dev_begin = .;
     PROVIDE(audio_dev_begin = .);
         KEEP(*(.audio_device))
     _audio_dev_end = .;
     PROVIDE(audio_dev_end = .);

     _audio_hwaccel_begin = .;
     PROVIDE(audio_hwaccel_begin = .);
         KEEP(*(.audio_hwaccel))
     _audio_hwaccel_end = .;
     PROVIDE(audio_hwaccel_end = .);

  . = ALIGN(4);
     media_code_begin = .;
         *(.media.*.text)

  . = ALIGN(4);
  *(.crossOver_code)
  *(.crossOver_const)
  *(.bt_crossOver_const)
  *(.bt_crossOver_code)
  . = ALIGN(4);

  *(.compressor_code)
  *(.compressor_const)
  *(.bt_compressor_const)
  *(.bt_compressor_code)
  . = ALIGN(4);

  *(.limiter_code)
  *(.limiter_const)
  *(.bt_limiter_const)
  *(.bt_limiter_code)
  . = ALIGN(4);
  *(.lib_pitchshift_code)
  *(.lib_pitchshift_const)
  . = ALIGN(4);
  *(.sur_cal_const)
  *(.sur_cal_code)
  . = ALIGN(4);
  *(.reverb_cal_sparse_code)

        . = ALIGN(4);
  *(.dvol_ram_code)
        . = ALIGN(4);
  *(.reverb_cal_const)
        . = ALIGN(4);
     *(.reverb_cal_code)
        . = ALIGN(4);






  *(.stream_code_copy_run)
  *(.mixer_code_out_run)
  *(.audio_codec_code_frame_file)

  *(.buf_sync_code_nor_run)

  *(.resample_fastcal_const)
  *(.resample_fastcal_code)
  *(.resample_fastcal_sparse_code)



        . = ALIGN(4);

     media_code_end = .;
  . = ALIGN(4);
  media_code_size = media_code_end - media_code_begin;

  . = ALIGN(4);
        media_text_end = .;

        media_code_total_size = media_text_end - media_text_start;


  . = ALIGN(4);
     update_target_begin = .;
     PROVIDE(update_target_begin = .);
     KEEP(*(.update_target))
     update_target_end = .;
     PROVIDE(update_target_end = .);
  . = ALIGN(4);

        *(.text*)
        *(.LOG_TAG_CONST*)
        *(.rodata*)
  . = ALIGN(4);

        text_code_end = .;

    } >code0


    . = ORIGIN(ram0);

   _data_code_begin = . ;
 bank_code_run_addr = .;
 common_code_run_addr = .;



    .data ALIGN(4):
   {


        *(.data_magic)
        . = ALIGN(4);

        *(.flushinv_icache)
        *(.volatile_ram_code)
        *(.os_critical_code)
        *(.chargebox_code)

   *(.os_code)
     *(.os_const)


        *(.ui_ram)

        *(.fat_data_code)

        *(.gpio_ram)
        *(.LED_code)
        *(.LED_const)
  . = ALIGN(4);



      *(.usr_timer_const)
  *(.usr_timer_code)
     *(.timer_const)
  *(.timer_code)
     *(.cbuf_const)
  *(.cbuf_code)
  *(.fm_data_code)
  *(.fm_data_const)
  . = ALIGN(4);



        *(.fm_code)


        . = ALIGN(4);

        media_data_code_start = .;
        . = ALIGN(4);
  *(.sbc_eng_code)

        *(.pcm_code)
        *(.pcm_const)
  *(.audio_dec_pcm_code)
  *(.audio_dec_pcm_const)
  *(.stream_code_nor_run)
  *(.mixer_code_nor_run)
  *(.audio_codec_code)
  *(.audio_codec_code_read_file)
  *(.audio_src_code_nor_run)
  *(.src_base_code_nor_run)
  *(.audio_dac_code_nor_run)
  *(.audio_cfifo_code_nor_run)
  *(.audio_track_code)
  *(.audio_track_const)

        . = ALIGN(4);
        media_data_code_end = .;

        . = ALIGN(4);

     _data_code_end = . ;

     _cpu_store_begin = . ;
  . = ALIGN(4);
        *(.data*)

  *(.cvsd_data)

  . = ALIGN(4);
  dec_board_param_mem_begin = .;
  KEEP(*(.dec_board_param_mem))
  dec_board_param_mem_end = .;

        . = ALIGN(4);
  *(.sbc_eng_code)
        . = ALIGN(4);

        . = ALIGN(32);


        btstack_data_start = .;
        *(.bt_stack_data)
        *(.ble_db_data)
        *(.ble_sm_data)
        *(.ble_att_data)
        *(.ble_gatt_data)


        BTSTACK_LE_HOST_MESH_DATA_START = .;
  . = (. +3) & ~ 3;
        _net_buf_pool_list = .;
        *(._net_buf_pool.static.*)

        *(.ble_mesh_data)
        *(.ble_mesh_tinycrypt_data)
        BTSTACK_LE_HOST_MESH_DATA_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_DATA_START);


        btstack_data_end = .;

        . = ALIGN(4);


        btctler_data_start = .;

        BTCTLER_CONTROLLER_DATA_START = .;
        *(.bt_rf_data)
  *(.vendor_manager_data)
  *(.device_manager_data)
  *(.hci_controller_data)
  *(.hci_interface_data)
        BTCTLER_CONTROLLER_DATA_SIZE = ABSOLUTE(. - BTCTLER_CONTROLLER_DATA_START);

        BTCTLER_LE_CONTROLLER_DATA_START = .;
        *(.ble_ll_data)
        *(.ble_hci_data)
        *(.classic_hci_data)
        *(.ble_rf_data)
        BTCTLER_LE_CONTROLLER_DATA_SIZE = ABSOLUTE(. - BTCTLER_LE_CONTROLLER_DATA_START);

        BTCTLER_CL_DATA_START = .;
        *(.classic_lmp_data)
        *(.classic_lmp_auth_data)
        *(.classic_lmp_bigint_data)
        *(.classic_lmp_crypt_data)
        *(.classic_lmp_ecdh_data)
        *(.classic_lmp_linkbulk_data)
        *(.classic_lmp_hmac_data)
        *(.classic_rf_data)
        *(.classic_bb_data)
        BTCTLER_CL_DATA_SIZE = ABSOLUTE(. - BTCTLER_CL_DATA_START);

        btctler_data_end = .;

        . = ALIGN(4);


  . = ALIGN(4);
        system_data_start = .;

  _video_subdev_begin = .;
  PROVIDE(video_subdev_begin = .);
  KEEP(*(.video_subdev.0))
  KEEP(*(.video_subdev.1))
  KEEP(*(.video_subdev.2))
  KEEP(*(.video_subdev.3))
  KEEP(*(.video_subdev.4))
  KEEP(*(.video_subdev.5))
  _video_subdev_end = .;
  PROVIDE(video_subdev_end = .);

  _audio_subdev_begin = .;
  PROVIDE(audio_subdev_begin = .);
  KEEP(*(.audio_subdev.0))
  KEEP(*(.audio_subdev.1))
  KEEP(*(.audio_subdev.2))
  KEEP(*(.audio_subdev.3))
  _audio_subdev_end = .;
  PROVIDE(audio_subdev_end = .);

  _iic_device_begin = .;
  PROVIDE(iic_device_begin = .);
  KEEP(*(.iic))
  _iic_device_end = .;
  PROVIDE(iic_device_end = .);

        _avin_spi_device_begin = .;
  PROVIDE(avin_spi_device_begin = .);
  KEEP(*(.sw_spi))
  _avin_spi_device_end = .;
  PROVIDE(avin_spi_device_end = .);

  _video_dev_begin = .;
  PROVIDE(video_dev_begin = .);
  KEEP(*(.video_device))
  _video_dev_end = .;
  PROVIDE(video_dev_end = .);

  _key_driver_ops_begin = .;
  PROVIDE(key_driver_ops_begin = .);
  KEEP(*(.key_driver_ops))
  _key_driver_ops_end = .;
  PROVIDE(key_driver_ops_end = .);

  _touch_driver_begin = .;
  PROVIDE(touch_driver_begin = .);
  KEEP(*(.touch_driver))
  _touch_driver_end = .;
  PROVIDE(touch_driver_end = .);

  _static_hi_timer_begin = .;
  PROVIDE(static_hi_timer_begin = .);
  KEEP(*(.hi_timer))
  _static_hi_timer_end = .;
  PROVIDE(static_hi_timer_end = .);

  _sys_cpu_timer_begin = .;
  PROVIDE(sys_cpu_timer_begin = .);
  KEEP(*(.sys_cpu_timer))
  _sys_cpu_timer_end = .;
  PROVIDE(sys_cpu_timer_end = .);

     _sys_config_begin = .;
     PROVIDE(sys_config_begin = .);
     KEEP(*(.sys_cfg))
     _sys_config_end = .;
     PROVIDE(sys_config_end = .);

     _sys_fat_begin = .;
     PROVIDE(sys_fat_begin = .);
     KEEP(*(.fs_fat))
     _sys_fat_end = .;
     PROVIDE(sys_fat_end = .);

  _app_begin = .;
  PROVIDE(app_begin = .);
  KEEP(*(.app))
  _app_end = .;
  PROVIDE(app_end = .);

  _os_begin = .;
  PROVIDE(os_begin = .);




     *(.os_str)
     *(.os_data)
  _os_end = .;
  PROVIDE(os_end = .);

     *(.crypto_ecdh_data)
     *(.crypto_data)

     *(.mem_data)
        *(.os_port_data)

     *(.uECC_data)
     *(.ECDH_sample_data)

         __movable_slot_start = .;
         *(movable.slot.*);
         __movable_slot_end = .;

        system_data_end = .;

  . = ALIGN(4);



        media_data_start = .;

        . = ALIGN(4);
  EQ_COEFF_BASE = . ;
  . = EQ_COEFF_BASE + 4 * EQ_SECTION_NUM * 14;

        . = ALIGN(4);
        *(.aec_data)
        . = ALIGN(4);
        *(.res_data)
        . = ALIGN(4);
        *(.ns_data)
        . = ALIGN(4);
        *(.der_data)
        . = ALIGN(4);
        *(.nlp_data)
        . = ALIGN(4);
        *(.qmf_data)
        . = ALIGN(4);
        *(.fft_data)
        . = ALIGN(4);
  *(.bt_audioplc_data)
  *(.sms_data)

        *(.pcm_data)

  *(.g729_data)
  *(.g726_data)
  *(.wtg_dec_data)
  *(.wtgv2_data)
  *(.wtgv2dec_data)
  *(.mp3_data)
  *(.mp3_dec_data)
  *(.msbc_data)
  *(.mty_data)
  *(.mp3tsy_dec_data)
  *(.sbc_data)
  *(.sine_data)

  *(.wma_data)
  *(.wma_dec_data)

  *(.amr_data)
  *(.midi_data)

  *(.audio_decoder_data)



        *(.mp3_encode_data)
  *(.media_device_data)
  *(.audio_encoder_data)
  *(.mixer_data)
  *(.stream_data)
  *(.dec_server_data)
  *(.rec_server_data)
  *(.auto_mute_data)
  *(.plc_data)
  *(.wireless_sync_data)

  *(.resample_cal_data)
  *(.bt_compressor_data)
  *(.bt_crossOver_data)
  *(.bt_limiter_data)
  *(.bt_compressor_sparse_data)
  *(.bt_limiter_sparse_data)
  *(.bt_crossOver_sparse_data)

  *(.iap_data)
  *(.audio_bfilt_data)
  *(.audio_buf_sync_data)
  *(.audio_dec_app_data)
  *(.audio_dec_app_sync_data)
  *(.audio_dig_vol_data)
  *(.audio_echo_reverb_data)
  *(.audio_echo_src_data)
  *(.audio_energy_detect_data)
  *(.audio_eq_data)
  *(.audio_drc_data)
  *(.audio_eq_drc_apply_data)
  *(.audio_equalloudness_data)
  *(.audio_howling_data)
  *(.audio_noisegate_data)
  *(.audio_output_dac_data)
  *(.audio_pemafrow_data)
  *(.audio_pitch_data)
  *(.audio_pitchspeed_data)
  *(.audio_surround_data)
  *(.audio_vbass_data)
  *(.audio_vocal_remove_data)
  *(.audio_vocal_tract_synthesis_data)
  *(.eq_config_data)
  *(.eq_config_protocol_data)
  *(.spectrum_eq_data)
  *(.spectrum_fft_data)
  *(.audio_localtws_data)
  *(.audio_dec_localtws_data)
  *(.audio_dec_a2dp_data)
  *(.audio_dec_esco_data)
  *(.audio_dec_file_data)
  *(.audio_dec_pcm_data)
  *(.audio_resample_data)
  *(.audio_splicing_data)
  *(.channel_switch_data)
  *(.sw_drc_data)
  *(.aptx_decoder_data)
  *(.ldac_decoder_data)
  *(.speex_encoder_data)
  *(.sbc_encoder_data)
  *(.opus_encoder_data)
  *(.prevent_task_fill_data)
  *(.audio_cfifo_data)
  *(.audio_track_data)
  *(.audio_adc_data)
  *(.audio_dac_data)
  *(.audio_linein_data)
  *(.audio_link_data)
  *(.audio_sample_sync_data)
  *(.audio_wireless_sync_data)
  *(.audio_spdif_data)
  *(.audio_src_data)
  *(.audio_src_base_data)
  *(.effectrs_sync_data)
  *(.eq_design_data)
  *(.hw_eq_data)
  *(.iic_data)
  *(.mic_cap_data)
  *(.sbc_hwaccel_data)
  *(.resample_fastcal_data)
  *(.aac_data)
  *(.alac_data)
        . = ALIGN(4);
  *(.dec2tws_mem)
        media_data_end = .;

  . = ALIGN(4);

   } > ram0

    .irq_stack ALIGN(32) :
    {
  *(.stack_magic)
        _cpu0_sstack_begin = .;
        PROVIDE(cpu0_sstack_begin = .);
        *(.stack)
        _cpu0_sstack_end = .;
        PROVIDE(cpu0_sstack_end = .);
     _stack_end = . ;
  *(.stack_magic0)

  . = ALIGN(4);
  *(.boot_info)

    } > ram0

    .bss ALIGN(32) :
    {
        *(.usb_h_dma)
        *(.usb_ep0)
        *(.dec_mix_buff)
        *(.sd0_var)
        *(.sd1_var)
        *(.dac_buff)

        *(.bss)
        *(COMMON)
  *(.cvsd_bss)

        *(.volatile_ram)


        btctler_bss_start = .;

        BTCTLER_CONTROLLER_BSS_START = .;

        *(.bd_base)
        *(.bredr_rxtx_bulk)
        acl_tx_pool = .;
        *(.bredr_tx_bulk)

        acl_tx_pool_end = acl_tx_pool + (3 * 1024);



        . = acl_tx_pool_end;

        acl_rx_pool = .;
        *(.bredr_rx_bulk)

        acl_rx_pool_end = acl_rx_pool + (12 * 1024);



        . = acl_rx_pool_end;

        tws_bulk_pool = .;



        tws_bulk_pool_end = tws_bulk_pool;

        . = tws_bulk_pool_end;

        *(.bt_rf_bss)
  *(.hci_controller_bss)
  *(.hci_interface_bss)
  *(.device_manager_bss)
  *(.vendor_manager_bss)
        BTCTLER_CONTROLLER_BSS_SIZE = ABSOLUTE(. - BTCTLER_CONTROLLER_BSS_START);

        BTCTLER_LE_CONTROLLER_BSS_START = .;
        *(.ble_hci_bss)
        *(.ble_ll_bss)
        *(.ble_rf_bss)
        BTCTLER_LE_CONTROLLER_BSS_SIZE = ABSOLUTE(. - BTCTLER_LE_CONTROLLER_BSS_START);

        BTCTLER_CL_BSS_START = .;
        *(.classic_rf_bss)
        *(.classic_lmp_bss)
        *(.classic_lmp_auth_bss)
        *(.classic_lmp_bigint_bss)
        *(.classic_lmp_crypt_bss)
        *(.classic_lmp_ecdh_bss)
        *(.classic_lmp_linkbulk_bss)
        *(.classic_lmp_hmac_bss)
        *(.classic_bb_bss)
        *(.classic_hci_bss)
        BTCTLER_CL_BSS_SIZE = ABSOLUTE(. - BTCTLER_CL_BSS_START);

        btctler_bss_end = .;


        btstack_bss_start = .;
        *(.bt_stack_bss)
        *(.ble_db_bss)
        *(.ble_sm_bss)
        *(.ble_att_bss)
        *(.ble_gatt_bss)
        *(.btstack_pool)


        BTSTACK_LE_HOST_MESH_BSS_START = .;
        *(.ble_mesh_bss)
        *(.ble_mesh_tinycrypt_bss)
        BTSTACK_LE_HOST_MESH_BSS_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_BSS_START);

        btstack_bss_end = .;


        system_bss_start = .;
        . = ALIGN(4);
        *(.os_bss)
        *(.mem_heap)
  *(.memp_memory_x)
        *(.mem_bss)
        *(.os_port_bss)

        *(.uECC_bss)
        *(.ECDH_sample_bss)

        system_bss_end = .;


        media_bss_start = .;
        . = ALIGN(4);
  *(.aec_bss)
  *(.res_bss)
  *(.ns_bss)
  *(.der_bss)
  *(.nlp_bss)
  *(.qmf_bss)
  *(.bt_audioplc_bss)
  *(.fft_bss)
  *(.sms_bss)
        *(.pcm_bss)

  *(.g729_bss)
  *(.g726_bss)
  *(.wtg_dec_bss)
  *(.wtgv2_bss)
  *(.wtgv2dec_bss)
  *(.mp3_bss)
  *(.mp3_dec_bss)
  *(.msbc_bss)
  *(.mty_bss)
  *(.mp3tsy_dec_bss)
  *(.sbc_bss)
  *(.sine_bss)

  *(.wma_bss)
  *(.wma_dec_bss)

  *(.amr_bss)
  *(.midi_bss)

  *(.audio_decoder_bss)



        *(.mp3_encode_bss)
  *(.media_device_bss)
  *(.audio_encoder_bss)
  *(.mixer_bss)
  *(.stream_bss)
  *(.dec_server_bss)
  *(.rec_server_bss)
  *(.auto_mute_bss)
  *(.plc_bss)
  *(.wireless_sync_bss)

  *(.audio_buf)

  *(.resample_cal_bss)
  *(.bt_crossOver_bss)
  *(.bt_compressor_bss)
  *(.bt_limiter_bss)
  *(.bt_limiter_sparse_bss)
  *(.bt_crossOver_sparse_bss)
  *(.bt_compressor_sparse_bss)

  *(.iap_bss)
  *(.audio_bfilt_bss)
  *(.audio_buf_sync_bss)
  *(.audio_dec_app_bss)
  *(.audio_dec_app_sync_bss)
  *(.audio_dig_vol_bss)
  *(.audio_echo_reverb_bss)
  *(.audio_echo_src_bss)
  *(.audio_energy_detect_bss)
  *(.audio_eq_bss)
  *(.audio_drc_bss)
  *(.audio_eq_drc_apply_bss)
  *(.audio_equalloudness_bss)
  *(.audio_howling_bss)
  *(.audio_noisegate_bss)
  *(.audio_output_dac_bss)
  *(.audio_pemafrow_bss)
  *(.audio_pitch_bss)
  *(.audio_pitchspeed_bss)
  *(.audio_surround_bss)
  *(.audio_vbass_bss)
  *(.audio_vocal_remove_bss)
  *(.audio_vocal_tract_synthesis_bss)
  *(.eq_config_bss)
  *(.eq_config_protocol_bss)
  *(.spectrum_eq_bss)
  *(.spectrum_fft_bss)
  *(.audio_localtws_bss)
  *(.audio_dec_localtws_bss)
  *(.audio_dec_a2dp_bss)
  *(.audio_dec_esco_bss)
  *(.audio_dec_file_bss)
  *(.audio_dec_pcm_bss)
  *(.audio_resample_bss)
  *(.audio_splicing_bss)
  *(.channel_switch_bss)
  *(.sw_drc_bss)
  *(.aptx_decoder_bss)
  *(.ldac_decoder_bss)
  *(.speex_encoder_bss)
  *(.sbc_encoder_bss)
  *(.opus_encoder_bss)
  *(.prevent_task_fill_bss)
  *(.audio_cfifo_bss)
  *(.audio_track_bss)
  *(.audio_adc_bss)
  *(.audio_dac_bss)
  *(.audio_linein_bss)
  *(.audio_link_bss)
  *(.audio_sample_sync_bss)
  *(.audio_wireless_sync_bss)
  *(.audio_spdif_bss)
  *(.audio_src_bss)
  *(.audio_src_base_bss)
  *(.effectrs_sync_bss)
  *(.eq_design_bss)
  *(.hw_eq_bss)
  *(.iic_bss)
  *(.mic_cap_bss)
  *(.sbc_hwaccel_bss)
  *(.resample_fastcal_bss)
        media_bss_end = .;


  . = (( . + 31) / 32 * 32);

  . = ALIGN(4);
    } > ram0

    _cpu_store_end = . ;

    _prp_store_begin = . ;
    .prp_bss ALIGN(32) :
    {





  . = (( . + 31) / 32 * 32);
    } > ram0
    _prp_store_end = . ;

    .bss ALIGN(32) :
 {
  NVRAM_DATA_START = .;
  *(.non_volatile_ram)
  NVRAM_DATA_SIZE = ABSOLUTE(. - NVRAM_DATA_START);
  . = ALIGN(4);
  NVRAM_END = .;
  _nv_pre_begin = . ;

        *(.src_filt)
        *(.src_dma)
  . = ALIGN(4);

    } > ram0

 overlay_begin = .;
 OVERLAY : NOCROSSREFS AT(0x200000) SUBALIGN(4)
    {

  .overlay_aec
  {
   LONG(0xFFFFFFFF);

   o_aec_end = .;

   *(.aec_mem)
            *(.msbc_enc)
  }
  .overlay_mp3
  {
   LONG(0xFFFFFFFF);

   o_mp3_end = .;

   *(.mp3_mem)
   *(.mp3_ctrl_mem)
   *(.mp3pick_mem)
   *(.mp3pick_ctrl_mem)
  }
  .overlay_wma
  {
   LONG(0xFFFFFFFF);

   o_wma_end = .;

   *(.wma_mem)
   *(.wma_ctrl_mem)
   *(.wmapick_mem)
   *(.wmapick_ctrl_mem)
  }
  .overlay_wav
  {
   *(.wav_dec_sparse_code)
   *(.wav_dec_sparse_const)

   *(.wav_dec_code)
   *(.wav_dec_const)
   *(.wav_dec_data)

   *(.wav_data)
   *(.wav_const)
   *(.wav_code)


   LONG(0xFFFFFFFF);


   *(.wav_bss)
   *(.wav_dec_bss)
   *(.wav_mem)
   *(.wav_ctrl_mem)
  }

  .overlay_ape
        {
            *(.ape_mem)
            *(.ape_ctrl_mem)

   *(.ape_dec_sparse_code)
   *(.ape_dec_sparse_const)

   *(.ape_dec_code)
   *(.ape_dec_const)
   *(.ape_dec_data)
   *(.ape_dec_bss)

   LONG(0xFFFFFFFF);


   *(.ape_bss)
   *(.ape_data)
   *(.ape_const)
   *(.ape_code)

  }
  .overlay_flac
        {
   *(.flac_dec_sparse_code)
   *(.flac_dec_sparse_const)

   *(.flac_dec_code)
   *(.flac_dec_const)
   *(.flac_dec_data)

   *(.flac_data)
   *(.flac_const)
   *(.flac_code)

   LONG(0xFFFFFFFF);


            *(.flac_mem)
            *(.flac_ctrl_mem)
   *(.flac_dec_bss)
   *(.flac_bss)
  }

  .overlay_m4a
        {
   *(.m4a_dec_code)
   *(.m4a_dec_ff_const)
   *(.m4a_dec_const)
   *(.m4a_dec_data)

   *(.m4a_data)
   *(.m4a_const)
   *(.m4a_code)
   *(.m4apick_mem)
   *(.m4apick_ctrl_mem)

   LONG(0xFFFFFFFF);


            *(.m4a_mem)
            *(.m4a_ctrl_mem)
   *(.m4a_dec_bss)
   *(.m4a_bss)
   *(.aac_ctrl_mem)
   *(.aac_bss)

   *(.alac_ctrl_mem)
   *(.alac_bss)
  }


  .overlay_amr
        {
   *(.amr_dec_sparse_code)
   *(.amr_dec_sparse_const)
   *(.amr_dec_code)

   *(.amr_dec_data)

   LONG(0xFFFFFFFF);


            *(.amr_mem)
            *(.amr_ctrl_mem)
   *(.amr_dec_bss)
  }
  .overlay_dts
        {
   *(.dts_dec_code)
   *(.dts_dec_data)

   *(.dts_data)
   *(.dts_const)
   *(.dts_code)

   LONG(0xFFFFFFFF);


            *(.dts_mem)
            *(.dts_ctrl_mem)
   *(.dts_dec_bss)
   *(.dts_bss)

  }



  .overlay_fm
  {
   LONG(0xFFFFFFFF);

   o_fm_end = .;

   *(.fm_mem)
   *(.linein_pcm_mem)
  }
        .overlay_pc
  {

            *(.usb_audio_play_dma)
            *(.usb_audio_rec_dma)
            *(.uac_rx)
            *(.mass_storage)

            *(.usb_msd_dma)
            *(.usb_hid_dma)
            *(.usb_iso_dma)
            *(.usb_cdc_dma)
            *(.uac_var)
            *(.usb_config_var)
            *(.cdc_var)

  }
    } > ram0

 overlay_end = .;

 ASSERT(overlay_end <= overlay_begin + 64k, "overlay overflow 64k!")

    RAM_USED = .;


 . =ORIGIN(ram1);

    .mmu_tlb ALIGN(0x4000):
    {
        *(.mmu_tlb_segment);
    } > ram1


    .bss1 ALIGN(32) :
 {
    } > ram1
    RAM1_USED = .;
}





SECTIONS
{
    .data : ALIGN(4)
    {
        update_data_start = .;

        update_data_end = .;
    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        update_bss_start = .;

        *(.update_bss)
        update_bss_end = .;
    } > ram0

    .text : ALIGN(4)
    {
  update_code_start = .;

  *(.bt_updata_ram_code)
  *(.update_const)
  *(.update_code)

  update_code_end = .;
    } > code0


    UPDATE_CODE_TOTAL_SIZE = update_code_end - update_code_start;
}

SECTIONS
{
    .data : ALIGN(4)
    {
        driver_data_start = .;

        CLOCK_DATA_START = .;
        *(.clock_data)
        CLOCK_DATA_SIZE = ABSOLUTE(. - CLOCK_DATA_START);

        *(.debug_data)
        *(.power_data)

        *(.uart_data)


        driver_data_end = .;

    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        driver_bss_start = .;

        CLOCK_BSS_START = .;
        *(.clock_bss)
        CLOCK_BSS_SIZE = ABSOLUTE(. - CLOCK_BSS_START);

        *(.debug_bss)
        *(.power_bss)

        *(.uart_bss)

  *(.sd_var)

        driver_bss_end = .;
    } > ram0

    .text : ALIGN(4)
    {
        driver_code_start = .;

  . = ALIGN(4);
        _SPI_CODE_START = . ;
        *(.spi_code)
  . = ALIGN(4);
        _SPI_CODE_END = . ;

        clock_critical_handler_begin = .;
        KEEP(*(.clock_critical_txt))
        clock_critical_handler_end = .;


        CLOCK_CODE_START = .;
        *(.clock_code)
        *(.clock_const)
        CLOCK_CODE_SIZE = ABSOLUTE(. - CLOCK_CODE_START);

        *(.debug_code)
        *(.debug_const)

        *(.power_code)
        *(.power_const)

        *(.uart_code)
        *(.uart_const)

        driver_code_end = .;
  . = ALIGN(4);
    } > code0


    DRIVER_RAM_TOTAL = (driver_data_end - driver_data_start) + (driver_bss_end - driver_bss_start);
    DRIVER_CODE_TOTAL = (driver_code_end - driver_code_start);
}


text_begin = ADDR(.text) ;
text_size = SIZEOF(.text) ;
text_end = ADDR(.text) + SIZEOF(.text) ;

bss_begin = ADDR(.bss) ;
bss_size = SIZEOF(.bss);

bss1_begin = ADDR(.bss1) ;
bss1_size = SIZEOF(.bss1);

data_addr = ADDR(.data) ;
data_begin = text_begin + text_size;
data_size = SIZEOF(.data) ;

psram_vaddr = ADDR(.psram_text) ;
psram_laddr = text_begin + text_size + data_size;
psram_text_size = SIZEOF(.psram_text) ;

bank_code_load_addr = data_begin + data_size;




aec_addr = ADDR(.overlay_aec);
aec_begin = text_begin + text_size + data_size;
aec_size = o_aec_end - aec_addr;


wav_addr = ADDR(.overlay_wav);
wav_begin = aec_begin + aec_size;
wav_size = SIZEOF(.overlay_wav);

ape_addr = ADDR(.overlay_ape);
ape_begin = wav_begin + wav_size;
ape_size = SIZEOF(.overlay_ape);

flac_addr = ADDR(.overlay_flac);
flac_begin = ape_begin + ape_size;
flac_size = SIZEOF(.overlay_flac);

m4a_addr = ADDR(.overlay_m4a);
m4a_begin = flac_begin + flac_size;
m4a_size = SIZEOF(.overlay_m4a);

amr_addr = ADDR(.overlay_amr);
amr_begin = m4a_begin + m4a_size;
amr_size = SIZEOF(.overlay_amr);

dts_addr = ADDR(.overlay_dts);
dts_begin = amr_begin + amr_size;
dts_size = SIZEOF(.overlay_dts);

fm_addr = ADDR(.overlay_fm);
fm_begin = dts_begin + dts_size;
fm_size = o_fm_end - fm_addr;
_HEAP_BEGIN = RAM_USED;
PROVIDE(HEAP_BEGIN = RAM_USED);

_HEAP_END = RAM_END;
PROVIDE(HEAP_END = RAM_END);

_HEAP_SIZE = HEAP_END - HEAP_BEGIN;
PROVIDE(HEAP_SIZE = HEAP_END - HEAP_BEGIN);

_HEAP1_BEGIN = RAM1_USED;
PROVIDE(HEAP1_BEGIN = RAM1_USED);

_HEAP1_END = RAM1_END;
PROVIDE(HEAP1_END = RAM1_END);

_HEAP1_SIZE = HEAP1_END - HEAP1_BEGIN;
PROVIDE(HEAP1_SIZE = HEAP1_END - HEAP1_BEGIN);

_MALLOC_SIZE = HEAP_SIZE + HEAP1_SIZE;
PROVIDE(MALLOC_SIZE = HEAP_SIZE + HEAP1_SIZE);
