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

#ifdef ENABLE_MT_DEBUG
#define MT_DEBUG(x) DEBUG(x)
#define MT_DEBUG_ADDR(x) DEBUG(("bdaddr = %x:%x:%lx\n", x.nap, x.uap, x.lap))
#else
#define MT_DEBUG(X)
#define MT_DEBUG_ADDR(x)
int sinkEnableDebug(void);
#endif

extern ExamplePluginTaskdata csr_cvsd_8k_1mic_plugin;
extern ExamplePluginTaskdata csr_sbc_1mic_plugin;
extern ExamplePluginTaskdata csr_cvsd_8k_2mic_plugin;
extern ExamplePluginTaskdata csr_sbc_2mic_plugin;

/* Example Plugins */
#define CVSD1MIC_EXAMPLE (TaskData *)&csr_cvsd_8k_1mic_plugin
#define SBC1MIC_EXAMPLE (TaskData *)&csr_sbc_1mic_plugin
#define CVSD2MIC_EXAMPLE (TaskData *)&csr_cvsd_8k_2mic_plugin
#define SBC2MIC_EXAMPLE (TaskData *)&csr_sbc_2mic_plugin

#define MULTITALK_PSM 0x0055
#define MULTITALK_PROTOCOL_PSM 0x0057

#define PS_ROUTE_TABLE 3

typedef enum MTType
{
    MT_HEAD,
    MT_NODE,
    MT_TAIL
} MTType;

typedef struct RouteTable
{
    uint8 count;
    uint8 nop;
    uint8 reconnect_index;
    uint8 nop2;
    bdaddr item[8];
} RouteTable;

#if 0
static void RouteTableAddChild(RouteTable *rt, uint8 ttl, bdaddr addr);
#endif
static void BdaddrToArray(bdaddr *addr, uint8 *array);
static bdaddr ArrayToBdaddr(const uint8 *array);

static void RouteTablePushChild(RouteTable *rt, bdaddr addr);
static bdaddr RouteTableGet(RouteTable *rt, uint8 index);
static bool RouteTableIsNotSaved(RouteTable *rt);
static uint8 RouteTableIsContain(RouteTable *rt, bdaddr *addr);
static void RouteTableSort(RouteTable *rt);
#if 0
static void RouteTableSetHead(RouteTable *rt, bdaddr addr);
static void RouteTableSetTail(RouteTable *rt, bdaddr addr);
#endif
static void mtSendRouteTable(RouteTable *rt);
static void mtClearRouteTable(RouteTable *rt);
static void mtSaveRouteTable(RouteTable *rt, uint8 mt_type, uint8 rt_idx);
static void mtLoadRouteTable(RouteTable *rt);
static void mtResetPairList(void);
void mtRouteTableAdjust(bdaddr child_addr);

static void ACLSend(Sink sink, const uint8_t *data, uint16 packet_size);
static void ACLBroadcastEvent(Sink sink, uint16 event);

static void mtBroadcastDisconnected(int mt_device, bdaddr my_addr);
static void mtSendFindTail(void);
static void mtSendCheckTTL(uint8 ttl);
static void mtBroadcastConnectedCount(uint8 count);

static int BdaddrCompare(bdaddr *_1, bdaddr *_2);

#define PowerAmpOn() PioSetPio(14, pio_drive, TRUE)
#define PowerAmpOff() PioSetPio(14, pio_drive, FALSE)

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
    int i = 0, j = 0;
    for(i=0; i<rt->count; i++)
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

#if 0
void RouteTableSetHead(RouteTable *rt, bdaddr addr)
{
    uint8 idx = RouteTableIsContain(rt, &addr);
    if (idx > 0)
    {
        bdaddr temp = rt->item[0];
        rt->item[0] = addr;
        rt->item[idx - 1] = temp;
    }
}

void RouteTableSetTail(RouteTable *rt, bdaddr addr)
{
    uint8 idx = RouteTableIsContain(rt, &addr);
    if (idx > 0)
    {
        bdaddr temp = rt->item[rt->count - 1];
        rt->item[rt->count - 1] = addr;
        rt->item[idx - 1] = temp;
    }
}

