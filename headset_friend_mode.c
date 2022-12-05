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
        if (mt->status == MT_ST_PARING || RouteTableIsContain(&mt->route_table, &msg->bd_addr))
        {
            DEBUG(("MT: handleMTL2capConnectInd type=%d, status = %d\n", mt->mt_type, mt->status));

            if (mt->mt_mode == CLOSE_MODE)
            {
                DEBUG(("MT: handleMTL2capConnectInd cannot allow connected in close mode \n"));
                return FALSE;
            }
            else if (mt->mt_type > MT_HEAD && mt->status != MT_ST_CONNECTED)
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
        int delay = 1;
        if(mt->mt_device[MT_PARENT].state != MT_SYN_Connected)
        {
            if(msg->status == l2cap_connect_failed)
            {
                delay = 20;
            }
            else
            {
                delay = 5;
            }
        }
        else
        {
            uint8 connected = mt->connected_device;
            uint8 i = 0;
            for(i=0; i<8; i++)
            {
                if((connected & 0x80) == 0x80)
                {
                    delay = i*2;
                    break;
                }
                connected = connected << 1;
            }
        }
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

            if (mt->status == MT_ST_CONNECTING || mt->status == MT_ST_PARING)
            {
                MT_DEBUG(("MT: Send Route Table\n"));
                MessageSendLater(mt->app_task, EventSysMultiTalkSendRouteTable, NULL, 500);
            }

            mtScoConnect(msg->sink);
        }
        else if(msg->status >= l2cap_connect_failed)
        {
            mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
            BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
        }
        if (msg->status == l2cap_connect_failed) /* 128 */
        {
            /*
                ??????????????????????????
            */
            if (mt->status == MT_ST_RECONNECTING)
            {
                mt->reconnect_count++;
                MessageCancelAll(mt->app_task, EventSysMultiTalkReconnect);
                MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(delay));
            }
            if (mt->status == MT_ST_CONNECTING)
            {
                mt->connect_idx++;
                {
                    uint8 *msg = PanicUnlessMalloc(sizeof(uint8));
                    MessageCancelAll(mt->app_task, EventMultiTalkConnect);
                    MessageSendLater(mt->app_task, EventMultiTalkConnect, msg, D_SEC(1));
                }
            }
        }
        if (msg->status == l2cap_connect_failed_remote_reject) /* 130 */
        {
            MT_DEBUG(("MT: l2cap_connect_failed_remote_reject status = %d\n", mt->status));
            if (mt->status == MT_ST_RECONNECTING)
            {
                mt->reconnect_count++;
                MT_DEBUG(("MT: RECONNECT Next\n"));
                MessageCancelAll(mt->app_task, EventSysMultiTalkReconnect);
                MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(delay));
            }
            if (mt->status == MT_ST_CONNECTING)
            {
                MT_DEBUG(("MT: CONNECT Next\n"));
                mt->connect_idx++;
                {
                    uint8 *msg = PanicUnlessMalloc(sizeof(uint8));
                    MessageCancelAll(mt->app_task, EventMultiTalkConnect);
                    MessageSendLater(mt->app_task, EventMultiTalkConnect, msg, D_SEC(1));
                }
            }
        }
        if (msg->status == l2cap_connect_failed_security) /* 132 */
        {
            linkPolicyCheckRoles();
            deviceManagerRemoveDevice(&msg->addr);
            if (mt->status == MT_ST_CONNECTING)
            {
                uint8 *m = PanicUnlessMalloc(sizeof(uint8));
                MessageCancelAll(mt->app_task, EventMultiTalkConnect);
                MessageSendLater(mt->app_task, EventMultiTalkConnect, m, D_SEC(delay));
            }
            if (mt->status == MT_ST_RECONNECTING)
            {
                mt->reconnect_count++;
                MessageCancelAll(mt->app_task, EventSysMultiTalkReconnect);
                MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(2));
            }
        }
        if (msg->status == l2cap_connect_timeout) /* 136 */
        {
            if (mt->status == MT_ST_CONNECTING)
            {
                mt->connect_idx++;
                {
                    uint8 *msg = PanicUnlessMalloc(sizeof(uint8));
                    MessageCancelAll(mt->app_task, EventMultiTalkConnect);
                    MessageSendLater(mt->app_task, EventMultiTalkConnect, msg, D_SEC(1));
                }
            }
            if (mt->status == MT_ST_RECONNECTING)
            {
                mt->reconnect_count++;
                MessageCancelAll(mt->app_task, EventSysMultiTalkReconnect);
                MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(delay));
            }
        }
        if (msg->status == l2cap_connect_error) /* 137 */
        {
            linkPolicyCheckRoles();
            deviceManagerRemoveDevice(&msg->addr);
            if (mt->status == MT_ST_RECONNECTING)
            {
                mt->reconnect_count++;
                MessageCancelAll(mt->app_task, EventSysMultiTalkReconnect);
                MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(delay));
            }
            if (mt->status == MT_ST_CONNECTING)
            {
                {
                    uint8 *msg = PanicUnlessMalloc(sizeof(uint8));
                    MessageCancelAll(mt->app_task, EventMultiTalkConnect);
                    MessageSendLater(mt->app_task, EventMultiTalkConnect, msg, D_SEC(2));
                }
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
        if(mt->status == MT_ST_CONNECTED)
        {
            mt->status = MT_ST_LINKLOSS;
        }
        stateManagerEnterConnectableState(FALSE);
        stateManagerUpdateState();

        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }

        if(!mtSendFindTail())
        {
            mt->connected_device = RouteTableGetIndex(&mt->route_table, &mt->addr);
            mt->total_connected = 1;
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
        else
        {
            mt->connected_device &= ~(RouteTableGetIndex(&mt->route_table, &mt->mt_device[MT_PARENT].bt_addr));
        }
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

        if(!mtSendCheckTTL(RouteTableGetIndex(&mt->route_table, &mt->addr)))
        {
            mt->connected_device = RouteTableGetIndex(&mt->route_table, &mt->addr);
            mt->total_connected = 1;
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
        else
        {
            mt->connected_device &= ~(RouteTableGetIndex(&mt->route_table, &mt->mt_device[MT_CHILD].bt_addr));
        }    

        mt->reconnect_count = 1;
        MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(1));
        
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
    if(mtGetConnectDevices() == 0 && mt->status == MT_ST_STAY_DISCONNET)
    {
        mt->connected_device = RouteTableGetIndex(&mt->route_table, &mt->addr);
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
                if(!RouteTableIsNotSaved(&mt->route_table))
                {
                    mtSendCheckTTL(RouteTableGetIndex(&mt->route_table, &mt->addr));
                }

                if (mt->status == MT_ST_RECONNECTING || mt->status == MT_ST_LINKLOSS || mt->status == MT_ST_PARING || mt->status == MT_ST_WAITING_CONNECT)
                {
                    if (mt->mt_device[MT_CHILD].state <= MT_L2CAP_Connecting)
                    {
                        uint8 *msg = PanicUnlessMalloc(sizeof(uint8));
                        if (mt->status == MT_ST_PARING)
                        {
                            MessageSendLater(mt->app_task, EventMultiTalkConnect, msg, D_SEC(1));
                        }
                        else
                        {
                            MessageCancelAll(mt->app_task, EventSysMultiTalkReconnect);
                            MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(1));
                        }
                        mt->mt_type = MT_TAIL;
                    }
                }
            }
            else
            {
                mt->mt_type = MT_NODE;
                mt->status = MT_ST_CONNECTED;
                mtSendFindTail();
            }

            stateManagerEnterConnectedState();
            stateManagerUpdateState();

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

            mtSaveRouteTable(&mt->route_table, mt->mt_type);

            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            if (mtGetConnectDevices() == 2 || mt->mt_type <= MT_HEAD)
            {
                stateManagerEnterConnectedState();
                stateManagerUpdateState();
                mt->status = MT_ST_CONNECTED;
                if(mt->mt_type > MT_HEAD)
                {
                    mt->mt_type = MT_NODE;
                }
            }
            else
            {
                stateManagerUpdateState();
            }

            MessageCancelAll(mt->app_task, EventMultiTalkConnect);
            MessageCancelAll(mt->app_task, EventSysMultiTalkReconnect);

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
        RouteTablePushChild(&mt->route_table, mt->addr);
        mt->total_connected = 1;
        mt->mt_type = MT_NODE;
        MessageCancelAll(task, EventMultiTalkConnect);
        MessageCancelAll(task, EventMultiTalkReconnect);
        MessageCancelAll(task, EventSysMultiTalkReconnect);
        deviceManagerRemoveAllDevices();
        mtResetPairList();
        mt->connected_device = RouteTableGetIndex(&mt->route_table, &mt->addr);
        PowerAmpOn();
        break;
    case EventMultiTalkConnect: /* enter talk connect */
        MT_DEBUG(("MT: EventMultiTalkConnect\n"));
        sinkInquirySetInquiryState(inquiry_complete);
        inquiryStop();

        if (message == NULL)
        {
            if (mt->status == MT_ST_PARING)
            {
                mt->mt_type = MT_PAIRHEAD;
            }
        }

        PowerAmpOn();

        if (mt->status == MT_ST_PARING || mt->status == MT_ST_CONNECTING)
        {
            if (mt->total_connected == mt->route_table.count)
            {
                if (mt->mt_device[MT_CHILD].state != MT_SYN_Connected)
                {
                    mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
                    BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
                }
                mt->status = MT_ST_CONNECTED;
                stateManagerEnterConnectedState();
                MessageCancelAll(task, EventMultiTalkConnect);
                break;
            }
            if (mt->status == MT_ST_PARING && mt->mt_type == MT_PAIRHEAD)
            {
                RouteTableSort(&mt->route_table);
                mt->connected_device |= RouteTableGetIndex(&mt->route_table, &mt->addr);
            }
            mt->status = MT_ST_CONNECTING;
            if (!mtRssiConnect())
            {
                mt->connect_idx++;
                MT_DEBUG(("MT: Connect Next %d\n", mt->connect_idx));
                {
                    uint8 *msg = PanicUnlessMalloc(sizeof(uint8));
                    MessageSendLater(mt->app_task, EventMultiTalkConnect, msg, 300);
                }
            }
        }

        break;
    case EventMultiTalkEndPair: /* talk pair finish */
        MT_DEBUG(("MT: EventMultiTalkQuitPair\n"));
        if (mt->status == MT_ST_PARING)
        {
            mt->status = MT_ST_NOCONNECT;
            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            stateManagerEnterConnectableState(FALSE);
            stateManagerUpdateState();
            MessageCancelAll(task, EventMultiTalkConnect);
            AudioPlay(AP_MULTI_TALK_QUIT_PAIR, TRUE);
        }
        break;
    case EventSysMultiTalkEnterFriendMode:
    case EventMultiTalkReconnect:
        {
            MT_DEBUG(("MT: EventMultiTalkReconnect\n"));
            MessageSend(task, EventSysMultiTalkReconnect, NULL);
        }
        break;
    case EventSysMultiTalkReconnect:
    {
        uint8 index = (mt->reconnect_count) % mt->route_table.count;
        MT_DEBUG(("EventSysMultiTalkReconnect to index:%d\n", index));
        /*
            if(mt->status == MT_ST_WAITING_CONNECT)
            {
                MT_DEBUG(("EventSysMultiTalkReconnect waiting connect, cancel reconnect\n"));
                break;
            }
        */

        mt->status = MT_ST_RECONNECTING;
        if (mt->route_table.count && mt->total_connected == mt->route_table.count)
        {
            if (mt->mt_device[MT_CHILD].state != MT_SYN_Connected)
            {
                mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
                BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
            }
            mt->status = MT_ST_CONNECTED;
            stateManagerEnterConnectedState();
            break;
        }
        if (!mtConnect(&mt->route_table.item[index]))
        {
            mt->reconnect_count++;
            MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, 300);
        }
    }
    break;
    case EventSysMultiTalkLeaveFriendMode:
    case EventMultiTalkDisconnect: /* quit talk */
        MT_DEBUG(("MT: EventMultiTalkDisconnect\n"));
        mt->status = MT_ST_STAY_DISCONNET;
        mt->total_connected = 1;
        mt->connected_device = RouteTableGetIndex(&mt->route_table, &mt->addr);
        stateManagerEnterConnectableState(FALSE);
        stateManagerUpdateState();
        mtDisconnect();
        MessageCancelAll(task, EventMultiTalkConnect);
        MessageCancelAll(task, EventMultiTalkReconnect);
        MessageCancelAll(task, EventSysMultiTalkReconnect);

        MessageSendLater(task, EventSysMultiTalkLeaveFriendModeDelay, NULL, D_SEC(1));
        break;
    case EventSysMultiTalkLeaveFriendModeDelay:
        mt->mt_device[MT_PARENT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_PARENT].bt_addr);
        mt->mt_device[MT_PARENT].acl_sink = NULL;
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
        mt->mt_device[MT_CHILD].acl_sink = NULL;
        stateManagerUpdateState();
        MessageSend(mt->app_task, EventSysMultiTalkFriendModeLeaved, NULL);
        break;
    case EventSysMultiTalkInquiryDevices:
    {
        if (mt->status == MT_ST_PARING)
        {
            uint8 count = *(uint8 *)message;
            uint8 idx = 0;
            inquiry_result_t *result = sinkinquiryGetInquiryResults();
            if (count >= mt->route_table.count)
            {
                AudioPlay(AP_MULTI_TALK_1_DISCOVERD + count, TRUE);
            }
            MT_DEBUG(("MT: searched %d-%d device", count, mt->route_table.count));
            if (result)
            {
                for (idx = 0; idx < count; idx++)
                {
                    RouteTablePushChild(&mt->route_table, result[idx].bd_addr);
                }
            }
        }
        if (mt->status == MT_ST_RECONNECTING)
        {
            uint8 count = *(uint8 *)message;
            uint8 idx = 0;
            inquiry_result_t *result = sinkinquiryGetInquiryResults();
            if (result)
            {
                for (idx = 0; idx < count; idx++)
                {
                    /* 连接比自己地址更小 */
                    if (BdaddrCompare(&mt->addr, &result[idx].bd_addr))
                    {
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
        /* todo */
        if (mt->mt_type == MT_TAIL && mt->total_connected < mt->route_table.count)
        {
            if (mt->status == MT_ST_CONNECTED || mt->status == MT_ST_LINKLOSS)
            {
                MessageSendLater(task, EventSysMultiTalkReconnect, NULL, D_SEC(1));
            }
        }
        ret = TRUE;
        break;
    case EventSysMultiTalkCurrentDevices:
    {
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
