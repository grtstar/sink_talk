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
#include "sink_volume.h"

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
#include <audio_plugin_voice_variants.h>

#include "headset_multi_talk.h"
#include "headset_multi_pair.h"
#include "audio_prompt.h"
#include "csr_common_example_plugin.h"
#include "headset_multi_comm.h"
#include "headset_uart.h"
#include "headset_ag.h"

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
#define PS_COUPLE_ADDR 4
#define PS_AUTH_KEY     0x27CF

MTData *mt;

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

uint8 RouteTableGetIndex(RouteTable *rt, bdaddr *addr)
{
    uint8 i = 0;
    for (i = 0; i < rt->count; i++)
    {
        if (BdaddrIsSame(&rt->item[i], addr))
        {
            return 1 << (7 - i);
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
    ConfigStore(141, NULL, 0);
}

typedef struct CoupleInfo
{
    bdaddr addr;
    uint8 type;
} CoupleInfo;

void mtSaveCoupleAddr(bdaddr *addr, uint8 couple_type)
{
    CoupleInfo ci;
    ci.addr = *addr;
    ci.type = couple_type;
    ConfigStore(PS_COUPLE_ADDR, &ci, sizeof(CoupleInfo));
}

bool mtLoadCoupleAddr(bdaddr *addr, uint8 *couple_type)
{
    uint16 len = 0;
    CoupleInfo ci = {{0}};
    len = ConfigRetrieve(PS_COUPLE_ADDR, &ci, sizeof(CoupleInfo));
    *addr = ci.addr;
    *couple_type = ci.type;
    return len > 0;
}

void mtResetCoupleAddr(void)
{
    CoupleInfo ci = {{0}};
    ConfigStore(PS_COUPLE_ADDR, &ci, sizeof(CoupleInfo));
    if (mt != NULL)
    {
        BdaddrSetZero(&mt->couple_addr);
    }
}

#if 0
const uint16 syn_conftab[] =
    {
        L2CAP_AUTOPT_SEPARATOR,                 /* START */
        L2CAP_AUTOPT_MTU_IN, 0x037F,            /* Maximum inbound MTU - 895 bytes */
        L2CAP_AUTOPT_MTU_OUT, 0x0030,           /* Minimum acceptable outbound MTU - 48 bytes */
        L2CAP_AUTOPT_FLUSH_IN, 0x0000, 0x0000,  /* Min acceptable remote flush timeout - zero*/
        0xFFFF, 0xFFFF,                         /* Max acceptable remote flush timeout - infinite*/
        L2CAP_AUTOPT_FLUSH_OUT, 0xFFFF, 0xFFFF, /* Min local flush timeout - infinite */
        0xFFFF, 0xFFFF,                         /* Max local flush timeout - infinite */
        L2CAP_AUTOPT_TERMINATOR                 /* END */
};
#endif

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

bool authOK = FALSE;

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
    }
    else
    {
        mt->mt_type = MT_NODE;
    }

    mt->connected_device = RouteTableGetIndex(&mt->route_table, &mt->addr);

    ConnectionL2capRegisterRequest(&mt->mt_task, MULTITALK_FRIEND_PSM, 0);
    ConnectionL2capRegisterRequest(&mt->mt_task, MULTITALK_NEARBY_PSM, 0);
    ConnectionL2capRegisterRequest(&mt->mt_task, MULTITALK_COUPLE_PSM, 0);

    BdaddrSetZero(&mt->couple_addr);
    mtLoadCoupleAddr(&mt->couple_addr, &mt->couple_type);

    BdaddrSetZero(&mt->header_addr[0]);
    BdaddrSetZero(&mt->header_addr[1]);
    mt->header_addr[0] = mt->addr;

    mt->mt_mode = CLOSE_MODE;

    UartGetHeadsetAddr();
    {
        uint32 key = CalcEncryptKey(mt->addr.nap, mt->addr.uap, mt->addr.lap, PROJECT_ID, COMPANY_ID);
        uint16 auth_key[2];
        PsFullRetrieve(PS_AUTH_KEY, auth_key, 2);
        if(key != (((uint32)auth_key[0])<<16 | auth_key[1]))
        {
            /*MessageSendLater(mt->app_task, EventUsrPowerOff, NULL, 8000);*/
        }
        else
        {
            authOK = TRUE;
        }
    }
}

