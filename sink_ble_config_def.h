
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_ble_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_BLE_CONFIG_DEF_H_
#define _SINK_BLE_CONFIG_DEF_H_

#include "config_definition.h"

#define SINK_BLE_READONLY_CONFIG_BLK_ID 943

typedef struct {
    unsigned LeAuthenticatedPayloadTO_s;
    unsigned BleAdvertisingFilters:7;
    unsigned BleMaxPeripheralConn:2;
    unsigned BleMaxCentralConn:2;
    unsigned padding:5;
} sink_ble_readonly_config_def_t;

#endif /* _SINK_BLE_CONFIG_DEF_H_ */
