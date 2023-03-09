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
#include "audio_prompt.h"
#include "csr_common_example_plugin.h"
#include "headset_ag.h"
#include "headset_uart.h"

extern MTData *mt;
extern const uint16 syn_conftab[];
extern TaskData acl_parent_task;
extern TaskData acl_child_task;

static bool mtConnectCouple(bdaddr *bd_addr);

bool mtSendPeerAddr(bdaddr *addr)
{
    uint8 d[1 + 8] = {0};
    d[0] = ACLMSG_PEER_ADDR;
    BdaddrToArray(addr, &d[1]);
    return ACLSend(mt->mt_device[MT_RIGHT].acl_sink, d, sizeof(d));
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
    if(state == 0)  /* peer connect success */
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

    mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
    BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);

    mt->mt_device[MT_RIGHT].bt_addr = *bd_addr;
    mt->mt_device[MT_RIGHT].state = MT_L2CAP_Connecting;
    mtACLConnect(bd_addr, MULTITALK_NEARBY_PSM);
    return TRUE;
}

bool handleMTL2capConnectIndCoupleMode(CL_L2CAP_CONNECT_IND_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capConnectIndCoupleMode: addr=%x:%x:%lx, psm=0x%x, identifier=0x%x, connID=0x%x\n",
              msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap, msg->psm,
              msg->identifier, msg->connection_id));
    if (msg->psm != MULTITALK_NEARBY_PSM)
    {
        MT_DEBUG(("MT: psm error\n"));
        return FALSE;
    }
    if(mt->nearby_connected == 8)
    {
        MT_DEBUG(("MT: connected device is 8\n"));
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
                MT_DEBUG(("MT: handleMTL2capConnectIndCoupleMode peer is my cai\n"));
                mt->mt_device[MT_LEFT].state = MT_L2CAP_Connecting;
                mt->mt_device[MT_LEFT].bt_addr = msg->bd_addr;
                mt->mt_type = MT_NODE;
                return TRUE;
            }
        }
    }
    return FALSE;
}

