
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_volume_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_VOLUME_CONFIG_DEF_H_
#define _SINK_VOLUME_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    int no_of_steps;
    int dB_min;
    int volume_knee_value_1;
    int dB_knee_value_1;
    int volume_knee_value_2;
    int dB_knee_value_2;
    int dB_max;
    unsigned volume_curve_label:1;
    unsigned padding:15;
} vol_mod_gain_curve_t;

typedef struct {
    unsigned A2dpGain:8;
    unsigned Tone:8;
    unsigned VolGain:8;
    unsigned IncVol:4;
    unsigned DecVol:4;
} VolMapping_t;

#define SINK_VOLUME_WRITEABLE_CONFIG_BLK_ID 480

typedef struct {
    vol_mod_gain_curve_t volgroup_config[2];
    VolMapping_t gVolMaps[16];
    unsigned gVolButtonsInverted:1;
    unsigned padding:15;
} sink_volume_writeable_config_def_t;

#define SINK_VOLUME_READONLY_CONFIG_BLK_ID 533

typedef struct {
    unsigned StoreCurrentSinkVolSrcTimeout_s;
    unsigned MuteSpeakerAndMic:1;
    unsigned AdjustVolumeWhilstMuted:1;
    unsigned VolumeChangeCausesUnMute:1;
    unsigned padding:13;
} sink_volume_readonly_config_def_t;

#endif /* _SINK_VOLUME_CONFIG_DEF_H_ */
