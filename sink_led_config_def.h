
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_led_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_LED_CONFIG_DEF_H_
#define _SINK_LED_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned Event;
    unsigned OverideLED:8;
    unsigned Speed:8;
    unsigned FilterToCancel:5;
    unsigned FollowerLEDDelay:4;
    unsigned FilterType:3;
    unsigned Colour:3;
    unsigned SpeedAction:1;
    unsigned OverideDisable:1;
    unsigned padding:15;
} LEDFilter_t;

typedef struct {
    unsigned OnTime:8;
    unsigned OffTime:8;
    unsigned RepeatTime:8;
    unsigned TimeOut:8;
    unsigned DimTime:8;
    unsigned LED_A:8;
    unsigned LED_B:8;
    unsigned NumFlashes:4;
    unsigned Colour:3;
    unsigned OverideDisable:1;
} LEDPattern_t;

typedef struct {
    unsigned state;
    LEDPattern_t pattern;
} LEDStatePattern_t;

typedef struct {
    unsigned event;
    LEDPattern_t pattern;
} LEDEventPattern_t;

typedef struct {
    unsigned TriCol_a:8;
    unsigned TriCol_b:8;
    unsigned TriCol_c:8;
    unsigned padding:8;
} PioTriColLeds_t;

#define SINK_LED_STATE_PATTERN_CONFIG_BLK_ID 723

typedef struct {
    LEDStatePattern_t pStatePatterns[1];
} sink_led_state_pattern_config_def_t;

#define SINK_LED_EVENT_PATTERN_CONFIG_BLK_ID 827

typedef struct {
    LEDEventPattern_t pEventPatterns[1];
} sink_led_event_pattern_config_def_t;

#define SINK_LED_EVENT_FILTER_CONFIG_BLK_ID 856

typedef struct {
    LEDFilter_t pEventFilters[1];
} sink_led_event_filter_config_def_t;

#define SINK_LED_READONLY_CONFIG_BLK_ID 924

typedef struct {
    PioTriColLeds_t TriColLeds;
    unsigned LedEnablePIO:8;
    unsigned LedTimeMultiplier:2;
    unsigned OverideFilterPermanentlyOn:1;
    unsigned QueueLEDEvents:1;
    unsigned ResetLEDEnableStateAfterReset:1;
    unsigned ChargerTerminationLEDOveride:1;
    unsigned padding:2;
} sink_led_readonly_config_def_t;

#define SINK_LED_SESSION_CONFIG_BLK_ID 931

typedef struct {
    unsigned gLEDSEnabled:1;
    unsigned padding:15;
} sink_led_session_config_def_t;

#endif /* _SINK_LED_CONFIG_DEF_H_ */
