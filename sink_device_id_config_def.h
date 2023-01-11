
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_device_id_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_DEVICE_ID_CONFIG_DEF_H_
#define _SINK_DEVICE_ID_CONFIG_DEF_H_

#include "config_definition.h"

#define DEVICE_ID_READ_ONLY_CONFIG_BLK_ID 463

typedef struct {
    unsigned vendor_id_source;
    unsigned vendor_id;
    unsigned product_id;
    unsigned bcd_version;
    unsigned sw_version_msb_hi;
    unsigned sw_version_msb_lo;
    unsigned sw_version_lsb_hi;
    unsigned sw_version_lsb_lo;
} device_id_read_only_config_def_t;

#endif /* _SINK_DEVICE_ID_CONFIG_DEF_H_ */