void RouteTableAddChild(RouteTable *rt, uint8 ttl, bdaddr addr)
{
    MT_DEBUG(("MT: Add Child %d ", ttl));
    MT_DEBUG_ADDR(addr);
    if (ttl >= 8)
    {
        MT_DEBUG(("MT: ttl error\n"));
        return;
    }
    if (RouteTableIsContain(rt, &addr))
    {
        return;
    }

    rt->item[ttl] = addr;
    if (rt->count < ttl + 1)
    {
        rt->count = ttl + 1;
    }
}
#endif

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

uint16 saved = 0;
void mtSaveRouteTable(RouteTable *rt, uint8 mt_type, uint8 rt_idx)
{
    if (mt_type == MT_HEAD)
    {
        rt->nop = 0xA5;
    }
    else
    {
        rt->nop = 0x55;
    }
    rt->reconnect_index = rt_idx;
    saved = ConfigStore(PS_ROUTE_TABLE, rt, sizeof(RouteTable)); /* DSP49 */
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

typedef enum DevcieSate
{
    MT_L2CAP_Disconnected,
    MT_L2CAP_Connecting,
    MT_L2CAP_Connected,
    MT_SYN_Connecting,
    MT_SYN_Connected,
    MT_L2CAP_Disconnecting,
    MT_SYN_Disconnecting,
    MT_SYN_Disconnected
} DevcieSate;

typedef enum MTStatus
{
    MT_ST_NOCONNECT,
    MT_ST_PARING,
    MT_ST_CONNECTING,
    MT_ST_CONNECTED,
    MT_ST_LINKLOSS,
    MT_ST_WAITING_CONNECT,
    MT_ST_RECONNECTING,
    MT_ST_STAY_DISCONNET
} MTStatus;

typedef enum ACLMsg
{
    ACLMSG_EVENT,
    ACLMSG_ONE_CONNECTED,
    ACLMSG_ONE_DISCONNECTED,
    ACLMSG_FIND_TAIL,
    ACLMSG_CHECK_TTL,
    ACLMSG_DEVICE_COUNT,
    ACLMSG_ROUTE_TABLE
} ACLMsg;

typedef struct MTDevice
{
    DevcieSate state : 4;
    sync_link_type link_type : 4;
    BITFIELD unused : 8;
    bdaddr bt_addr;
    Sink acl_sink;
    Sink audio_sink;

} MTDevice;

typedef struct MTData
{
    Task app_task;
    TaskData mt_task;
    MTDevice mt_device[2];
    hfp_common_plugin_params_t plugin_params;
    MTType mt_type : 2;
    MTStatus status : 4;
    BITFIELD total_connected : 4;
    BITFIELD connect_idx : 3;
    BITFIELD reconnect_count : 3;
    int8 index_rt;
    RouteTable route_table;
    bdaddr addr;
    bdaddr head_addr;
    uint8_t is_2talk;
} MTData;

static const uint16 syn_conftab[] =
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

static MTData *mt;

TaskData acl_parent_task = {acl_parent_handler};
TaskData acl_child_task = {acl_child_handler};

static int enableDebug = 1;

void mt_handler(Task task, MessageId id, Message message)
{
    MT_DEBUG(("MT: mt_handler id = 0x%x\n", id));
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
    PsFullRetrieve(1, temp, 4);
    mt->addr.nap = temp[3];
    mt->addr.uap = temp[2];
    mt->addr.lap = ((uint32)temp[0]) << 16 | temp[1];
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
    mt->index_rt = mt->route_table.reconnect_index;

    ConnectionL2capRegisterRequest(&mt->mt_task, MULTITALK_PSM, 0);

    if (mt->addr.lap == 0xA5A5)
    {
        enableDebug = 1;
    }
}

/* enter paring mode */
bool mtEnterPairing(void)
{
    return TRUE;
}

/*
    enter rssi inquiry mode
    ????????????????????????
    ?????????????????????
*/
void mtInquiry(void)
{
}

bool mtRssiConnect(void)
{
    bdaddr bd_addr = RouteTableGet(&mt->route_table, mt->connect_idx % mt->route_table.count);

    MT_DEBUG(("MT: mtRssiConnect to "));
    MT_DEBUG_ADDR(bd_addr);
    if (!BdaddrIsSame(&bd_addr, &mt->head_addr) && !BdaddrIsSame(&bd_addr, &mt->addr) && !BdaddrIsSame(&bd_addr, &mt->mt_device[MT_PARENT].bt_addr))
    {
        deviceManagerRemoveDevice(&bd_addr);
        return mtConnect(mt->connect_idx);
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

bool mtConnect(int index)
{
    bdaddr *bd_addr = &mt->route_table.item[index];
    MT_DEBUG(("MT: connect idx:%d, rt:%d\n", index, mt->index_rt));
    if (index < mt->index_rt)
    {
        return FALSE;
    }
    if (!BdaddrIsSame(bd_addr, &mt->route_table.item[0]) && !BdaddrIsSame(bd_addr, &mt->head_addr) && !BdaddrIsSame(bd_addr, &mt->addr) && !BdaddrIsSame(bd_addr, &mt->mt_device[MT_PARENT].bt_addr))
    {
        mt->mt_device[MT_CHILD].bt_addr = *bd_addr;
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Connecting;
        mtACLConnect(bd_addr);
        return TRUE;
    }
    else
    {
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
    }
    return FALSE;
}

void mtReconnect(void)
{
}

void mtACLConnect(bdaddr *bd_addr)
{
    MT_DEBUG(("MT: mtACLConnect to "));
    MT_DEBUG_ADDR((*bd_addr));
    ConnectionSetPageTimeout(16384);
    ConnectionL2capConnectRequest(&mt->mt_task, bd_addr, MULTITALK_PSM, MULTITALK_PSM, sizeof(syn_conftab), (uint16 *)syn_conftab);
}

void mtACLDisconnect(int device)
{
    if (mt->mt_device[device].acl_sink != NULL)
    {
        mt->mt_device[device].state = MT_L2CAP_Disconnecting;
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
    }
}

void handleMTL2capRegisterCfm(CL_L2CAP_REGISTER_CFM_T *msg)
{
    MT_DEBUG(("handleMTL2capRegisterCfm status=%d, prim=0x%x\n", msg->status, msg->psm));
    if (msg->psm == MULTITALK_PSM)
    {
        ConnectionSyncRegister(&mt->mt_task);
    }
}

void handleMTL2capConnectInd(CL_L2CAP_CONNECT_IND_T *msg)
{
    bool can_recv = FALSE;

    /*
        ?????????????????????????????????????????????
    */
    MT_DEBUG(("MT: handleMTL2capConnectInd: addr=%x:%x:%lx, psm=0x%x, identifier=0x%x, connID=0x%x\n",
              msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap, msg->psm,
              msg->identifier, msg->connection_id));

    if (mt->mt_device[MT_PARENT].state == MT_L2CAP_Disconnected || mt->mt_device[MT_PARENT].state >= MT_L2CAP_Disconnecting)
    {
        /* ?????????????? */
        MT_DEBUG(("MT: handleMTL2capConnectInd parant is disconnect\n"));
        if (mt->status == MT_ST_PARING || RouteTableIsContain(&mt->route_table, &msg->bd_addr))
        {
            DEBUG(("MT: handleMTL2capConnectInd type=%d, status = %d\n", mt->mt_type, mt->status));

            /*if (mt->mt_device[MT_CHILD].state >= MT_L2CAP_Connecting && BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_CHILD].bt_addr))*/
            if (mt->mt_device[MT_CHILD].state >= MT_L2CAP_Connecting && mt->mt_device[MT_CHILD].state < MT_SYN_Connected)
            {
                mt->status = MT_ST_WAITING_CONNECT;
                mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
                BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
                MessageCancelAll(mt->app_task, EventSysMultiTalkReconnect);
                MessageCancelAll(mt->app_task, EventMultiTalkReconnect);
                if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_CHILD].bt_addr))
                {
                    DEBUG(("MT: handleMTL2capConnectInd cannot allow child connected to parent\n"));
                    can_recv = FALSE;
                }
                else
                {
                    DEBUG(("MT: handleMTL2capConnectInd interupt child connecting\n"));
                    mt->mt_device[MT_PARENT].state = MT_L2CAP_Connecting;
                    mt->mt_device[MT_PARENT].bt_addr = msg->bd_addr;
                    mt->mt_type = MT_NODE;
                    can_recv = TRUE;
                }
                
            }
            else if (mt->status == MT_ST_NOCONNECT || mt->status == MT_ST_STAY_DISCONNET)
            {
                DEBUG(("MT: handleMTL2capConnectInd cannot allow connected before click connect key\n"));
                can_recv = FALSE;
            }
            else if (mt->mt_type != MT_HEAD && mt->status != MT_ST_CONNECTED)
            {
                if (BdaddrIsSame(&msg->bd_addr, &mt->mt_device[MT_CHILD].bt_addr))
                {
                    DEBUG(("MT: handleMTL2capConnectInd cannot allow child connected to parent\n"));
                    can_recv = FALSE;
                }
                else
                {
                    MT_DEBUG(("MT: handleMTL2capConnectInd peer is my cai\n"))
                    mt->mt_device[MT_PARENT].state = MT_L2CAP_Connecting;
                    mt->mt_device[MT_PARENT].bt_addr = msg->bd_addr;
                    mt->mt_type = MT_NODE;
                    can_recv = TRUE;
                }
            }
        }
    }
    if (can_recv)
    {
        ConnectionL2capConnectResponse(&mt->mt_task, TRUE, MULTITALK_PSM, msg->connection_id,
                                       msg->identifier, sizeof(syn_conftab), (uint16 *)syn_conftab);
    }
    else
    {
        ConnectionL2capConnectResponse(&mt->mt_task, FALSE, MULTITALK_PSM, msg->connection_id,
                                       msg->identifier, sizeof(syn_conftab), (uint16 *)syn_conftab);
    }
}

