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
#include "headset_multi_comm.h"
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

    if (mt->mt_device[MT_RIGHT].state != MT_SYN_Connected)
    {
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);

        mt->mt_device[MT_RIGHT].bt_addr = *bd_addr;
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Connecting;
    }
    else if (mt->mt_device[MT_LEFT].state != MT_SYN_Connected)
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

    mtACLConnect(bd_addr, MULTITALK_NEARBY_PSM);
    return TRUE;
}

/* 接收到其他设备的连接 */
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
    if (mt->nearby_connected == 8)
    {
        MT_DEBUG(("MT: connected device is 8\n"));
        return FALSE;
    }
    if (mt->connect_token != TOKEN_IDLE)
    {
        if (BdaddrCompare(&mt->header_addr[0], &msg->bd_addr) == 1 || BdaddrCompare(&mt->header_addr[1], &msg->bd_addr) == 1)
        {
            MT_DEBUG(("MT: token held, cannot allow larger [%x:%x:%lx] than header [%x:%x:%lx] [%x:%x:%lx]\n", msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap,
                      mt->header_addr[0].nap, mt->header_addr[0].uap, mt->header_addr[0].lap,
                      mt->header_addr[1].nap, mt->header_addr[1].uap, mt->header_addr[1].lap));
            return FALSE;
        }
    }

    if (mt->mt_device[MT_LEFT].state == MT_ST_WAITING_CONNECT && BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_LEFT].bt_addr))
    {
        MT_DEBUG(("MT: handleMTL2capConnectIndNearbyMode peer is in waiting\n"));
        mt->mt_device[MT_LEFT].state = MT_L2CAP_WaitConnect;
        mt->mt_device[MT_LEFT].bt_addr = msg->bd_addr;
        mt->mt_type = MT_NODE;
        return TRUE;
    }

    if (mt->mt_device[MT_RIGHT].state == MT_ST_WAITING_CONNECT && BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_RIGHT].bt_addr))
    {
        MT_DEBUG(("MT: handleMTL2capConnectIndNearbyMode peer is in waiting\n"));
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_WaitConnect;
        mt->mt_device[MT_RIGHT].bt_addr = msg->bd_addr;
        mt->mt_type = MT_NODE;
        return TRUE;
    }

    if (mt->mt_device[MT_LEFT].state == MT_L2CAP_Disconnected || mt->mt_device[MT_LEFT].state >= MT_L2CAP_Disconnecting)
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
            if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_RIGHT].bt_addr))
            {
                DEBUG(("MT: handleMTL2capConnectIndNearbyMode cannot allow child connected to parent\n"));
                return FALSE;
            }
            else
            {
                MT_DEBUG(("MT: handleMTL2capConnectIndNearbyMode peer is my cai\n"));
                mt->mt_device[MT_LEFT].state = MT_L2CAP_WaitConnect;
                mt->mt_device[MT_LEFT].bt_addr = msg->bd_addr;
                mt->mt_type = MT_NODE;
                return TRUE;
            }
        }
    }
    /* allow become child */
    if (mt->mt_device[MT_RIGHT].state == MT_L2CAP_Disconnected || mt->mt_device[MT_RIGHT].state >= MT_L2CAP_Disconnecting)
    {
        MT_DEBUG(("MT: handleMTL2capConnectIndNearbyMode child is disconnect\n"));

        if (mt->mt_mode == CLOSE_MODE)
        {
            DEBUG(("MT: handleMTL2capConnectIndNearbyMode cannot allow connected in close mode \n"));
            return FALSE;
        }
        else
        {
            {
                MT_DEBUG(("MT: handleMTL2capConnectIndNearbyMode peer is my cai to child\n"));
                mt->mt_device[MT_RIGHT].state = MT_L2CAP_WaitConnect;
                mt->mt_device[MT_RIGHT].bt_addr = msg->bd_addr;
                mt->mt_type = MT_NODE;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* 主动连接 */
void handleMTL2capConnectCfmNearbyMode(CL_L2CAP_CONNECT_CFM_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capConnectCfmNearbyMode: addr=%x:%x:%lx, status=%d\n",
              msg->addr.nap, msg->addr.uap, msg->addr.lap, msg->status));
    if(msg->psm_local != MULTITALK_NEARBY_PSM)
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

        MessageSinkTask(msg->sink, &acl_parent_task);

        if (msg->status == l2cap_connect_success)
        {
            if (mt->mt_device[MT_LEFT].state == MT_L2CAP_Connecting)
            {
                mt->mt_device[MT_LEFT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_LEFT].acl_sink = msg->sink;

                /* 连上后需要检测是否成�如果成环则断开 */
                mt->status = MT_ST_CHECKLOOP;
                mt->sco_expend_dev = MT_LEFT;
                MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoop, NULL, D_SEC(0.5));
            }
            else
            {
                mt->mt_device[MT_LEFT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_LEFT].acl_sink = msg->sink;
            }

            linkPolicyCheckRoles();
        }
        else if (msg->status >= l2cap_connect_failed)
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
                    inquiryStop();
                    stateManagerEnterConnectedState();
                }
                else
                {
                    mt->status = MT_ST_SEARCHING;
                }
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

            MessageSinkTask(msg->sink, &acl_child_task);

            if (mt->mt_device[MT_RIGHT].state == MT_L2CAP_Connecting)
            {
                mt->mt_device[MT_RIGHT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_RIGHT].acl_sink = msg->sink;
                mt->mt_type = MT_NODE;

                /* 连上后需要检测是否成�如果成环则断开 */
                mt->status = MT_ST_CHECKLOOP;
                mt->sco_expend_dev = MT_RIGHT;
                MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoop, NULL, D_SEC(0.5));
            }
            else
            {
                mt->mt_device[MT_RIGHT].state = MT_L2CAP_Connected;
                mt->mt_device[MT_RIGHT].acl_sink = msg->sink;
                mt->mt_type = MT_NODE;
            }

            linkPolicyCheckRoles();
        }
        else if (msg->status >= l2cap_connect_failed)
        {
            BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
            mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
            if (mt->status == MT_ST_CONNECTING)
            {
                mt->status = MT_ST_SEARCHING;
            }

            if (msg->status == l2cap_connect_error) /* 137 */
            {
                deviceManagerRemoveDevice(&msg->addr);
            }
            mtSendConnectToken(MT_LEFT, TOKEN_RELEASE);
        }
    }
}

