
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_tones_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_TONES_CONFIG_DEF_H_
#define _SINK_TONES_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned event;
    unsigned tone:8;
    unsigned padding:8;
} tone_config_type_t;

#define SINK_TONE_WRITEABLE_CONFIG_BLK_ID 621

typedef struct {
    tone_config_type_t gEventTones[1];
} sink_tone_writeable_config_def_t;

#define CONFIGTONE_RDATA_CONFIG_BLK_ID 717

typedef struct {
    unsigned gVariableTones[1];
} configtone_rdata_config_def_t;

#define SINK_TONE_READONLY_CONFIG_BLK_ID 722

typedef struct {
    unsigned FixedToneVolumeLevel:5;
    unsigned PlayTonesAtFixedVolume:1;
    unsigned PlayLocalVolumeTone:1;
    unsigned QueueVolumeTones:1;
    unsigned QueueEventTones:1;
    unsigned MuteToneFixedVolume:1;
    unsigned padding:6;
} sink_tone_readonly_config_def_t;

#endif /* _SINK_TONES_CONFIG_DEF_H_ */
