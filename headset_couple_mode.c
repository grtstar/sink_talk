#include "sink_a2dp.h"
#include "sink_anc.h"
#include "sink_configmanager.h"
#include "sink_devicemanager.h"
#include "sink_config.h"
#include "sink_wired.h"
#include "sink_usb.h"
#include "sink_slc.h"
#include "sink_auth.h"
#include "sink_pio.h"
#include "sink_avrcp.h"
#include "sink_device_id.h"
#include "sink_link_policy.h"
#include "sink_powermanager.h"
#include "sink_peer_qualification.h"
#include "sink_audio.h"
#include "sink_audio_routing.h"
#include "sink_music_processing.h"
#include "sink_inquiry.h"
#include "sink_volume.h"
#include "sink_main_task.h"
#include "sink_malloc_debug.h"
#include "sink_devicemanager.h"
#include "sink_device_id.h"
#include "sink_events.h"
#define ENABLE_MT_DEBUG
#include "sink_debug.h"
#include "sink_audio_prompts.h"
#include "sink_statemanager.h"
#include "sink_scan.h"

#include <connection.h>
#include <a2dp.h>
#include <hfp.h>
#include <stdlib.h>
#include <audio.h>
#include <audio_plugin_if.h>
#include <audio_config.h>
#include <sink.h>
#include <bdaddr.h>
#include <vm.h>
#include <config_store.h>
#include <byte_utils.h>
#include <broadcast_context.h>
#include <audio_plugin_voice_prompts_variants.h>
#include <ps.h>

#include "headset_multi_talk.h"
#include "headset_multi_pair.h"
#include "audio_prompt.h"
#include "csr_common_example_plugin.h"
#include "headset_ag.h"
#include "headset_uart.h"

extern MTData *mt;
extern TaskData acl_parent_task;
extern TaskData acl_child_task;

static bool mtConnectCouple(bdaddr *bd_addr);

bool mtSendPeerAddr(int ch, bdaddr *addr)
{
    uint8 d[1 + 8] = {0};
    d[0] = ACLMSG_PEER_ADDR;
    BdaddrToArray(addr, &d[1]);
    return ACLSend(mt->mt_device[ch].acl_sink, d, sizeof(d));
}

void ACLProcessParentDataCouple(const uint8_t *data, int size)
{
    switch (data[0])
    {
    case ACLMSG_PEER_ADDR: /* send to 3024 */
    {
        bdaddr addr = ArrayToBdaddr(&data[1]);
        UartSendPeerAddr(&addr);
    }
    break;
    default:
        break;
    }
}

void mtPeerStateCoupleMode(uint8 state)
{
    if (state == 0) /* peer connect success */
    {
        MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);
        mt->couple_type = COUPLE_MT_WITH_PEER;
    }
    else
    {
        MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);
        mt->couple_type = COUPLE_MT_NO_PEER;
    }
}

bool mtConnectCouple(bdaddr *bd_addr)
{
    MT_DEBUG(("MT: mtConnectCouple to "));
    MT_DEBUG_ADDR((*bd_addr));
    if (mt->mt_device[MT_LEFT].state < MT_L2CAP_WaitConnect)
    {
        mt->mt_device[MT_LEFT].bt_addr = *bd_addr;
        mt->mt_device[MT_LEFT].state = MT_L2CAP_Connecting;
        mtACLConnect(bd_addr, MULTITALK_COUPLE_PSM);
        return TRUE;
    }
    MT_DEBUG(("MT: no pos to connect\n"));
    return FALSE;
}

