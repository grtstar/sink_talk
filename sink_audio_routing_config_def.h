
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_audio_routing_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_AUDIO_ROUTING_CONFIG_DEF_H_
#define _SINK_AUDIO_ROUTING_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned mic_bias_0_voltage;
    unsigned mic_bias_1_voltage;
} mic_bias_drive_settings_t;

typedef struct {
    unsigned voice_quality;
    unsigned music_quality;
} audio_quality_settings_t;

typedef struct {
    unsigned spdif_supported_data_types:4;
    unsigned dut_input:3;
    unsigned audio_input_routing:2;
    unsigned manual_source_selection:1;
    unsigned force_resampling_of_tones:1;
    unsigned use_one_mic_back_channel:1;
    unsigned use_two_mic_back_channel:1;
    unsigned enableMixingOfVoiceAndAudio:1;
    unsigned padding:2;
} AudioPluginFeatures_t;

typedef struct {
    unsigned threshold;
    unsigned trigger_time;
} silence_detect_settings_t;

typedef struct {
    audio_mic_params_t mic_1a;
    audio_mic_params_t mic_1b;
    audio_mic_params_t mic_2a;
    audio_mic_params_t mic_2b;
    audio_mic_params_t mic_3a;
    audio_mic_params_t mic_3b;
} common_mic_params_t_t;

typedef struct {
    analogue_input_params_t analogue_in;
} common_analogue_params_t;

typedef struct {
    unsigned filter;
    unsigned freq;
    unsigned gain;
    unsigned Q;
} user_eq_band_t;

typedef struct {
    unsigned preGain;
    user_eq_band_t bands[5];
} user_eq_bank_t;

#define SINK_AUDIO_READONLY_CONFIG_BLK_ID 539

typedef struct {
    audio_quality_settings_t audio_quality;
    AudioPluginFeatures_t PluginFeatures;
    mic_bias_drive_settings_t mic_bias_drive_voltage;
    common_mic_params_t_t common_mic_params;
    analogue_input_params_t analogue_input;
    silence_detect_settings_t SilenceDetSettings;
    unsigned AudioAmpUnmuteTime_ms;
    unsigned AudioAmpMuteTime_ms;
    unsigned audio_switch_delay_after_disconn;
    unsigned TwsAudioActiveDelay_ms;
    unsigned AudioActivePIO:8;
    unsigned aux_out_detect:8;
    unsigned PowerOnPIO:8;
    unsigned AudioMutePIO:8;
    unsigned voice_mic_a:4;
    unsigned voice_mic_b:4;
    unsigned seqSourcePriority1:4;
    unsigned seqSourcePriority2:4;
    unsigned seqSourcePriority3:4;
    unsigned seqSourcePriority4:4;
    unsigned seqSourcePriority5:4;
    unsigned seqSourcePriority6:4;
    unsigned seqSourcePriority7:4;
    unsigned defaultSource:3;
    unsigned TwsMasterAudioRouting:2;
    unsigned TwsSlaveAudioRouting:2;
    unsigned UseLowPowerAudioCodecs:1;
    unsigned AssumeAutoSuspendOnCall:1;
    unsigned AudioRenderingMode:1;
    unsigned padding:2;
} sink_audio_readonly_config_def_t;

#define SINK_AUDIO_WRITEABLE_CONFIG_BLK_ID 572

typedef struct {
    unsigned requested_audio_source;
    user_eq_bank_t PEQ;
} sink_audio_writeable_config_def_t;

#endif /* _SINK_AUDIO_ROUTING_CONFIG_DEF_H_ */
