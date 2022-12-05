
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_upgrade_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_UPGRADE_CONFIG_DEF_H_
#define _SINK_UPGRADE_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned physical_partition_1:8;
    unsigned physical_partition_2:8;
    unsigned logical_type:4;
    unsigned padding:12;
} logical_partition_pattern_t;

typedef struct {
    unsigned enable_app_config_reset:1;
    unsigned protect_audio_during_upgrade:1;
    unsigned padding:14;
} upgrade_config_t;

#define SINK_UPGRADE_READONLY_CONFIG_BLK_ID 1134

typedef struct {
    upgrade_config_t upgrade_config;
    logical_partition_pattern_t logical_partitions_array[1];
} sink_upgrade_readonly_config_def_t;

#endif /* _SINK_UPGRADE_CONFIG_DEF_H_ */
