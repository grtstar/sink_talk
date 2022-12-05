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

extern ExamplePluginTaskdata csr_cvsd_8k_1mic_plugin;
extern ExamplePluginTaskdata csr_sbc_1mic_plugin;
extern ExamplePluginTaskdata csr_cvsd_8k_2mic_plugin;
extern ExamplePluginTaskdata csr_sbc_2mic_plugin;

/* Example Plugins */
#define CVSD1MIC_EXAMPLE (TaskData *)&csr_cvsd_8k_1mic_plugin
#define SBC1MIC_EXAMPLE (TaskData *)&csr_sbc_1mic_plugin
#define CVSD2MIC_EXAMPLE (TaskData *)&csr_cvsd_8k_2mic_plugin
#define SBC2MIC_EXAMPLE (TaskData *)&csr_sbc_2mic_plugin


#define PS_ROUTE_TABLE 3



void BdaddrToArray(bdaddr *addr, uint8 *d)
{
    d[0] = addr->lap >> 24;
    d[1] = addr->lap >> 16;
    d[2] = addr->lap >> 8;
    d[3] = addr->lap >> 0;

    d[4] = addr->uap;

    d[5] = addr->nap >> 8;
    d[6] = addr->nap >> 0;
}

bdaddr ArrayToBdaddr(const uint8 *data)
{
    bdaddr addr;
    addr.lap = (((uint32)data[0]) << 24) | (((uint32)data[1]) << 16) | (((uint32)data[2]) << 8) | data[3];
    addr.uap = data[4];
    addr.nap = (((uint16)data[5]) << 8) | data[6];
    return addr;
}

uint8 RouteTableIsContain(RouteTable *rt, bdaddr *addr)
{
    uint8 index = 0;
    for (index = 0; index < rt->count; index++)
    {
        if (BdaddrIsSame(&(rt->item[index]), addr))
        {
            return index + 1;
        }
    }
    return 0;
}

void RouteTableSort(RouteTable *rt)
{
    uint8 i = 0, j = 0;
    for(i=1; i<rt->count; i++)
    {
        for(j=i; j<rt->count; j++)
        {
            if(BdaddrCompare(&rt->item[i], &rt->item[j]) > 0)
            {
                bdaddr temp = rt->item[i];
                rt->item[i] = rt->item[j];
                rt->item[j] = temp;
            }
        }
    }
}

uint8 RouteTableGetIndex(RouteTable *rt, bdaddr *addr)
{
    uint8 i = 0;
    for(i=0; i<rt->count; i++)
    {
        if(BdaddrIsSame(&rt->item[i], addr))
        {
            return 1<<(7-i);
        }
    }
    return 0;
}

bool RouteTableIsNotSaved(RouteTable *rt)
{
    return rt->nop != 0xA5 && rt->nop != 0x55;
}

void RouteTablePushChild(RouteTable *rt, bdaddr addr)
{
    MT_DEBUG(("MT: Push Child "));
    MT_DEBUG_ADDR(addr);

    if (BdaddrIsZero(&addr))
    {
        return;
    }

    if (rt->count >= sizeof(rt->item))
    {
        MT_DEBUG(("MT: Route Table is Full"));
        return;
    }
    if (!RouteTableIsContain(rt, &addr))
    {
        rt->item[rt->count++] = addr;
    }
}

bdaddr RouteTableGet(RouteTable *rt, uint8 index)
{
    bdaddr addr;
    BdaddrSetZero(&addr);
    if (index >= rt->count)
    {
        return addr;
    }
    return rt->item[index];
}

void mtSaveRouteTable(RouteTable *rt, uint8 mt_type)
{
    if (mt_type == MT_HEAD || mt_type == MT_PAIRHEAD)
    {
        rt->nop = 0xA5;
    }
    else
    {
        rt->nop = 0x55;
    }
    ConfigStore(PS_ROUTE_TABLE, rt, sizeof(RouteTable)); /* DSP49 */
}

void mtLoadRouteTable(RouteTable *rt)
{
    ConfigRetrieve(PS_ROUTE_TABLE, rt, sizeof(RouteTable)); /* DSP49 */
    if (rt->nop != 0x55 && rt->nop != 0xA5)
    {
        memset(rt, 0, sizeof(RouteTable));
    }
}

void mtClearRouteTable(RouteTable *rt)
{
    memset(rt, 0, sizeof(RouteTable));
    ConfigStore(PS_ROUTE_TABLE, rt, sizeof(RouteTable));
}

void mtResetPairList(void)
{
    uint16_t pl[2];
    pl[0] = 0xFFFF;
    pl[1] = 0xFFFF;
    ConfigStore(141, NULL, 0);
}