void handleMTL2capConnectCfm(CL_L2CAP_CONNECT_CFM_T *msg)
{
    /*
        ??????????????????????????????????
        ????????????????????????sco ?????
    */
    MT_DEBUG(("MT: handleMTL2capConnectCfm: addr=%x:%x:%lx, status=%d\n",
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

            ConnectionSetLinkSupervisionTimeout(msg->sink, 16000);
            linkPolicyUseMultiTalkSettings(msg->sink);

            MessageSinkTask(msg->sink, &acl_child_task);

            linkPolicyCheckRoles();

            mtRouteTableAdjust(msg->addr);
            if (mt->status == MT_ST_CONNECTING || mt->status == MT_ST_PARING || mt->status == MT_ST_RECONNECTING)
            {
                mtSendRouteTable(&mt->route_table);
            }

            mtScoConnect(msg->sink);
        }
        else if (mt->status == MT_ST_WAITING_CONNECT)
        {
            MessageCancelAll(mt->app_task, EventSysMultiTalkReconnect);
            MessageCancelAll(mt->app_task, EventMultiTalkReconnect);
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
                MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(1));
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
                MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(1));
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
                MessageSendLater(mt->app_task, EventMultiTalkConnect, m, D_SEC(2));
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
                MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(1));
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
                MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(2));
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