bool handleMTL2capConnectIndCoupleMode(CL_L2CAP_CONNECT_IND_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capConnectIndCoupleMode: addr=%x:%x:%lx, psm=0x%x, identifier=0x%x, connID=0x%x\n",
              msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap, msg->psm,
              msg->identifier, msg->connection_id));
    if (msg->psm != MULTITALK_COUPLE_PSM)
    {
        MT_DEBUG(("MT: psm error\n"));
        return FALSE;
    }
    if (mt->mt_device[MT_LEFT].state == MT_L2CAP_Disconnected || mt->mt_device[MT_LEFT].state >= MT_L2CAP_Disconnecting)
    {
        /* ?????????????? */
        MT_DEBUG(("MT: handleMTL2capConnectIndCoupleMode parant is disconnect\n"));
        {
            if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_RIGHT].bt_addr))
            {
                DEBUG(("MT: handleMTL2capConnectIndCoupleMode cannot allow child connected to parent\n"));
                return FALSE;
            }
            else 
            {
                if(mt->mt_mode == COUPLE_MODE_PAIRING)
                {
                    MT_DEBUG(("MT: handleMTL2capConnectIndCoupleMode peer is my cai\n"));
                    mt->mt_device[MT_LEFT].state = MT_L2CAP_WaitConnect;
                    mt->mt_device[MT_LEFT].bt_addr = msg->bd_addr;
                    mt->mt_type = MT_NODE;
                    return TRUE;
                }
                else if(mt->mt_mode == COUPLE_MODE || mt->mt_mode == CLOSE_MODE)
                {
                    if(BdaddrIsSame(&msg->bd_addr, &mt->couple_addr))
                    {
                        MT_DEBUG(("MT: handleMTL2capConnectIndCoupleMode peer is my couple\n"));
                        mt->mt_device[MT_LEFT].state = MT_L2CAP_WaitConnect;
                        mt->mt_device[MT_LEFT].bt_addr = msg->bd_addr;
                        mt->mt_type = MT_NODE;
                        mt->mt_mode = COUPLE_MODE;
                        PowerAmpOn();
                        return TRUE;
                    }
                    else
                    {
                        MT_DEBUG(("MT: handleMTL2capConnectIndCoupleMode peer is not couple:%x:%x:%lx\n", mt->couple_addr.nap, mt->couple_addr.uap, mt->couple_addr.lap));
                    }
                }
                
            }
        }
    }
    return FALSE;
}

void handleMTL2capConnectCfmCoupleMode(CL_L2CAP_CONNECT_CFM_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capConnectCfmCoupleMode: addr=%x:%x:%lx, status=%d\n",
              msg->addr.nap, msg->addr.uap, msg->addr.lap, msg->status));
    
    if (BdaddrIsSame(&msg->addr, &mt->mt_device[MT_RIGHT].bt_addr))
    {
        MT_DEBUG(("MT: Child\n"));
        if (msg->status == l2cap_connect_success)
        {
            if (mt->mt_device[MT_RIGHT].state == MT_L2CAP_Connecting)
            {   
                mt->mt_device[MT_RIGHT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_RIGHT].acl_sink = msg->sink;
                mt->mt_type = MT_NODE;

                ConnectionSetLinkSupervisionTimeout(msg->sink, 4000);
                linkPolicyUseMultiTalkSettings(msg->sink);
                MessageSinkTask(msg->sink, &acl_child_task);
                 mt->mt_device[MT_RIGHT].state = MT_SYN_Connecting;
                mtScoConnect(msg->sink);
            }
            else if (mt->mt_device[MT_RIGHT].state == MT_L2CAP_WaitConnect)
            {
                mt->mt_device[MT_RIGHT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_RIGHT].acl_sink = msg->sink;
                MessageSinkTask(msg->sink, &acl_parent_task);
            }
            
            linkPolicyCheckRoles();
           
        }
        else if (msg->status >= l2cap_connect_failed)
        {
            BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
            mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;

            if (msg->status == l2cap_connect_error || msg->status >= l2cap_connect_failed_security) /* 137 */
            {
                if(mt->mt_mode == COUPLE_MODE_PAIRING)
                {
                    deviceManagerRemoveDevice(&msg->addr);
                }
            }
            if(mt->mt_mode == COUPLE_MODE_PAIRING)
            {
                mt->status = MT_ST_PARING;
            }
            else 
            {
                if (mt->status == MT_ST_CONNECTED  || mt->status == MT_ST_CONNECTING)
                {
                    MessageSendLater(mt->app_task, EventSysMultiTalkCoupleModeReconnect, NULL, 5000);
                }
            }
        }
    }
    else if (BdaddrIsSame(&msg->addr, &mt->mt_device[MT_LEFT].bt_addr))
    {
        MT_DEBUG(("MT: Parent\n"));
        if (msg->status == l2cap_connect_success)
        {
            if (mt->mt_device[MT_LEFT].state == MT_L2CAP_Connecting)
            {
                mt->mt_device[MT_LEFT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_LEFT].acl_sink = msg->sink;

                ConnectionSetLinkSupervisionTimeout(msg->sink, 4000);
                linkPolicyUseMultiTalkSettings(msg->sink);

                MessageSinkTask(msg->sink, &acl_parent_task);
                mt->mt_device[MT_LEFT].state = MT_SYN_Connecting;
                mtScoConnect(msg->sink);
            }
            else if (mt->mt_device[MT_LEFT].state == MT_L2CAP_WaitConnect)
            {
                mt->mt_device[MT_LEFT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_LEFT].acl_sink = msg->sink;
                MessageSinkTask(msg->sink, &acl_parent_task);
            }
            linkPolicyCheckRoles();
        }
        else if (msg->status >= l2cap_connect_failed)
        {
            BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
            mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;

            if (msg->status == l2cap_connect_error || msg->status >= l2cap_connect_failed_security) /* 137 */
            {
                if(mt->mt_mode == COUPLE_MODE_PAIRING)
                {
                    deviceManagerRemoveDevice(&msg->addr);
                }
            }
            if(mt->mt_mode == COUPLE_MODE_PAIRING)
            {
                mt->status = MT_ST_PARING;
            }
            else 
            {
                if (mt->status == MT_ST_CONNECTED  || mt->status == MT_ST_CONNECTING)
                {
                    MessageSendLater(mt->app_task, EventSysMultiTalkCoupleModeReconnect, NULL, 5000);
                }
            }
        }
    }

}