const uint16 syn_conftab[] =
    {
        L2CAP_AUTOPT_SEPARATOR,                 /* START */
        L2CAP_AUTOPT_MTU_IN, 0x037F,            /* Maximum inbound MTU - 895 bytes */
        L2CAP_AUTOPT_MTU_OUT, 0x0060,           /* Minimum acceptable outbound MTU - 96 bytes */
        L2CAP_AUTOPT_FLUSH_IN, 0x0000, 0x0000,  /* Min acceptable remote flush timeout - zero*/
        0xFFFF, 0xFFFF,                         /* Max acceptable remote flush timeout - infinite*/
        L2CAP_AUTOPT_FLUSH_OUT, 0xFFFF, 0xFFFF, /* Min local flush timeout - infinite */
        0xFFFF, 0xFFFF,                         /* Max local flush timeout - infinite */
        L2CAP_AUTOPT_TERMINATOR                 /* END */
};

MTData *mt;

TaskData acl_parent_task = {acl_parent_handler};
TaskData acl_child_task = {acl_child_handler};

void mt_handler(Task task, MessageId id, Message message)
{
    MT_DEBUG(("MT: mt_handler id = 0x%x (%d)\n", id, id));
    switch (id)
    {
    case CL_L2CAP_REGISTER_CFM:
        /* code */
        handleMTL2capRegisterCfm((CL_L2CAP_REGISTER_CFM_T *)message);
        break;
    case CL_L2CAP_CONNECT_IND:
        /* code */
        handleMTL2capConnectInd((CL_L2CAP_CONNECT_IND_T *)message);
        break;
    case CL_L2CAP_CONNECT_CFM:
        /* code */
        handleMTL2capConnectCfm((CL_L2CAP_CONNECT_CFM_T *)message);
        break;
    case CL_L2CAP_DISCONNECT_IND:
        /* code */
        handleMTL2capDisconInd((CL_L2CAP_DISCONNECT_IND_T *)message);
        break;
    case CL_L2CAP_DISCONNECT_CFM:
        /* code */
        handleMTL2capDisconCfm((CL_L2CAP_DISCONNECT_CFM_T *)message);
        break;
    case CL_DM_SYNC_REGISTER_CFM:
        /* code */
        handleMTSynRegisterCfm((CL_DM_SYNC_REGISTER_CFM_T *)message);
        break;
    case CL_DM_SYNC_UNREGISTER_CFM:
        /* code */

        break;
    case CL_DM_SYNC_CONNECT_IND:
        /* code */
        handleMTSynConnInd((CL_DM_SYNC_CONNECT_IND_T *)message);
        break;
    case CL_DM_SYNC_CONNECT_CFM:
        /* code */
        handleMTSynConnCfm((CL_DM_SYNC_CONNECT_CFM_T *)message);
        break;
    case CL_DM_SYNC_DISCONNECT_IND:
        /* code */
        handleMTSynDisconInd((CL_DM_SYNC_DISCONNECT_IND_T *)message);
        break;
    case CL_DM_SYNC_RENEGOTIATE_IND:
        break;
    default:
        break;
    }
}

void acl_parent_handler(Task task, MessageId id, Message message)
{
    (void)task;
    (void)(message);

    switch (id)
    {
    case MESSAGE_MORE_DATA:
    {
        uint16 packet_size;
        MessageMoreData *msg = (MessageMoreData *)message;
        while ((packet_size = SourceBoundary(msg->source)) != 0)
        {
            const uint8 *in = SourceMap(msg->source);
            /* recevie acl data */
            ACLProcessParentData(in, packet_size);
            SourceDrop(msg->source, packet_size);
        }
    }
    break;
    case MESSAGE_MORE_SPACE:

        break;
    }
}

void acl_child_handler(Task task, MessageId id, Message message)
{
    (void)task;
    (void)(message);

    switch (id)
    {
    case MESSAGE_MORE_DATA:
    {
        uint16 packet_size;
        MessageMoreData *msg = (MessageMoreData *)message;
        while ((packet_size = SourceBoundary(msg->source)) != 0)
        {
            const uint8 *in = SourceMap(msg->source);
            /* recevie acl data */
            ACLProcessChildData(in, packet_size);
            SourceDrop(msg->source, packet_size);
        }
    }
    break;
    case MESSAGE_MORE_SPACE:
        break;
    }
}

