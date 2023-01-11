
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_multi_channel_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_MULTI_CHANNEL_CONFIG_DEF_H_
#define _SINK_MULTI_CHANNEL_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    int audio_output_channel_volume_trim;
    unsigned audio_output_channel_label:3;
    unsigned audio_output_hw_type:3;
    unsigned audio_output_hw_instance:2;
    unsigned audio_output_channel:1;
    unsigned padding:7;
} audio_output_routing_t;

#define SINK_AUDIO_OUTPUT_READONLY_CONFIG_BLK_ID 443

typedef struct {
    audio_output_routing_t route[7];
    int fixed_hardware_gain;
    unsigned volume_scaling_method_aux:2;
    unsigned volume_scaling_method_main:2;
    unsigned enable_24_bit_resolution:1;
    unsigned padding:11;
} sink_audio_output_readonly_config_def_t;

#endif /* _SINK_MULTI_CHANNEL_CONFIG_DEF_H_ */