void handleMTL2capDisconIndCoupleMode(CL_L2CAP_DISCONNECT_IND_T *msg)
{
    /* Disconnect passive */
    MT_DEBUG(("MT: handleMTL2capDisconIndCoupleMode: status=%d\n", msg->status));
    if (msg->sink == mt->mt_device[MT_LEFT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconIndCoupleMode: parent\n"));
        mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        mt->mt_device[MT_LEFT].acl_sink = NULL;

        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }
        mt->status = MT_ST_LINKLOSS;
        MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
    }
    if (msg->sink == mt->mt_device[MT_RIGHT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconIndCoupleMode: child\n"));
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        mt->mt_device[MT_RIGHT].acl_sink = NULL;

        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }
        mt->status = MT_ST_LINKLOSS;
        MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
    }
}

void handleMTL2capDisconCfmCoupleMode(CL_L2CAP_DISCONNECT_CFM_T *msg)
{
    /* Disconnect active */
    MT_DEBUG(("MT: handleMTL2capDisconCfmCoupleMode: status=%d\n", msg->status));
    if (msg->sink == mt->mt_device[MT_LEFT].acl_sink)
    {
        mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
        mt->mt_device[MT_LEFT].acl_sink = NULL;
        stateManagerUpdateState();
    }
    else if (msg->sink == mt->mt_device[MT_RIGHT].acl_sink)
    {
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
        mt->mt_device[MT_RIGHT].acl_sink = NULL;
        stateManagerUpdateState();
    }
    else
    {
        MT_DEBUG(("MT: not Now link\n"));
        return;
    }
    if (mtGetConnectDevices() == 0 && mt->status == MT_ST_STAY_DISCONNET)
    {
        mt->nearby_connected = 0;
        if(mt->status != MT_ST_PARING)
        {
            MessageSend(mt->app_task, EventSysMultiTalkCoupleModeLeaved, NULL);
        }
        MessageCancelAll(mt->app_task, EventSysMultiTalkLeaveCoupleModeDelay);
    }
}

void handleMTSynConnIndCoupleMode(CL_DM_SYNC_CONNECT_IND_T *msg)
{
}

