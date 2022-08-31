
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gaia_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_GAIA_CONFIG_DEF_H_
#define _SINK_GAIA_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned pio:8;
    unsigned padding:8;
} gaia_pio_pattern_t;

#define GAIA_READ_ONLY_CONFIG_BLK_ID 972

typedef struct {
    unsigned StoreCurrentPEQSettingsTimeout_s;
    unsigned dfu_partition:4;
    unsigned GaiaEnableSession:1;
    unsigned GaiaRemainConnected:1;
    unsigned padding:10;
} gaia_read_only_config_def_t;

#define GAIA_PIO_TRANSLATION_CONFIG_BLK_ID 978

typedef struct {
    gaia_pio_pattern_t gaia_pio_array[1];
} gaia_pio_translation_config_def_t;

#endif /* _SINK_GAIA_CONFIG_DEF_H_ */
