
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_usb_charger_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_USB_CHARGER_CONFIG_DEF_H_
#define _SINK_USB_CHARGER_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned current:8;
    unsigned charger_config:4;
    unsigned boost:2;
    unsigned charge:1;
    unsigned vsel:1;
    unsigned power_off:1;
    unsigned disable_leds:1;
    unsigned padding:14;
} sink_charge_current_t;

#define SINK_USB_POWER_READONLY_CONFIG_BLK_ID 907

typedef struct {
    sink_charge_current_t charge[11];
} sink_usb_power_readonly_config_def_t;

#endif /* _SINK_USB_CHARGER_CONFIG_DEF_H_ */