void handleMTSynConnCfmCoupleMode(CL_DM_SYNC_CONNECT_CFM_T *msg)
{
    MT_DEBUG(("MT: handleMTSynConnCfmCoupleMode status=%d, link_typs = %d\n",
              msg->status, msg->link_type));
    MT_DEBUG_ADDR(msg->bd_addr);
    if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_LEFT].bt_addr))
    {
        switch (msg->status)
        {
        case hci_success:
            mt->mt_device[MT_LEFT].state = MT_SYN_Connected;
            mt->mt_device[MT_LEFT].audio_sink = msg->audio_sink;
            mt->mt_device[MT_LEFT].link_type = msg->link_type;

            audioUpdateAudioRouting();

            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            mtInquiryStop();

            mt->status = MT_ST_CONNECTED;
            stateManagerEnterConnectedState();
            stateManagerUpdateState();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);
            if(!BdaddrIsSame(&mt->couple_addr, &msg->bd_addr))
            {
                deviceManagerRemoveDevice(&mt->couple_addr);
            }
            mt->couple_addr = msg->bd_addr;
            mt->couple_type = COUPLE_MT_WITH_PEER;
            mtSaveCoupleAddr(&mt->couple_addr, COUPLE_MT_WITH_PEER);
            mtSendPeerAddr(MT_LEFT, &mt->headset_addr);
            MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);

            break;
        case hci_error_page_timeout:
        case hci_error_auth_fail:
        case hci_error_key_missing:
        case hci_error_conn_timeout:
        case hci_error_max_nr_of_conns:
        case hci_error_max_nr_of_sco:
        case hci_error_rej_by_remote_no_res:
        default:
            MT_DEBUG(("MT: sync error\n"));
            break;
        }
    }
    else if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_RIGHT].bt_addr))
    {
        switch (msg->status)
        {
        case hci_success:
            mt->mt_device[MT_RIGHT].state = MT_SYN_Connected;
            mt->mt_device[MT_RIGHT].audio_sink = msg->audio_sink;
            mt->mt_device[MT_RIGHT].link_type = msg->link_type;

            audioUpdateAudioRouting();

            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            mtInquiryStop();

            mt->status = MT_ST_CONNECTED;
            stateManagerEnterConnectedState();
            stateManagerUpdateState();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);
            if(!BdaddrIsSame(&mt->couple_addr, &msg->bd_addr))
            {
                deviceManagerRemoveDevice(&mt->couple_addr);
            }
            mt->couple_addr = msg->bd_addr;
            mt->couple_type = COUPLE_MT_WITH_PEER;
            mtSaveCoupleAddr(&mt->couple_addr, COUPLE_MT_WITH_PEER);
            mtSendPeerAddr(MT_RIGHT, &mt->headset_addr);
            MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);

            break;
        case hci_error_page_timeout:
        case hci_error_auth_fail:
        case hci_error_key_missing:
        case hci_error_conn_timeout:
        case hci_error_max_nr_of_conns:
        case hci_error_max_nr_of_sco:
        case hci_error_rej_by_remote_no_res:
        default:
            MT_DEBUG(("MT: sync error\n"));
            break;
        }
    }
    else
    {
        MT_DEBUG(("MT: A loss conection, disconnect it?\n"));
    }
}

void handleMTSynDisconIndCoupleMode(CL_DM_SYNC_DISCONNECT_IND_T *msg)
{
    MT_DEBUG(("MT: handleMTSynDisconIndCoupleMode status=%d, reason = %d\n",
              msg->status, msg->reason));

    if (msg->audio_sink == mt->mt_device[MT_LEFT].audio_sink)
    {
        MT_DEBUG(("MT: handleMTSynDisconIndCoupleMode parent\n"));
        if (msg->reason == hci_success || mt->mt_device[MT_LEFT].state == MT_SYN_Disconnecting)
        {
            mtACLDisconnect(MT_LEFT);
        }

        mt->mt_device[MT_LEFT].state = MT_SYN_Disconnected;
        mt->mt_device[MT_LEFT].audio_sink = NULL;

        if (msg->status == hci_success)
        {
            mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        }
    }
    else if (msg->audio_sink == mt->mt_device[MT_RIGHT].audio_sink)
    {
        MT_DEBUG(("MT: handleMTSynDisconIndCoupleMode child\n"));
        if (msg->reason == hci_success || mt->mt_device[MT_RIGHT].state == MT_SYN_Disconnecting)
        {
            mtACLDisconnect(MT_RIGHT);
        }

        mt->mt_device[MT_RIGHT].state = MT_SYN_Disconnected;
        mt->mt_device[MT_RIGHT].audio_sink = NULL;

        if (msg->status == hci_success)
        {
            mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        }
    }
    else
    {
        MT_DEBUG(("MT: handleMTSynDisconIndCoupleMode who"));
    }
    audioUpdateAudioRouting();
}

