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

bool mtConnectNearby(bdaddr *bd_addr);

void ACLProcessParentDataNearby(const uint8_t *data, int size)
{
    switch (data[0])
    {
    case ACLMSG_CHECK_TAIL:
        if (!mtSendCheckTail())
        {
            mtSendCheckHead(1);
        }
        break;

    case ACLMSG_HEAD_ADDR:
        if (data[1] != 0)
        {
            mt->nearby_connected = data[1];
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
        else
        {
            mt->nearby_connected--;
        }
        mt->head_addr = ArrayToBdaddr(&data[2]);
        if (!mtBroadcastHeadAddr(data[1]))
        {
            mt->status = MT_ST_SEARCHING;
        }
        break;
    case ACLMSG_NEARBY_DISCONNECT:
    {
         mt->nearby_connected--;
        mtSendDisconnect();
    }
    break;
    default:
        break;
    }
}

void ACLProcessChildDataNearby(const uint8_t *data, int size)
{
    switch (data[0])
    {
    case ACLMSG_CHECK_HEAD:
        if (!mtSendCheckHead(data[1] + 1))
        {
            mt->nearby_connected = data[1] + 1;
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
            mtBroadcastHeadAddr(data[1] + 1);
        }
        break;
    default:
        break;
    }
}

bool mtConnectNearby(bdaddr *bd_addr)
{
    MT_DEBUG(("MT: mtConnectMearby to "));
    MT_DEBUG_ADDR((*bd_addr));

    mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
    BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);

    mt->mt_device[MT_CHILD].bt_addr = *bd_addr;
    mt->mt_device[MT_CHILD].state = MT_L2CAP_Connecting;
    mtACLConnect(bd_addr, MULTITALK_NEARBY_PSM);
    return TRUE;
}

bool handleMTL2capConnectIndNearbyMode(CL_L2CAP_CONNECT_IND_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capConnectIndNearbyMode: addr=%x:%x:%lx, psm=0x%x, identifier=0x%x, connID=0x%x\n",
              msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap, msg->psm,
              msg->identifier, msg->connection_id));
    if (msg->psm != MULTITALK_NEARBY_PSM)
    {
        MT_DEBUG(("MT: psm error\n"));
        return FALSE;
    }
    if (mt->mt_device[MT_PARENT].state == MT_L2CAP_Disconnected || mt->mt_device[MT_PARENT].state >= MT_L2CAP_Disconnecting)
    {
        /* ?????????????? */
        MT_DEBUG(("MT: handleMTL2capConnectIndNearbyMode parant is disconnect\n"));

        if (mt->mt_mode == CLOSE_MODE)
        {
            DEBUG(("MT: handleMTL2capConnectIndNearbyMode cannot allow connected in close mode \n"));
            return FALSE;
        }
        else
        {
            if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_CHILD].bt_addr))
            {
                DEBUG(("MT: handleMTL2capConnectIndNearbyMode cannot allow child connected to parent\n"));
                return FALSE;
            }
            else
            {
                MT_DEBUG(("MT: handleMTL2capConnectIndNearbyMode peer is my cai\n"));
                mt->mt_device[MT_PARENT].state = MT_L2CAP_Connecting;
                mt->mt_device[MT_PARENT].bt_addr = msg->bd_addr;
                mt->mt_type = MT_NODE;
                return TRUE;
            }
        }
    }
    return FALSE;
}

void handleMTL2capConnectCfmNearbyMode(CL_L2CAP_CONNECT_CFM_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capConnectCfmNearbyMode: addr=%x:%x:%lx, status=%d\n",
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
}