void mtInit(Task task)
{
    uint16 temp[4];
    mt = PanicUnlessMalloc(sizeof(MTData));
    memset(mt, 0, sizeof(MTData));
    mt->app_task = task;
    mt->mt_task.handler = mt_handler;
    mt->mt_type = MT_HEAD;
    mt->total_connected = 1;
    mt->nearby_connected = 1;
    PsFullRetrieve(1, temp, 4);
    mt->addr.nap = temp[3];
    mt->addr.uap = temp[2];
    mt->addr.lap = ((uint32)temp[0]) << 16 | temp[1];
    mt->head_addr = mt->addr;
    mt->route_table.item[0] = mt->addr;
    mt->status = MT_ST_NOCONNECT;

    mtLoadRouteTable(&mt->route_table);
    if (RouteTableIsNotSaved(&mt->route_table))
    {
        mt->mt_type = MT_NODE;
    }
    if (mt->route_table.nop == 0xA5)
    {
        mt->mt_type = MT_HEAD;
        RouteTableSort(&mt->route_table);
    }
    else
    {
        mt->mt_type = MT_NODE;
    }

    mt->connected_device = RouteTableGetIndex(&mt->route_table, &mt->addr);

    ConnectionL2capRegisterRequest(&mt->mt_task, MULTITALK_FRIEND_PSM, 0);
    ConnectionL2capRegisterRequest(&mt->mt_task, MULTITALK_NEARBY_PSM, 0);

    mt->mt_mode = CLOSE_MODE;
}

bool mtRssiConnect(void)
{
    bdaddr bd_addr = RouteTableGet(&mt->route_table, mt->connect_idx % mt->route_table.count);

    MT_DEBUG(("MT: mtRssiConnect to "));
    MT_DEBUG_ADDR(bd_addr);
    if(RouteTableGetIndex(&mt->route_table, &bd_addr) & mt->connected_device)
    {
        MT_DEBUG(("MT: device is connected, omit\n"));
        return FALSE;
    }
    if (!BdaddrIsSame(&bd_addr, &mt->addr))
    {
        deviceManagerRemoveDevice(&bd_addr);
        return mtConnect(&bd_addr);
    }
    return FALSE;
}

void mtDisconnect(void)
{
    if (mt->mt_device[MT_PARENT].state == MT_SYN_Connected)
    {
        mtScoDisconnect(MT_PARENT);
    }
    else
    {
        mtACLDisconnect(MT_PARENT);
    }
    if (mt->mt_device[MT_CHILD].state == MT_SYN_Connected)
    {
        mtScoDisconnect(MT_CHILD);
    }
    else
    {
        mtACLDisconnect(MT_CHILD);
    }
}

bool mtConnect(bdaddr *bd_addr)
{
    MT_DEBUG(("MT: mtConnect to "));
    MT_DEBUG_ADDR((*bd_addr));

    mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
    BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);

    if(mt->mt_device[MT_PARENT].state >= MT_L2CAP_Connected && BdaddrIsSame(&mt->mt_device[MT_PARENT].bt_addr, bd_addr))
    {
        MT_DEBUG(("MT: cannot connect to parent, omit\n"));
        return FALSE;
    }

    if(RouteTableGetIndex(&mt->route_table, bd_addr) & mt->connected_device)
    {
        MT_DEBUG(("MT: device is connected [%02X], omit\n", mt->connected_device));
        return FALSE;
    }
    if(RouteTableGetIndex(&mt->route_table, bd_addr) > mt->connected_device)
    {
        MT_DEBUG(("MT: device is pefer than my head [%02X], omit\n", mt->connected_device));
        return FALSE;
    }

    if(mt->mt_device[MT_CHILD].state == MT_SYN_Connected)
    {
        MT_DEBUG(("MT: child is connected [%02X], omit\n", mt->connected_device));
        return FALSE;
    }

    mt->mt_device[MT_CHILD].bt_addr = *bd_addr;
    mt->mt_device[MT_CHILD].state = MT_L2CAP_Connecting;
    mtACLConnect(bd_addr, MULTITALK_FRIEND_PSM);
    return TRUE;
}

void mtACLConnect(bdaddr *bd_addr, uint16 psm)
{
    MT_DEBUG(("MT: mtACLConnect to "));
    MT_DEBUG_ADDR((*bd_addr));
    ConnectionSetPageTimeout(16384);
    ConnectionL2capConnectRequest(&mt->mt_task, bd_addr, psm, psm, sizeof(syn_conftab), (uint16 *)syn_conftab);
}

void mtACLDisconnect(int device)
{
    if (mt->mt_device[device].acl_sink != NULL)
    {
        ConnectionL2capDisconnectRequest(&mt->mt_task, mt->mt_device[device].acl_sink);
    }
    else
    {
        mt->mt_device[device].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[device].bt_addr);
    }
}

