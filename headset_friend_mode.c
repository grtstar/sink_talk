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
#define ENABLE_MT_DEBUGx
#include "sink_debug.h"
#include "sink_audio_prompts.h"
#include "sink_statemanager.h"
#include "sink_scan.h"
#include "sink_tones.h"

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
#include "headset_multi_comm.h"
#include "headset_multi_pair.h"
#include "audio_prompt.h"
#include "csr_common_example_plugin.h"

extern MTData *mt;
extern TaskData acl_parent_task;
extern TaskData acl_child_task;

#define MT_DEBUGx(x)

#define mtInquiryStart() \
{\
    if(mt->mt_mode == FREIEND_MODE)\
    {\
        if(mtGetConnectDevices())\
            mtInquiryPair(inquiry_session_friend, FALSE);\
        else\
            mtInquiryPair(inquiry_session_friend, TRUE);\
    }\
    if(mt->mt_mode == FREIEND_MODE_PAIRING)\
    {\
        mtInquiryPair(inquiry_session_multi_talk, TRUE);\
    }\
}

bool mtConnect(bdaddr *bd_addr)
{
    MT_DEBUG(("MT: mtConnect to "));
    MT_DEBUG_ADDR((*bd_addr));
    mtInquiryStop();
    if (RouteTableIsNotSaved(&mt->route_table) || RouteTableIsContain(&mt->route_table, bd_addr))
    {
        if (mt->mt_device[MT_RIGHT].state < MT_L2CAP_WaitConnect)
        {
            mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
            BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);

            mt->mt_device[MT_RIGHT].bt_addr = *bd_addr;
            mt->mt_device[MT_RIGHT].state = MT_L2CAP_Connecting;
        }
        else if (mt->mt_device[MT_LEFT].state < MT_L2CAP_WaitConnect)
        {
            mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
            BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);

            mt->mt_device[MT_LEFT].bt_addr = *bd_addr;
            mt->mt_device[MT_LEFT].state = MT_L2CAP_Connecting;
        }
        else
        {
            MT_DEBUG(("MT: no pos to connect\n"));
            return FALSE;
        }

        mtACLConnect(bd_addr, MULTITALK_FRIEND_PSM);
        return TRUE;
    }
    return FALSE;
}
bool handleMTL2capConnectIndFriendMode(CL_L2CAP_CONNECT_IND_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capConnectInd: addr=%x:%x:%lx, psm=0x%x, identifier=0x%x, connID=0x%x\n",
              msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap, msg->psm,
              msg->identifier, msg->connection_id));
    if (msg->psm != MULTITALK_FRIEND_PSM)
    {
        MT_DEBUG(("MT: psm error\n"));
        return FALSE;
    }
    if (mt->total_connected >= 8)
    {
        MT_DEBUG(("MT: connected device >= 8\n"));
        return FALSE;
    }
    if (mt->mt_mode == FREIEND_MODE)
    {
        if (!RouteTableIsContain(&mt->route_table, &msg->bd_addr))
        {
            MT_DEBUG(("MT: not in table\n"));
            return FALSE;
        }
    }

    if (mt->connect_token != TOKEN_IDLE)
    {
        if (BdaddrCompare(&mt->header_addr[0], &msg->bd_addr) == -1 || BdaddrCompare(&mt->header_addr[1], &msg->bd_addr) == -1)
        {
            MT_DEBUG(("MT: token held, cannot allow larger [%x:%x:%lx] than header [%x:%x:%lx] [%x:%x:%lx]\n", msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap,
                      mt->header_addr[0].nap, mt->header_addr[0].uap, mt->header_addr[0].lap,
                      mt->header_addr[1].nap, mt->header_addr[1].uap, mt->header_addr[1].lap));
            return FALSE;
        }
    }
	if(BdaddrIsSame(&msg->bd_addr, &mt->header_addr[0]) || BdaddrIsSame(&msg->bd_addr, &mt->header_addr[1]))
    {
        MT_DEBUG(("MT: cannot allow header connect me\n"));
        return FALSE;
    }
    if(mt->status == MT_ST_CONNECTING)
    {
        if(BdaddrCompare(&mt->addr, &msg->bd_addr) == 1)
        {
            MT_DEBUG(("MT: delay\n"));
            mt->last_connect_time = VmGetClock();
        }
    }

    if (mt->mt_device[MT_LEFT].state == MT_L2CAP_Disconnected || mt->mt_device[MT_LEFT].state >= MT_L2CAP_Disconnecting)
    {
        /* ?????????????? */
        MT_DEBUG(("MT: handleMTL2capConnectInd parant is disconnect\n"));
        if (mt->mt_mode == FREIEND_MODE_PAIRING || RouteTableIsContain(&mt->route_table, &msg->bd_addr))
        {
            DEBUG(("MT: handleMTL2capConnectInd type=%d, status = %d\n", mt->mt_type, mt->status));

            if (mt->status != MT_ST_SEARCHING && mt->status != MT_ST_PARING)
            {
                DEBUG(("MT: handleMTL2capConnectIndFriendMode cannot allow connected in non search status \n"));
                return FALSE;
            }
            else
            {
                if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_RIGHT].bt_addr))
                {
                    DEBUG(("MT: handleMTL2capConnectInd cannot allow child connected to parent\n"));
                    return FALSE;
                }
                else
                {
                    MT_DEBUG(("MT: handleMTL2capConnectInd peer is my cai\n"));
                    mt->mt_device[MT_LEFT].state = MT_L2CAP_WaitConnect;
                    mt->mt_device[MT_LEFT].bt_addr = msg->bd_addr;
                    mt->mt_type = MT_NODE;
                    return TRUE;
                }
            }
        }
    }

    if (mt->mt_device[MT_RIGHT].state == MT_L2CAP_Disconnected || mt->mt_device[MT_RIGHT].state >= MT_L2CAP_Disconnecting)
    {
        MT_DEBUG(("MT: handleMTL2capConnectIndFriendMode child is disconnect\n"));

        if (mt->status != MT_ST_SEARCHING && mt->status != MT_ST_PARING)
        {
            DEBUG(("MT: handleMTL2capConnectIndFriendMode cannot allow connected in non search status \n"));
            return FALSE;
        }
        else
        {
            {
                MT_DEBUG(("MT: handleMTL2capConnectIndFriendMode peer is my cai to child\n"));
                mt->mt_device[MT_RIGHT].state = MT_L2CAP_WaitConnect;
                mt->mt_device[MT_RIGHT].bt_addr = msg->bd_addr;
                mt->mt_type = MT_NODE;
                return TRUE;
            }
        }
    }
    return FALSE;
}