void handleMTL2capDisconIndNearbyMode(CL_L2CAP_DISCONNECT_IND_T *msg)
{
    /* Disconnect passive */
    MT_DEBUG(("MT: handleMTL2capDisconIndNearbyMode: status=%d\n", msg->status));
    if (msg->sink == mt->mt_device[MT_LEFT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconIndNearbyMode: parent\n"));
        mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
        mt->mt_device[MT_LEFT].acl_sink = NULL;
        stateManagerUpdateState();

        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }

        inquiryStop();
        if (mt->status != MT_ST_NOCONNECT)
        {
            inquiryPair(inquiry_session_nearby, mtGetConnectDevices() == 0);

            mt->status = MT_ST_SEARCHING;
        }

        mt->header_addr[0] = mt->addr;
        BdaddrSetZero(&mt->header_addr[1]);
        if (!mtBroadcastHeaderAddr1(MT_RIGHT, 1, &mt->addr))
        {
            mt->nearby_connected = 1;
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
    }
    if (msg->sink == mt->mt_device[MT_RIGHT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconIndNearbyMode: child\n"));
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
        mt->mt_device[MT_RIGHT].acl_sink = NULL;
        stateManagerUpdateState();

        if (msg->status == l2cap_disconnect_successful)
        {
            ConnectionL2capDisconnectResponse(msg->identifier, msg->sink);
        }

        inquiryStop();
        if (mt->status != MT_ST_NOCONNECT)
        {
            inquiryPair(inquiry_session_nearby, mtGetConnectDevices() == 0);

            mt->status = MT_ST_SEARCHING;
        }

        mt->header_addr[0] = mt->addr;
        BdaddrSetZero(&mt->header_addr[1]);
        if (!mtBroadcastHeaderAddr1(MT_LEFT, 1, &mt->addr))
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
    if (msg->sink == mt->mt_device[MT_LEFT].acl_sink)
    {
        mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
        mt->mt_device[MT_LEFT].acl_sink = NULL;
        mt->head_addr = mt->addr;
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
    }
    if (mtGetConnectDevices() == 0)
    {
        mt->nearby_connected = 0;
        mt->head_addr = mt->addr;
        mt->header_addr[0] = mt->addr;
        BdaddrSetZero(&mt->header_addr[1]);
        mt->status = MT_ST_NOCONNECT;
        MessageSend(mt->app_task, EventSysMultiTalkNeabyModeLeaved, NULL);
        MessageCancelAll(mt->app_task, EventSysMultiTalkLeaveNearbyModeDelay);
    }
}

void handleMTSynConnIndNearbyMode(CL_DM_SYNC_CONNECT_IND_T *msg)
{
    MT_DEBUG(("MT: handleMTSynConnIndNearbyMode: addr=%x:%x:%lx\n",
              msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap));
}

void handleMTSynConnCfmNearbyMode(CL_DM_SYNC_CONNECT_CFM_T *msg)
{
    MT_DEBUG(("MT: handleMTSynConnCfmNearbyMode status=%d, link_typs = %d, addr=%x:%x:%lx\n",
              msg->status, msg->link_type, msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap));
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

            if (mt->status == MT_ST_CONNECTING)
            {
                MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
            }

            if (mtGetConnectDevices() == 2)
            {
                mt->mt_type = MT_NODE;
                mt->status = MT_ST_CONNECTED;
                inquiryStop();
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
            MT_DEBUG(("MT: right sync connected\n"));

            audioUpdateAudioRouting();

            if (mt->status == MT_ST_CONNECTING)
            {
                MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
            }

            if (mtGetConnectDevices() == 2)
            {
                mt->mt_type = MT_NODE;
                mt->status = MT_ST_CONNECTED;
                inquiryStop();
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
            BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
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
            BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
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
        mt->header_addr[0] = mt->addr;
        BdaddrSetZero(&mt->header_addr[1]);
        PowerAmpOn();
        break;
    case EventSysMultiTalkLeaveNearbyMode:
        inquiryStop();
        stateManagerEnterConnectableState(FALSE);
        if (mtGetConnectDevices() == 0)
        {
            mt->status = MT_ST_NOCONNECT;
            MessageSend(task, EventSysMultiTalkLeaveNearbyModeDelay, NULL);
        }
        else
        {
            mtDisconnect();
            MessageSendLater(task, EventSysMultiTalkLeaveNearbyModeDelay, NULL, D_SEC(1));
        }
        mt->connect_token = TOKEN_IDLE;
        break;
    case EventSysMultiTalkLeaveNearbyModeDelay:
        mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
        mt->mt_device[MT_LEFT].acl_sink = NULL;
        mt->mt_device[MT_RIGHT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
        mt->mt_device[MT_RIGHT].acl_sink = NULL;

        stateManagerUpdateState();
        mt->status = MT_ST_NOCONNECT;
        mt->nearby_connected = 0;
        MessageSend(mt->app_task, EventSysMultiTalkNeabyModeLeaved, NULL);
        break;
    case EventSysMultiTalkInquiryDevices:
    {
        if (mt->mt_mode == NEARBY_MODE)
        {
            if (mt->nearby_connected == 8)
            {
                inquiryStop();
                break;
            }
            if (mt->connect_token == TOKEN_IDLE)
            {
                if (mt->status == MT_ST_PARING || mt->status == MT_ST_SEARCHING)
                {
                    inquiry_result_t *result = sinkinquiryGetInquiryResults();
                    if (result)
                    {
                        /* 连接比自己头地址更大的设  */
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
                            mtConnectNearby(&result[0].bd_addr);
                            mt->status = MT_ST_CONNECTING;
                            break;
                        }
                        else
                        {
                            MT_DEBUG(("MT: search %x:%x:%lx, but header is [%x:%x:%lx] [%x:%x:%lx]\n",
                                      result[0].bd_addr.nap,
                                      result[0].bd_addr.uap,
                                      result[0].bd_addr.lap,
                                      mt->header_addr[0].nap,
                                      mt->header_addr[0].uap,
                                      mt->header_addr[0].lap,
                                      mt->header_addr[1].nap,
                                      mt->header_addr[1].uap,
                                      mt->header_addr[1].lap));
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
    case EventSysMultiTalkCheckLoop:
    {
        if (!mtSendCheckLoop(mt->sco_expend_dev, 0))
        {
            mtBroadcastHeaderAddr1(!mt->sco_expend_dev, 1, &mt->addr);
        }
        else
        {
            MessageSendLater(mt->app_task, EventSysMultiTalkCheckLoop, NULL, D_SEC(5));
        }
    }
    break;
    default:
        break;
    }
    return FALSE;
}
