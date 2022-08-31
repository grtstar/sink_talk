
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_pio_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_PIO_CONFIG_DEF_H_
#define _SINK_PIO_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned pio_num:8;
    unsigned pio_function:8;
} pio_function_pattern_t;

#define PIO_WRITEABLE_CONFIG_BLK_ID 92

typedef struct {
    unsigned user_pio_state:1;
    unsigned padding:15;
} pio_writeable_config_def_t;

#define PIO_READONLY_CONFIG_BLK_ID 97

typedef struct {
    unsigned pio_invert_0_15;
    unsigned pio_invert_16_31;
    unsigned pio_invert_32_47;
    unsigned pio_invert_48_63;
    unsigned pio_invert_64_79;
    unsigned pio_invert_80_95;
    unsigned pio_mapping_0_15;
    unsigned pio_mapping_16_31;
    unsigned pio_mapping_32_47;
    unsigned pio_mapping_48_63;
    unsigned pio_mapping_64_79;
    unsigned pio_mapping_80_95;
    unsigned user_pio:8;
    unsigned padding:8;
} pio_readonly_config_def_t;

#define CONFIGURE_PIO_FUNCTION_CONFIG_BLK_ID 114

typedef struct {
    pio_function_pattern_t pio_function_array[1];
} configure_pio_function_config_def_t;

#endif /* _SINK_PIO_CONFIG_DEF_H_ */
