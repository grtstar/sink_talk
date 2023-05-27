###########################################################
# Makefile generated by xIDE                               
#                                                          
# Project: headset
# Configuration: Headset-Gaming
# Generated: ���� 5�� 27 21:48:37 2023
#                                                          
# WARNING: Do not edit this file. Any changes will be lost 
#          when the project is rebuilt.                    
#                                                          
###########################################################

OUTPUT=headset
OUTDIR=C:/ADK_CSR867x.WIN.4.4.0.21/apps/sink-talk
HARDWARE_INDEX=4
DEFS=-DENABLE_BATTERY_OPERATION -DDEVICE_ID_PSKEY -DGATT_ENABLED -DENABLE_AVRCP -DINCLUDE_CVC -DTHREE_WAY_CALLING -DENABLE_USB -DCOPY_USB_MS_README -DENABLE_PBAP -DENABLE_GAIA -DINCLUDE_A2DP_EXTRA_CODECS -DINCLUDE_FASTSTREAM -DENABLE_UPGRADE -DENABLE_MULTI_TALK -DENABLE_AG -DENABLE_WATCHDOG -DENABLE_AHI_SPI -DTEST_HF_INDICATORS -DENABLE_AHI_TEST_WRAPPER 

DEBUGTRANSPORT=SPITRANS=USB SPIPORT=0
SW_VARIANT=Headset-Gaming
HW_VARIANT=CNS10001v4
EXECUTION_MODE=hw_default
STACKSIZE=0
TRANSPORT=raw
FIRMWARE=unified
HARDWARE=gordon
PANIC_ON_PANIC=1
FIRMWAREIMAGE=
LIBRARY_VERSION=
GENERATE_MAP=1