void handleMTL2capConnectCfmFriendMode(CL_L2CAP_CONNECT_CFM_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capConnectCfmFriendMode: addr=%x:%x:%lx, status=%d\n",
              msg->addr.nap, msg->addr.uap, msg->addr.lap, msg->status));
    if (msg->psm_local != MULTITALK_FRIEND_PSM)
    {
        if (msg->status == l2cap_connect_success)
        {
            MT_DEBUG(("MT: psm(%d) error, disconnect\n", msg->psm_local));
            ConnectionL2capDisconnectRequest(&mt->mt_task, msg->sink);
        }
    }
    if (BdaddrIsSame(&msg->addr, &mt->mt_device[MT_LEFT].bt_addr))
    {
        MT_DEBUG(("MT: left state = %d\n", mt->mt_device[MT_LEFT].state));
        ConnectionSetLinkSupervisionTimeout(msg->sink, 4000);
        linkPolicyUseMultiTalkSettings(msg->sink);

        if (msg->status == l2cap_connect_success)
        {
            BdaddrSetZero(&mt->last_connect_addr);

            if (mt->mt_device[MT_LEFT].state == MT_L2CAP_Connecting)
            {
                mt->mt_device[MT_LEFT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_LEFT].acl_sink = msg->sink;
                MessageSinkTask(msg->sink, &acl_parent_task);

                /* 连上后需要检测是否成�如果成环则断开 */
                if(mt->mt_device[MT_RIGHT].state >= MT_L2CAP_Connected)
                {
                    mt->status = MT_ST_CHECKLOOP;
                    mt->sco_expend_dev = MT_LEFT;
                    MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoop, NULL, D_SEC(0.5));
                    MT_DEBUG(("MT: Check loop\n"));
                }
                else
                {
                    MT_DEBUG(("MT: maybe no loop\n"));
                    mt->status = MT_ST_NOLOOP;
                    mt->sco_expend_dev = MT_LEFT;
                    MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoop, NULL, D_SEC(0.5));
                }
            }
            else if (mt->mt_device[MT_LEFT].state == MT_L2CAP_WaitConnect)
            {
                mt->mt_device[MT_LEFT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_LEFT].acl_sink = msg->sink;
                MessageSinkTask(msg->sink, &acl_parent_task);
            }

            linkPolicyCheckRoles();
        }
        else if (msg->status > l2cap_connect_failed || (msg->status == l2cap_connect_failed && mt->status == MT_ST_CONNECTING))
        {
            BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
            mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
            mtSendConnectToken(MT_RIGHT, TOKEN_RELEASE);
            if (mt->status == MT_ST_CONNECTING)
            {
                if (mtGetConnectDevices() == 2)
                {
                    mt->mt_type = MT_NODE;
                    mt->status = MT_ST_CONNECTED;
                    mtInquiryStop();
                    stateManagerEnterConnectedState();
                }
                else
                {
                    mtInquiryStart();
                    mt->status = MT_ST_SEARCHING;
                }
            }
            if (msg->status == l2cap_connect_error || msg->status >= l2cap_connect_failed_config_rejected) /* 137 */
            {
                deviceManagerRemoveDevice(&msg->addr);
            }
        }
    }
    else if (BdaddrIsSame(&msg->addr, &mt->mt_device[MT_RIGHT].bt_addr))
    {
        MT_DEBUG(("MT: right\n"));
        if (msg->status == l2cap_connect_success)
        {
            ConnectionSetLinkSupervisionTimeout(msg->sink, 4000);
            linkPolicyUseMultiTalkSettings(msg->sink);

            BdaddrSetZero(&mt->last_connect_addr);

            if (mt->mt_device[MT_RIGHT].state == MT_L2CAP_Connecting)
            {
                mt->mt_device[MT_RIGHT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_RIGHT].acl_sink = msg->sink;
                mt->mt_type = MT_NODE;
                MessageSinkTask(msg->sink, &acl_child_task);

                /* 连上后需要检测是否成�如果成环则断开 */
                if(mt->mt_device[MT_LEFT].state >= MT_L2CAP_Connected)
                {
                    mt->status = MT_ST_CHECKLOOP;
                    mt->sco_expend_dev = MT_RIGHT;
                    MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoop, NULL, D_SEC(0.5));
                    MT_DEBUG(("MT: Check loop\n"));
                }
                else
                {
                    MT_DEBUG(("MT: maybe no loop\n"));
                    mt->status = MT_ST_NOLOOP;
                    mt->sco_expend_dev = MT_RIGHT;
                    MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoop, NULL, D_SEC(0.5));
                }
            }
            else if (mt->mt_device[MT_RIGHT].state == MT_L2CAP_WaitConnect)
            {
                mt->mt_device[MT_RIGHT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_RIGHT].acl_sink = msg->sink;
                mt->mt_type = MT_NODE;
                MessageSinkTask(msg->sink, &acl_child_task);
            }

            linkPolicyCheckRoles();
        }
        else if (msg->status > l2cap_connect_failed || (msg->status == l2cap_connect_failed && mt->status == MT_ST_CONNECTING))
        {
            BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
            mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
            if (mt->status == MT_ST_CONNECTING)
            {
                mtInquiryStart();
                mt->status = MT_ST_SEARCHING;
            }

            if (msg->status == l2cap_connect_error || msg->status == l2cap_connect_failed_security) /* 137 */
            {
                deviceManagerRemoveDevice(&msg->addr);
            }
            mtSendConnectToken(MT_LEFT, TOKEN_RELEASE);
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
    if (msg->sink == mt->mt_device[MT_LEFT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconInd: parent\n"));
        mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
        mt->mt_device[MT_LEFT].acl_sink = NULL;
        stateManagerUpdateState();

        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }
         mtInquiryStop();
        if (mt->status != MT_ST_NOCONNECT)
        {
            if (mt->mt_mode == FREIEND_MODE)
            {
                mt->status = MT_ST_SEARCHING;
            }
            else
            {
                mt->status = MT_ST_PARING;
            }
            mtInquiryStart();
        }
        mt->header_addr[0] = mt->addr;
        BdaddrSetZero(&mt->header_addr[1]);
        if (!mtBroadcastHeaderAddr1(MT_RIGHT, 1, &mt->addr))
        {
            mt->total_connected = 1;
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
    }
    if (msg->sink == mt->mt_device[MT_RIGHT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconInd: child\n"));
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
        mt->mt_device[MT_RIGHT].acl_sink = NULL;
        stateManagerUpdateState();
        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }
        mtInquiryStop();
        if (mt->status != MT_ST_NOCONNECT)
        {
            if (mt->mt_mode == FREIEND_MODE)
            {
                mt->status = MT_ST_SEARCHING;
            }
            else
            {
                mt->status = MT_ST_PARING;
            }
            mtInquiryStart();
        }
        mt->header_addr[0] = mt->addr;
        BdaddrSetZero(&mt->header_addr[1]);
        if (!mtBroadcastHeaderAddr1(MT_LEFT, 1, &mt->addr))
        {
            mt->total_connected = 1;
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
    }
    mt->connect_token = TOKEN_IDLE;
}

void handleMTL2capDisconCfmFriendMode(CL_L2CAP_DISCONNECT_CFM_T *msg)
{
    /*
        ??? parent ??????????????????????????
        ??? child ??????????????????????????m
        ??????? child??N ?????????child->child
    */
    MT_DEBUG(("MT: handleMTL2capDisconCfm: status=%d\n", msg->status));
    if (msg->sink == mt->mt_device[MT_LEFT].acl_sink)
    {
        mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
        mt->mt_device[MT_LEFT].acl_sink = NULL;
        mt->head_addr = mt->addr;
        MT_DEBUG(("MT: handleMTL2capDisconCfm parent\n"));
        if(mt->status == MT_ST_CHECKLOOP || mt->status == MT_ST_NOLOOP)
        {
            if(mt->sco_expend_dev == MT_LEFT)
            {
                mt->status = MT_ST_SEARCHING;
            }
        }
        stateManagerUpdateState();
    }
    else if (msg->sink == mt->mt_device[MT_RIGHT].acl_sink)
    {
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
        mt->mt_device[MT_RIGHT].acl_sink = NULL;
        MT_DEBUG(("MT: handleMTL2capDisconCfm child\n"));
        if(mt->status == MT_ST_CHECKLOOP || mt->status == MT_ST_NOLOOP)
        {
            if(mt->sco_expend_dev == MT_RIGHT)
            {
                mt->status = MT_ST_SEARCHING;
            }
        }
        stateManagerUpdateState();
    }
    else
    {
        MT_DEBUG(("MT: not Now link\n"));
        return;
    }
    if (mtGetConnectDevices() == 0 && mt->status == MT_ST_STAY_DISCONNET)
    {
        mt->total_connected = 0;
        mt->head_addr = mt->addr;
        mt->header_addr[0] = mt->addr;
        BdaddrSetZero(&mt->header_addr[1]);
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

    if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_LEFT].bt_addr))
    {
        switch (msg->status)
        {
        case hci_success:
            mt->mt_device[MT_LEFT].state = MT_SYN_Connected;
            mt->mt_device[MT_LEFT].audio_sink = msg->audio_sink;
            mt->mt_device[MT_LEFT].link_type = msg->link_type;
            MT_DEBUG(("MT: left sync connected\n"));
            audioUpdateAudioRouting();

            if (mtGetConnectDevices() == 2)
            {
                mt->mt_type = MT_NODE;
                mt->status = MT_ST_CONNECTED;
                mtInquiryStop();
                stateManagerEnterConnectedState();
            }
            else
            {
                mt->status = MT_ST_SEARCHING;
            }
            stateManagerUpdateState();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);

            break;
        case hci_error_page_timeout:
        case hci_error_auth_fail:
        case hci_error_key_missing:
        case hci_error_conn_timeout:
        case hci_error_max_nr_of_conns:
        case hci_error_max_nr_of_sco:
        case hci_error_rej_by_remote_no_res:
        default:
            if(mt->mt_device[MT_LEFT].state == MT_SYN_Connecting)
            {
                mtACLDisconnect(MT_LEFT);
            }
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
            MT_DEBUG(("MT: right sync connected\n"));

            audioUpdateAudioRouting();

            if (mtGetConnectDevices() == 2)
            {
                mt->mt_type = MT_NODE;
                mt->status = MT_ST_CONNECTED;
                mtInquiryStop();
                stateManagerEnterConnectedState();
            }
            else
            {
                mt->status = MT_ST_SEARCHING;
            }
            stateManagerUpdateState();
            MessageCancelAll(mt->app_task, EventSysRssiPairReminder);

            break;
        case hci_error_page_timeout:
        case hci_error_auth_fail:
        case hci_error_key_missing:
        case hci_error_conn_timeout:
        case hci_error_max_nr_of_conns:
        case hci_error_max_nr_of_sco:
        case hci_error_rej_by_remote_no_res:
        default:
            if(mt->mt_device[MT_RIGHT].state == MT_SYN_Connecting)
            {
                mtACLDisconnect(MT_RIGHT);
            }
            break;
        }
    }
    else
    {
        MT_DEBUG(("MT: A loss conection, disconnect sync?\n"));
        if(msg->status == hci_success)
        {
            ConnectionSyncDisconnect(msg->audio_sink, hci_error_oetc_user);
        }
    }
}