void handleMTL2capDisconIndNearbyMode(CL_L2CAP_DISCONNECT_IND_T *msg)
{
    /* Disconnect passive */
    MT_DEBUG(("MT: handleMTL2capDisconIndNearbyMode: status=%d\n", msg->status));
    if (msg->sink == mt->mt_device[MT_PARENT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconIndNearbyMode: parent\n"));
        mt->mt_device[MT_PARENT].state = MT_L2CAP_Disconnected;
        mt->mt_device[MT_PARENT].acl_sink = NULL;
        stateManagerEnterConnDiscoverableState(FALSE);
        stateManagerUpdateState();

        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }

        mt->head_addr = mt->addr;
        if(!mtSendCheckTail())
        {
            mt->nearby_connected = 1;
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
    }
    if (msg->sink == mt->mt_device[MT_CHILD].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconIndNearbyMode: child\n"));
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        mt->mt_device[MT_CHILD].acl_sink = NULL;
        stateManagerUpdateState();

        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }

        inquiryPair(inquiry_session_nearby, FALSE);
        mt->status = MT_ST_SEARCHING;

        if(!mtSendCheckHead(1))
        {
            mt->nearby_connected = 1;
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
    }
}

void handleMTL2capDisconCfmNearbyMode(CL_L2CAP_DISCONNECT_CFM_T *msg)
{
    /* Disconnect active */
    MT_DEBUG(("MT: handleMTL2capDisconCfmNearbyMode: status=%d\n", msg->status));
    if (msg->sink == mt->mt_device[MT_PARENT].acl_sink)
    {
        mt->mt_device[MT_PARENT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_PARENT].bt_addr);
        mt->mt_device[MT_PARENT].acl_sink = NULL;
        stateManagerUpdateState();
    }
    else if (msg->sink == mt->mt_device[MT_CHILD].acl_sink)
    {
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
        mt->mt_device[MT_CHILD].acl_sink = NULL;
        stateManagerUpdateState();
    }
    else
    {
        MT_DEBUG(("MT: not Now link\n"));
    }
    if (mtGetConnectDevices() == 0)
    {
        mt->nearby_connected = 0;
        MessageSend(mt->app_task, EventSysMultiTalkNeabyModeLeaved, NULL);
        MessageCancelAll(mt->app_task, EventSysMultiTalkLeaveNearbyModeDelay);
    }
}

void handleMTSynConnIndNearbyMode(CL_DM_SYNC_CONNECT_IND_T *msg)
{
}

void handleMTSynConnCfmNearbyMode(CL_DM_SYNC_CONNECT_CFM_T *msg)
{
    MT_DEBUG(("MT: handleMTSynConnCfmNearbyMode status=%d, link_typs = %d\n",
              msg->status, msg->link_type));
    MT_DEBUG_ADDR(msg->bd_addr);
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
                mtSendCheckHead(1);
                sinkDisableDiscoverable();
                stateManagerEnterConnectableState(FALSE);
            }
            else
            {
                mt->mt_type = MT_NODE;
                mt->status = MT_ST_CONNECTED;
                mtSendCheckTail();
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

void handleMTSynDisconIndNearbyMode(CL_DM_SYNC_DISCONNECT_IND_T *msg)
{
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
    audioUpdateAudioRouting();
}

bool processEventMultiTalkNearbyMode(Task task, MessageId id, Message message)
{
    switch (id)
    {
    case EventSysMultiTalkEnterNearbyMode:
        inquiryPair(inquiry_session_nearby, TRUE);
        mt->status = MT_ST_PARING;
        PowerAmpOn();
        break;
    case EventSysMultiTalkLeaveNearbyMode:
        mtDisconnect();
        inquiryStop();
        stateManagerEnterConnectableState(FALSE);
        MessageSendLater(task, EventSysMultiTalkLeaveNearbyModeDelay, NULL, D_SEC(1));
        break;
    case EventSysMultiTalkLeaveNearbyModeDelay:
        mt->mt_device[MT_PARENT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_PARENT].bt_addr);
        mt->mt_device[MT_PARENT].acl_sink = NULL;
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
        mt->mt_device[MT_CHILD].acl_sink = NULL;
        stateManagerUpdateState();
        mt->nearby_connected = 0;
        MessageSend(mt->app_task, EventSysMultiTalkNeabyModeLeaved, NULL);
        break;
    case EventSysMultiTalkInquiryDevices:
    {
        if (mt->mt_mode == NEARBY_MODE)
        {
            if (mt->status == MT_ST_PARING || mt->status == MT_ST_SEARCHING)
            {
                inquiry_result_t *result = sinkinquiryGetInquiryResults();
                if (result)
                {
                    /* 连接比自己头地址更大的设  */
                    if (BdaddrCompare(&mt->head_addr, &result[0].bd_addr) == -1)
                    {
                        mtConnectNearby(&result[0].bd_addr);
                        mt->status = MT_ST_CONNECTING;
                        break;
                    }
                }
            }
        }
        break;
    }
    case EventSysMultiTalkDeviceConnected:
        if (mt->nearby_connected >= 2)
        {
            AudioPlay(AP_MULTI_TALK_2_CONNECTED + mt->nearby_connected - 2, TRUE);
        }
        break;
    case EventSysMultiTalkCurrentDevices:
    {
        if (mt->nearby_connected < 1)
        {
            mt->nearby_connected = 1;
        }
        MT_DEBUG(("Play nearby connected:%d\n", mt->nearby_connected));
        AudioPlay(AP_MULTI_TALK_1_PERSON + mt->nearby_connected - 1, TRUE);
    }
    break;
    default:
        break;
    }
    return FALSE;
}
