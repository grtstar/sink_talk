
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_a2dp_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_A2DP_CONFIG_DEF_H_
#define _SINK_A2DP_CONFIG_DEF_H_

#include "config_definition.h"

#define A2DP_CONFIG_BLK_ID 33

typedef struct {
    unsigned A2dpConnectionUpdateDelay_ms;
    unsigned A2dpLinkLossReconnectionTime_s;
    unsigned DefaultA2dpVolume;
    unsigned A2dpOptionalCodecsEnabled:7;
    unsigned a2dpclass:2;
    unsigned EnableA2dpStreaming:1;
    unsigned sbc_uses_medium_bitpool:1;
    unsigned EnableA2dpMediaOpenOnConnection:1;
    unsigned padding:4;
} a2dp_config_def_t;

#define A2DP_SESSION_CONFIG_BLK_ID 41

typedef struct {
    unsigned selected_eq_bank:3;
    unsigned audio_enhancement_disable:1;
    unsigned subwoofer_bypass:1;
    unsigned speaker_eq_bypass:1;
    unsigned eq_flat_enable:1;
    unsigned user_eq_bypass:1;
    unsigned bass_enhance_bypass:1;
    unsigned spatial_enhance_bypass:1;
    unsigned compander_bypass:1;
    unsigned dither_bypass:1;
    unsigned padding:4;
} a2dp_session_config_def_t;

#endif /* _SINK_A2DP_CONFIG_DEF_H_ */
