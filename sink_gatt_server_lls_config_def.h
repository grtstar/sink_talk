
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_server_lls_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_GATT_SERVER_LLS_CONFIG_DEF_H_
#define _SINK_GATT_SERVER_LLS_CONFIG_DEF_H_

#include "config_definition.h"

#define GATT_LLS_READONLY_CONFIG_BLK_ID 964

typedef struct {
    unsigned LinkLossTimer_s;
    unsigned LinkLossAlertStopTimeout_s;
} gatt_lls_readonly_config_def_t;

#endif /* _SINK_GATT_SERVER_LLS_CONFIG_DEF_H_ */
