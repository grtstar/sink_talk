
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_pbap_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_PBAP_CONFIG_DEF_H_
#define _SINK_PBAP_CONFIG_DEF_H_

#include "config_definition.h"

#define PBAP_READ_ONLY_CONFIG_BLK_ID 943

typedef struct {
    unsigned pbap_enabled:1;
    unsigned padding:15;
} pbap_read_only_config_def_t;

#endif /* _SINK_PBAP_CONFIG_DEF_H_ */
