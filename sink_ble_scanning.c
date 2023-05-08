/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_ble_scanning.c

DESCRIPTION
    BLE Scanning functionality
*/

#include "sink_ble_scanning.h"

#include "sink_ble_advertising.h"
#include "sink_gatt_manager.h"
#include "sink_debug.h"
#include "sink_ble.h"
#include "sink_ba_receiver.h"

#include "sink_gatt_client_battery.h"
#include "sink_gatt_client_hid.h"
#include "sink_gatt_client_dis.h"
#include "sink_gatt_client_ias.h"
#include "sink_gatt_client_ancs.h"
#include "sink_gatt_client_spc.h"
#include "sink_gatt_client_hrs.h"

#include "headset_multi_pair.h"
#include "sink_main_task.h"

#include <gatt.h>
#include <csrtypes.h>

#ifdef GATT_ENABLED

/* Macro for BLE AD Data Debug */
#ifdef DEBUG_BLE
#include <stdio.h>
#define BLE_SCAN_DEBUG(x) DEBUG(x)
#else
#define BLE_SCAN_DEBUG(x)
#endif

/****************************************************************************/
void bleClearScanData(void)
{
    BLE_SCAN_DEBUG(("BLE: Clear scan filters\n"));
    ConnectionBleClearAdvertisingReportFilter();
}

/****************************************************************************/
void bleStartScanning(bool white_list, bool fast)
{
    ble_scanning_parameters_t scan_params;

    BLE_SCAN_DEBUG(("BLE: Start scanning fast=[%u] white_list=[%u]\n", fast, white_list));
    bleClearScanData();
    bleAddScanFilters();
    /* Get the scan parameters for the current mode */
    sinkBleGetScanningParameters(fast, &scan_params);

    /* Set the BLE Scan parameters */
    ConnectionDmBleSetScanParametersReq(FALSE, FALSE, white_list, scan_params.interval, scan_params.window);

    /* Enable scanning */
    ConnectionDmBleSetScanEnable(TRUE);
}

/****************************************************************************/
void bleStopScanning(void)
{
    /* Stop scanning, this will stop advertisements from BLE devices to be recieved by the application */
    BLE_SCAN_DEBUG(("BLE: Stop scanning\n"));
    ConnectionDmBleSetScanEnable(FALSE);
}

/****************************************************************************/
void bleHandleScanResponse(const CL_DM_BLE_ADVERTISING_REPORT_IND_T *ind)
{
    /* BA receiver handles all reponses when active. */
    BLE_SCAN_DEBUG(("BLE: Scan response: addr:%x:%x:%lx, num:%d\n",
                    ind->current_taddr.addr.nap,
                    ind->current_taddr.addr.uap,
                    ind->current_taddr.addr.lap,
                    ind->num_reports));
    if (BA_RECEIVER_MODE_ACTIVE)
    {
        sinkReceiverHandleIVAdvert(ind);
    }
    else
    {
        if (0)
        {
            ble_gap_event_t event;
            ble_gap_event_args_t args;

            /* Send GAP event to attempt connection to advertising device */
            event.id = ble_gap_event_central_conn_attempt;
            args.central_conn_attempt.current_taddr = ind->current_taddr;
            args.central_conn_attempt.permanent_taddr = ind->permanent_taddr;
            event.args = &args;
            sinkBleGapEvent(event);
        }
        else
        {
            CL_DM_BLE_ADVERTISING_REPORT_IND_T *message = (CL_DM_BLE_ADVERTISING_REPORT_IND_T *)PanicUnlessMalloc(sizeof(CL_DM_BLE_ADVERTISING_REPORT_IND_T) + ind->size_advertising_data);
            memmove(message, ind, sizeof(CL_DM_BLE_ADVERTISING_REPORT_IND_T) + ind->size_advertising_data);
            MessageSend(&theSink.task, EventSysMultiTalkBleInquiryDevices, message);
        }
    }
}

/****************************************************************************/
void bleAddScanFilters(void)
{
    uint8 advertising_filter = sinkBleGapGetAdvertisingFilter();

    /* Setup advertising filters for supported services */
    if (advertising_filter & (1 << ANCS_AD_BIT))
    {
        sinkGattAncsClientSetupAdvertisingFilter();
    }
    if (advertising_filter & (1 << BATTERY_AD_BIT))
    {
        gattBatteryClientSetupAdvertisingFilter();
    }
    if (advertising_filter & (1 << DIS_AD_BIT))
    {
        sinkGattDisClientSetupAdvertisingFilter();
    }
    if (advertising_filter & (1 << HID_AD_BIT))
    {
        sinkGattHidClientSetupAdvertisingFilter();
    }
    if (advertising_filter & (1 << IAS_AD_BIT))
    {
        sinkGattIasClientSetupAdvertisingFilter();
    }
    if (advertising_filter & (1 << SPC_AD_BIT))
    {
        sinkGattSpClientSetupAdvertisingFilter();
    }
    if (advertising_filter & (1 << HRS_AD_BIT))
    {
        sinkGattHrsClientSetupAdvertisingFilter();
    }
    if (sinkMultiTalkPairServiceEnabled())
    {
        BLE_SCAN_DEBUG(("MTP Set up Adv filter\n"));
        ConnectionBleAddAdvertisingReportFilter(ble_ad_type_more_uuid16, 2, 2, mtGetAdvertisingFilter());
        ConnectionBleAddAdvertisingReportFilter(ble_ad_type_complete_uuid16, 2, 2, mtGetAdvertisingFilter());
    }
}

#endif /* GATT_ENABLED */