bool processEventMultiTalkCoupleMode(Task task, MessageId id, Message message)
{
    if(mt->mt_mode != COUPLE_MODE && mt->mt_mode != COUPLE_MODE_PAIRING)
    {
        return FALSE;
    }
    switch (id)
    {
    case EventSysMultiTalkEnterCoupleMode:
        PowerAmpOn();
        stateManagerEnterConnectableState(FALSE);
        mt->mic_mute = FALSE;
        if (!BdaddrIsZero(&mt->couple_addr))
        {
            mt->status = MT_ST_CONNECTING;
            mt->couple_reconnect_retry = 10000;
            MessageSendLater(task, EventSysRssiPairReminder, NULL, D_SEC(6));
            MessageSendLater(task, EventSysMultiTalkCoupleModeReconnect, NULL, 1200);
        }
        break;
    case EventSysMultiTalkLeaveCoupleMode:
        MessageCancelAll(task, EventSysRssiPairReminder);
        mtInquiryStop();
        MessageCancelAll(task, EventSysMultiTalkCoupleModeReconnect);
        if (mt->couple_type == COUPLE_MT_WITH_PEER || mt->couple_type == COUPLE_MT_NO_PEER)
        {
            if (mtGetConnectDevices() == 0)
            {
                MessageSend(task, EventSysMultiTalkCoupleModeLeaved, NULL);
            }
            else
            {
                MessageSendLater(task, EventSysMultiTalkLeaveCoupleModeDelay, NULL, D_SEC(2));
            }
            mtDisconnect();
            mt->status = MT_ST_STAY_DISCONNET;
        }
        else
        {
            /* ag */
            if (mt->status == MT_ST_CONNECTED || mt->status == MT_ST_CONNECTING || mt->status == MT_ST_RECONNECTING)
            {
                if (AgIsConnected())
                {
                    AgDisconnect();
                    MessageSendLater(task, EventSysMultiTalkLeaveCoupleModeDelay, NULL, D_SEC(2));
                }
                else
                {
                    MessageSend(task, EventSysMultiTalkCoupleModeLeaved, NULL);
                }
            }
            else
            {
                MessageSend(task, EventSysMultiTalkCoupleModeLeaved, NULL);
            }
            mt->status = MT_ST_STAY_DISCONNET;
            mtInquiryStop();
            MessageCancelAll(task, EventSysMultiTalkCoupleModeReconnect);
        }
        break;
    case EventSysMultiTalkCoupleModeReconnect:
        if (mt->couple_type == COUPLE_MT_WITH_PEER || mt->couple_type == COUPLE_MT_NO_PEER)
        {
            /* attemp to reconnect close mode peer */
            if (mt->couple_reconnect_retry > 0)
            {
                mt->couple_reconnect_retry--;
				mt->status = MT_ST_CONNECTING;
                mtConnectCouple(&mt->couple_addr);
            }
            else
            {
                mtInquiryPair(inquiry_session_2talk, FALSE);
                mt->status = MT_ST_SEARCHING;
            }
        }
        else /* ag */
        {
            {
	            ConnectionSetPageTimeout(0);
                AgConnect(&mt->couple_addr);
                mt->status = MT_ST_CONNECTING;
            }
            
        }
        break;
    case EventSysMultiTalkPairingCoupleMode:
        mt->mic_mute = FALSE;
        mt->status = MT_ST_PARING;
        MessageCancelAll(task, EventSysMultiTalkCoupleModeReconnect);
        if (AgIsConnected())
        {
            AgDisconnect();
        }
        mtDisconnect();
        mtInquiryStop();
        inquiryStop();
        PowerAmpOn();
        mtInquiryPair(inquiry_session_2talk_pairing, FALSE);
        inquiryPair(inquiry_session_ag, TRUE);
        stateManagerUpdateState();
        MessageCancelAll(task, EventSysMultiTalkPairingTimeoutCoupleMode);
        MessageSendLater(task, EventSysMultiTalkPairingTimeoutCoupleMode, NULL, D_SEC(120));
        break;
    case EventSysMultiTalkPairingTimeoutCoupleMode:
        if(mt->mt_mode == COUPLE_MODE_PAIRING)
        {
            mt->status = MT_ST_NOCONNECT;
            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);
            stateManagerEnterConnectableState(FALSE);
            AudioPlay(AP_TOW_TALK_PAIR_FAILED, TRUE);
            mt->mt_mode = CLOSE_MODE;
            MessageCancelAll(task, EventSysMultiTalkPairingTimeoutCoupleMode);
        }
        break;
    case EventSysMultiTalkQuitPairingCoupleMode:
        if(mt->mt_mode == COUPLE_MODE_PAIRING)
        {
            mt->status = MT_ST_NOCONNECT;
            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);
            stateManagerEnterConnectableState(FALSE);
            MessageCancelAll(task, EventSysMultiTalkPairingTimeoutCoupleMode);
            mt->mt_mode = CLOSE_MODE;
            AudioPlay(AP_TWO_TALK_QUIT_PAIR, TRUE);
        }
        break;
    case EventSysMultiTalkLeaveCoupleModeDelay:
        mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
        mt->mt_device[MT_LEFT].acl_sink = NULL;
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
        mt->mt_device[MT_RIGHT].acl_sink = NULL;
        stateManagerEnterConnectableState(FALSE);
        stateManagerUpdateState();
        MessageSend(mt->app_task, EventSysMultiTalkCoupleModeLeaved, NULL);
        break;
    case EventSysMultiTalkInquiryDevices:
    {
        if (mt->status == MT_ST_PARING)
        {
            inquiry_result_t *result = sinkinquiryGetInquiryResults();
            if (result && sinkInquiryIsInqSession2Talk())
            {
                if (result[0].peer_device == remote_device_peer)
                {
                    /* 连接比自己头地址更大的设  */
                    if (BdaddrCompare(&mt->head_addr, &result[0].bd_addr) == -1)
                    {
                        deviceManagerRemoveDevice(&result[0].bd_addr);
                        mtConnectCouple(&result[0].bd_addr);
                        mt->status = MT_ST_CONNECTING;
                        mt->couple_type = COUPLE_MT_WITH_PEER;
                        break;
                    }
                }
                else /* ag */
                {
                    if(!BdaddrIsZero(&result[0].bd_addr))
                    {
                        deviceManagerRemoveDevice(&result[0].bd_addr);
                        AgConnect(&result[0].bd_addr);
                        mt->status = MT_ST_CONNECTING;
                    }
                }
            }
        }

        break;
    }
    case EventSysMultiTalkBleInquiryDevices:
    {
        if (mt->mt_mode == COUPLE_MODE_PAIRING && mt->status == MT_ST_PARING)
        {
            CL_DM_BLE_ADVERTISING_REPORT_IND_T *ind = (CL_DM_BLE_ADVERTISING_REPORT_IND_T *)message;
            if (BdaddrCompare(&mt->head_addr, &ind->current_taddr.addr) == -1)
            {
                if(mt->mt_mode == COUPLE_MODE_PAIRING)
                {
                    deviceManagerRemoveDevice(&ind->current_taddr.addr);
                }
                mtConnectCouple(&ind->current_taddr.addr);
                mt->status = MT_ST_CONNECTING;
                mt->couple_type = COUPLE_MT_WITH_PEER;
            }
        }
        if(mt->mt_mode == COUPLE_MODE && mt->status == MT_ST_SEARCHING)
        {
            CL_DM_BLE_ADVERTISING_REPORT_IND_T *ind = (CL_DM_BLE_ADVERTISING_REPORT_IND_T *)message;
            if (ind && BdaddrIsSame(&ind->current_taddr.addr, &mt->couple_addr) 
                    && BdaddrCompare(&mt->head_addr, &mt->couple_addr) == -1)
            {
                mt->status = MT_ST_CONNECTING;
                mtConnectCouple(&mt->couple_addr);
            }
        }
    }
    break;
    case EventSysMultiTalkDeviceConnected:
        if (mt->mt_mode == COUPLE_MODE_PAIRING)
        {
            AudioPlay(AP_TWO_TALK_PAIR_SUCC, TRUE);
        }
        else
        {
            AudioPlay(AP_TOW_TALK_CONNECTED, TRUE);
        }
        MessageCancelAll(task, EventSysMultiTalkPairingTimeoutCoupleMode);
        MessageCancelAll(task, EventSysRssiPairReminder);
        mt->mt_mode = COUPLE_MODE;
        mt->prepare_paring = 0;
        mt->couple_reconnect_retry = 0;
        break;
    case EventSysMultiTalkCurrentDevices:
    {
        AudioPlay(AP_TALK_DISCONNECTED, TRUE);
        stateManagerEnterConnectableState(FALSE);
        stateManagerUpdateState();
        MessageSendLater(task, EventSysRssiPairReminder, NULL, D_SEC(5));
        MessageCancelAll(task, EventSysMultiTalkCoupleModeReconnect);
        MessageSendLater(task, EventSysMultiTalkCoupleModeReconnect, NULL, D_SEC(5));
    }
    break;
    case EventSysAGSlcConnectCfm:
    {
        uint8 *status = (uint8 *)message;
        if (*status != aghfp_connect_success)
        {
            if (mt->mt_mode == COUPLE_MODE_PAIRING)
            {
                mt->status = MT_ST_PARING;
				if(*status == aghfp_connect_failed)
				{
                	deviceManagerRemoveDevice(AgGetBdaddr());
				}
            }
            else if(mt->couple_type == COUPLE_AG)
            {
                /* todo: reconnect? */
                {
                    mt->status = MT_ST_RECONNECTING;
                    MessageCancelAll(task, EventSysMultiTalkCoupleModeReconnect);
                    MessageSendLater(mt->app_task, EventSysMultiTalkCoupleModeReconnect, NULL, D_SEC(5));
                }
            }
        }
        else
        {
            mtInquiryStop();
            MessageCancelAll(task, EventSysMultiTalkCoupleModeReconnect);
        }
    }

    break;
    case EventSysAGAudioConnectCfm:
    {
        uint8 *status = (uint8 *)message;
        if (*status == aghfp_success)
        {
            if(!BdaddrIsSame(&mt->couple_addr, AgGetBdaddr()))
            {
                deviceManagerRemoveDevice(&mt->couple_addr);
            }
            mt->couple_addr = *AgGetBdaddr();
            mt->couple_type = COUPLE_AG;
            mtSaveCoupleAddr(&mt->couple_addr, COUPLE_AG);
            MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);
            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);
            MessageCancelAll(task, EventSysMultiTalkCoupleModeReconnect);

            mt->status = MT_ST_CONNECTED;
            stateManagerEnterConnectedState();
            stateManagerUpdateState();
        }
    }
    break;
    case EventSysAGSlcDisconnectInd:
    {
        /* todo reconnect */
        uint8 *status = (uint8 *)message;
        if(mt->couple_type != COUPLE_AG)
        {
            DEBUG(("CP:not ag\n"));
            break;
        }
        if (*status == aghfp_disconnect_success)
        {
            DEBUG(("CP:disconnectd\n"));
            if(mt->status == MT_ST_STAY_DISCONNET)
            {
                MessageCancelAll(task, EventSysMultiTalkLeaveCoupleModeDelay);
                MessageSend(mt->app_task, EventSysMultiTalkCoupleModeLeaved, NULL);
            }
            else
            {
                if(mt->status != MT_ST_PARING)
                {
                    mt->status = MT_ST_LINKLOSS;
                    MessageCancelAll(mt->app_task, EventSysMultiTalkCurrentDevices);
                    MessageSendLater(mt->app_task, EventSysMultiTalkCurrentDevices, NULL, D_SEC(5));
                }
            }
        }
        else
        {
            DEBUG(("CP:linkloss\n"));
            if(mt->status != MT_ST_PARING)
            {
                mt->status = MT_ST_LINKLOSS;
                MessageCancelAll(mt->app_task, EventSysMultiTalkCurrentDevices);
                MessageSendLater(mt->app_task, EventSysMultiTalkCurrentDevices, NULL, D_SEC(5));

            }
        }
        stateManagerEnterConnectableState(FALSE);
        stateManagerUpdateState();
    }
    break;
    default:
        break;
    }
    return FALSE;
}