void handleMTL2capDisconInd(CL_L2CAP_DISCONNECT_IND_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capDisconInd: status=%d\n", msg->status));
    if (msg->sink == mt->mt_device[MT_PARENT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconInd: parent\n"));
        mt->mt_device[MT_PARENT].state = MT_L2CAP_Disconnected;
        mt->mt_device[MT_PARENT].acl_sink = NULL;
        mt->status = MT_ST_LINKLOSS;
        stateManagerEnterConnectableState(FALSE);

        if (msg->status == l2cap_disconnect_link_loss)
        {
            mtBroadcastDisconnected(MT_ALL, mt->addr);

            mt->total_connected--;
            if (mt->total_connected < 1)
            {
                mt->total_connected = 1;
            }
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
        else if (msg->status == l2cap_disconnect_successful)
        {
            mtBroadcastDisconnected(MT_ALL, mt->addr);
            mt->total_connected--;
            if (mt->total_connected < 1)
            {
                mt->total_connected = 1;
            }
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
        else
        {
        }
    }
    if (msg->sink == mt->mt_device[MT_CHILD].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconInd: child\n"));
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        mt->mt_device[MT_CHILD].acl_sink = NULL;
        if (msg->status == l2cap_disconnect_link_loss && (mt->mt_device[MT_PARENT].state == MT_SYN_Connected || mt->mt_type == MT_HEAD))
        {
            mtBroadcastDisconnected(MT_ALL, mt->addr);

            mt->total_connected--;
            if (mt->total_connected < 1)
            {
                mt->total_connected = 1;
            }
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);

            mt->reconnect_count = 1;
            MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(1));
        }
        else if (msg->status == l2cap_disconnect_successful && (mt->mt_device[MT_PARENT].state == MT_SYN_Connected || mt->mt_type == MT_HEAD))
        {
            mtBroadcastDisconnected(MT_ALL, mt->addr);
            mt->total_connected--;
            if (mt->total_connected < 1)
            {
                mt->total_connected = 1;
            }
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);

            mt->reconnect_count = 2;
            MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(1));
        }
        else if ((mt->mt_device[MT_PARENT].state == MT_SYN_Connected || mt->mt_type == MT_HEAD))
        {
            mtBroadcastDisconnected(MT_ALL, mt->addr);
            mt->total_connected--;
            if (mt->total_connected < 1)
            {
                mt->total_connected = 1;
            }
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);

            mt->reconnect_count = 2;
            MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, D_SEC(1));
        }
    }
}

