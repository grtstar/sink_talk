
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_private_data_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_PRIVATE_DATA_CONFIG_DEF_H_
#define _SINK_PRIVATE_DATA_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned AutoSwitchOffTime_s;
    unsigned DisablePowerOffAfterPOnTime_s;
    unsigned PairModeTimeout_s;
    unsigned ConnectableTimeout_s;
    unsigned PairModeTimeoutIfPDL_s;
    unsigned PairingModeReminderInterval_s;
    unsigned EncryptionRefreshTimeout_m;
    unsigned SecondAGConnectDelayTime_s;
    unsigned DefragCheckTimer_s;
    unsigned AudioAmpPDownTimeInLimbo_s;
    unsigned LimboTimeout_s;
} sinkdata_timer_t;

typedef struct {
    unsigned key_size:8;
    unsigned key_minimum:8;
} defrag_config_t;

typedef struct {
    unsigned max_remote_ssr_latency;
    unsigned min_remote_ssr_timeout;
    unsigned min_local_ssr_timeout;
} ssr_params_t;

typedef struct {
    ssr_params_t sco_params;
    ssr_params_t slc_params;
} subrate_t;

#define SINKDATA_READONLY_CONFIG_BLK_ID 118

typedef struct {
    sinkdata_timer_t private_data_timers;
    defrag_config_t defrag;
    subrate_t ssr_data;
    unsigned ReconnectionAttempts;
    unsigned pdl_size;
    unsigned partitions_free:8;
    unsigned DiscoIfPDLLessThan:4;
    unsigned ReconnectLastAttempts:3;
    unsigned ReconnectOnPanic:1;
    unsigned PowerDownOnDiscoTimeout:2;
    unsigned ActionOnPowerOn:2;
    unsigned PairIfPDLLessThan:2;
    unsigned ActionOnPanicReset:2;
    unsigned PowerOffAfterPDLReset:1;
    unsigned RemainDiscoverableAtAllTimes:1;
    unsigned DisablePowerOffAfterPowerOn:1;
    unsigned PowerOffOnlyIfVRegEnLow:1;
    unsigned AllowAutomaticPowerOffWhenCharging:1;
    unsigned pair_mode_en:1;
    unsigned AutoReconnectPowerOn:1;
    unsigned EnterPairingModeOnFailureToConn:1;
    unsigned DoNotDiscoDuringLinkLoss:1;
    unsigned ManInTheMiddle:1;
    unsigned UseDiffConnectedEventAtPowerOn:1;
    unsigned SecurePairing:1;
    unsigned ResetAfterPowerOffComplete:1;
    unsigned AutoPowerOnAfterInitialisation:1;
    unsigned DisableRoleSwitching:1;
    unsigned GoConnectableDuringLinkLoss:1;
    unsigned DisableCompletePowerOff:1;
    unsigned EncryptOnSLCEstablishment:1;
    unsigned padding:6;
} sinkdata_readonly_config_def_t;

#define SINKDATA_WRITEABLE_CONFIG_BLK_ID 145

typedef struct {
    unsigned multipoint_enable:1;
    unsigned padding:15;
} sinkdata_writeable_config_def_t;

#endif /* _SINK_PRIVATE_DATA_CONFIG_DEF_H_ */
