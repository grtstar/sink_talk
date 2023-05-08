
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_power_manager_charger_configuration_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_POWER_MANAGER_CHARGER_CONFIGURATION_CONFIG_DEF_H_
#define _SINK_POWER_MANAGER_CHARGER_CONFIGURATION_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned limit;
    unsigned current_external_devices:8;
    unsigned padding:8;
} charger_progress_structure_t;

#define CHARGER_PROGRESS_CONFIG_BLK_ID 1107

typedef struct {
    charger_progress_structure_t chg_progress;
} charger_progress_config_def_t;

#endif /* _SINK_POWER_MANAGER_CHARGER_CONFIGURATION_CONFIG_DEF_H_ */
