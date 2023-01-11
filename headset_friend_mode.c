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

extern MTData *mt;
extern const uint16 syn_conftab[];
extern TaskData acl_parent_task;
extern TaskData acl_child_task;

bool handleMTL2capConnectIndFriendMode(CL_L2CAP_CONNECT_IND_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capConnectInd: addr=%x:%x:%lx, psm=0x%x, identifier=0x%x, connID=0x%x\n",
              msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap, msg->psm,
              msg->identifier, msg->connection_id));
    if(msg->psm != MULTITALK_FRIEND_PSM)
    {
        MT_DEBUG(("MT: psm error\n"));
        return FALSE;
    }
    if (mt->mt_device[MT_PARENT].state == MT_L2CAP_Disconnected || mt->mt_device[MT_PARENT].state >= MT_L2CAP_Disconnecting)
    {
        /* ?????????????? */
        MT_DEBUG(("MT: handleMTL2capConnectInd parant is disconnect\n"));
        if (RouteTableIsNotSaved(&mt->route_table) || RouteTableIsContain(&mt->route_table, &msg->bd_addr))
        {
            DEBUG(("MT: handleMTL2capConnectInd type=%d, status = %d\n", mt->mt_type, mt->status));

            if (mt->mt_mode == CLOSE_MODE)
            {
                DEBUG(("MT: handleMTL2capConnectInd cannot allow connected in close mode \n"));
                return FALSE;
            }
            else
            {
                if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_CHILD].bt_addr))
                {
                    DEBUG(("MT: handleMTL2capConnectInd cannot allow child connected to parent\n"));
                    return FALSE;
                }
                else
                {
                    MT_DEBUG(("MT: handleMTL2capConnectInd peer is my cai\n"));
                    mt->mt_device[MT_PARENT].state = MT_L2CAP_Connecting;
                    mt->mt_device[MT_PARENT].bt_addr = msg->bd_addr;
                    mt->mt_type = MT_NODE;
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

void handleMTL2capConnectCfmFriendMode(CL_L2CAP_CONNECT_CFM_T *msg)
{
    /*
        ??????????????????????????????????
        ????????????????????????sco ?????
    */
    MT_DEBUG(("MT: handleMTL2capConnectCfmFriendMode: addr=%x:%x:%lx, status=%d\n",
              msg->addr.nap, msg->addr.uap, msg->addr.lap, msg->status));
    if (BdaddrIsSame(&msg->addr, &mt->mt_device[MT_PARENT].bt_addr))
    {
        MT_DEBUG(("MT: Parent\n"));
        if (msg->status == l2cap_connect_success)
        {
            mt->mt_device[MT_PARENT].state = MT_L2CAP_Connected;
            mt->mt_device[MT_PARENT].acl_sink = msg->sink;

            ConnectionSetLinkSupervisionTimeout(msg->sink, 16000);
            linkPolicyUseMultiTalkSettings(msg->sink);

            MessageSinkTask(msg->sink, &acl_parent_task);

            linkPolicyCheckRoles();
        }
    }
    else if (BdaddrIsSame(&msg->addr, &mt->mt_device[MT_CHILD].bt_addr))
    {
        MT_DEBUG(("MT: Child\n"));
        if (msg->status == l2cap_connect_success)
        {
            mt->mt_device[MT_CHILD].state = MT_L2CAP_Connected;
            mt->mt_device[MT_CHILD].acl_sink = msg->sink;
            mt->mt_type = MT_NODE;

            ConnectionSetLinkSupervisionTimeout(msg->sink, 16000);
            linkPolicyUseMultiTalkSettings(msg->sink);

            MessageSinkTask(msg->sink, &acl_child_task);

            linkPolicyCheckRoles();

            mtScoConnect(msg->sink);
        }
        else if (msg->status >= l2cap_connect_failed)
        {
            BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
            if(mt->status == MT_ST_CONNECTING)
            {
                mt->status = MT_ST_SEARCHING;
            }

            if (msg->status == l2cap_connect_error) /* 137 */
            {
                deviceManagerRemoveDevice(&msg->addr);
            }
        }
    }
    else
    {
        MT_DEBUG(("MT: A loss conection, disconnect it?\n"));
    }
}

void handleMTL2capDisconIndFriendMode(CL_L2CAP_DISCONNECT_IND_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capDisconInd: status=%d\n", msg->status));
    if (msg->sink == mt->mt_device[MT_PARENT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconInd: parent\n"));
        mt->mt_device[MT_PARENT].state = MT_L2CAP_Disconnected;
        mt->mt_device[MT_PARENT].acl_sink = NULL;
		mt->head_addr = mt->addr;
        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }

        mt->head_addr = mt->addr;
        if(!mtSendFindTail(0))
        {
            inquiryStop();
            inquiryPair(inquiry_session_multi_talk, TRUE);
            mt->total_connected = 1;
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
        else
        {
            stateManagerEnterConnDiscoverableState(FALSE);
        }
        stateManagerUpdateState();

    }
    if (msg->sink == mt->mt_device[MT_CHILD].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconInd: child\n"));
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        mt->mt_device[MT_CHILD].acl_sink = NULL;
        stateManagerUpdateState();
        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }
        inquiryStop();
        inquiryPair(inquiry_session_multi_talk, TRUE);
        sinkDisableDiscoverable();
        mt->status = MT_ST_SEARCHING;
        
        if(!mtSendCheckTTL(1))
        {
            mt->total_connected = 1;
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
        else
        {
            sinkDisableDiscoverable();
        }
    }
}

void handleMTL2capDisconCfmFriendMode(CL_L2CAP_DISCONNECT_CFM_T *msg)
{
    /*
        ??? parent ??????????????????????????
        ??? child ??????????????????????????m
        ??????? child??N ?????????child->child
    */
    MT_DEBUG(("MT: handleMTL2capDisconCfm: status=%d\n", msg->status));
    if (msg->sink == mt->mt_device[MT_PARENT].acl_sink)
    {
        mt->mt_device[MT_PARENT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_PARENT].bt_addr);
        mt->mt_device[MT_PARENT].acl_sink = NULL;
        mt->head_addr = mt->addr;
        MT_DEBUG(("MT: handleMTL2capDisconCfm parent\n"));
        stateManagerUpdateState();
    }
    else if (msg->sink == mt->mt_device[MT_CHILD].acl_sink)
    {
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
        mt->mt_device[MT_CHILD].acl_sink = NULL;
        MT_DEBUG(("MT: handleMTL2capDisconCfm child"));
        stateManagerUpdateState();
    }
    else
    {
        MT_DEBUG(("MT: not Now link\n"));
    }
    if(mtGetConnectDevices() == 0 && mt->status == MT_ST_STAY_DISCONNET)
    {
        mt->head_addr = mt->addr;
        mt->status = MT_ST_NOCONNECT;
        MessageSend(mt->app_task, EventSysMultiTalkFriendModeLeaved, NULL);
        MessageCancelAll(mt->app_task, EventSysMultiTalkLeaveFriendModeDelay);
    }
}

void handleMTSynConnIndFriendMode(CL_DM_SYNC_CONNECT_IND_T *msg)
{
}

void handleMTSynConnCfmFriendMode(CL_DM_SYNC_CONNECT_CFM_T *msg)
{
    MT_DEBUG(("MT: handleMTSynConnCfm status=%d, link_typs = %d\n", msg->status, msg->link_type));
    MT_DEBUG_ADDR(msg->bd_addr);

    /*
        ???? Audio
    */

    if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_PARENT].bt_addr))
    {
        switch (msg->status)
        {
        case hci_success:
            mt->mt_device[MT_PARENT].state = MT_SYN_Connected;
            mt->mt_device[MT_PARENT].audio_sink = msg->audio_sink;
            mt->mt_device[MT_PARENT].link_type = msg->link_type;

            audioUpdateAudioRouting();

            
            if (mtGetConnectDevices() < 2)
            {
                mtSendCheckTTL(1);
                sinkDisableDiscoverable();
                stateManagerEnterConnectableState(FALSE);
                MT_DEBUG(("MT: disable discoverable\n"));
            }
            else
            {
                mt->mt_type = MT_NODE;
                if(mt->status != MT_ST_PARING)
                {
                    mt->status = MT_ST_CONNECTED;
                }
                
                mtSendFindTail(0);
                stateManagerEnterConnectedState();
            }
            stateManagerUpdateState();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);

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
    else if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_CHILD].bt_addr))
    {
        switch (msg->status)
        {
        case hci_success:
            mt->mt_device[MT_CHILD].state = MT_SYN_Connected;
            mt->mt_device[MT_CHILD].audio_sink = msg->audio_sink;
            mt->mt_device[MT_CHILD].link_type = msg->link_type;

            audioUpdateAudioRouting();

            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            
            if (mtGetConnectDevices() == 2)
            {
                stateManagerEnterConnectedState();
                stateManagerUpdateState();
            }
            else
            {
                stateManagerEnterConnDiscoverableState(FALSE);
                stateManagerUpdateState();
            }
            mt->status = MT_ST_CONNECTED;

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
            mtScoConnect(mt->mt_device[MT_CHILD].acl_sink);
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

void handleMTSynDisconIndFriendMode(CL_DM_SYNC_DISCONNECT_IND_T *msg)
{
    /*
        ???Audio?????????????????????
    */
    MT_DEBUG(("MT: handleMTSynDisconInd status=%d, reason = %d\n",
              msg->status, msg->reason));

    if (msg->audio_sink == mt->mt_device[MT_PARENT].audio_sink)
    {
        MT_DEBUG(("MT: handleMTSynDisconInd parent\n"));
        if (msg->reason == hci_success || mt->mt_device[MT_PARENT].state == MT_SYN_Disconnecting)
        {
            mtACLDisconnect(MT_PARENT);
        }

        mt->mt_device[MT_PARENT].state = MT_SYN_Disconnected;
        mt->mt_device[MT_PARENT].audio_sink = NULL;

        if (msg->reason == 0)
        {
            BdaddrSetZero(&mt->mt_device[MT_PARENT].bt_addr);
        }
    }
    else if (msg->audio_sink == mt->mt_device[MT_CHILD].audio_sink)
    {
        MT_DEBUG(("MT: handleMTSynDisconInd child\n"));
        if (msg->reason == hci_success || mt->mt_device[MT_CHILD].state == MT_SYN_Disconnecting)
        {
            mtACLDisconnect(MT_CHILD);
        }

        mt->mt_device[MT_CHILD].state = MT_SYN_Disconnected;
        mt->mt_device[MT_CHILD].audio_sink = NULL;

        if (msg->reason == 0)
        {
            BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
        }
    }
    else
    {
        MT_DEBUG(("MT: handleMTSynDisconInd who"));
    }

    if(mtGetConnectDevices() == 0)
    {
        mt->connected_device = RouteTableGetIndex(&mt->route_table, &mt->addr);
    }

    audioUpdateAudioRouting();
}

bool processEventMultiTalkFriendMode(Task task, MessageId id, Message message)
{
    bool ret = FALSE;
    switch (id)
    {
    case EventMultiTalkEnterPair:
        MT_DEBUG(("MT: Enter Pair\n"));
        mt->status = MT_ST_PARING;
        AudioPlay(AP_MULTI_TALK_FRIEND_MODE_PAIR, TRUE);
        inquiryPair(inquiry_session_multi_talk, TRUE);
        mtDisconnect();
        mtClearRouteTable(&mt->route_table);
        mt->total_connected = 1;
        mt->mt_type = MT_NODE;
        deviceManagerRemoveAllDevices();
        mtResetPairList();
        PowerAmpOn();
        break;
    case EventMultiTalkConnect: /* enter talk connect */
        MT_DEBUG(("MT: EventMultiTalkConnect\n"));
        if(mt->mt_mode == FREIEND_MODE_PAIRING)
        {
            if(mt->total_connected > 1)
            {
                mt->mt_mode = FREIEND_MODE;
                if(!mtSendFindTail(1))
                {
                    RouteTablePushChild(&mt->route_table, mt->addr);
                    mtReportRouteTable(&mt->route_table);
                }
            }
            else
            {
                MT_DEBUG(("MT: No connect, do nothing\n"));
            }
        }
        break;
    case EventMultiTalkEndPair: /* talk pair finish */
        MT_DEBUG(("MT: EventMultiTalkQuitPair\n"));
        if (mt->mt_mode == FREIEND_MODE_PAIRING)
        {
            mt->status = MT_ST_NOCONNECT;
            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            stateManagerEnterConnectableState(FALSE);
            stateManagerUpdateState();
            mtDisconnect();
            MessageSendLater(task, EventSysMultiTalkLeaveFriendModeDelay, NULL, D_SEC(1));
            AudioPlay(AP_MULTI_TALK_QUIT_PAIR, TRUE);
        }
        break;
    case EventSysMultiTalkEnterFriendMode:
    case EventMultiTalkReconnect:
        {
            MT_DEBUG(("MT: EventSysMultiTalkEnterFriendMode\n"));
            mt->head_addr = mt->addr;
            mt->status = MT_ST_SEARCHING;
            inquiryPair(inquiry_session_multi_talk, TRUE);
            PowerAmpOn();
        }
        break;
    break;
    case EventSysMultiTalkLeaveFriendMode:
    case EventMultiTalkDisconnect: /* quit talk */
        MT_DEBUG(("MT: EventMultiTalkDisconnect\n"));
        mt->status = MT_ST_STAY_DISCONNET;
        mt->total_connected = 1;
        inquiryStop();
        stateManagerEnterConnectableState(FALSE);
        stateManagerUpdateState();
        if(mtGetConnectDevices() == 0)
        {
            mt->status = MT_ST_NOCONNECT;
            MessageSend(task, EventSysMultiTalkLeaveFriendModeDelay, NULL);
        }
        else
        {
            mtDisconnect();
            MessageSendLater(task, EventSysMultiTalkLeaveFriendModeDelay, NULL, D_SEC(1));
        }
        break;
    case EventSysMultiTalkLeaveFriendModeDelay:
        mt->mt_device[MT_PARENT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_PARENT].bt_addr);
        mt->mt_device[MT_PARENT].acl_sink = NULL;
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
        mt->mt_device[MT_CHILD].acl_sink = NULL;
        mt->status = MT_ST_NOCONNECT;
        stateManagerUpdateState();
        MessageSend(mt->app_task, EventSysMultiTalkFriendModeLeaved, NULL);
        break;
    case EventSysMultiTalkInquiryDevices:
    {
        if (mt->status == MT_ST_PARING || mt->status == MT_ST_SEARCHING)
        {
            uint8 i = 0;
            uint8 *count = (uint8*)message;
            inquiry_result_t *result = sinkinquiryGetInquiryResults();
            if (result)
            {
                for(i=0; i<*count; i++)
                {
                    /* 连接比自己头地址更大的设  */
                    if (BdaddrCompare(&mt->head_addr, &result[i].bd_addr) == -1)
                    {
                        if(mtConnect(&result[i].bd_addr))
                        {
                            mt->status = MT_ST_CONNECTING;
                        }
                        break;
                    }
                }
            }
        }
    }
        ret = TRUE;
        break;
    case EventSysMultiTalkDeviceConnected:
        if (mt->total_connected >= 2)
        {
            AudioPlay(AP_MULTI_TALK_2_CONNECTED + mt->total_connected - 2, TRUE);
        }
        mt->mt_mode = FREIEND_MODE;
        ret = TRUE;
        break;
    case EventSysMultiTalkCurrentDevices:
    {
        if (mt->total_connected < 1)
        {
            mt->total_connected = 1;
        }
        AudioPlay(AP_MULTI_TALK_1_PERSON + mt->total_connected - 1, TRUE);
    }
    break;
    case EventSysMultiTalkSendRouteTable:
        mtSendRouteTable(&mt->route_table);
        break;
    default:
        break;
    }
    return ret;
}
