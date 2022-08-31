
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_inquiry_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_INQUIRY_CONFIG_DEF_H_
#define _SINK_INQUIRY_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    int tx_power;
    int threshold;
    int diff_threshold;
    unsigned cod_filter_hi;
    unsigned cod_filter_lo;
    int conn_threshold;
    int conn_diff_threshold;
    unsigned max_responses:8;
    unsigned timeout:8;
    unsigned resume_timeout:8;
    unsigned num_results:4;
    unsigned connect_if_in_pdl:1;
    unsigned dont_pair_if_in_pdl:1;
    unsigned try_all_discovered:1;
    unsigned pair_on_pdl_reset:1;
} rssi_pairing_t;

typedef struct {
    unsigned page_scan_interval;
    unsigned page_scan_window;
    unsigned inquiry_scan_interval;
    unsigned inquiry_scan_window;
} radio_config_type_t;

#define SINK_INQUIRY_READONLY_CONFIG_BLK_ID 425

typedef struct {
    rssi_pairing_t rssi;
    radio_config_type_t radio;
    unsigned InquiryTimeout_s;
} sink_inquiry_readonly_config_def_t;

#endif /* _SINK_INQUIRY_CONFIG_DEF_H_ */