void handleMTL2capDisconCfm(CL_L2CAP_DISCONNECT_CFM_T *msg)
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
    }
    else if (msg->sink == mt->mt_device[MT_CHILD].acl_sink)
    {
        mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
        mt->mt_device[MT_CHILD].acl_sink = NULL;
    }
    else
    {
        MT_DEBUG(("MT: not Now link\n"));
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

    MT_DEBUG(("MT: handleMTSynConnCfm status=%d, link_typs = %d\n",
              msg->status, msg->link_type));
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

            mtSaveRouteTable(&mt->route_table, mt->mt_type, mt->index_rt);

            if (mtGetConnectDevices() < 2)
            {
                mtSendCheckTTL(1);

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
                    }
                }
            }
            else
            {
                mtSendFindTail();
            }

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

            mtSaveRouteTable(&mt->route_table, mt->mt_type, mt->index_rt);

            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            if (mtGetConnectDevices() == 2 || mt->mt_type == MT_HEAD)
            {
                stateManagerEnterConnectedState();
                mt->status = MT_ST_CONNECTED;
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

void handleMTSynDisconInd(CL_DM_SYNC_DISCONNECT_IND_T *msg)
{
    /*
        ???Audio?????????????????????
    */
    MT_DEBUG(("MT: handleMTSynDisconInd status=%d, reason = %d\n",
              msg->status, msg->reason));

    if (msg->audio_sink == mt->mt_device[MT_PARENT].audio_sink)
    {
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

    audioUpdateAudioRouting();
}

void ACLBroadcastEvent(Sink sink, uint16 event)
{
    uint8 d[3];
    d[0] = 0;
    d[1] = event >> 8;
    d[2] = event;
    ACLSend(sink, d, 3);
}

#if 0
void mtBroadcastConnected(void)
{
    uint8 d[1];
    d[0] = ACLMSG_ONE_CONNECTED;
    ACLSend(mt->mt_device[MT_PARENT].acl_sink, d, sizeof(d));
    ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, sizeof(d));
}
#endif

void mtBroadcastDisconnected(int mt_device, bdaddr my_addr)
{
    uint8 d[8];
    d[0] = ACLMSG_ONE_DISCONNECTED;
    BdaddrToArray(&my_addr, &d[1]);
    if (mt_device == MT_ALL)
    {
        ACLSend(mt->mt_device[MT_PARENT].acl_sink, d, sizeof(d));
        ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, sizeof(d));
    }
    else
    {
        ACLSend(mt->mt_device[mt_device].acl_sink, d, sizeof(d));
    }
}

void mtSendFindTail(void)
{
    uint8 d[1];
    d[0] = ACLMSG_FIND_TAIL;
    ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, sizeof(d));
}

void mtSendCheckTTL(uint8 ttl)
{
    uint8 d[2];
    MT_DEBUG(("MT: Send Check TTL\n"));
    d[0] = ACLMSG_CHECK_TTL;
    d[1] = ttl;
    ACLSend(mt->mt_device[MT_PARENT].acl_sink, d, sizeof(d));
}