void handleMTSynDisconIndFriendMode(CL_DM_SYNC_DISCONNECT_IND_T *msg)
{
    /*
        ???Audio?????????????????????
    */
    MT_DEBUG(("MT: handleMTSynDisconInd status=%d, reason = %d\n",
              msg->status, msg->reason));

    if (msg->audio_sink == mt->mt_device[MT_LEFT].audio_sink)
    {
        MT_DEBUG(("MT: handleMTSynDisconInd parent\n"));
        if (msg->reason == hci_success || mt->mt_device[MT_LEFT].state == MT_SYN_Disconnecting)
        {
            mtACLDisconnect(MT_LEFT);
        }

        mt->mt_device[MT_LEFT].state = MT_SYN_Disconnected;
        mt->mt_device[MT_LEFT].audio_sink = NULL;

        if (msg->reason == 0)
        {
            mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        }
    }
    else if (msg->audio_sink == mt->mt_device[MT_RIGHT].audio_sink)
    {
        MT_DEBUG(("MT: handleMTSynDisconInd child\n"));
        if (msg->reason == hci_success || mt->mt_device[MT_RIGHT].state == MT_SYN_Disconnecting)
        {
            mtACLDisconnect(MT_RIGHT);
        }

        mt->mt_device[MT_RIGHT].state = MT_SYN_Disconnected;
        mt->mt_device[MT_RIGHT].audio_sink = NULL;

        if (msg->reason == 0)
        {
            mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        }
    }
    else
    {
        MT_DEBUG(("MT: handleMTSynDisconInd who"));
    }

    if (mtGetConnectDevices() == 0)
    {
        mt->connected_device = RouteTableGetIndex(&mt->route_table, &mt->addr);
    }

    audioUpdateAudioRouting();
}