bool mtRssiConnect(void)
{
    bdaddr bd_addr = RouteTableGet(&mt->route_table, mt->connect_idx % mt->route_table.count);

    MT_DEBUG(("MT: mtRssiConnect to "));
    MT_DEBUG_ADDR(bd_addr);
    if (RouteTableGetIndex(&mt->route_table, &bd_addr) & mt->connected_device)
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

void mtPowerOff(void)
{
    mtScoDisconnect(MT_LEFT);
    mtScoDisconnect(MT_RIGHT);
    mtACLDisconnect(MT_LEFT);
    mtACLDisconnect(MT_RIGHT);
    mt->mt_device[MT_LEFT].state = MT_L2CAP_Disconnected;
    BdaddrSetZero(&mt->mt_device[MT_LEFT].bt_addr);
    mt->mt_device[MT_RIGHT].acl_sink = NULL;
    mt->mt_device[MT_RIGHT].audio_sink = NULL;
    mt->mt_device[MT_RIGHT].state = MT_LEFT;
    BdaddrSetZero(&mt->mt_device[MT_RIGHT].bt_addr);
    mt->mt_device[MT_RIGHT].acl_sink = NULL;
    mt->mt_device[MT_RIGHT].audio_sink = NULL;
}
void mtDisconnect(void)
{
    if (mt->mt_device[MT_LEFT].state >= MT_SYN_Connecting)
    {
        MT_DEBUG(("MT: mtDisconnect left sco\n"));
        mtScoDisconnect(MT_LEFT);
    }
    else 
    {
        mtACLDisconnect(MT_LEFT);
    }

    if (mt->mt_device[MT_RIGHT].state >= MT_SYN_Connecting)
    {
        MT_DEBUG(("MT: mtDisconnect right sco\n"));
        mtScoDisconnect(MT_RIGHT);
    }
    else 
    {
        mtACLDisconnect(MT_RIGHT);
    }
}

void mtACLConnect(bdaddr *bd_addr, uint16 psm)
{
    MT_DEBUG(("MT: mtACLConnect to "));
    MT_DEBUG_ADDR((*bd_addr));
    ConnectionSetPageTimeout(16384);
    ConnectionL2capConnectRequest(&mt->mt_task, bd_addr, psm, psm, 0, 0);
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
        MT_DEBUG(("MT: Disconnect sco 0\n"));
        mt->mt_device[device].state = MT_SYN_Disconnecting;
        ConnectionSyncDisconnect(mt->mt_device[device].audio_sink, hci_error_oetc_user);
    }
    else if (mt->mt_device[device].state == MT_SYN_Connecting)
    {
        MT_DEBUG(("MT: Disconnect sco 0\n"));
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
    if (mt->mt_mode == FREIEND_MODE || mt->mt_mode == FREIEND_MODE_PAIRING)
    {
        can_recv = handleMTL2capConnectIndFriendMode(msg);
        psm = MULTITALK_FRIEND_PSM;
    }
    if (mt->mt_mode == NEARBY_MODE)
    {
        can_recv = handleMTL2capConnectIndNearbyMode(msg);
        psm = MULTITALK_NEARBY_PSM;
    }
    if (mt->mt_mode == COUPLE_MODE || mt->mt_mode == COUPLE_MODE_PAIRING || mt->mt_mode == CLOSE_MODE)
    {
        can_recv = handleMTL2capConnectIndCoupleMode(msg);
        psm = MULTITALK_COUPLE_PSM;
    }

    if (can_recv)
    {
        ConnectionL2capConnectResponse(&mt->mt_task, TRUE, psm, msg->connection_id,
                                       msg->identifier, 0, 0);
    }
    else
    {
        ConnectionL2capConnectResponse(&mt->mt_task, FALSE, psm, msg->connection_id,
                                       msg->identifier, 0, 0);
    }
}

void handleMTL2capConnectCfm(CL_L2CAP_CONNECT_CFM_T *msg)
{
    if (mt->mt_mode == FREIEND_MODE || mt->mt_mode == FREIEND_MODE_PAIRING)
    {
        handleMTL2capConnectCfmFriendMode(msg);
    }
    if (mt->mt_mode == NEARBY_MODE)
    {
        handleMTL2capConnectCfmNearbyMode(msg);
    }
    if (mt->mt_mode == COUPLE_MODE || mt->mt_mode == COUPLE_MODE_PAIRING)
    {
        handleMTL2capConnectCfmCoupleMode(msg);
    }
}

void handleMTL2capDisconInd(CL_L2CAP_DISCONNECT_IND_T *msg)
{
    if (mt->mt_mode == FREIEND_MODE || mt->mt_mode == FREIEND_MODE_PAIRING)
    {
        handleMTL2capDisconIndFriendMode(msg);
    }
    if (mt->mt_mode == NEARBY_MODE)
    {
        handleMTL2capDisconIndNearbyMode(msg);
    }
    if (mt->mt_mode == COUPLE_MODE || mt->mt_mode == COUPLE_MODE_PAIRING)
    {
        handleMTL2capDisconIndCoupleMode(msg);
    }
}

void handleMTL2capDisconCfm(CL_L2CAP_DISCONNECT_CFM_T *msg)
{
    if (mt->mt_mode == FREIEND_MODE || mt->mt_mode == FREIEND_MODE_PAIRING)
    {
        handleMTL2capDisconCfmFriendMode(msg);
    }
    if (mt->mt_mode == NEARBY_MODE)
    {
        handleMTL2capDisconCfmNearbyMode(msg);
    }
    if (mt->mt_mode == COUPLE_MODE || mt->mt_mode == COUPLE_MODE_PAIRING)
    {
        handleMTL2capDisconCfmCoupleMode(msg);
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

    if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_LEFT].bt_addr) && mt->mt_device[MT_LEFT].state == MT_L2CAP_Connected)
    {
        ConnectionSyncConnectResponse(&mt->mt_task, &msg->bd_addr, TRUE, &config_params);
        mt->mt_device[MT_LEFT].state = MT_SYN_Connecting;
    }
    if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_RIGHT].bt_addr) && mt->mt_device[MT_RIGHT].state == MT_L2CAP_Connected)
    {
        ConnectionSyncConnectResponse(&mt->mt_task, &msg->bd_addr, TRUE, &config_params);
        mt->mt_device[MT_RIGHT].state = MT_SYN_Connecting;
    }
    else
    {
        ConnectionSyncConnectResponse(&mt->mt_task, &msg->bd_addr, FALSE, &config_params);
    }
}