void mtScoConnect(Sink sink_acl)
{
    sync_config_params config_params;
    config_params.packet_type = sync_all_sco;
    config_params.tx_bandwidth = 8000;
    config_params.rx_bandwidth = 8000;
    config_params.retx_effort = sync_retx_disabled;
    config_params.max_latency = 16;
    config_params.voice_settings = sync_air_coding_cvsd;
    MT_DEBUG(("MT: Connect sco\n"));
    if (!SinkIsValid(sink_acl))
    {
        MT_DEBUG(("MT: Soc Sink error !\n"));
        return;
    }
    ConnectionSyncConnectRequest(&mt->mt_task, sink_acl, &config_params);
}

void mtScoDisconnect(int device)
{
    if (mt->mt_device[device].audio_sink != NULL)
    {
        mt->mt_device[device].state = MT_SYN_Disconnecting;
        ConnectionSyncDisconnect(mt->mt_device[device].audio_sink, hci_error_oetc_user);
    }
    else if (mt->mt_device[device].state == MT_SYN_Connecting)
    {
        mt->mt_device[device].state = MT_L2CAP_Connected;
        mtACLDisconnect(device);
    }
}

void handleMTL2capRegisterCfm(CL_L2CAP_REGISTER_CFM_T *msg)
{
    MT_DEBUG(("handleMTL2capRegisterCfm status=%d, prim=0x%x\n", msg->status, msg->psm));
    if (msg->psm == MULTITALK_FRIEND_PSM)
    {
        ConnectionSyncRegister(&mt->mt_task);
    }
}

void handleMTL2capConnectInd(CL_L2CAP_CONNECT_IND_T *msg)
{
    bool can_recv = FALSE;
    uint16 psm = MULTITALK_FRIEND_PSM;
    if(mt->mt_mode == FREIEND_MODE)
    {
        can_recv = handleMTL2capConnectIndFriendMode(msg);
        psm = MULTITALK_FRIEND_PSM;
    }
    if(mt->mt_mode == NEARBY_MODE)
    {
        can_recv = handleMTL2capConnectIndNearbyMode(msg);
        psm = MULTITALK_NEARBY_PSM;
    }

    if (can_recv)
    {
        ConnectionL2capConnectResponse(&mt->mt_task, TRUE, psm, msg->connection_id,
                                       msg->identifier, sizeof(syn_conftab), (uint16 *)syn_conftab);
    }
    else
    {
        ConnectionL2capConnectResponse(&mt->mt_task, FALSE, psm, msg->connection_id,
                                       msg->identifier, sizeof(syn_conftab), (uint16 *)syn_conftab);
    }
}

void handleMTL2capConnectCfm(CL_L2CAP_CONNECT_CFM_T *msg)
{
    if(mt->mt_mode == FREIEND_MODE)
    {
        handleMTL2capConnectCfmFriendMode(msg);
    }
    if(mt->mt_mode == NEARBY_MODE)
    {
        handleMTL2capConnectCfmNearbyMode(msg);
    }
}

void handleMTL2capDisconInd(CL_L2CAP_DISCONNECT_IND_T *msg)
{
    if(mt->mt_mode == FREIEND_MODE)
    {
        handleMTL2capDisconIndFriendMode(msg);
    }
    if(mt->mt_mode == NEARBY_MODE)
    {
        handleMTL2capDisconIndNearbyMode(msg);
    }
}

void handleMTL2capDisconCfm(CL_L2CAP_DISCONNECT_CFM_T *msg)
{
    if(mt->mt_mode == FREIEND_MODE)
    {
        handleMTL2capDisconCfmFriendMode(msg);
    }
    if(mt->mt_mode == NEARBY_MODE)
    {
        handleMTL2capDisconCfmNearbyMode(msg);
    }
}

void handleMTSynRegisterCfm(CL_DM_SYNC_REGISTER_CFM_T *msg)
{

}

void handleMTSynConnInd(CL_DM_SYNC_CONNECT_IND_T *msg)
{
    sync_config_params config_params;
    config_params.packet_type = sync_all_sco;
    config_params.tx_bandwidth = 8000;
    config_params.rx_bandwidth = 8000;
    config_params.retx_effort = sync_retx_disabled;
    config_params.max_latency = 16;
    config_params.voice_settings = sync_air_coding_cvsd;

    if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_PARENT].bt_addr) && mt->mt_device[MT_PARENT].state == MT_L2CAP_Connected)
    {
        ConnectionSyncConnectResponse(&mt->mt_task, &msg->bd_addr, TRUE, &config_params);
        mt->mt_device[MT_PARENT].state = MT_SYN_Connecting;
    }
    else
    {
        ConnectionSyncConnectResponse(&mt->mt_task, &msg->bd_addr, FALSE, &config_params);
    }
}

