
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_power_manager_pmu_monitor_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_POWER_MANAGER_PMU_MONITOR_CONFIG_DEF_H_
#define _SINK_POWER_MANAGER_PMU_MONITOR_CONFIG_DEF_H_

#include "config_definition.h"

#define SINK_PMU_MON_CONFIG_CONFIG_BLK_ID 1034

typedef struct {
    unsigned min_charge_i;
    unsigned max_charge_i;
    unsigned monitor_period_active;
    unsigned monitor_period_idle;
    unsigned monitor_period_nopwr;
    unsigned charger_i_step:8;
    unsigned no_incr_i_temp:8;
    unsigned decr_i_temp:8;
    unsigned padding:8;
} sink_pmu_mon_config_config_def_t;

#endif /* _SINK_POWER_MANAGER_PMU_MONITOR_CONFIG_DEF_H_ */
