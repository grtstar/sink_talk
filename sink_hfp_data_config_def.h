
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_hfp_data_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_HFP_DATA_CONFIG_DEF_H_
#define _SINK_HFP_DATA_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned event;
    unsigned at_cmd;
} at_commands_events_t;

typedef struct {
    unsigned data;
} at_command_data_t;

typedef struct {
    unsigned supported_codecs:3;
    unsigned codec_bandwidths:2;
    unsigned caller_name:1;
    unsigned raw_text:1;
    unsigned sms:1;
    unsigned batt_level:1;
    unsigned pwr_source:1;
    unsigned padding:6;
} hfp_qtil_features_t;

typedef struct {
    unsigned service:2;
    unsigned signal_strength:2;
    unsigned roaming_status:2;
    unsigned battery_charge:2;
    unsigned padding:8;
} hfp_optional_indicators_t;

typedef struct {
    unsigned supported_wbs_codecs_def;
    unsigned hf_indicators_mask_def;
    hfp_qtil_features_t qtil_supported_features_def;
    hfp_optional_indicators_t optional_indicators_def;
    unsigned supported_features_def:9;
    unsigned supported_profile_def:3;
    unsigned disable_nrec_def:1;
    unsigned extended_errors_def:1;
    unsigned multipoint_def:1;
    unsigned padding:1;
    unsigned link_loss_time_def:8;
    unsigned link_loss_interval_def:8;
} hfp_init_params_t;

typedef struct {
    unsigned bandwidth_low;
    unsigned bandwidth_high;
    unsigned max_latency;
    unsigned voice_settings;
    unsigned retransmission_effort;
} hfp_audio_params_t;

typedef struct {
    unsigned audio_plugin:4;
    unsigned ActionOnCallTransfer:2;
    unsigned AutoAnswerOnConnect:1;
    unsigned EnableSyncMuteMicrophones:1;
    unsigned LNRCancelsVoiceDialIfActive:1;
    unsigned ForceEV3S1ForSco2:1;
    unsigned padding:6;
} sink_hfp_features_config_t;

typedef struct {
    unsigned MuteRemindTime;
    unsigned NwsServiceIndicatorRepeatTime_s;
    unsigned MissedCallIndicateTime_s;
    unsigned MissedCallIndicateAttempts;
} sink_hfp_feature_timeouts_t;

typedef struct {
    unsigned CallActivePIO:8;
    unsigned IncomingRingPIO:8;
    unsigned OutgoingRingPIO:8;
    unsigned padding:8;
} sink_hfp_pio_data_t;

typedef struct {
    hfp_audio_params_t audio_params;
    unsigned packet_types:10;
    unsigned additional_audio_params:1;
    unsigned padding:5;
} HFP_features_type_t;

#define SINK_HFP_DATA_READONLY_CONFIG_BLK_ID 46

typedef struct {
    sink_hfp_features_config_t hfp_features_config;
    sink_hfp_feature_timeouts_t hfp_feature_timeouts;
    sink_hfp_pio_data_t hfp_pio_data;
    HFP_features_type_t hfp_supported_features;
    hfp_init_params_t hfp_init_params;
    unsigned hfp_default_volume:4;
    unsigned padding:12;
} sink_hfp_data_readonly_config_def_t;

#define SINK_HFP_AT_COMMANDS_CONFIG_BLK_ID 70

typedef struct {
    at_command_data_t at_commands[1];
} sink_hfp_at_commands_config_def_t;

#define SINK_HFP_EVENT_AT_COMMAND_TABLE_CONFIG_BLK_ID 74

typedef struct {
    at_commands_events_t event_at_commands[1];
} sink_hfp_event_at_command_table_config_def_t;

#define SINK_HFP_DATA_WRITEABLE_CONFIG_BLK_ID 78

typedef struct {
    unsigned phone_number[10];
} sink_hfp_data_writeable_config_def_t;

#endif /* _SINK_HFP_DATA_CONFIG_DEF_H_ */