void handleMTSynConnCfm(CL_DM_SYNC_CONNECT_CFM_T *msg)
{
    if(mt->mt_mode == FREIEND_MODE)
    {
        handleMTSynConnCfmFriendMode(msg);
    }
    if(mt->mt_mode == NEARBY_MODE)
    {
        handleMTSynConnCfmNearbyMode(msg);
    }
}

void handleMTSynDisconInd(CL_DM_SYNC_DISCONNECT_IND_T *msg)
{
    if(mt->mt_mode == FREIEND_MODE)
    {
        handleMTSynDisconIndFriendMode(msg);
    }
    if(mt->mt_mode == NEARBY_MODE)
    {
        handleMTSynDisconIndNearbyMode(msg);
    }
}

void ACLBroadcastEvent(Sink sink, uint16 event)
{
    uint8 d[3];
    d[0] = 0;
    d[1] = event >> 8;
    d[2] = event;
    ACLSend(sink, d, 3);
}

bool mtBroadcastCurrentCount(int count)
{
    uint8 d[3];
    MT_DEBUG(("MT: mtBroadcastCurrentCount %d\n", count));

    d[0] = ACLMSG_CURRENT_CONNECTED;
    d[1] = count;
    d[2] = mt->connected_device;
    return ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, sizeof(d));
}

bool mtSendFindTail(void)
{
    uint8 d[1];
    d[0] = ACLMSG_FIND_TAIL;
    return ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, sizeof(d));
}

bool mtSendCheckTTL(uint8 ttl)
{
    uint8 d[2];
    MT_DEBUG(("MT: Send Check TTL\n"));
    d[0] = ACLMSG_CHECK_TTL;
    d[1] = ttl;
    return ACLSend(mt->mt_device[MT_PARENT].acl_sink, d, sizeof(d));
}

bool mtBroadcastConnectedCount(uint8 count)
{
    uint8 d[3];
    MT_DEBUG(("MT: mtBroadcastConnectedCount %d\n", count));

    d[0] = ACLMSG_DEVICE_COUNT;
    d[1] = count;
    d[2] = mt->connected_device;
    return ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, sizeof(d));
}

bool mtSendCheckTail(void)
{
    uint8 d[1];
    d[0] = ACLMSG_CHECK_TAIL;
    return ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, sizeof(d));
}

bool mtSendCheckHead(uint8 count)
{
    uint8 d[2];
    MT_DEBUG(("MT: Send Check Head, count = %d\n", count));
    d[0] = ACLMSG_CHECK_HEAD;
    d[1] = count;
    return ACLSend(mt->mt_device[MT_PARENT].acl_sink, d, sizeof(d));
}

bool mtSendDisconnect(void)
{
    uint8 d[1];
    d[0] = ACLMSG_NEARBY_DISCONNECT;
    return ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, sizeof(d));
}

bool mtBroadcastHeadAddr(uint8 count)
{
    uint8 d[2 + 8] = {0};
    d[0] = ACLMSG_HEAD_ADDR;
    d[1] = count;
    BdaddrToArray(&mt->head_addr, &d[2]);
    return ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, sizeof(d));
}

void mtRouteTableAdjust(bdaddr child_addr)
{
    RouteTable *rt = &mt->route_table;
    uint8 i = 0, u = 0;
    for (i = 0; i < rt->count; i++)
    {
        if (BdaddrIsSame(&(rt->item[i]), &mt->addr))
        {
            break;
        }
    }
    if (i < rt->count - 1)
    {
        i++;
        for (u = 0; u < rt->count; u++)
        {
            if (BdaddrIsSame(&(rt->item[u]), &child_addr))
            {
                break;
            }
        }
        if (u < rt->count)
        {
            bdaddr t = rt->item[i];
            rt->item[i] = rt->item[u];
            rt->item[u] = t;
        }
    }
}

void mtSendRouteTable(RouteTable *rt)
{
#if 1
    uint8 d[2 + 8 * 8] = {0};
    uint8 i = 0;
    d[0] = ACLMSG_ROUTE_TABLE;
    d[1] = rt->count;
    for (i = 0; i < rt->count; i++)
    {
        BdaddrToArray(&rt->item[i], &d[2 + 7 * i]);
    }
    ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, 2 + 7 * rt->count);
#endif
}

