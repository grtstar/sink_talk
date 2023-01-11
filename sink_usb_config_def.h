
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_usb_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_USB_CONFIG_DEF_H_
#define _SINK_USB_CONFIG_DEF_H_

#include "config_definition.h"

#define SINK_USB_READONLY_CONFIG_BLK_ID 988

typedef struct {
    unsigned device_class;
    unsigned plugin_index:4;
    unsigned attach_timeout:4;
    unsigned deconfigured_timeout:4;
    unsigned plugin_type:2;
    unsigned audio_always_on:1;
    unsigned pause_when_switching_source:1;
} sink_usb_readonly_config_def_t;

#define SINK_USB_WRITEABLE_CONFIG_BLK_ID 994

typedef struct {
    unsigned usb_main_volume;
    unsigned usb_aux_volume;
} sink_usb_writeable_config_def_t;

#endif /* _SINK_USB_CONFIG_DEF_H_ */