LIBS=-laudio_plugin_common -lps_dynamic_config_store -lahi -lconfig_store -ldisplay_plugin_cns10010_scroll -ldisplay_example_plugin -ldisplay -lavrcp -lpower -lpower_onchip -lpower_onchip_adapter -lconnection -lbdaddr -lhfp_min_cfm -lregion -lservice -lgain_utils -laudio -lcsr_cvc_common_plugin_all -lcsr_voice_prompts_plugin -lcsr_a2dp_decoder_common_plugin_stereo_bidir -la2dp -lobex -lsdp_parse -lmd5 -lusb_device_class -lspp_common -lspps -lsppc -lpblock -lcsr_dut_audio_plugin -lpio_common -lpbapc -lmapc -lfm_rx_plugin -lfm_rx_api -lswat -lcsr_i2s_audio_plugin -lgaia -lgatt -lgatt_imm_alert_client -lgatt_apple_notification_client -lgatt_battery_client -lgatt_device_info_client -lgatt_hid_client -lbyte_utils -lupgrade -lgatt_manager -lgatt_battery_server -lgatt_server -lgatt_gap_server -lgatt_transmit_power_server -lgatt_imm_alert_server -lgatt_link_loss_server -laudio_output -lgatt_client -lgatt_scan_params_client -lgatt_heart_rate_server -lgatt_device_info_server -lgatt_heart_rate_client -lvmal -laudio_config -lbroadcast -lscm -lbroadcast_stream_service_record -laudio_plugin_music_variants -laudio_instance -lhid_upgrade -lhid -laudio_output_unused -laudio_plugin_voice_variants -lrwcp_server -lcsr_voice_assistant_plugin -lvoice_assistant_packetiser -llocal_device -ltws_synchronised_control 
INPUTS=\
      headset.mak\
      csr_cvc_common_plugin/csr_cvc_common.c\
      csr_cvc_common_plugin/csr_cvc_common.h\
      csr_cvc_common_plugin/csr_cvc_common_ctx.h\
      csr_cvc_common_plugin/csr_cvc_common_dsp_if.c\
      csr_cvc_common_plugin/csr_cvc_common_dsp_if.h\
      csr_cvc_common_plugin/csr_cvc_common_dsp_message_handler.c\
      csr_cvc_common_plugin/csr_cvc_common_dsp_message_handler.h\
      csr_cvc_common_plugin/csr_cvc_common_if.h\
      csr_cvc_common_plugin/csr_cvc_common_io_if.c\
      csr_cvc_common_plugin/csr_cvc_common_io_if.h\
      csr_cvc_common_plugin/csr_cvc_common_plugin.c\
      csr_cvc_common_plugin/csr_cvc_common_plugin.h\
      csr_cvc_common_plugin/csr_cvc_common_state_kick.c\
      csr_cvc_common_plugin/csr_cvc_common_state_machine.c\
      csr_cvc_common_plugin/csr_cvc_common_state_machine.h\
      connection/bluestack_handler.c\
      connection/bluestack_handler.h\
      connection/common.c\
      connection/common.h\
      connection/connection.h\
      connection/connection_no_ble.h\
      connection/connection_private.h\
      connection/connection_shim.c\
      connection/connection_shim.h\
      connection/connection_tdl.c\
      connection/connection_tdl.h\
      connection/ConnectionGetRole.c\
      connection/ConnectionInquire.c\
      connection/ConnectionInquireCancel.c\
      connection/ConnectionL2capUnregisterRequest.c\
      connection/ConnectionReadBleAdChanTxPwr.c\
      connection/ConnectionReadBtVersion.c\
      connection/ConnectionReadClassOfDevice.c\
      connection/ConnectionReadEirData.c\
      connection/ConnectionReadInquiryMode.c\
      connection/ConnectionReadInquiryTx.c\
      connection/ConnectionReadRemoteName.c\
      connection/ConnectionReadTxPower.c\
      connection/ConnectionSdpAttributeSearchRequest.c\
      connection/ConnectionSdpTerminatePrimitiveRequest.c\
      connection/ConnectionSmAddAuthDevice.c\
      connection/ConnectionSmChangeLinkKey.c\
      connection/ConnectionSmEncryptionKeyRefresh.c\
      connection/ConnectionSmGetAuthDevice.c\
      connection/ConnectionSmRegisterOutgoingService.c\
      connection/ConnectionSmSendKeypressNotificationRequest.c\
      connection/ConnectionSmSetSdpSecurityOut.c\
      connection/ConnectionSmSetSecurityLevel.c\
      connection/ConnectionSmSetTrustLevel.c\
      connection/ConnectionSmUnRegisterOutgoingService.c\
      connection/ConnectionSmUpdateMruDevice.c\
      connection/ConnectionSyncRenegotiate.c\
      connection/ConnectionSyncUnregister.c\
      connection/ConnectionWriteInquiryAccessCode.c\
      connection/ConnectionWriteInquiryscanActivity.c\
      connection/ConnectionWriteInquiryTx.c\
      connection/ConnectionWritePagescanActivity.c\
      connection/dm_acl.c\
      connection/dm_bad_message_handler.c\
      connection/dm_bad_message_handler.h\
      connection/dm_baseband_cache.c\
      connection/dm_baseband_cod.c\
      connection/dm_baseband_flushtimeout.c\
      connection/dm_baseband_handler.c\
      connection/dm_baseband_handler.h\
      connection/dm_baseband_name.c\
      connection/dm_baseband_scan.c\
      connection/dm_ble_advertising.c\
      connection/dm_ble_advertising.h\
      connection/dm_ble_channel_selection_algorithm.c\
      connection/dm_ble_channel_selection_algorithm.h\
      connection/dm_ble_connection_parameters_update.c\
      connection/dm_ble_connection_parameters_update.h\
      connection/dm_ble_handler.c\
      connection/dm_ble_handler.h\
      connection/dm_ble_latency.c\
      connection/dm_ble_latency.h\
      connection/dm_ble_phy_preferences.c\
      connection/dm_ble_phy_preferences.h\
      connection/dm_ble_random_address.c\
      connection/dm_ble_random_address.h\
      connection/dm_ble_scanning.c\
      connection/dm_ble_security.c\
      connection/dm_ble_security.h\
      connection/dm_ble_set_connection_parameters.c\
      connection/dm_ble_white_list.c\
      connection/dm_crypto_api.c\
      connection/dm_crypto_api_handler.c\
      connection/dm_crypto_api_handler.h\
      connection/dm_csb.c\
      connection/dm_csb_handler.c\
      connection/dm_csb_handler.h\
      connection/dm_dut.c\
      connection/dm_dut_handler.c\
      connection/dm_dut_handler.h\
      connection/dm_info_clock_offset.c\
      connection/dm_info_handler.c\
      connection/dm_info_handler.h\
      connection/dm_info_link_quality.c\
      connection/dm_info_local_addr.c\
      connection/dm_info_local_version.c\
      connection/dm_info_remote_version.c\
      connection/dm_info_rssi.c\
      connection/dm_info_supp_features.c\
      connection/dm_init.c\
      connection/dm_init.h\
      connection/dm_inquiry.c\
      connection/dm_inquiry_handler.c\
      connection/dm_inquiry_handler.h\
      connection/dm_link_policy.c\
      connection/dm_link_policy_handler.c\
      connection/dm_link_policy_handler.h\
      connection/dm_page_timeout.c\
      connection/dm_page_timeout_handler.c\
      connection/dm_page_timeout_handler.h\
      connection/dm_read_ble_ad_chan_tx_pwr.c\
      connection/dm_read_ble_ad_chan_tx_pwr.h\
      connection/dm_read_tx_power_handler.c\
      connection/dm_read_tx_power_handler.h\
      connection/dm_security_attribute.c\
      connection/dm_security_authenticate.c\
      connection/dm_security_authorise.c\
      connection/dm_security_encrypt.c\
      connection/dm_security_handler.c\
      connection/dm_security_handler.h\
      connection/dm_security_init.c\
      connection/dm_security_init.h\
      connection/dm_security_mode.c\
      connection/dm_security_oob.c\
      connection/dm_security_sdp.c\
      connection/dm_security_service.c\
      connection/dm_sync.c\
      connection/dm_sync_handler.c\
      connection/dm_sync_handler.h\
      connection/init.c\
      connection/init.h\
      connection/l2cap.c\
      connection/l2cap_handler.c\
      connection/l2cap_handler.h\
      connection/l2cap_init.c\
      connection/l2cap_init.h\
      connection/rfc.c\
      connection/rfc_handler.c\
      connection/rfc_handler.h\
      connection/rfc_init.c\
      connection/rfc_init.h\
      connection/sdp_handler.c\
      connection/sdp_handler.h\
      connection/sdp_init.c\
      connection/sdp_init.h\
      connection/sdp_mtu.c\
      connection/sdp_register.c\
      connection/sdp_search.c\
      connection/sdp_service_attr_search.c\
      connection/tcp.c\
      connection/tcp_handler.c\
      connection/tcp_handler.h\
      connection/tcp_init.c\
      connection/tcp_init.h\
      connection/udp.c\
      connection/udp_handler.c\
      connection/udp_handler.h\
      connection/udp_init.c\
      connection/udp_init.h\
      aghfp/aghfp.h\
      aghfp/aghfp_audio.c\
      aghfp/aghfp_audio_handler.c\
      aghfp/aghfp_audio_handler.h\
      aghfp/aghfp_call.c\
      aghfp/aghfp_call.h\
      aghfp/aghfp_call_manager.c\
      aghfp/aghfp_call_manager.h\
      aghfp/aghfp_common.c\
      aghfp/aghfp_common.h\
      aghfp/aghfp_csr_features.c\
      aghfp/aghfp_csr_features.h\
      aghfp/aghfp_hf_indicators.c\
      aghfp/aghfp_hf_indicators.h\
      aghfp/aghfp_hfp_service_record.h\
      aghfp/aghfp_hsp_service_record.h\
      aghfp/aghfp_indicators.c\
      aghfp/aghfp_indicators.h\
      aghfp/aghfp_init.c\
      aghfp/aghfp_init.h\
      aghfp/aghfp_link_manager.c\
      aghfp/aghfp_misc_handler.c\
      aghfp/aghfp_misc_handler.h\
      aghfp/aghfp_ok.c\
      aghfp/aghfp_ok.h\
      aghfp/aghfp_parse.c\
      aghfp/aghfp_parse.h\
      aghfp/aghfp_parse_handler.c\
      aghfp/aghfp_private.h\
      aghfp/aghfp_profile_handler.c\
      aghfp/aghfp_profile_handler.h\
      aghfp/aghfp_receive_data.c\
      aghfp/aghfp_receive_data.h\
      aghfp/aghfp_rfc.c\
      aghfp/aghfp_rfc.h\
      aghfp/aghfp_sdp.c\
      aghfp/aghfp_sdp.h\
      aghfp/aghfp_send_data.c\
      aghfp/aghfp_send_data.h\
      aghfp/aghfp_shim.c\
      aghfp/aghfp_shim.h\
      aghfp/aghfp_slc.c\
      aghfp/aghfp_slc_handler.c\
      aghfp/aghfp_slc_handler.h\
      aghfp/aghfp_user_data.c\
      aghfp/aghfp_user_data_handler.c\
      aghfp/aghfp_user_data_handler.h\
      aghfp/aghfp_wbs.c\
      aghfp/aghfp_wbs.h\
      aghfp/aghfp_wbs_handler.c\
      aghfp/aghfp_wbs_handler.h\
      sink_gatt_client_ancs.c\
      main.c\
      sink_a2dp.c\
      sink_a2dp_capabilities.c\
      sink_anc.c\
      sink_at_commands.c\
      sink_audio.c\
      sink_audio_prompts.c\
      sink_audio_routing.c\
      sink_auth.c\
      sink_auto_power_off.c\
      sink_avrcp.c\
      sink_avrcp_browsing.c\
      sink_avrcp_browsing_channel.c\
      sink_avrcp_qualification.c\
      sink_ble.c\
      sink_ble_advertising.c\
      sink_ble_gap.c\
      sink_ba_ble_gap.c\
      sink_ble_scanning.c\
      sink_buttonmanager.c\
      sink_buttons.c\
      sink_callmanager.c\
      sink_config.c\
      sink_configmanager.c\
      sink_csr_features.c\
      sink_device_id.c\
      sink_devicemanager.c\
      sink_display.c\
      sink_dut.c\
      sink_fm.c\
      sink_gaia.c\
      sink_gatt.c\
      sink_gatt_client.c\
      sink_gatt_client_ba.c\
      sink_gatt_client_battery.c\
      sink_gatt_client_dis.c\
      sink_gatt_client_gatt.c\
      sink_gatt_client_hid.c\
      sink_gatt_client_hrs.c\
      sink_gatt_client_ias.c\
      sink_gatt_client_spc.c\
      sink_gatt_db.c\
      sink_gatt_hid_remote_control.c\
      sink_gatt_init.c\
      sink_gatt_manager.c\
      sink_gatt_server.c\
      sink_gatt_server_ba.c\
      sink_gatt_server_battery.c\
      sink_gatt_server_dis.c\
      sink_gatt_server_gap.c\
      sink_gatt_server_gatt.c\
      sink_gatt_server_hrs.c\
      sink_gatt_server_ias.c\
      sink_gatt_server_lls.c\
      sink_gatt_server_tps.c\
      sink_hf_indicators.c\
      sink_indicators.c\
      sink_init.c\
      sink_input_manager.c\
      sink_inquiry.c\
      sink_ir_remote_control.c\
      sink_led_manager.c\
      sink_leds.c\
      sink_link_policy.c\
      sink_linkloss.c\
      sink_malloc_debug.c\
      sink_mapc.c\
      sink_multi_channel.c\
      sink_multipoint.c\
      sink_music_processing.c\
      sink_nfc.c\
      sink_partymode.c\
      sink_pbap.c\
      sink_peer.c\
      sink_peer_qualification.c\
      sink_pio.c\
      sink_powermanager.c\
      sink_scan.c\
      sink_slc.c\
      sink_hfp_data.c\
      sink_speech_recognition.c\
      sink_statemanager.c\
      sink_swat.c\
      sink_tones.c\
      sink_upgrade.c\
      sink_usb.c\
      sink_utils.c\
      sink_volume.c\
      sink_wired.c\
      sink_gatt_common.c\
      sink_sc.c\
      sink_ble_sc.c\
      sink_audio_indication.c\
      sink_event_queue.c\
      sink_a2dp_capabilities.c\
      sink_watchdog.c\
      sink_gatt_hid_qualification.c\
      sink_private_data.c\
      ahi_host_usb.c\
      ahi_test.c\
      sink_ahi.c\
      ahi_host_spi.c\
      sink_hid.c\
      sink_ba.c\
      sink_ba_broadcaster.c\
      sink_ba_receiver.c\
      sink_main_task.c\
      sink_va.c\
      sink_spp_qualification.c\
      headset_uart.c\
      headset_multi_talk.c\
      csr_common_example_plugin.c\
      csr_common_example.c\
      audio.c\
      protocol_hex.c\
      headset_ag.c\
      audio_prompt.c\
      headset_friend_mode.c\
      headset_nearby_mode.c\
      headset_couple_mode.c\
      headset_multi_comm.c\
      headset_multi_pair.c\
      headset_multi_pair.h\
      sink_debug.h\
      sink_ble_gap.h\
      sink_a2dp.h\
      sink_anc.h\
      sink_at_commands.h\
      sink_audio.h\
      sink_audio_prompts.h\
      sink_audio_routing.h\
      sink_auth.h\
      sink_auto_power_off.h\
      sink_avrcp.h\
      sink_avrcp_browsing.h\
      sink_avrcp_browsing_channel.h\
      sink_avrcp_qualification.h\
      sink_ble.h\
      sink_ble_advertising.h\
      sink_ba_ble_gap.h\
      sink_ble_scanning.h\
      sink_buttonmanager.h\
      sink_buttons.h\
      sink_callmanager.h\
      sink_config.h\
      sink_configmanager.h\
      sink_csr_features.h\
      sink_development.h\
      sink_device_id.h\
      sink_devicemanager.h\
      sink_display.h\
      sink_dut.h\
      sink_events.h\
      sink_extendedstates.h\
      sink_fm.h\
      sink_gaia.h\
      sink_gatt.h\
      sink_gatt_client.h\
      sink_gatt_client_ancs.h\
      sink_gatt_client_ba.h\
      sink_gatt_client_battery.h\
      sink_gatt_client_dis.h\
      sink_gatt_client_gatt.h\
      sink_gatt_client_hid.h\
      sink_gatt_client_hrs.h\
      sink_gatt_client_ias.h\
      sink_gatt_client_spc.h\
      sink_gatt_db.h\
      sink_gatt_device.h\
      sink_gatt_hid_remote_control.h\
      sink_gatt_init.h\
      sink_gatt_manager.h\
      sink_gatt_server.h\
      sink_gatt_server_ba.h\
      sink_gatt_server_battery.h\
      sink_gatt_server_dis.h\
      sink_gatt_server_gap.h\
      sink_gatt_server_gatt.h\
      sink_gatt_server_hrs.h\
      sink_gatt_server_ias.h\
      sink_gatt_server_lls.h\
      sink_gatt_server_tps.h\
      sink_hf_indicators.h\
      sink_indicators.h\
      sink_init.h\
      sink_input_manager.h\
      sink_inquiry.h\
      sink_ir_remote_control.h\
      sink_led_manager.h\
      sink_leddata.h\
      sink_leds.h\
      sink_link_policy.h\
      sink_linkloss.h\
      sink_malloc_debug.h\
      sink_mapc.h\
      sink_multi_channel.h\
      sink_multipoint.h\
      sink_music_processing.h\
      sink_nfc.h\
      sink_partymode.h\
      sink_pbap.h\
      sink_peer.h\
      sink_peer_qualification.h\
      sink_pio.h\
      sink_powermanager.h\
      sink_private.h\
      sink_scan.h\
      sink_slc.h\
      sink_hfp_data.h\
      sink_speech_recognition.h\
      sink_statemanager.h\
      sink_states.h\
      sink_swat.h\
      sink_tones.h\
      sink_upgrade.h\
      sink_usb.h\
      sink_usb_descriptors.h\
      sink_utils.h\
      sink_volume.h\
      sink_wired.h\
      sink_gatt_common.h\
      sink_sc.h\
      sink_ble_sc.h\
      sink_audio_indication.h\
      sink_event_queue.h\
      sink_accessory.h\
      sink_watchdog.h\
      sink_gatt_hid_qualification.h\
      sink_private_data.h\
      ahi_host_usb.h\
      ahi_test.h\
      sink_ahi.h\
      ahi_host_spi.h\
      sink_hid.h\
      sink_hid_device_service_record.h\
      sink_ba_broadcaster.h\
      sink_ba.h\
      sink_ba_common.h\
      sink_ba_receiver.h\
      sink_main_task.h\
      sink_va.h\
      sink_spp_qualification.h\
      headset_uart.h\
      headset_multi_talk.h\
      protocol_hex.h\
      headset_ag.h\
      sink_gatt_db.db
# Project-specific options
ble_support=1
board=0
characters=1
config_storage=0
enable_ble=1
enable_broadcast_audio=0
messages=1
pcvalue=1
primitives=1
timestamps=1

-include headset.mak
include $(BLUELAB)/Makefile.vm