bool ACLSend(Sink sink, const uint8_t *data, uint16 packet_size)
{
    uint16 sink_offset = 0;
    if (SinkIsValid(sink))
    {
        if ((sink_offset = SinkClaim(sink, packet_size)) != 0xFFFF)
        {
            memmove(SinkMap(sink) + sink_offset, data, packet_size);
            PanicFalse(SinkFlush(sink, packet_size));
        }
        else
        {
            Panic();
        }
    }
    else
    {
        MT_DEBUG(("MT: acl sink invalid\n"));
        return FALSE;
    }
    return TRUE;
}

void ACLProcessChildData(const uint8_t *data, int size)
{
    int i = 0;
    uint16 msg = 0;
    MT_DEBUG(("MT: ACL recv from Child:\n"));
    for (i = 0; i < size; i++)
    {
        MT_DEBUG(("%02X ", data[i]));
        if ((i & 0x7) == 0x7)
        {
            MT_DEBUG(("\n"));
        }
    }
    MT_DEBUG(("\n"));
    switch (data[0])
    {
    case ACLMSG_EVENT:
        /* code */
        msg = data[1] << 8 | data[2];
        if (msg == EventMultiTalkEndPair)
        {
            uint8 *message = PanicUnlessMalloc(sizeof(uint8));
            ACLBroadcastEvent(mt->mt_device[MT_PARENT].acl_sink, EventMultiTalkEndPair);
            MessageSend(mt->app_task, EventMultiTalkEndPair, message);
        }
        break;
    case ACLMSG_CHECK_TTL:
    {
        if (mt->mt_device[MT_PARENT].acl_sink == NULL)
        {
            int count;
            mt->connected_device = data[1] | RouteTableGetIndex(&mt->route_table, &mt->addr);
            count = GetOnes(mt->connected_device);
            mt->total_connected = count;
            if(mt->total_connected == mt->route_table.count && mt->mt_type == MT_PAIRHEAD)
            {
                mt->mt_type = MT_HEAD;
                mtBroadcastConnectedCount(count);
                MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);
            }
            else
            {
                mtBroadcastCurrentCount(count);
                MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
            }
        }
        else
        {
            mtSendCheckTTL(data[1] | RouteTableGetIndex(&mt->route_table, &mt->addr));
        }
    }
    break;
    default:
        break;
    }
    ACLProcessChildDataNearby(data, size);
}

void ACLProcessParentData(const uint8_t *data, int size)
{
    int i = 0;
    uint16 msg = 0;
    MT_DEBUG(("MT: ACL recv from Parent:\n"));
    for (i = 0; i < size; i++)
    {
        MT_DEBUG(("%02X ", data[i]));
        if ((i & 0x7) == 0x7)
        {
            MT_DEBUG(("\n"));
        }
    }
    MT_DEBUG(("\n"));
    switch (data[0])
    {
    case ACLMSG_EVENT:
        /* code */
        msg = data[1] << 8 | data[2];
        if (msg == EventMultiTalkEndPair)
        {
            uint8 *message = PanicUnlessMalloc(sizeof(uint8));
            ACLBroadcastEvent(mt->mt_device[MT_CHILD].acl_sink, EventMultiTalkEndPair);
            MessageSend(mt->app_task, EventMultiTalkEndPair, message);
        }
        break;
    break;
    case ACLMSG_FIND_TAIL:
    {
        if (mt->mt_device[MT_CHILD].acl_sink == NULL)
        {
            mtSendCheckTTL(RouteTableGetIndex(&mt->route_table, &mt->addr));
        }
        else
            mtSendFindTail();
    }
    break;
    case ACLMSG_DEVICE_COUNT:
    {
        mt->connected_device = data[2] | RouteTableGetIndex(&mt->route_table, &mt->addr);
        if(data[1] != 0)
        {
            mt->total_connected = data[1];
            MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);
        }
        mtBroadcastConnectedCount(data[1]);

    }
    break;
    case ACLMSG_CURRENT_CONNECTED:
    {
        mt->connected_device = data[2] | RouteTableGetIndex(&mt->route_table, &mt->addr);
        if(data[1] != 0)
        {
            mt->total_connected = data[1];
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
        mtBroadcastCurrentCount(data[1]);
    }
    break;
    case ACLMSG_ROUTE_TABLE:
    {
        int i, count = data[1];
        mt->route_table.count = count;
        for (i = 0; i < count; i++)
        {
            mt->route_table.item[i] = ArrayToBdaddr(&data[2 + 7 * i]);
        }
        if(RouteTableIsNotSaved(&mt->route_table))  /*    解决配网人数报错问题   */
        {
            if(mt->mt_device[MT_PARENT].state == MT_SYN_Connected && (mtGetConnectDevices() < 2))
            {
                mtSendCheckTTL(RouteTableGetIndex(&mt->route_table, &mt->addr));
            }
        }
        mtSaveRouteTable(&mt->route_table, MT_NODE);
    }
    break;
    default:
        break;
    }
    ACLProcessParentDataNearby(data, size);
}

