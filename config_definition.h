
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    config_definition.h

DESCRIPTION 
    Header file for the configuration definition.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _CONFIG_DEFINITION_H_
#define _CONFIG_DEFINITION_H_

#include <csrtypes.h>

const void *ConfigDefinitionGetConstData(void);
uint16 ConfigDefinitionGetConstDataSize(void);

typedef struct {
    unsigned gain:5;
    unsigned instance:2;
    unsigned pre_amp:1;
    unsigned enable_24_bit_resolution:1;
    unsigned padding:7;
} analogue_input_params_t;

typedef struct {
    unsigned pio:7;
    unsigned gain:5;
    unsigned bias_config:2;
    unsigned instance:2;
    unsigned pre_amp:1;
    unsigned is_digital:1;
    unsigned padding:14;
} audio_mic_params_t;

typedef struct {
    unsigned current_limit_threshold;
    unsigned current_recovery_threshold;
    unsigned current_limit_read_period;
    unsigned positive_current_step:8;
    unsigned negative_current_step:8;
} current_determination_structure_t;

typedef struct {
    unsigned period_no_chg;
    unsigned source:8;
    unsigned period_chg:8;
} power_adc_t;

typedef struct {
    power_adc_t adc;
} power_vref_config_t;

typedef struct {
    power_adc_t adc;
    unsigned ignore_increase_no_chg:1;
    unsigned padding:15;
} power_vbat_config_t;

typedef struct {
    power_adc_t adc;
    unsigned limits[7];
    unsigned pio:8;
    unsigned ics:8;
    unsigned delay:4;
    unsigned raw_limits:1;
    unsigned padding:11;
} power_vthm_config_t;

typedef struct {
    power_adc_t adc;
} power_vchg_config_t;

typedef struct {
    power_vref_config_t vref;
} power_config_vref_t;

typedef struct {
    power_config_vref_t p_config_vref;
} sink_power_config_vref_t;

typedef struct {
    power_vbat_config_t vbat;
} power_config_vbat_t;

typedef struct {
    power_config_vbat_t p_config_vbat;
} sink_power_config_vbat_t;

typedef struct {
    power_vthm_config_t vthm;
} power_config_vthm_t;

typedef struct {
    power_config_vthm_t p_config_vthm;
} sink_power_config_vthm_t;

typedef struct {
    power_vchg_config_t vchg;
} power_config_vchg_t;

typedef struct {
    power_config_vchg_t p_config_vchg;
} sink_power_config_vchg_t;

#endif /* _CONFIG_DEFINITION_H_ */