void mtBroadcastConnectedCount(uint8 count)
{
    uint8 d[3 + 8 * 7], i;
    RouteTable *rt = &mt->route_table;
    MT_DEBUG(("MT: mtBroadcastConnectedCount %d\n", count));

    if (mt->mt_device[MT_CHILD].state == MT_SYN_Connected)
    {
        mtRouteTableAdjust(mt->mt_device[MT_CHILD].bt_addr);
    }

    d[0] = ACLMSG_DEVICE_COUNT;
    d[1] = count;
    d[2] = rt->count;
    for (i = 0; i < rt->count; i++)
    {
        BdaddrToArray(&rt->item[i], &d[3 + 7 * i]);
    }
    ACLSend(mt->mt_device[MT_CHILD].acl_sink, d, 3 + 7 * rt->count);
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

void ACLSend(Sink sink, const uint8_t *data, uint16 packet_size)
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
    }
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
    case ACLMSG_ONE_DISCONNECTED:
    {
        mtBroadcastDisconnected(MT_PARENT, ArrayToBdaddr(&data[1]));
        mt->total_connected--;
        MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        break;
    }
    case ACLMSG_CHECK_TTL:
    {
        if (mt->mt_device[MT_PARENT].acl_sink == NULL)
        {
            mtBroadcastConnectedCount(data[1] + 1);
            mt->total_connected = data[1] + 1;
            MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);
        }
        else
        {
            mtSendCheckTTL(data[1] + 1);
        }
    }
    break;
    default:
        break;
    }
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
    case ACLMSG_ONE_DISCONNECTED:
    {
        mtBroadcastDisconnected(MT_CHILD, ArrayToBdaddr(&data[1]));
        mt->head_addr = ArrayToBdaddr(&data[1]);
        mt->total_connected--;
        MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
    }
    break;
    case ACLMSG_FIND_TAIL:
    {
        if (mt->mt_device[MT_CHILD].acl_sink == NULL)
        {
            mtSendCheckTTL(1);
        }
        else
            mtSendFindTail();
    }
    break;
    case ACLMSG_DEVICE_COUNT:
    {
        int count = data[2];
        mt->total_connected = data[1];
        mt->route_table.count = data[2];
        for (i = 0; i < count; i++)
        {
            mt->route_table.item[i] = ArrayToBdaddr(&data[3 + 7 * i]);
            if (BdaddrIsSame(&mt->route_table.item[i], &mt->addr))
            {
                mt->index_rt = mt->connect_idx = mt->reconnect_count = i + 1;
            }
        }
        BdaddrSetZero(&mt->head_addr);
        mtBroadcastConnectedCount(data[1]);
        MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);
    }
    break;
    case ACLMSG_ROUTE_TABLE:
    {
        int i, count = data[1];
        mt->route_table.count = count;
        for (i = 0; i < count; i++)
        {
            mt->route_table.item[i] = ArrayToBdaddr(&data[2 + 7 * i]);
            if (BdaddrIsSame(&mt->route_table.item[i], &mt->addr))
            {
                mt->index_rt = mt->connect_idx = mt->reconnect_count = i + 1;
            }
        }
        mtSaveRouteTable(&mt->route_table, MT_NODE, mt->index_rt);
    }
    break;
    default:
        break;
    }
}