bool processEventMultiTalk(Task task, MessageId id, Message message)
{
    if(id == EventMultiTalkKeyClick)
    {
        if(mt->mt_mode == CLOSE_MODE)
        {
            mt->mt_mode = NEARBY_MODE;
            MessageSend(task, EventSysMultiTalkEnterNearbyMode, NULL);
        }
        else if(mt->mt_mode == NEARBY_MODE)
        {
            MessageSend(task, EventSysMultiTalkLeaveNearbyMode, NULL);
            return FALSE;
        }
        else if(mt->mt_mode == FREIEND_MODE)
        {
            if(mt->status == MT_ST_PARING)
            {
                MessageSend(task, EventMultiTalkConnect, NULL);
            }
            else
            {
                MessageSend(task, EventSysMultiTalkLeaveFriendMode, NULL);
                return FALSE;
            }
        } 
        else if(mt->mt_mode == COUPLE_MODE)
        {

        }

        switch (mt->mt_mode)
        {
        case CLOSE_MODE:
            AudioPlay(AP_MULTI_TALK_OFF, TRUE);
            PowerAmpOff();
            break;
        case NEARBY_MODE:
            AudioPlay(AP_MULTI_TALK_NEARBY_MODE, TRUE);
            break;
        case FREIEND_MODE:
            AudioPlay(AP_MULTI_TALK_FRIEND_MODE, TRUE);
            break;
        case COUPLE_MODE:
            AudioPlay(AP_TWO_TALK_ON, TRUE);
            break;
        default:
            break;
        }
    }
    if(id == EventMultiTalkKeyPress)
    {
        if(mt->mt_mode == CLOSE_MODE)
        {
            sinkState state = stateManagerGetState();
            switch (state)
            {
            case deviceConnectable:
                id = EventMultiTalkEnterPair;
                mt->mt_mode = FREIEND_MODE;
                break;
            default:
                break;
            }
        }
        else if(mt->mt_mode == FREIEND_MODE)
        {
            sinkState state = stateManagerGetState();
            switch (state)
            {
            case deviceConnectable:
                id = EventMultiTalkEnterPair;
                break;
            case deviceConnDiscoverable:
                id = EventMultiTalkEndPair;
                mt->mt_mode = CLOSE_MODE;
                break;
            default:
                break;
            }
        }
        else if(mt->mt_mode == NEARBY_MODE)
        {
        }
    }
    if(id == EventSysMultiTalkNeabyModeLeaved)
    {
        if(!RouteTableIsNotSaved(&mt->route_table))
        {
            mt->mt_mode = FREIEND_MODE;
            MessageSend(task, EventSysMultiTalkEnterFriendMode, NULL);
            AudioPlay(AP_MULTI_TALK_FRIEND_MODE, TRUE);
        }
        else 
        {
            mt->mt_mode = CLOSE_MODE;
            AudioPlay(AP_MULTI_TALK_OFF, TRUE);
            PowerAmpOff();
        }
    }
    if(id == EventSysMultiTalkFriendModeLeaved)
    {
        if(0)   /* couple talk */
        {

        }
        else
        {
            mt->mt_mode = CLOSE_MODE;
            AudioPlay(AP_MULTI_TALK_OFF, TRUE);
            PowerAmpOff();
        }
    }
    if(mt->mt_mode == FREIEND_MODE)
    {
        processEventMultiTalkFriendMode(task, id, message);
    }
    if(mt->mt_mode == NEARBY_MODE)
    {
        processEventMultiTalkNearbyMode(task, id, message);
    }
    return FALSE;
}