bool processEventMultiTalkFriendMode(Task task, MessageId id, Message message)
{
    if (mt->mt_mode != FREIEND_MODE && mt->mt_mode != FREIEND_MODE_PAIRING)
    {
        return FALSE;
    }
    switch (id)
    {
    case EventUsrEnterPairing:
        if(mt->mt_mode == FREIEND_MODE_PAIRING)
        {
            mt->status = MT_ST_NOCONNECT;
            sinkInquirySetInquiryState(inquiry_complete);
            mtInquiryStop();
            stateManagerEnterConnectableState(FALSE);
            mtDisconnect();
            mt->connect_token = TOKEN_IDLE;
            mtLoadRouteTable(&mt->route_table);
            mt->mt_mode = CLOSE_MODE;
            stateManagerUpdateState();
        }
        break;
    case EventMultiTalkEnterPair:
        MT_DEBUG(("MT: Enter Pair\n"));
        mt->mic_mute = FALSE;
        mt->status = MT_ST_PARING;
        mtDisconnect();
        AudioPlay(AP_MULTI_TALK_FRIEND_MODE_PAIR, TRUE);
        mtInquiryStop();
        memset(&mt->route_table, 0, sizeof(RouteTable));
        mt->header_addr[0] = mt->addr;
        BdaddrSetZero(&mt->header_addr[1]);
        mt->total_connected = 1;
        mt->mt_type = MT_NODE;
        mtInquiryPair(inquiry_session_multi_talk, TRUE);
        PowerAmpOn();
        MessageCancelAll(task, EventSysMultiTalkFriendSaveListDelay);
        break;
    case EventMultiTalkConnect: /* enter talk connect */
        MT_DEBUG(("MT: EventMultiTalkConnect\n"));
        if (mt->mt_mode == FREIEND_MODE_PAIRING)
        {
            if (mt->total_connected > 1 && mt->total_connected <= 8)
            {
                if (!mtSendFindTail(MT_RIGHT, 1))
                {
                    RouteTablePushChild(&mt->route_table, mt->addr);
                    mtReportRouteTable(MT_LEFT, &mt->route_table);
                    MessageSendLater(task, EventSysMultiTalkFriendSaveListDelay, NULL, D_SEC(5));
                }
            }
            else
            {
                if(mt->total_connected > 8)
                {
                    AudioPlay(AP_MULTI_TALK_9_PAIRED, TRUE);
                }
                AudioPlay(AP_SAVE_FREIND_FAILED, TRUE);
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
            mtInquiryStop();
            stateManagerEnterConnectableState(FALSE);
            stateManagerUpdateState();
            mtDisconnect();
            MessageSendLater(task, EventSysMultiTalkLeaveFriendModeDelay, NULL, D_SEC(2));
            AudioPlay(AP_MULTI_TALK_QUIT_PAIR, TRUE);
            mt->connect_token = TOKEN_IDLE;
            mtLoadRouteTable(&mt->route_table);
            MessageCancelAll(task, EventSysMultiTalkFriendSaveListDelay);
        }
        break;
    case EventSysMultiTalkEnterFriendMode:
    {
        mt->mic_mute = FALSE;
    }
    case EventMultiTalkReconnect:
    {
        MT_DEBUG(("MT: EventSysMultiTalkEnterFriendMode\n"));
        mt->head_addr = mt->addr;
        mt->status = MT_ST_SEARCHING;
        mt->header_addr[0] = mt->addr;
        BdaddrSetZero(&mt->header_addr[1]);
        mtInquiryPair(inquiry_session_friend, TRUE);
        PowerAmpOn();
    }
    break;
    case EventSysMultiTalkLeaveFriendMode:
    case EventMultiTalkDisconnect: /* quit talk */
        MT_DEBUG(("MT: EventMultiTalkDisconnect\n"));
        mt->total_connected = 1;
        mtInquiryStop();
        if (mtGetConnectDevices() == 0)
        {
            mt->status = MT_ST_NOCONNECT;
            MessageSend(task, EventSysMultiTalkLeaveFriendModeDelay, NULL);
        }
        else
        {
            mt->status = MT_ST_STAY_DISCONNET;
            MessageSendLater(task, EventSysMultiTalkLeaveFriendModeDelay, NULL, D_SEC(2));
        }
        mtDisconnect();
        mt->connect_token = TOKEN_IDLE;
        break;
    case EventSysMultiTalkLeaveFriendModeDelay:
        mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
        mt->mt_device[MT_LEFT].acl_sink = NULL;
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
        mt->mt_device[MT_RIGHT].acl_sink = NULL;
        mt->status = MT_ST_NOCONNECT;
        mt->total_connected = 0;
        stateManagerEnterConnectableState(FALSE);
        stateManagerUpdateState();
        MessageSend(mt->app_task, EventSysMultiTalkFriendModeLeaved, NULL);
        break;
    case EventSysMultiTalkBleInquiryDevices:
    {
		CL_DM_BLE_ADVERTISING_REPORT_IND_T *ind = (CL_DM_BLE_ADVERTISING_REPORT_IND_T *)message;
        if(BdaddrCompare(&mt->addr, &ind->current_taddr.addr) == 1 && !BdaddrIsSame(&mt->header_addr[0], &ind->current_taddr.addr) && !BdaddrIsSame(&mt->header_addr[1], &ind->current_taddr.addr))
        {
            mt->last_connect_time = VmGetClock();
        }
        if (ind)
        {
            /* MT_DEBUG(("MT: search %x:%x:%lx\n", 
                    ind->current_taddr.addr.nap,
                        ind->current_taddr.addr.uap,
                        ind->current_taddr.addr.lap)); */
        }
        if (mt->total_connected >= 8)
        {
            MT_DEBUG(("MT: but total_connected >= 8\n"));
            break;
        }
        if (mt->status != MT_ST_SEARCHING && mt->status != MT_ST_PARING)
        {
            MT_DEBUG(("MT: search device, but status is %d\n", mt->status));
            break;
        }
        if (VmGetClock() - mt->last_connect_time < D_SEC(5))
        {   
            /* MT_DEBUG(("MT: but same as last, delay 5s\n")); */
            break;
        }

        if (mt->mt_mode == FREIEND_MODE || mt->mt_mode == FREIEND_MODE_PAIRING)
        {
            if (mt->connect_token == TOKEN_IDLE)
            {
                if (mt->status == MT_ST_SEARCHING || mt->status == MT_ST_PARING)
                {
                    if (ind)
                    {
                        if (RouteTableIsContain(&mt->route_table, &ind->current_taddr.addr) || mt->mt_mode == FREIEND_MODE_PAIRING)
                        {
                            /* 连接比自己地址更大的设  */
                            if (BdaddrCompare(&mt->addr, &ind->current_taddr.addr) == -1 &&
                                !BdaddrIsSame(&mt->header_addr[0], &ind->current_taddr.addr) &&
                                !BdaddrIsSame(&mt->header_addr[1], &ind->current_taddr.addr)  &&
                                !BdaddrIsSame(&mt->mt_device[MT_LEFT].bt_addr, &ind->current_taddr.addr) &&
                                !BdaddrIsSame(&mt->mt_device[MT_RIGHT].bt_addr, &ind->current_taddr.addr))
                            {
                                if (mt->mt_device[0].state >= MT_L2CAP_Connected)
                                {
                                    mtSendConnectToken(0, TOKEN_HELD);
                                }
                                else if (mt->mt_device[1].state >= MT_L2CAP_Connected)
                                {
                                    mtSendConnectToken(1, TOKEN_HELD);
                                }
                                mt->last_connect_addr = ind->current_taddr.addr;
                                mt->status = MT_ST_CONNECTING;
                                if(mt->mt_mode == FREIEND_MODE_PAIRING)
                                {
                                    deviceManagerRemoveDevice(&mt->last_connect_addr);
                                }
                                MessageSendLater(task, EventSysMultiTalkConnect, NULL, D_SEC(1));
                                break;
                            }
                            else
                            {
                                MT_DEBUG(("but header is [%x:%x:%lx]",
                                          mt->header_addr[0].nap,
                                          mt->header_addr[0].uap,
                                          mt->header_addr[0].lap));
                                MT_DEBUG((" [%x:%x:%lx]\n",
                                          mt->header_addr[1].nap,
                                          mt->header_addr[1].uap,
                                          mt->header_addr[1].lap));
                            }
                        }
                        else
                        {
                            MT_DEBUG(("MT: but not in route table\n"));
                        }
                    }
                }
            }
            else
            {
                MT_DEBUG(("MT: search device, but no token\n"));
            }
        }
        return TRUE;
    }
#if 0
    case EventSysMultiTalkInquiryDevices:
    {
        if (mt->mt_mode == FREIEND_MODE)
        {
            if (mt->connect_token == TOKEN_IDLE)
            {
                if (mt->status == MT_ST_SEARCHING)
                {
                    inquiry_result_t *result = sinkinquiryGetInquiryResults();
                    if (result && sinkInquiryIsInqSessionFriend())
                    {
                        if (RouteTableIsContain(&mt->route_table, &result[0].bd_addr))
                        {
                            /* 连接比自己地址更大的设  */
                            if (BdaddrCompare(&mt->addr, &result[0].bd_addr) == -1 &&
                                !BdaddrIsSame(&mt->header_addr[0], &result[0].bd_addr) &&
                                !BdaddrIsSame(&mt->header_addr[1], &result[0].bd_addr) &&
                                !BdaddrIsSame(&mt->mt_device[MT_LEFT].bt_addr, &result[0].bd_addr) &&
                                !BdaddrIsSame(&mt->mt_device[MT_RIGHT].bt_addr, &result[0].bd_addr))
                            {
                                if (mt->mt_device[0].state >= MT_L2CAP_Connected)
                                {
                                    mtSendConnectToken(0, TOKEN_HELD);
                                }
                                else if (mt->mt_device[1].state >= MT_L2CAP_Connected)
                                {
                                    mtSendConnectToken(1, TOKEN_HELD);
                                }
                                mtConnect(&result[0].bd_addr);
                                mt->status = MT_ST_CONNECTING;
                                break;
                            }
                            else
                            {
                                MT_DEBUG(("MT: search %x:%x:%lx ", result[0].bd_addr.nap,
                                          result[0].bd_addr.uap,
                                          result[0].bd_addr.lap));
                                MT_DEBUG(("but header is [%x:%x:%lx] [%x:%x:%lx]\n",
                                      mt->header_addr[0].nap,
                                      mt->header_addr[0].uap,
                                      mt->header_addr[0].lap,
                                      mt->header_addr[1].nap,
                                      mt->header_addr[1].uap,
                                      mt->header_addr[1].lap));
                            }
                        }
                        else
                        {
                            MT_DEBUG(("MT: search %x:%x:%lx, but not in route table\n",
                                      result[0].bd_addr.nap,
                                      result[0].bd_addr.uap,
                                      result[0].bd_addr.lap));
                        }
                    }
                }
                else
                {
                    MT_DEBUG(("MT: search device, but status is %d\n", mt->status));
                }
            }
            else
            {
                MT_DEBUG(("MT: search device, but no token\n"));
            }
        }
        if (mt->mt_mode == FREIEND_MODE_PAIRING)
        {
            if (mt->status == MT_ST_PARING || mt->status == MT_ST_SEARCHING)
            {
                inquiry_result_t *result = sinkinquiryGetInquiryResults();
                if (result &&  sinkInquiryIsInqSessionMuliTalk())
                {
                    if (mt->mt_device[MT_RIGHT].state != MT_L2CAP_Disconnected)
                    {
                        MT_DEBUG(("MT: right is connected, dont pair\n"));
                        break;
                    }
                    {
                        /* 连接比自己地址更大的设  */
                        bdaddr head = mt->addr;
                        if (BdaddrCompare(&head, &mt->header_addr[0]) == 1 && !BdaddrIsZero(&mt->header_addr[0]))
                        {
                            head = mt->header_addr[0];
                        }
                        if (BdaddrCompare(&head, &mt->header_addr[1]) == 1 && !BdaddrIsZero(&mt->header_addr[1]))
                        {
                            head = mt->header_addr[1];
                        }
                        if (BdaddrCompare(&head, &result[0].bd_addr) == -1 &&
                            !BdaddrIsSame(&mt->header_addr[0], &result[0].bd_addr) &&
                            !BdaddrIsSame(&mt->header_addr[1], &result[0].bd_addr) &&
                            !BdaddrIsSame(&mt->mt_device[MT_LEFT].bt_addr, &result[0].bd_addr) &&
                            !BdaddrIsSame(&mt->mt_device[MT_RIGHT].bt_addr, &result[0].bd_addr))
                        {
                            if (mt->mt_device[0].state >= MT_L2CAP_Connected)
                            {
                                mtSendConnectToken(0, TOKEN_HELD);
                            }
                            else if (mt->mt_device[1].state >= MT_L2CAP_Connected)
                            {
                                mtSendConnectToken(1, TOKEN_HELD);
                            }
                            mtConnect(&result[0].bd_addr);
                            mt->status = MT_ST_CONNECTING;
                            break;
                        }
                        else
                        {
                            MT_DEBUG(("MT: search %x:%x:%lx ", result[0].bd_addr.nap,
                                          result[0].bd_addr.uap,
                                          result[0].bd_addr.lap));
                            MT_DEBUG(("but header is [%x:%x:%lx] [%x:%x:%lx]\n",
                                      mt->header_addr[0].nap,
                                      mt->header_addr[0].uap,
                                      mt->header_addr[0].lap,
                                      mt->header_addr[1].nap,
                                      mt->header_addr[1].uap,
                                      mt->header_addr[1].lap));
                        }
                    }
                }
            }
        }
        return TRUE;
    }
#endif
    case EventSysMultiTalkConnect:
        if(mt->status == MT_ST_CONNECTING)
        {
            if(!mtConnect(&mt->last_connect_addr))
            {
                mt->status = MT_ST_SEARCHING;
            }
        }
        break;
    case EventSysMultiTalkDeviceConnected:
        if (mt->total_connected >= 2 )
        {
            AudioPlay(AP_MULTI_TALK_2_CONNECTED + mt->total_connected - 2, TRUE);
        }
        mtInquiryStop();
        if (mtGetConnectDevices() <= 1)
        {
            mtInquiryPair(inquiry_session_friend, FALSE);
        }
        MessageCancelAll(task, EventSysMultiTalkFriendSaveListDelay);
        mt->mt_mode = FREIEND_MODE;
        stateManagerUpdateState();
        /* adjust volume */
        mtMicMute(AUDIO_MUTE_DISABLE, TRUE);
        MessageCancelAll(mt->app_task, EventSysMultiTalkCheckLoopTimeout);
        MessageCancelAll(mt->app_task, EventSysMultiTalkCheckLoop);
        return TRUE;
    case EventSysMultiTalkCurrentDevices:
    {
        if (mt->total_connected < 1)
        {
            mt->total_connected = 1;
        }
        if (mt->mt_mode == FREIEND_MODE)
        {
            AudioPlay(AP_MULTI_TALK_1_PERSON + mt->total_connected - 1, TRUE);
        }
        if (mt->mt_mode == FREIEND_MODE_PAIRING)
        {
            int connected = mt->total_connected;
            if(connected <= 1) connected = 1;
            if(connected >= 9) connected = 9;
            AudioPlay(AP_MULTI_TALK_1_PAIRED + connected - 1, TRUE);
        }
        MessageCancelAll(mt->app_task, EventSysMultiTalkCheckLoopTimeout);
        MessageCancelAll(mt->app_task, EventSysMultiTalkCheckLoop);
        mtInquiryStop();
        if(mtGetConnectDevices() < 2)
        {
            if (mt->status != MT_ST_NOCONNECT)
            {
                if (mt->mt_mode == FREIEND_MODE)
                {
                    if(mt->status == MT_ST_CONNECTED || mt->status == MT_ST_LINKLOSS)
		            {
		                mt->status = MT_ST_SEARCHING;
		            }
                }
                else
                {
                    mt->status = MT_ST_PARING;
                }
                mtInquiryStart();
            }
        } 
    } 
    break;
    case EventSysMultiTalkSendRouteTable:
        break;
    case EventSysMultiTalkCheckLoop:
    {
        if(mt->status == MT_ST_NOLOOP)
        {
            mtBroadcastHeaderAddr1(mt->sco_expend_dev, 1, &mt->addr);
            MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoop, NULL, D_SEC(5));
            MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoopTimeout, NULL, D_SEC(15));
        }
        else
        {
            if (!mtSendCheckLoop(mt->sco_expend_dev, 0))
            {
                mtBroadcastHeaderAddr1(!mt->sco_expend_dev, 1, &mt->addr);
            }
            else
            {
                MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoop, NULL, D_SEC(5));
                MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoopTimeout, NULL, D_SEC(15));
            }
        }
        break;
    }
    case EventSysMultiTalkCheckLoopTimeout:
    {
        MT_DEBUG(("MT: check loop timeout, disconnect\n"));
        MessageCancelAll(mt->app_task, EventSysMultiTalkCheckLoop);
        MessageCancelAll(mt->app_task, EventSysMultiTalkCheckLoopTimeout);
        mtACLDisconnect(mt->sco_expend_dev);
    }
    break;
    case EventSysMultiTalkFriendSaveListDelay:
    {
        AudioPlay(AP_SAVE_FREIND_FAILED, TRUE);
    }
    break;
    default:
        break;
    }
    return FALSE;
}