bool processEventMultiTalk(Task task, MessageId id, Message message)
{
    bool ret = FALSE;
    switch (id)
    {
    case EventMultiTalkEnterPair:
    {
        MT_DEBUG(("MT: Enter Pair\n"));
        mt->status = MT_ST_PARING;
        AudioPlay(AP_MULTI_TALK_PAIRING, TRUE);
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
        PowerAmpOn();
    }

    break;
    case EventMultiTalkConnect: /* enter talk connect */
        MT_DEBUG(("MT: EventMultiTalkConnect\n"));
        sinkInquirySetInquiryState(inquiry_complete);
        inquiryStop();

        if (message == NULL)
        {
            if (mt->status == MT_ST_PARING)
            {
                mt->mt_type = MT_HEAD;
            }
            if ((mt->status == MT_ST_CONNECTING || mt->status == MT_ST_RECONNECTING || mt->status == MT_ST_LINKLOSS) && mtGetConnectDevices() >= 1)
            {
                MessageSend(task, EventMultiTalkDisconnect, NULL);
                break;
            }
            if (mt->status == MT_ST_STAY_DISCONNET || mt->status == MT_ST_NOCONNECT)
            {
                MessageSend(task, EventMultiTalkReconnect, NULL);
                break;
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
                    mt->mt_type = MT_TAIL;
                }
                else
                {
                    mt->mt_type = MT_NODE;
                }
                mt->status = MT_ST_CONNECTED;
                stateManagerEnterConnectedState();
                MessageCancelAll(task, EventMultiTalkConnect);
                break;
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
            MessageCancelAll(task, EventMultiTalkConnect);
            AudioPlay(AP_MULTI_TALK_QUIT_PAIR, TRUE);
        }
        break;
    case EventMultiTalkReconnect:
        if (mt->is_2talk)
        {
            MT_DEBUG(("2T: Enter Pair\n"));
            AudioPlay(AP_TWO_TALK_PAIRING, TRUE);

            inquiryPair(inquiry_session_ag, TRUE);
            MessageCancelAll(task, EventMultiTalkConnect);
            MessageCancelAll(task, EventMultiTalkReconnect);
            MessageCancelAll(task, EventSysMultiTalkReconnect);
        }
        else
        {
            MT_DEBUG(("MT: EventMultiTalkReconnect\n"));

            if (RouteTableIsNotSaved(&mt->route_table))
            {
                break;
            }
            if (mt->status != MT_ST_NOCONNECT && mt->status != MT_ST_STAY_DISCONNET)
            {
                MessageSend(task, EventMultiTalkDisconnect, NULL);
                break;
            }
            AudioPlay(AP_MULTI_TALK_ON, TRUE);

            if (mt->mt_type == MT_HEAD)
            {
                MessageSend(task, EventSysMultiTalkReconnect, NULL);
            }
            else
            {
                mt->status = MT_ST_RECONNECTING;
                /* 先搜索再连接 */
                stateManagerEnterConnectableState(FALSE);
                if(mtGetConnectDevices() == 0)
                {
                    sinkEnableDiscoverable();
                }
            }
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
                mt->mt_type = MT_TAIL;
            }
            else
            {
                mt->mt_type = MT_NODE;
            }
            mt->status = MT_ST_CONNECTED;
            stateManagerEnterConnectedState();
            break;
        }
        if (!mtConnect(index))
        {
            mt->reconnect_count++;
            MessageSendLater(mt->app_task, EventSysMultiTalkReconnect, NULL, 300);
        }
    }
    break;
    case EventMultiTalkDisconnect: /* quit talk */
        MT_DEBUG(("MT: EventMultiTalkDisconnect\n"));
        mt->status = MT_ST_STAY_DISCONNET;
        mt->total_connected = 1;
        stateManagerEnterConnectableState(FALSE);
        mtDisconnect();
        MessageCancelAll(task, EventMultiTalkConnect);
        MessageCancelAll(task, EventMultiTalkReconnect);
        MessageCancelAll(task, EventSysMultiTalkReconnect);
        AudioPlay(AP_MULTI_TALK_OFF, TRUE);
        PowerAmpOff();
        break;
    case EventSysMultiTalkInquiryDevices:
    {
        if(mt->status == MT_ST_PARING)
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
        if(mt->status == MT_ST_RECONNECTING)
        {
            uint8 count = *(uint8 *)message;
            uint8 idx = 0;
            inquiry_result_t *result = sinkinquiryGetInquiryResults();
            if (result)
            {
                for (idx = 0; idx < count; idx++)
                {
                    /*连接比自己地址更小的*/
                    if(BdaddrCompare(&mt->addr, &result[idx].bd_addr))
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
        if (mt->mt_type == MT_TAIL && mt->total_connected < mt->route_table.count)
        {
            MessageSendLater(task, EventSysMultiTalkReconnect, NULL, D_SEC(1));
        }
        ret = TRUE;
        break;
    case EventSysMultiTalkCurrentDevices:
    {
        AudioPlay(AP_MULTI_TALK_1_PERSON + mt->total_connected - 1, TRUE);
    }
    break;
    case EventSysMultiTalkWatiConnect:
        if(mt->status == MT_ST_WAITING_CONNECT)
        {
            if(mt->mt_device[MT_CHILD].state == MT_L2CAP_Connecting)
            {
                mt->mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
                BdaddrSetZero(&mt->mt_device[MT_CHILD].bt_addr);
            }
        }
    break;
    case Event2TalkPrepareEnter:
        MT_DEBUG(("2T: Prepre Pair\n"));
        mt->is_2talk = 1;
        break;
    default:
        break;
    }
    return ret;
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
        connect_parameters->volume = 0xf;
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

int sinkEnableDebug(void)
{
    return enableDebug;
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