bool mtVoicePopulateConnectParameters(audio_connect_parameters *connect_parameters)
{
    Sink audio_sink = NULL;
    if (mt->mt_device[MT_PARENT].audio_sink != NULL)
    {
        audio_sink = mt->mt_device[MT_PARENT].audio_sink;
        MT_DEBUG(("MT: PopulateConnectParameters Parent Sink\n"));
    }
    else if (mt->mt_device[MT_CHILD].audio_sink != NULL)
    {
        audio_sink = mt->mt_device[MT_CHILD].audio_sink;
        MT_DEBUG(("MT: PopulateConnectParameters Child Sink\n"));
    }
    mt->plugin_params = *sinkHfpDataGetHfpPluginParams();
    if (mt->mt_device[MT_PARENT].audio_sink != NULL && mt->mt_device[MT_CHILD].audio_sink != NULL)
    {
        mt->plugin_params.usb_params.usb_sink = mt->mt_device[MT_PARENT].audio_sink;
        audio_sink = mt->mt_device[MT_CHILD].audio_sink;
        MT_DEBUG(("MT: PopulateConnectParameters Tow Sink\n"));
    }
    else
    {
        mt->plugin_params.usb_params.usb_sink = NULL;
    }
    if (audio_sink != NULL)
    {
        connect_parameters->app_task = mt->app_task;
        connect_parameters->audio_plugin = CVSD1MIC_EXAMPLE;
        connect_parameters->audio_sink = audio_sink;
        connect_parameters->features = sinkAudioGetPluginFeatures();
        connect_parameters->mode = AUDIO_MODE_CONNECTED;
        connect_parameters->params = &mt->plugin_params;
        connect_parameters->power = powerManagerGetLBIPM();
        connect_parameters->rate = 8000;
        connect_parameters->route = AUDIO_ROUTE_INTERNAL;
        connect_parameters->sink_type = AUDIO_SINK_ESCO;
        connect_parameters->volume = sinkHfpDataGetDefaultVolume();
        return TRUE;
    }
    return FALSE;
}

Sink mtGetActiveScoSink(void)
{
    uint16 sink_all = (uint16)mt->mt_device[MT_PARENT].audio_sink + (uint16)mt->mt_device[MT_CHILD].audio_sink;
    if (mt->mt_device[MT_PARENT].audio_sink == NULL)
    {
        /*return NULL;*/
    }
    return (Sink)sink_all;
}

bool isMTVoiceSink(Sink sink)
{
    uint16 sink_all = (uint16)mt->mt_device[MT_PARENT].audio_sink + (uint16)mt->mt_device[MT_CHILD].audio_sink;
    if (sink == (Sink)sink_all)
    {
        return TRUE;
    }
    return FALSE;
}

int mtGetConnectDevices(void)
{
    int count = (mt->mt_device[MT_PARENT].state == MT_SYN_Connected) + (mt->mt_device[MT_CHILD].state == MT_SYN_Connected);
    MT_DEBUG(("MT: Connected Device %d\n", count));
    return count;
}

bool mtCanPair(void)
{
    if (mt->status == MT_ST_RECONNECTING || mt->status == MT_ST_LINKLOSS || mt->status == MT_ST_CONNECTING || mt->status == MT_ST_WAITING_CONNECT)
    {
        return TRUE;
    }
    return FALSE;
}

bool mtIsOnlyChildConnect(void)
{
    if (mt->mt_device[MT_PARENT].state < MT_L2CAP_Connected && mt->mt_device[MT_CHILD].state >= MT_L2CAP_Connected)
    {
        return TRUE;
    }
    return FALSE;
}

bool mtIsOnlyParentConnect(void)
{
    if (mt->mt_device[MT_CHILD].state < MT_L2CAP_Connected && mt->mt_device[MT_PARENT].state >= MT_L2CAP_Connected)
    {
        return TRUE;
    }
    return FALSE;
}

Sink mtGetSink(int type)
{
    return mt->mt_device[type].acl_sink;
}

int BdaddrCompare(bdaddr *_1, bdaddr *_2)
{
    if(_1->lap > _2->lap)
    {
        return 1;
    }
    else if(_1->lap < _2->lap)
    {
        return -1;
    }
    if(_1->uap > _2->uap)
    {
        return 1;
    }
    else if(_1->uap < _2->uap)
    {
        return -1;
    }
    if(_1->nap > _2->nap)
    {
        return 1;
    }
    else if(_1->nap < _2->nap)
    {
        return -1;
    }
    return 0;
}

int GetOnes(uint8 v)
{
    int i=0, count = 0;
    for(i=0; i<8; i++)
    {
        count += !!(v & (1<<i));
    }
    return count;
}

bool mtHeadConnected(void)
{
    if(mt->mt_type <= MT_HEAD || mt->mt_type == MT_TAIL)
    {
        return mtGetConnectDevices() == 1;
    }
    if(mtGetConnectDevices() == 1 && mt->mt_device[MT_CHILD].state == MT_SYN_Connected)
    {
        return TRUE;
    }
    return FALSE;
}

bool mtNodeConnected(void)
{
    if(mt->mt_type == MT_NODE)
    {
        return mtGetConnectDevices() >= 1;
    }
    return FALSE;
}