void handleMTSynConnCfm(CL_DM_SYNC_CONNECT_CFM_T *msg)
{
    if (mt->mt_mode == FREIEND_MODE || mt->mt_mode == FREIEND_MODE_PAIRING)
    {
        handleMTSynConnCfmFriendMode(msg);
    }
    if (mt->mt_mode == NEARBY_MODE)
    {
        handleMTSynConnCfmNearbyMode(msg);
    }
    if (mt->mt_mode == COUPLE_MODE || mt->mt_mode == COUPLE_MODE_PAIRING)
    {
        handleMTSynConnCfmCoupleMode(msg);
    }
    enableAudioActivePio();
}

void handleMTSynDisconInd(CL_DM_SYNC_DISCONNECT_IND_T *msg)
{
    if (mt->mt_mode == FREIEND_MODE || mt->mt_mode == FREIEND_MODE_PAIRING)
    {
        handleMTSynDisconIndFriendMode(msg);
    }
    if (mt->mt_mode == NEARBY_MODE)
    {
        handleMTSynDisconIndNearbyMode(msg);
    }
    if (mt->mt_mode == COUPLE_MODE || mt->mt_mode == COUPLE_MODE_PAIRING)
    {
        handleMTSynDisconIndCoupleMode(msg);
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
    uint8 d[2 + 8] = {0};
    MT_DEBUG(("MT: mtBroadcastCurrentCount %d\n", count));
    d[0] = ACLMSG_CURRENT_CONNECTED;
    d[1] = count;
    BdaddrToArray(&mt->head_addr, &d[2]);
    return ACLSend(mt->mt_device[MT_RIGHT].acl_sink, d, sizeof(d));
}

bool mtSendFindTail(int ch, uint8 type)
{
    uint8 d[2];
    d[0] = ACLMSG_FIND_TAIL;
    d[1] = type;
    MT_DEBUG(("MT: mtSendFindTail\n"));
    return ACLSend(mt->mt_device[ch].acl_sink, d, sizeof(d));
}

bool mtSendCheckTTL(uint8 ttl)
{
    uint8 d[2];
    MT_DEBUG(("MT: Send Check TTL\n"));
    d[0] = ACLMSG_CHECK_TTL;
    d[1] = ttl;
    return ACLSend(mt->mt_device[MT_LEFT].acl_sink, d, sizeof(d));
}

bool mtBroadcastConnectedCount(uint8 count)
{
    uint8 d[3];
    MT_DEBUG(("MT: mtBroadcastConnectedCount %d\n", count));

    d[0] = ACLMSG_DEVICE_COUNT;
    d[1] = count;
    d[2] = mt->connected_device;
    return ACLSend(mt->mt_device[MT_RIGHT].acl_sink, d, sizeof(d));
}

bool mtSendCheckTail(void)
{
    uint8 d[1];
    d[0] = ACLMSG_CHECK_TAIL;
    return ACLSend(mt->mt_device[MT_RIGHT].acl_sink, d, sizeof(d));
}

bool mtSendCheckHead(uint8 count)
{
    uint8 d[2];
    MT_DEBUG(("MT: Send Check Head, count = %d\n", count));
    d[0] = ACLMSG_CHECK_HEAD;
    d[1] = count;
    return ACLSend(mt->mt_device[MT_LEFT].acl_sink, d, sizeof(d));
}

bool mtSendDisconnect(void)
{
    uint8 d[1];
    d[0] = ACLMSG_NEARBY_DISCONNECT;
    return ACLSend(mt->mt_device[MT_RIGHT].acl_sink, d, sizeof(d));
}

bool mtBroadcastHeadAddr(uint8 count)
{
    uint8 d[2 + 8] = {0};
    d[0] = ACLMSG_HEAD_ADDR;
    d[1] = count;
    BdaddrToArray(&mt->head_addr, &d[2]);
    return ACLSend(mt->mt_device[MT_RIGHT].acl_sink, d, sizeof(d));
}

bool mtCheckLoop(uint8 ttl, bdaddr *addr)
{
    uint8 d[2 + 8] = {0};
    d[0] = ACLMSG_CHECK_LOOP;
    d[1] = ttl;
    BdaddrToArray(&mt->head_addr, &d[2]);
    return ACLSend(mt->mt_device[MT_RIGHT].acl_sink, d, sizeof(d));
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

bool mtReportRouteTable(int ch, RouteTable *rt)
{
#if 1
    uint8 d[2 + 8 * 7] = {0};
    uint8 i = 0;
    d[0] = ACLMSG_REPORT_TABLE;
    d[1] = rt->count;
    for (i = 0; i < rt->count; i++)
    {
        BdaddrToArray(&rt->item[i], &d[2 + 7 * i]);
    }
    MT_DEBUG(("MT: mtReportRouteTable %d\n", ch));
    return ACLSend(mt->mt_device[ch].acl_sink, d, 2 + 7 * rt->count);
#endif
}

bool mtSendRouteTable(int ch, RouteTable *rt)
{
#if 1
    uint8 d[2 + 8 * 7] = {0};
    uint8 i = 0;
    d[0] = ACLMSG_ROUTE_TABLE;
    d[1] = rt->count;
    for (i = 0; i < rt->count; i++)
    {
        BdaddrToArray(&rt->item[i], &d[2 + 7 * i]);
    }
    MT_DEBUG(("MT: mtSendRouteTable %d\n", ch));
    return ACLSend(mt->mt_device[ch].acl_sink, d, 2 + 7 * rt->count);
#endif
}

bool mtSendAck(int ch, int id)
{
    uint8 d[2] = {ACLMSG_ACK};
    d[1] = id;
    return ACLSend(mt->mt_device[ch].acl_sink, d, sizeof(d));
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
    DEBUG(("MT: ACL recv from Child:\n"));
    for (i = 0; i < size; i++)
    {
        DEBUG(("%02X ", data[i]));
        if ((i & 0x7) == 0x7)
        {
            DEBUG(("\n"));
        }
    }
    DEBUG(("\n"));
    ProcessData(1, data, size);

    switch (data[0])
    {
    case ACLMSG_EVENT:
        /* code */
        msg = data[1] << 8 | data[2];
        if (msg == EventMultiTalkEndPair)
        {
            uint8 *message = PanicUnlessMalloc(sizeof(uint8));
            ACLBroadcastEvent(mt->mt_device[MT_LEFT].acl_sink, EventMultiTalkEndPair);
            MessageSend(mt->app_task, EventMultiTalkEndPair, message);
        }
        break;
    case ACLMSG_CHECK_TTL:
    {
        if (!mtSendCheckTTL(data[1] + 1))
        {
            mt->total_connected = data[1] + 1;
            mtBroadcastCurrentCount(data[1] + 1);
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
    }
    break;
    default:
        break;
    }
    ACLProcessChildDataNearby(data, size);
    ACLProcessParentDataCouple(data, size);
}

void ACLProcessParentData(const uint8_t *data, int size)
{
    int i = 0;
    uint16 msg = 0;
    DEBUG(("MT: ACL recv from Parent:\n"));
    for (i = 0; i < size; i++)
    {
        DEBUG(("%02X ", data[i]));
        if ((i & 0x7) == 0x7)
        {
            DEBUG(("\n"));
        }
    }
    DEBUG(("\n"));
    ProcessData(0, data, size);
    switch (data[0])
    {
    case ACLMSG_EVENT:
        /* code */
        msg = data[1] << 8 | data[2];
        if (msg == EventMultiTalkEndPair)
        {
            uint8 *message = PanicUnlessMalloc(sizeof(uint8));
            ACLBroadcastEvent(mt->mt_device[MT_RIGHT].acl_sink, EventMultiTalkEndPair);
            MessageSend(mt->app_task, EventMultiTalkEndPair, message);
        }
        break;
        break;
    case ACLMSG_DEVICE_COUNT:
    {
        if (data[1] != 0)
        {
            mt->total_connected = data[1];
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
        if (mtBroadcastConnectedCount(data[1]))
        {
            if (mt->total_connected != mt->route_table.count)
            {
                inquiryPair(inquiry_session_nearby, FALSE);
            }
        }
    }
    break;
    case ACLMSG_CURRENT_CONNECTED:
    {
        if (data[1] != 0)
        {
            mt->total_connected = data[1];
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
        mt->head_addr = ArrayToBdaddr(&data[2]);
        if (!mtBroadcastCurrentCount(data[1]))
        {
            mt->status = MT_ST_SEARCHING;
        }
    }
    break;
    default:
        break;
    }
    ACLProcessParentDataNearby(data, size);
    ACLProcessParentDataCouple(data, size);
}

uint8 powerOffTimes = 0;
bool processEventMultiTalk(Task task, MessageId id, Message message)
{
    if (id == EventUsrPowerOff)
    {
        mtPowerOff();
        if(AgIsConnected())
        {
            AgDisconnect();
        }
        mt->mt_mode = CLOSE_MODE;
        PowerAmpOff();
        disableAudioActivePio();
        return FALSE;
    }
    if (id == EventSysMuiltTalkPowerOn)
    {
        if (!BdaddrIsZero(&mt->couple_addr))
        {
            /*
            PowerAmpOn();
            mtInquiryPair(inquiry_session_2talk, FALSE);
            */
        }
        return FALSE;
    }
    if (id == EventMultiTalkUser1)
    {
        /* UartSendState(stateManagerGetExtendedState()); */
        if(!authOK)
        {
            MessageSendLater(&theSink.task, EventMultiTalkUser1, NULL, D_SEC(30));
            if (powerOffTimes++ > 120)
            {
                MessageSend(task, EventUsrPowerOff, NULL);
                powerOffTimes = 0;
                return FALSE;
            }
        }        
    }
    if (id == EventUsrEnterPairing)
    {
        mt->prepare_paring = 1;
    }
    if (id == EventUsrCancelPairing)
    {
        if (mt->prepare_paring == 1)
        {
            mt->prepare_paring = 0;
            if (mt->mt_mode == COUPLE_MODE_PAIRING)
            {
                MessageSendLater(task, EventSysMultiTalkQuitPairingCoupleMode, NULL, D_SEC(2));
            }
        }
    }
    if(id == EventSysPairingFail)
    {
        if (mt->prepare_paring == 1)
        {
            mt->prepare_paring = 0;
            if (mt->mt_mode == COUPLE_MODE_PAIRING)
            {
                MessageSendLater(task, EventSysMultiTalkPairingTimeoutCoupleMode, NULL, D_SEC(2));
            }
        }
    }

    if (id == EventSysPrimaryDeviceConnected || id == EventSysSecondaryDeviceConnected  || id == EventSysSCOLinkOpen || id == EventSysPairingSuccessful)
    {
        if (mt->prepare_paring == 1)
        {
            mt->prepare_paring = 0;
            if (mt->mt_mode == COUPLE_MODE_PAIRING)
            {
                MessageSendLater(task, EventSysMultiTalkQuitPairingCoupleMode, NULL, D_SEC(2));
            }
            return FALSE;
        }
        else
        {
            return FALSE;
        }
    }
    if (id == EventMultiTalkKeyClick)
    {
        if (mt->prepare_paring == 1)
        {
            switch (mt->mt_mode)
            {
            case CLOSE_MODE:
                AudioPlay(AP_TWO_TALK_PAIRING, TRUE);
                mt->mt_mode = COUPLE_MODE_PAIRING;
                MessageSend(task, EventSysMultiTalkPairingCoupleMode, NULL);
                break;
            case NEARBY_MODE:
                AudioPlay(AP_TWO_TALK_PAIRING, TRUE);
                MessageSend(task, EventSysMultiTalkLeaveNearbyMode, NULL);
                break;
            case FREIEND_MODE:
                AudioPlay(AP_TWO_TALK_PAIRING, TRUE);
                MessageSend(task, EventSysMultiTalkLeaveFriendMode, NULL);
                break;
            case COUPLE_MODE:
                AudioPlay(AP_TWO_TALK_PAIRING, TRUE);
                mt->mt_mode = COUPLE_MODE_PAIRING;
                MessageSend(task, EventSysMultiTalkPairingCoupleMode, NULL);
                break;
            case FREIEND_MODE_PAIRING:
                AudioPlay(AP_TWO_TALK_PAIRING, TRUE);
                MessageSend(task, EventMultiTalkEndPair, NULL);
                break;
            case COUPLE_MODE_PAIRING:
                MessageSend(task, EventSysMultiTalkQuitPairingCoupleMode, NULL);
                break;
            default:
                break;
            }
        }
        else
        {
            if (mt->mt_mode == CLOSE_MODE)
            {
                mt->mt_mode = NEARBY_MODE;
                MessageSend(task, EventSysMultiTalkEnterNearbyMode, NULL);
            }
            else if (mt->mt_mode == NEARBY_MODE)
            {
                MessageSend(task, EventSysMultiTalkLeaveNearbyMode, NULL);
                return FALSE;
            }
            else if (mt->mt_mode == FREIEND_MODE)
            {
                MessageSend(task, EventSysMultiTalkLeaveFriendMode, NULL);
                return FALSE;
            }
            else if (mt->mt_mode == FREIEND_MODE_PAIRING)
            {
                MessageSend(task, EventMultiTalkConnect, NULL);
                return FALSE;
            }
            else if (mt->mt_mode == COUPLE_MODE)
            {
                MessageSend(task, EventSysMultiTalkLeaveCoupleMode, NULL);
                return FALSE;
            }

            switch (mt->mt_mode)
            {
            case CLOSE_MODE:
                AudioPlay(AP_TWO_TALK_OFF, TRUE);
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
    }
    if (id == EventMultiTalkKeyPress)
    {
        mt->prepare_paring = 0;
        if (mt->mt_mode == FREIEND_MODE_PAIRING)
        {
            MessageSend(task, EventMultiTalkEndPair, NULL);
        }
        else
        {
            switch (mt->mt_mode)
            {
            case CLOSE_MODE:
                MessageSend(task, EventMultiTalkEnterPair, NULL);
                mt->mt_mode = FREIEND_MODE_PAIRING;
                break;
            case NEARBY_MODE:
                MessageSend(task, EventSysMultiTalkLeaveNearbyMode, NULL);
                mt->prepare_paring = 2;
                break;
            case FREIEND_MODE:
                MessageSend(task, EventSysMultiTalkLeaveFriendMode, NULL);
                mt->prepare_paring = 2;
                break;
            case COUPLE_MODE:
                MessageSend(task, EventSysMultiTalkLeaveCoupleMode, NULL);
                mt->prepare_paring = 2;
                break;
            case COUPLE_MODE_PAIRING:
                mt->prepare_paring = 2;
                MessageSend(task, EventSysMultiTalkQuitPairingCoupleMode, NULL);
                break;
            default:
                break;
            }
        }
    }
    if (id == EventSysMultiTalkNeabyModeLeaved)
    {
        if (mt->prepare_paring == 1)
        {
            mt->mt_mode = COUPLE_MODE_PAIRING;
            MessageSend(task, EventSysMultiTalkPairingCoupleMode, NULL);
            return FALSE;
        }
        if (mt->prepare_paring == 2)
        {
            mt->prepare_paring = 0;
            mt->mt_mode = FREIEND_MODE_PAIRING;
            MessageSend(task, EventMultiTalkEnterPair, NULL);
            return FALSE;
        }
        else if (!RouteTableIsNotSaved(&mt->route_table))
        {
            mt->mt_mode = FREIEND_MODE;
            MessageSend(task, EventSysMultiTalkEnterFriendMode, NULL);
            AudioPlay(AP_MULTI_TALK_FRIEND_MODE, TRUE);
        }
        else if (!BdaddrIsZero(&mt->couple_addr)) /* couple talk */
        {
            mt->mt_mode = COUPLE_MODE;
            MessageSend(task, EventSysMultiTalkEnterCoupleMode, NULL);
            AudioPlay(AP_TWO_TALK_ON, TRUE);
            /*AudioPlay(AP_MULTI_TALK_COUPLE_MODE, TRUE);*/
        }
        else
        {
            mt->mt_mode = CLOSE_MODE;
            AudioPlay(AP_TWO_TALK_OFF, TRUE);
            PowerAmpOff();
        }
    }
    if (id == EventSysMultiTalkFriendModeLeaved)
    {
        if (mt->prepare_paring == 1)
        {
            mt->mt_mode = COUPLE_MODE_PAIRING;
            MessageSend(task, EventSysMultiTalkPairingCoupleMode, NULL);
            return FALSE;
        }
        if (mt->prepare_paring == 2)
        {
            mt->prepare_paring = 0;
            mt->mt_mode = FREIEND_MODE_PAIRING;
            MessageSend(task, EventMultiTalkEnterPair, NULL);
            return FALSE;
        }
        if (mt->mt_mode == FREIEND_MODE)
        {
            if (!BdaddrIsZero(&mt->couple_addr)) /* couple talk */
            {
                mt->mt_mode = COUPLE_MODE;
                MessageSend(task, EventSysMultiTalkEnterCoupleMode, NULL);
                AudioPlay(AP_TWO_TALK_ON, TRUE);
                /*AudioPlay(AP_MULTI_TALK_COUPLE_MODE, TRUE);*/
            }
            else
            {
                AudioPlay(AP_TWO_TALK_OFF, TRUE);
                mt->mt_mode = CLOSE_MODE;
                PowerAmpOff();
            }
        }
        if (mt->mt_mode == FREIEND_MODE_PAIRING)
        {
            mt->mt_mode = CLOSE_MODE;
            stateManagerUpdateState();
            PowerAmpOff();
        }
    }
    if (id == EventSysMultiTalkCoupleModeLeaved)
    {
        if (mt->prepare_paring == 2)
        {
            mt->prepare_paring = 0;
            mt->mt_mode = FREIEND_MODE_PAIRING;
            MessageSend(task, EventMultiTalkEnterPair, NULL);
            MessageCancelAll(mt->app_task, EventSysMultiTalkLeaveCoupleModeDelay);
            return FALSE;
        }
        if (mt->mt_mode == COUPLE_MODE)
        {
            mt->mt_mode = CLOSE_MODE;
            AudioPlay(AP_TWO_TALK_OFF, TRUE);
            PowerAmpOff();
        }
        if (mt->mt_mode == COUPLE_MODE_PAIRING)
        {
            mt->mt_mode = CLOSE_MODE;
            if (mt->prepare_paring == 0)
            {
                AudioPlay(AP_TWO_TALK_QUIT_PAIR, TRUE);
            }
            PowerAmpOff();
        }
    }
    return FALSE;
}

bool mtVoicePopulateConnectParameters(audio_connect_parameters *connect_parameters)
{
    Sink audio_sink = NULL;
    bool two_sco = FALSE;
    if (mt->mt_device[MT_LEFT].audio_sink != NULL)
    {
        audio_sink = mt->mt_device[MT_LEFT].audio_sink;
        MT_DEBUG(("MT: PopulateConnectParameters Parent Sink\n"));
    }
    else if (mt->mt_device[MT_RIGHT].audio_sink != NULL)
    {
        audio_sink = mt->mt_device[MT_RIGHT].audio_sink;
        MT_DEBUG(("MT: PopulateConnectParameters Child Sink\n"));
    }
    mt->plugin_params = *sinkHfpDataGetHfpPluginParams();
    if (mt->mt_device[MT_LEFT].audio_sink != NULL && mt->mt_device[MT_RIGHT].audio_sink != NULL)
    {
        mt->plugin_params.usb_params.usb_sink = mt->mt_device[MT_LEFT].audio_sink;
        audio_sink = mt->mt_device[MT_RIGHT].audio_sink;
        two_sco = TRUE;
        MT_DEBUG(("MT: PopulateConnectParameters Tow Sink\n"));
    }
    else
    {
        mt->plugin_params.usb_params.usb_sink = NULL;
    }
    if (audio_sink != NULL)
    {
        connect_parameters->app_task = mt->app_task;

        connect_parameters->audio_plugin = two_sco ? CVSD1MIC_EXAMPLE : AudioPluginVoiceVariantsGetHfpPlugin(hfp_wbs_codec_mask_cvsd, sinkHfpDataGetAudioPlugin());
        connect_parameters->audio_sink = audio_sink;
        connect_parameters->features = sinkAudioGetPluginFeatures();
        connect_parameters->mode = AUDIO_MODE_CONNECTED;
        connect_parameters->params = &mt->plugin_params;
        connect_parameters->power = powerManagerGetLBIPM();
        connect_parameters->rate = 8000;
        connect_parameters->route = AUDIO_ROUTE_INTERNAL;
        connect_parameters->sink_type = AUDIO_SINK_ESCO;
        if (mt->mt_mode == FREIEND_MODE_PAIRING || mt->mt_mode == COUPLE_MODE_PAIRING)
        {
            sinkHfpDataSetDefaultVolume(12);
            connect_parameters->volume = 12;
            if(mt->mt_mode == FREIEND_MODE_PAIRING)
            {
                mtMicMute(AUDIO_MUTE_ENABLE, TRUE);
            }
            else 
            {
                mtMicMute(AUDIO_MUTE_DISABLE, TRUE);
            }
        }
        else 
        {
            if(!mt->mic_mute) mtMicMute(AUDIO_MUTE_DISABLE, TRUE);
            connect_parameters->volume = sinkHfpDataGetDefaultVolume();
        }
        DEBUG(("volume = %d\n", connect_parameters->volume));
        return TRUE;
    }
    return FALSE;
}

Sink mtGetActiveScoSink(void)
{
    uint16 sink_all = (uint16)mt->mt_device[MT_LEFT].audio_sink + (uint16)mt->mt_device[MT_RIGHT].audio_sink;
    if (mt->mt_device[MT_LEFT].audio_sink == NULL)
    {
        /*return NULL;*/
    }
    return (Sink)sink_all;
}

bool isMTVoiceSink(Sink sink)
{
    uint16 sink_all = (uint16)mt->mt_device[MT_LEFT].audio_sink + (uint16)mt->mt_device[MT_RIGHT].audio_sink;
    if (sink == (Sink)sink_all)
    {
        return TRUE;
    }
    return FALSE;
}

int mtGetConnectDevices(void)
{
    int count = (mt->mt_device[MT_LEFT].state == MT_SYN_Connected) + (mt->mt_device[MT_RIGHT].state == MT_SYN_Connected);
    MT_DEBUG(("MT: Connected Device %d\n", count));
    return count;
}

bool mtCanPair(bdaddr *addr)
{
    if(BdaddrIsSame(addr, &mt->couple_addr))
    {
        return TRUE;
    }
    /*
    if (addr->nap != mt->addr.nap || addr->uap != mt->addr.uap)
    {
        return FALSE;
    }
    */
    if (mt->status == MT_ST_RECONNECTING || mt->status == MT_ST_LINKLOSS ||
        mt->status == MT_ST_CONNECTING || mt->status == MT_ST_WAITING_CONNECT ||
        mt->status == MT_ST_PARING || mt->status == MT_ST_SEARCHING)
    {
        return TRUE;
    }
    return FALSE;
}

bool mtIsOnlyChildConnect(void)
{
    if (mt->mt_device[MT_LEFT].state < MT_L2CAP_Connected && mt->mt_device[MT_RIGHT].state >= MT_L2CAP_Connected)
    {
        return TRUE;
    }
    return FALSE;
}

bool mtIsOnlyParentConnect(void)
{
    if (mt->mt_device[MT_RIGHT].state < MT_L2CAP_Connected && mt->mt_device[MT_LEFT].state >= MT_L2CAP_Connected)
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
    if (_1->lap > _2->lap)
    {
        return 1;
    }
    else if (_1->lap < _2->lap)
    {
        return -1;
    }
    if (_1->uap > _2->uap)
    {
        return 1;
    }
    else if (_1->uap < _2->uap)
    {
        return -1;
    }
    if (_1->nap > _2->nap)
    {
        return 1;
    }
    else if (_1->nap < _2->nap)
    {
        return -1;
    }
    return 0;
}

int GetOnes(uint8 v)
{
    int i = 0, count = 0;
    for (i = 0; i < 8; i++)
    {
        count += !!(v & (1 << i));
    }
    return count;
}

bool mtHeadConnected(void)
{
    if (mt->mt_type <= MT_HEAD || mt->mt_type == MT_TAIL)
    {
        return mtGetConnectDevices() == 1;
    }
    if (mtGetConnectDevices() == 1)
    {
        return TRUE;
    }
    return FALSE;
}

bool mtNodeConnected(void)
{
    return mtGetConnectDevices() > 1;
}

bool mtIsFriendPairing(void)
{
    return mt->mt_mode == FREIEND_MODE_PAIRING;
}

void mtSetHeadsetAddr(bdaddr *addr)
{
    mt->headset_addr = *addr;
}

void MTResetDeviceList(void)
{
    mtClearRouteTable(&mt->route_table);
    mtResetCoupleAddr();
}

bool mtCheckPeerDevice(bdaddr *addr)
{
    if (addr->uap == mt->headset_addr.uap && addr->nap == mt->headset_addr.nap)
    {
        return TRUE;
    }
    return FALSE;
}

void mtMicMute(uint8 state, bool force)
{
    if (mtGetConnectDevices() >= 1 || AgIsConnected() || force)
    {
        if (state == AUDIO_MUTE_ENABLE)
        {
            MT_DEBUG(("Mute On\n"));
            mt->mic_mute = TRUE;
            volumeSetMuteState(audio_mute_group_mic, AUDIO_MUTE_ENABLE);
        }
        else
        {
            MT_DEBUG(("Mute Off\n"));
            volumeSetMuteState(audio_mute_group_mic, AUDIO_MUTE_DISABLE);
        }
        VolumeApplySoftMuteStates();
    }
}

void sinkGetMTVolume(volume_info *volume)
{
    volume->main_volume = sinkHfpDataGetDefaultVolume();
}
void sinkSetMTVolume(volume_info *volume)
{
    sinkHfpDataSetDefaultVolume(volume->main_volume);
}


uint32 CalcEncryptKey(uint32 address_nap, uint32 address_uap, uint32 address_lap, uint32 projectId, uint32 companyId)
{
	char key1[25] = "57F1079BE935E5792EC8C8A7";
	char key2[25] = "B6F1D0B948C8E359C37AF503";

	uint32 key = 0;
	uint32 k1 = 0;
	uint32 k2 = 0;
	uint32 k3 = 0;
	uint32 k4 = 0;
    uint8 i = 0;

	for (i = 0; i < sizeof(key1); i++){
		k1 += key1[(i * 3 + k1 + projectId) % 25] & 0xff;
		k2 += key2[(i * 5 + k2 + projectId) % 25] & 0xff;
		k3 += key1[(i * 7 + k3 + projectId) % 25] & 0xff;
		k4 += key2[(i * 9 + k4 + projectId) % 25] & 0xff;
	}

	for (i = 0; i < sizeof(key1); i++){
		k1 += key2[(i * 13 + k4 + companyId) % 25] & 0xff;
		k2 += key1[(i * 15 + k1 + companyId) % 25] & 0xff;
		k3 += key2[(i * 17 + k2 + companyId) % 25] & 0xff;
		k4 += key1[(i * 19 + k3 + companyId) % 25] & 0xff;
	}

	for (i = 0; i < sizeof(key1); i++){
		k1 += key1[(i * 23 + k4 + address_nap + address_uap + address_lap) % 25] & 0xff;
		k2 += key2[(i * 25 + k3 + address_nap + address_uap + address_lap) % 25] & 0xff;
		k3 += key2[(i * 27 + k2 + address_nap + address_uap + address_lap) % 25] & 0xff;
		k4 += key1[(i * 29 + k1 + address_nap + address_uap + address_lap) % 25] & 0xff;
	}

	key = ((k4 & 0xff) << 24) | ((k3 & 0xff) << 16) | ((k2 & 0xff) << 8) | ((k1 & 0xff) << 0);
	return key;
}