void handleMTL2capConnectCfmCoupleMode(CL_L2CAP_CONNECT_CFM_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capConnectCfmCoupleMode: addr=%x:%x:%lx, status=%d\n",
              msg->addr.nap, msg->addr.uap, msg->addr.lap, msg->status));
    if (BdaddrIsSame(&msg->addr, &mt->mt_device[MT_LEFT].bt_addr))
    {
        MT_DEBUG(("MT: Parent\n"));
        if (msg->status == l2cap_connect_success)
        {
            mt->mt_device[MT_LEFT].state = MT_L2CAP_Connected;
            mt->mt_device[MT_LEFT].acl_sink = msg->sink;

            ConnectionSetLinkSupervisionTimeout(msg->sink, 4000);
            linkPolicyUseMultiTalkSettings(msg->sink);

            MessageSinkTask(msg->sink, &acl_parent_task);

            linkPolicyCheckRoles();
            mtSendPeerAddr(&mt->headset_addr);
        }
    }
    else if (BdaddrIsSame(&msg->addr, &mt->mt_device[MT_RIGHT].bt_addr))
    {
        MT_DEBUG(("MT: Child\n"));
        if (msg->status == l2cap_connect_success)
        {
            mt->mt_device[MT_RIGHT].state = MT_L2CAP_Connected;
            mt->mt_device[MT_RIGHT].acl_sink = msg->sink;
            mt->mt_type = MT_NODE;

            ConnectionSetLinkSupervisionTimeout(msg->sink, 4000);
            linkPolicyUseMultiTalkSettings(msg->sink);

            MessageSinkTask(msg->sink, &acl_child_task);

            linkPolicyCheckRoles();
            mtSendPeerAddr(&mt->headset_addr);

            mtScoConnect(msg->sink);
        }
        else if (msg->status >= l2cap_connect_failed)
        {
            BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);

            if (msg->status == l2cap_connect_error) /* 137 */
            {
                deviceManagerRemoveDevice(&msg->addr);
            }
            if(mt->status == MT_ST_CONNECTED || mt->status == MT_ST_RECONNECTING)
            {
                MessageSendLater(mt->app_task, EventSysMultiTalkCoupleModeReconnect, NULL, D_SEC(3));
            }
            if(mt->status == MT_ST_CONNECTING)
            {
                mt->status = MT_ST_PARING;
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
        stateManagerEnterConnectableState(FALSE);
        stateManagerUpdateState();

        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }
        MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
    }
    if (msg->sink == mt->mt_device[MT_RIGHT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconIndCoupleMode: child\n"));
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        mt->mt_device[MT_RIGHT].acl_sink = NULL;
        stateManagerUpdateState();

        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }
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

        if(mt->status == MT_ST_CONNECTED || mt->status == MT_ST_RECONNECTING)
        {
            MessageSendLater(mt->app_task, EventSysMultiTalkCoupleModeReconnect, NULL, D_SEC(3));
        }
        if(mt->status == MT_ST_CONNECTING)
        {
            mt->status = MT_ST_PARING;
        }
    }
    else
    {
        MT_DEBUG(("MT: not Now link\n"));
    }
    if (mtGetConnectDevices() == 0)
    {
        mt->nearby_connected = 0;
        MessageSend(mt->app_task, EventSysMultiTalkCoupleModeLeaved, NULL);
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

            mt->status = MT_ST_CONNECTED;
            stateManagerEnterConnectedState();
            stateManagerUpdateState();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);

            mt->couple_addr = msg->bd_addr;
            mtSaveCoupleAddr(&mt->couple_addr, COUPLE_MT_WITH_PEER);

            MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);

            break;
        case hci_error_page_timeout:
            break;
        case hci_error_auth_fail:
            break;
        case hci_error_key_missing:
            break;
        case hci_error_conn_timeout:
            break;
        case hci_error_max_nr_of_conns:
            break;
        case hci_error_max_nr_of_sco:
            break;
        case hci_error_rej_by_remote_no_res:
            break;
        default:
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
    
            mt->mt_mode = COUPLE_MODE;
            stateManagerEnterConnectedState();
            stateManagerUpdateState();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);

            mt->couple_addr = msg->bd_addr;

            MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);

            break;
        case hci_error_page_timeout:
            break;
        case hci_error_auth_fail:
            break;
        case hci_error_key_missing:
            break;
        case hci_error_conn_timeout:
            break;
        case hci_error_max_nr_of_conns:
            break;
        case hci_error_max_nr_of_sco:
            break;
        case hci_error_rej_by_remote_no_res:
            /* todo : delay to connect */
            mtScoConnect(mt->mt_device[MT_RIGHT].acl_sink);
            break;
        default:
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

        if (msg->reason == 0)
        {
            BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
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

        if (msg->reason == 0)
        {
            BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
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
    switch (id)
    {
    case EventSysMultiTalkEnterCoupleMode:
        PowerAmpOn();
        stateManagerEnterConnectableState(FALSE);
        if(!BdaddrIsZero(&mt->couple_addr))
        {
            MessageSend(task, EventSysMultiTalkCoupleModeReconnect, NULL);
        }
        break;
    case EventSysMultiTalkLeaveCoupleMode:
        stateManagerEnterConnectableState(FALSE);
        MessageCancelAll(task, EventSysMultiTalkCoupleModeReconnect);
        if(mt->couple_type == COUPLE_MT_WITH_PEER || mt->couple_type == COUPLE_MT_NO_PEER)
        {
            if(mtGetConnectDevices() == 0)
            {
                MessageSend(task, EventSysMultiTalkCoupleModeLeaved, NULL);
            }
            else
            {
                mtDisconnect();
                MessageSendLater(task, EventSysMultiTalkLeaveCoupleModeDelay, NULL, D_SEC(1));
            }
        }
        else
        {
            /* ag */
            if(mt->status == MT_ST_CONNECTED || mt->status == MT_ST_CONNECTING || mt->status == MT_ST_RECONNECTING)
            {
                AgDisconnect();
                MessageSendLater(task, EventSysMultiTalkLeaveCoupleModeDelay, NULL, D_SEC(1));
            }
            else
            {
                MessageSend(task, EventSysMultiTalkCoupleModeLeaved, NULL);
            }
            MessageCancelAll(task, EventSysMultiTalkCoupleModeReconnect);
        }
        break;
    case EventSysMultiTalkCoupleModeReconnect:
        if(mt->couple_type == COUPLE_MT_WITH_PEER || mt->couple_type == COUPLE_MT_NO_PEER)
        {
             if (BdaddrCompare(&mt->head_addr, &mt->couple_addr) == -1)
             {
                mtConnectCouple(&mt->couple_addr);
                mt->status = MT_ST_RECONNECTING;
             }
        }
        else    /* ag */
        {
            deviceManagerRemoveDevice(&mt->couple_addr);
            AgConnect(&mt->couple_addr);
            mt->status = MT_ST_RECONNECTING;
            mt->couple_type = COUPLE_AG;
        }
        break;
    case EventSysMultiTalkPairingCoupleMode:
        if(mt->status == MT_ST_PARING)
        {
            mt->status = MT_ST_NOCONNECT;
            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);
            stateManagerEnterConnectableState(FALSE);
            MessageSend(task, EventSysMultiTalkLeaveCoupleMode, NULL);
        }
        else
        {
            inquiryPair(inquiry_session_ag, TRUE);
            mt->status = MT_ST_PARING;
            PowerAmpOn();
            AudioPlay(AP_TWO_TALK_PAIRING, TRUE);
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
            if (result)
            {
                uint8 *c = (uint8*)message;
                if(*c > 0)
                {
                    /* 连接比自己头地址更大的设  */
                    if (BdaddrCompare(&mt->head_addr, &result[0].bd_addr) == -1)
                    {
                        mtConnectCouple(&result[0].bd_addr);
                        mt->status = MT_ST_CONNECTING;
                        mt->couple_type = COUPLE_MT_WITH_PEER;
                        break;
                    }
                }
                else    /* ag */
                {
                    deviceManagerRemoveDevice(&result[0].bd_addr);
                    AgConnect(&result[0].bd_addr);
                    mt->status = MT_ST_CONNECTING;
                    mt->couple_type = COUPLE_AG;
                    mt->couple_addr = result[0].bd_addr;
                    mtSaveCoupleAddr(&mt->couple_addr, COUPLE_AG);
                }
            }
        }
        
        break;
    }
    case EventSysMultiTalkDeviceConnected:
        AudioPlay(AP_MULTI_TALK_2_CONNECTED, TRUE);
        mt->mt_mode = COUPLE_MODE;
        break;
    case EventSysMultiTalkCurrentDevices:
    {
        AudioPlay(AP_MULTI_TALK_1_PERSON, TRUE);
    }
    break;
    case EventSysAGSlcConnectCfm:
    {
        uint8 *status = (uint8 *)message;
        if(*status != aghfp_connect_success)
        {
            if(mt->mt_mode == COUPLE_MODE_PAIRING)
            {
                mt->status = MT_ST_PARING;
            }
            else
            {
                /* todo: reconnect? */
                MessageSendLater(mt->app_task, EventSysMultiTalkCoupleModeReconnect, NULL, D_SEC(1));
            }
        }
    }
    
    break;
    case EventSysAGAudioConnectCfm:
    {
        uint8 *status = (uint8 *)message;
        if(*status == aghfp_success)
        {
            MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);
            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);


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
        if(*status == aghfp_disconnect_success)
        {
            DEBUG(("CP:disconnectd\n"));
            mt->status = MT_ST_STAY_DISCONNET;
            stateManagerEnterConnectableState(FALSE);
            stateManagerUpdateState();
            MessageCancelAll(task, EventSysMultiTalkLeaveCoupleModeDelay);
            MessageSend(mt->app_task, EventSysMultiTalkCoupleModeLeaved, NULL);
        }
        else 
        {
            DEBUG(("CP:linkloss\n"));
            mt->status = MT_ST_LINKLOSS;
            AudioPlay(AP_MULTI_TALK_1_PERSON, TRUE);
        }
    }
    break;
    default:
        break;
    }
    return FALSE;
}
