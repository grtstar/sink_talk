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
#include "csr_common_example_plugin.h"

#ifdef ENABLE_MT_DEBUG
#define MT_DEBUG(x) DEBUG(x)
#define MT_DEBUG_ADDR(x) DEBUG(("bdaddr = %x:%x:%lx\n", x.nap, x.uap, x.lap))
#else
#define MT_DEBUG(X)
#define MT_DEBUG_ADDR(x)
#endif
/*
1. ??????ACL ????
2. ???? SCO ????
3. ???? ACL ????
4. ???? SCO ????
5. ?mic ?????sco out
6. ?sco in ?????speaker
7. ??? ACL ??????route map
8. ????????????????connectable
9. ????????route map ????????????
*/


extern ExamplePluginTaskdata csr_cvsd_8k_1mic_plugin;
extern ExamplePluginTaskdata csr_sbc_1mic_plugin;
extern ExamplePluginTaskdata csr_cvsd_8k_2mic_plugin;
extern ExamplePluginTaskdata csr_sbc_2mic_plugin;

/* Example Plugins */
#define CVSD1MIC_EXAMPLE    (TaskData *)&csr_cvsd_8k_1mic_plugin
#define SBC1MIC_EXAMPLE     (TaskData *)&csr_sbc_1mic_plugin
#define CVSD2MIC_EXAMPLE    (TaskData *)&csr_cvsd_8k_2mic_plugin
#define SBC2MIC_EXAMPLE     (TaskData *)&csr_sbc_2mic_plugin


#define MULTITALK_PSM 0x0055
#define MULTITALK_PROTOCOL_PSM 0x0057

typedef struct RouteTable
{
    uint8 count;
    uint8 nop;
    bdaddr  item[8];
} RouteTable;

static void RouteTableAddChild(RouteTable *rt, uint8 ttl, bdaddr addr);
static bdaddr RouteTableGet(RouteTable *rt, uint8 index);
static void BdaddrToArray(bdaddr *addr, uint8 *array);
static bdaddr ArrayToBdaddr(const uint8 *array);
static void mtSaveRouteTable(RouteTable *rt);
static void mtLoadRouteTable(RouteTable *rt);
static void mtClearRouteTable(RouteTable *rt);
static bool RouteTableCanWrite(RouteTable *rt);
static bool RouteTableIsContain(RouteTable *rt, bdaddr *addr);

static void ACLSendPositionToChild(Sink sink, uint8 position);
static void ACLSendAddrToParent(Sink sink, uint8 postion, bdaddr addr);
static void ACLSendRouteTable(Sink sink, RouteTable * table);
static void ACLSend(Sink sink, const uint8_t *data, uint16 packet_size);
static void ACLBroadcastEvent(Sink sink, uint16 event);

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

bdaddr ArrayToBdaddr(const uint8* data)
{
    bdaddr addr;
    addr.lap = (((uint32)data[0])<<24) | (((uint32)data[1])<<16) | (((uint32)data[2])<<8) | data[3];
    addr.uap = data[4];
    addr.nap = (((uint16)data[5])<<8) | data[6];
    return addr;
}

bool RouteTableIsContain(RouteTable *rt, bdaddr *addr)
{
    uint8 index = 0;
    for(index = 0; index < rt->count; index++)
    {
        if(BdaddrIsSame(&(rt->item[index]), addr))
        {
            return TRUE;
        }
    }
    return FALSE;
}

bool RouteTableCanWrite(RouteTable *rt)
{
    return rt->nop != 0x55;
}

void RouteTableAddChild(RouteTable *rt, uint8 ttl, bdaddr addr)
{
    MT_DEBUG(("MT: Add Child %d ", ttl));
    MT_DEBUG_ADDR(addr);
    if(ttl >= 8)
    {
        MT_DEBUG(("MT: ttl error\n"));
        return;
    }
    if(RouteTableIsContain(rt, &addr))
    {
        return;
    }

    rt->item[ttl] = addr;
    if(rt->count < ttl+1)
    {
        rt->count = ttl + 1;
    }
}

bdaddr RouteTableGet(RouteTable *rt, uint8 index)
{
    bdaddr addr;
    BdaddrSetZero(&addr);
    if(index >= rt->count)
    {
        return addr;
    }
    return rt->item[index];
}

void mtSaveRouteTable(RouteTable *rt)
{
    rt->nop = 0x55;
    ConfigStore(99, rt, sizeof(RouteTable));
}

void mtLoadRouteTable(RouteTable *rt)
{
    ConfigRetrieve(99, rt, sizeof(RouteTable));
    if(rt->nop != 0x55)
    {
        memset(rt, 0, sizeof(RouteTable));
    }
}

void mtClearRouteTable(RouteTable *rt)
{
    memset(rt, 0, sizeof(RouteTable));
    ConfigStore(99, rt, sizeof(RouteTable));
}


enum
{
    MT_L2CAP_Disconnected,
    MT_L2CAP_Connecting,
    MT_L2CAP_Connected,
    MT_SYN_Connecting,
    MT_SYN_Connected,
    MT_SYN_Disconnecting,
    MT_SYN_Disconnected,
    MT_L2CAP_Disconnecting
};

enum
{
    MT_PARENT,
    MT_CHILD
};

enum
{
    MT_ST_NOCONNECT,
    MT_ST_PARING,
    MT_ST_CONNECTING,
    MT_ST_CONNECTED,
    MT_ST_LINKLOSS,
    MT_ST_RECONNECTING
};

typedef enum MTType
{
    MT_HEAD,
    MT_NODE
}MTType;

/*
ACL ��??
TYPE DATA
0   EVENT
1   ROUTE-DOWN   TOTAL   PARENT_ADDR  
2   ROUTE-UP     TOTAL   CHILD_ADDR
*/

typedef enum ACLMsg
{
    ACLMSG_EVENT,
    ACLMSG_POSITION,
    ACLMSG_CHILD_ADDR,
    ACLMSG_DEVICE_COUNT,
    ACLMSG_ROUTE_TABLE
}ACLMsg;

typedef struct MTDevice
{
    int state;
    bdaddr bt_addr;
    Sink acl_sink;
    Sink audio_sink;
    sync_link_type  link_type;
} MTDevice;

typedef struct MTData
{
    Task app_task;
    TaskData mt_task;
    MTDevice mt_device[2];
    hfp_common_plugin_params_t  plugin_params;
    MTType  mt_type;
    uint8   total_connected;
    RouteTable route_table;
    uint8 position;
    uint8 reconnect_count;
    bdaddr  addr;
    uint8 status;
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

static MTData mt_data;

TaskData acl_parent_task = {acl_parent_handler};
TaskData acl_child_task = {acl_child_handler};

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
        handleMTL2capConnectCfm((CL_L2CAP_CONNECT_CFM_T *) message);
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
            MessageMoreData *msg = (MessageMoreData*)message;
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
            MessageMoreData *msg = (MessageMoreData*)message;
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
    mt_data.app_task = task;
    mt_data.mt_task.handler = mt_handler;
    mt_data.mt_type = MT_HEAD;
    mt_data.total_connected = 1;
    mt_data.position = 0;
    PsFullRetrieve(1, temp, 4);
    mt_data.addr.nap = temp[3];
    mt_data.addr.uap = temp[2];
    mt_data.addr.lap = ((uint32)temp[0])<<16 | temp[1];
    mt_data.route_table.item[0] = mt_data.addr;
    mt_data.status = MT_ST_NOCONNECT;

    mtLoadRouteTable(&mt_data.route_table);

    if(!RouteTableCanWrite(&mt_data.route_table))
    {
        bdaddr addr = RouteTableGet(&mt_data.route_table, 0);
        if(BdaddrIsSame(&addr , &mt_data.addr))
        {
            mt_data.mt_type = MT_HEAD;
        }
        else
        {
            mt_data.mt_type = MT_NODE;
        }
    }

    ConnectionL2capRegisterRequest(&mt_data.mt_task, MULTITALK_PSM, 0);
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
    bdaddr bd_addr;
    bd_addr.uap = 0;
    bd_addr.lap = 0;
    bd_addr.nap = 0;

    /* 
        ???????????? parent 
    */
    if(!BdaddrIsZero(&mt_data.mt_device[MT_PARENT].bt_addr))
    {
        bd_addr = (mt_data.mt_device[MT_PARENT].bt_addr);
    }

    MT_DEBUG(("MT: inquiryConnectTalkFirst\n"));
    if(inquiryConnectTalkFirst(bd_addr))
    {
        return TRUE;
    }
    return FALSE;
}

void mtDisconnect(void)
{
    mtScoDisconnect(MT_PARENT);
    mtScoDisconnect(MT_CHILD);
    mtACLDisconnect(MT_PARENT);
    mtACLDisconnect(MT_CHILD);
}

void mtConnect(bdaddr *bd_addr)
{
    mt_data.mt_device[MT_CHILD].bt_addr = *bd_addr;
    mt_data.mt_device[MT_CHILD].state = MT_L2CAP_Connecting;
    mtACLConnect(bd_addr);
}

void mtReconnect(void)
{
}

void mtACLConnect(bdaddr *bd_addr)
{
    MT_DEBUG(("MT: mtACLConnect to "));
    MT_DEBUG_ADDR((*bd_addr));
    ConnectionSetPageTimeout(16384);
    ConnectionL2capConnectRequest(&mt_data.mt_task, bd_addr, MULTITALK_PSM, MULTITALK_PSM, sizeof(syn_conftab), (uint16 *)syn_conftab);
}

void mtACLDisconnect(int device)
{
    if(mt_data.mt_device[device].acl_sink != NULL)
    {
        ConnectionL2capDisconnectRequest(&mt_data.mt_task, mt_data.mt_device[device].acl_sink);
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
    ConnectionSyncConnectRequest(&mt_data.mt_task, sink_acl, &config_params);
}

void mtScoDisconnect(int device)
{
    if(mt_data.mt_device[device].acl_sink != NULL)
    {
        ConnectionSyncDisconnect(mt_data.mt_device[device].acl_sink, hci_error_oetc_user);
    }
}

void handleMTL2capRegisterCfm(CL_L2CAP_REGISTER_CFM_T *msg)
{
    MT_DEBUG(("handleMTL2capRegisterCfm status=%d, prim=0x%x\n", msg->status, msg->psm));
    if (msg->psm == MULTITALK_PSM)
    {
        ConnectionSyncRegister(&mt_data.mt_task);
    }
}

void handleMTL2capConnectInd(CL_L2CAP_CONNECT_IND_T *msg)
{
     bool can_recv = FALSE;
     
    /* 
        ??????????????????????????????��?????????????
    */
    MT_DEBUG(("MT: handleMTL2capConnectInd: addr=%x:%x:%lx, psm=0x%x, identifier=0x%x, connID=0x%x\n",
              msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap, msg->psm,
              msg->identifier, msg->connection_id));

   
    if (mt_data.mt_device[MT_PARENT].state == MT_L2CAP_Disconnected)
    {
        /* ??????????��?? */
        MT_DEBUG(("MT: handleMTL2capConnectInd parant is disconnect\n"));
        if(RouteTableCanWrite(&mt_data.route_table) || RouteTableIsContain(&mt_data.route_table, &msg->bd_addr))
        {
            MT_DEBUG(("MT: handleMTL2capConnectInd peer is my cai\n"))
            mt_data.mt_device[MT_PARENT].state = MT_L2CAP_Connecting;
            mt_data.mt_device[MT_PARENT].bt_addr = msg->bd_addr;
            mt_data.mt_type = MT_NODE;
            can_recv = TRUE;
        }
    }
    if (can_recv)
    {
        ConnectionL2capConnectResponse(&mt_data.mt_task, TRUE, MULTITALK_PSM, msg->connection_id,
                                       msg->identifier, sizeof(syn_conftab), (uint16 *)syn_conftab);
    }
    else
    {
        ConnectionL2capConnectResponse(&mt_data.mt_task, FALSE, MULTITALK_PSM, msg->connection_id,
                                       msg->identifier, sizeof(syn_conftab), (uint16 *)syn_conftab);
    }
}

void handleMTL2capConnectCfm(CL_L2CAP_CONNECT_CFM_T *msg)
{
    /*
        ??????????????????��??????????????
        ???????��???????????????sco ?????
    */
    MT_DEBUG(("MT: handleMTL2capConnectCfm: addr=%x:%x:%lx, status=%d\n",
              msg->addr.nap, msg->addr.uap, msg->addr.lap, msg->status));
    if (BdaddrIsSame(&msg->addr, &mt_data.mt_device[MT_PARENT].bt_addr))
    {
        if(msg->status == l2cap_connect_success)
        {
            mt_data.mt_device[MT_PARENT].state = MT_L2CAP_Connected;
            mt_data.mt_device[MT_PARENT].acl_sink = msg->sink;
    
            MessageSinkTask( msg->sink, &acl_parent_task);
        }
    }
    else if (BdaddrIsSame(&msg->addr, &mt_data.mt_device[MT_CHILD].bt_addr))
    {
        if(msg->status == l2cap_connect_success)
        {
            mt_data.mt_device[MT_CHILD].state = MT_L2CAP_Connected;
            mt_data.mt_device[MT_CHILD].acl_sink = msg->sink;
            MessageSinkTask( msg->sink, &acl_child_task);
            mtScoConnect(msg->sink);
        } 
        if(msg->status == l2cap_connect_failed)   /* 128 */
        {
            /*
                ????��????????????????????
            */
           if(mt_data.status == MT_ST_RECONNECTING)
           {
               mt_data.reconnect_count++;
               MessageSendLater(mt_data.app_task, EventMultiTalkReconnect, NULL, D_SEC(1));
           }
           if(mt_data.status == MT_ST_CONNECTING)
           {
                uint8* msg =  PanicUnlessMalloc(sizeof(uint8));
                sinkInquirySetInquiryState(inquiry_idle);
                MessageSendLater(mt_data.app_task, EventMultiTalkConnect, msg, D_SEC(1));
           }
        }
        if(msg->status == l2cap_connect_failed_remote_reject)   /* 130 */
        {
           if(mt_data.status == MT_ST_RECONNECTING)
           {
               mt_data.reconnect_count++;
               MessageSendLater(mt_data.app_task, EventMultiTalkReconnect, NULL, D_SEC(1));
           }
            if(mt_data.status == MT_ST_CONNECTING)
           {
                uint8* msg =  PanicUnlessMalloc(sizeof(uint8));
                sinkInquirySetInquiryState(inquiry_idle);
                MessageSendLater(mt_data.app_task, EventMultiTalkConnect, msg, D_SEC(1));
           }
        }
        if(msg->status == l2cap_connect_failed_security)    /* 132 */
        {
            deviceManagerRemoveDevice(&msg->addr);
            MessageSendLater(mt_data.app_task, EventMultiTalkReconnect, NULL, D_SEC(1));
        }
        if(msg->status == l2cap_connect_timeout)    /* 136 */
        {
            if(mt_data.status == MT_ST_CONNECTING)
           {
                uint8* msg =  PanicUnlessMalloc(sizeof(uint8));
                sinkInquirySetInquiryState(inquiry_idle);
                MessageSendLater(mt_data.app_task, EventMultiTalkConnect, msg, D_SEC(1));
           }
        }
        if(msg->status == l2cap_connect_error)    /* 137 */
        {
            if(mt_data.status == MT_ST_RECONNECTING)
            {
                mt_data.reconnect_count++;
                MessageSendLater(mt_data.app_task, EventMultiTalkReconnect, NULL, D_SEC(1));
            }
            if(mt_data.status == MT_ST_CONNECTING)
           {
                uint8* msg =  PanicUnlessMalloc(sizeof(uint8));
                sinkInquirySetInquiryState(inquiry_idle);
                MessageSendLater(mt_data.app_task, EventMultiTalkConnect, msg, D_SEC(1));
           }
        }
    }
}

void handleMTL2capDisconInd(CL_L2CAP_DISCONNECT_IND_T *msg)
{
    MT_DEBUG(("MT: handleMTL2capDisconInd: status=%d\n", msg->status));
    if(msg->sink == mt_data.mt_device[MT_PARENT].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconInd: parent\n"));
        mt_data.mt_device[MT_PARENT].state = MT_L2CAP_Disconnected;
        mt_data.mt_device[MT_CHILD].acl_sink = NULL;
        stateManagerEnterConnectableState(FALSE);
        mt_data.status = MT_ST_LINKLOSS;
    }
    if(msg->sink == mt_data.mt_device[MT_CHILD].acl_sink)
    {
        MT_DEBUG(("MT: handleMTL2capDisconInd: child\n"));
        mt_data.mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        mt_data.mt_device[MT_CHILD].acl_sink = NULL;
        if(msg->status == l2cap_disconnect_link_loss)
        {
            mt_data.reconnect_count = 1;
            MessageSendLater(mt_data.app_task, EventMultiTalkReconnect, NULL, D_SEC(1));
        }
        else
        {
            mt_data.reconnect_count = 2;
            MessageSendLater(mt_data.app_task, EventMultiTalkReconnect, NULL, D_SEC(1));
        }
    }
}

void handleMTL2capDisconCfm(CL_L2CAP_DISCONNECT_CFM_T *msg)
{
    /*
        ??? parent ??????????????????????????
        ??? child ?????????????��???????????m
        ??????? child??N ?????????child->child
    */
   MT_DEBUG(("MT: handleMTL2capDisconCfm: status=%d\n", msg->status));
    if(msg->sink == mt_data.mt_device[MT_PARENT].acl_sink)
    {
        mt_data.mt_device[MT_PARENT].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt_data.mt_device[MT_PARENT].bt_addr); 
        mt_data.mt_device[MT_PARENT].acl_sink = NULL;
    }
    if(msg->sink == mt_data.mt_device[MT_CHILD].acl_sink)
    {
        mt_data.mt_device[MT_CHILD].state = MT_L2CAP_Disconnected;
        BdaddrSetZero(&mt_data.mt_device[MT_CHILD].bt_addr); 
        mt_data.mt_device[MT_CHILD].acl_sink = NULL;
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

    if(BdaddrIsSame(&msg->bd_addr, &mt_data.mt_device[MT_PARENT].bt_addr) && mt_data.mt_device[MT_PARENT].state == MT_L2CAP_Connected)
    {
        ConnectionSyncConnectResponse(&mt_data.mt_task, &msg->bd_addr, TRUE, &config_params);
        mt_data.mt_device[MT_PARENT].state = MT_SYN_Connecting;
    }
    else
    {
        ConnectionSyncConnectResponse(&mt_data.mt_task, &msg->bd_addr, FALSE, &config_params);
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

    if (BdaddrIsSame(&msg->bd_addr, &mt_data.mt_device[MT_PARENT].bt_addr))
    {
        switch (msg->status)
        {
        case hci_success:
            mt_data.mt_device[MT_PARENT].state = MT_SYN_Connected;
            mt_data.mt_device[MT_PARENT].audio_sink = msg->audio_sink;
            mt_data.mt_device[MT_PARENT].link_type = msg->link_type;
            
            audioUpdateAudioRouting();

            if(mt_data.status == MT_ST_PARING)
            {
                uint8* msg =  PanicUnlessMalloc(sizeof(uint8));
                MessageSendLater(mt_data.app_task, EventMultiTalkConnect, msg, D_SEC(1));
            }
            if(mt_data.status == MT_ST_RECONNECTING || mt_data.status == MT_ST_LINKLOSS)
            {
                if(mt_data.total_connected < mt_data.route_table.count)
                {
                    mt_data.reconnect_count = 1;
                    MessageSendLater(mt_data.app_task, EventMultiTalkReconnect, NULL, D_SEC(1));
                }
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
        default:
            break;
        }
    }
    else if (BdaddrIsSame(&msg->bd_addr, &mt_data.mt_device[MT_CHILD].bt_addr))
    {
        switch (msg->status)
        {
        case hci_success:
            mt_data.mt_device[MT_CHILD].state = MT_SYN_Connected;
            mt_data.mt_device[MT_CHILD].audio_sink = msg->audio_sink;
            mt_data.mt_device[MT_CHILD].link_type = msg->link_type;
           
            audioUpdateAudioRouting();

            ACLSendPositionToChild(mt_data.mt_device[MT_CHILD].acl_sink, mt_data.position + 1);            

            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            /*if(mtGetConnectDevices() == 2)*/
            stateManagerEnterConnectedState();       
            mt_data.status = MT_ST_CONNECTED;     

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
        default:
            break;
        }
    }
}

void handleMTSynDisconInd(CL_DM_SYNC_DISCONNECT_IND_T *msg)
{
    /*
        ???Audio?????????????????��??
    */
    MT_DEBUG(("MT: handleMTSynDisconInd status=%d, reason = %d\n",
              msg->status, msg->reason));

   
    if (msg->audio_sink == mt_data.mt_device[MT_PARENT].audio_sink)
    {
        mt_data.mt_device[MT_PARENT].state = MT_SYN_Disconnected;
    }
    else if (msg->audio_sink == mt_data.mt_device[MT_CHILD].audio_sink)
    {
        mt_data.mt_device[MT_CHILD].state = MT_SYN_Disconnected;
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

void ACLSendRouteTable(Sink sink, RouteTable * table)
{
    uint8 d[2 + 7*8];
    uint8 i = 0;
    d[0] = ACLMSG_ROUTE_TABLE;
    d[1] = table->count;
    for(i=0; i<table->count; i++)
    {
        BdaddrToArray(&table->item[i], &d[2+i*7]);
    }
    ACLSend(sink, d, 2 + 7*table->count);
}

void ACLSendPositionToChild(Sink sink, uint8 position)
{
    uint8 d[2];
    d[0] = ACLMSG_POSITION;
    d[1] = position;
    ACLSend(sink, d, sizeof(d));
}

void ACLSendAddrToParent(Sink sink, uint8 postion, bdaddr addr)
{
    uint8 d[9];
 
    MT_DEBUG(("MT: ACLSendAddrToParent\n"));
    d[0] = ACLMSG_CHILD_ADDR;
    d[1] = postion;
    
    BdaddrToArray(&addr, &d[2]);

    ACLSend(sink, d, sizeof(d));
}

void ACLSend(Sink sink, const uint8_t *data, uint16 packet_size)
{
    uint16 sink_offset=0;
    if (SinkIsValid(sink))
    {
        if((sink_offset=SinkClaim(sink, packet_size)) != 0xFFFF)
        {
            memmove(SinkMap(sink)+sink_offset, data, packet_size);
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
    for(i=0; i<size; i++)
    {
        MT_DEBUG(("%02X ", data[i]));
        if((i & 0x7) == 0x7)
        {
            MT_DEBUG(("\n"));
        }
    }
    MT_DEBUG(("\n"));
    switch (data[0])
    {
    case ACLMSG_EVENT:
        /* code */
        msg = data[1]<<8 | data[2]; 
        if(msg == EventMultiTalkEndPair)
        {
            uint8 *message = PanicUnlessMalloc(sizeof(uint8));
            ACLBroadcastEvent(mt_data.mt_device[MT_PARENT].acl_sink, EventMultiTalkEndPair);
            MessageSend(mt_data.app_task, EventMultiTalkEndPair, message);
        }
        break;
    case ACLMSG_CHILD_ADDR:
    {
        bdaddr addr = ArrayToBdaddr(&data[2]);
        if(mt_data.mt_type != MT_HEAD)
        {
            ACLSendAddrToParent(mt_data.mt_device[MT_PARENT].acl_sink, data[1], addr);
        }
        else
        {
            if(RouteTableCanWrite(&mt_data.route_table))
            {
                RouteTableAddChild(&mt_data.route_table, data[1]-1, addr);
            }
        }
        mt_data.total_connected = data[1];
        if(data[1] >=2 && data[1]<=8)
        {
            AudioPromptPlay((TaskData *)&csr_voice_prompts_plugin, data[1]+5, TRUE, FALSE);
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
    for(i=0; i<size; i++)
    {
        MT_DEBUG(("%02X ", data[i]));
        if((i & 0x7) == 0x7)
        {
            MT_DEBUG(("\n"));
        }
    }
    MT_DEBUG(("\n"));
    switch (data[0])
    {
    case ACLMSG_EVENT:
        /* code */
        msg = data[1]<<8 | data[2]; 
        if(msg == EventMultiTalkEndPair)
        {
            uint8 *message = PanicUnlessMalloc(sizeof(uint8));
            ACLBroadcastEvent(mt_data.mt_device[MT_CHILD].acl_sink, EventMultiTalkEndPair);
            MessageSend(mt_data.app_task, EventMultiTalkEndPair, message);
        }
        break;
    case ACLMSG_POSITION:
        if(mt_data.position == 0)
        {
            mt_data.position = data[1];
            mt_data.total_connected = mt_data.position + 1;
        }
        else
        {
            /* 
                0 1 2 3 4 5 6 7
                0 1 2 5(3) 6 7
            */
           mt_data.total_connected -= mt_data.position - data[1];
        }

        ACLSendPositionToChild(mt_data.mt_device[MT_CHILD].acl_sink, mt_data.position + 1);
        
        if(mt_data.total_connected >= 2 && mt_data.total_connected <= 8)
        {
            AudioPromptPlay((TaskData *)&csr_voice_prompts_plugin, mt_data.total_connected+5, TRUE, FALSE);
        }
            
        ACLSendAddrToParent(mt_data.mt_device[MT_PARENT].acl_sink,  mt_data.total_connected, mt_data.addr);
        break;
    case ACLMSG_ROUTE_TABLE:
        {
            uint8 i = 0;
            mt_data.route_table.count = data[1];
            for(i=0; i<data[1]; i++)
            {
                mt_data.route_table.item[i] = ArrayToBdaddr(&data[2+7*i]);
            }
            mtSaveRouteTable(&mt_data.route_table);

            ACLSendRouteTable(mt_data.mt_device[MT_CHILD].acl_sink, &mt_data.route_table);
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
        MT_DEBUG(("MT: Enter Pair\n"));
        mt_data.status = MT_ST_PARING;
        AudioPromptPlay((TaskData *) &csr_voice_prompts_plugin, 5, TRUE, FALSE);
        inquiryPair( inquiry_session_normal, TRUE );
        mtDisconnect();
        mtClearRouteTable(&mt_data.route_table);
        mt_data.total_connected = 1;
        break;  
    case EventMultiTalkConnect:         /* enter talk connect */
        MT_DEBUG(("MT: EventMultiTalkConnect\n"));
        if(message == NULL)
        {
            if(mt_data.status == MT_ST_CONNECTED || (mt_data.status == MT_ST_PARING && mtGetConnectDevices() >= 1))
            {
                MessageSend(task, EventMultiTalkEndPair, NULL);
                break;
            }
            if(mt_data.status == MT_ST_NOCONNECT && RouteTableCanWrite(&mt_data.route_table))
            {
                mt_data.status = MT_ST_RECONNECTING;
                if(mt_data.mt_type == MT_HEAD)
                {
                    MessageSend(task, EventMultiTalkReconnect, NULL);
                }
                break;
            }
            
        }
        mt_data.status = MT_ST_CONNECTING;
        if(mtRssiConnect())
        {
            AudioPromptPlay((TaskData *) &csr_voice_prompts_plugin, 6, TRUE, FALSE);
            MessageCancelAll(task, EventMultiTalkConnect);
            ret = TRUE;
        }
        else
        {
            uint8* msg =  PanicUnlessMalloc(sizeof(uint8));
            MessageSendLater(mt_data.app_task, EventMultiTalkConnect, msg, D_SEC(3));
        }
        break;
    case EventMultiTalkEndPair:         /* talk pair finish */
        MT_DEBUG(("MT: EventMultiTalkEndPair\n"));
        if(mtGetConnectDevices() >= 1)
        {
            mt_data.status = MT_ST_CONNECTED;

            
            sinkInquirySetInquiryState(inquiry_complete);
            inquiryStop();
            MessageCancelAll(task, EventMultiTalkConnect);
            
            if(mt_data.position == mt_data.total_connected - 1)
            {
                stateManagerEnterConnectableState(FALSE);
            }
            else
            {
                stateManagerEnterConnectedState();
            }

            if(RouteTableCanWrite(&mt_data.route_table))
            {
                if(message == NULL)
                {
                    ACLBroadcastEvent(mt_data.mt_device[MT_PARENT].acl_sink, EventMultiTalkEndPair);
                    ACLBroadcastEvent(mt_data.mt_device[MT_CHILD].acl_sink, EventMultiTalkEndPair);
                }
                MessageSend(mt_data.app_task, EventMultiTalk2DevicePairFinish+mt_data.total_connected-2, NULL);
                if(mt_data.mt_type == MT_HEAD)
                {
                    mtSaveRouteTable(&mt_data.route_table);
                    ACLSendRouteTable(mt_data.mt_device[MT_CHILD].acl_sink, &mt_data.route_table);
                }
            }
            
  
        } 
        break;
    case EventMultiTalkReconnect:
        {
            uint8 index = (mt_data.reconnect_count + mt_data.position)  % mt_data.route_table.count;            
            bdaddr addr = RouteTableGet(&mt_data.route_table, index);
            MT_DEBUG(("EventMultiTalkReconnect to index:%d\n", index));
            mt_data.status = MT_ST_RECONNECTING;
            if(BdaddrIsZero(&addr))
            {

            }
            else
            {
                mtConnect(&addr);
            }
        }
    break;
    case EventMultiTalkDisconnect:         /* quit talk */
        mtDisconnect();
        break;
    case EventMultiTalk2DevicePaired:
    case EventMultiTalk3DevicePaired:
    case EventMultiTalk4DevicePaired:
    case EventMultiTalk5DevicePaired:
    case EventMultiTalk6DevicePaired:
    case EventMultiTalk7DevicePaired:
    case EventMultiTalk8DevicePaired:
        /* AudioPromptPlayEvent(id, TRUE); */
        AudioPromptPlay((TaskData *) &csr_voice_prompts_plugin, mt_data.total_connected-2 + 7, TRUE, FALSE);
        ret = TRUE;
        break;
    case EventMultiTalk2DevicePairFinish:
    case EventMultiTalk3DevicePairFinish:
    case EventMultiTalk4DevicePairFinish:
    case EventMultiTalk5DevicePairFinish:
    case EventMultiTalk6DevicePairFinish:
    case EventMultiTalk7DevicePairFinish:
    case EventMultiTalk8DevicePairFinish:
        /* AudioPromptPlayEvent(id, TRUE); */
        AudioPromptPlay((TaskData *) &csr_voice_prompts_plugin, mt_data.total_connected-2 + 14, TRUE, FALSE);
        ret = TRUE;
        break;
    default:
        break;
    }
    return ret;
}


bool mtVoicePopulateConnectParameters(audio_connect_parameters *connect_parameters)
{
    Sink audio_sink = NULL;
    if(mt_data.mt_device[MT_PARENT].audio_sink != NULL)
    {
        audio_sink = mt_data.mt_device[MT_PARENT].audio_sink;
        MT_DEBUG(("MT: PopulateConnectParameters Parent Sink\n"));
    }
    else if(mt_data.mt_device[MT_CHILD].audio_sink != NULL)
    {
        audio_sink = mt_data.mt_device[MT_CHILD].audio_sink;
        MT_DEBUG(("MT: PopulateConnectParameters Child Sink\n"));
    }
    mt_data.plugin_params = *sinkHfpDataGetHfpPluginParams();
    if(mt_data.mt_device[MT_PARENT].audio_sink != NULL && mt_data.mt_device[MT_CHILD].audio_sink != NULL)
    {
        mt_data.plugin_params.usb_params.usb_sink = mt_data.mt_device[MT_PARENT].audio_sink;
        audio_sink = mt_data.mt_device[MT_CHILD].audio_sink;
        MT_DEBUG(("MT: PopulateConnectParameters Tow Sink\n"));
    }
    else
    {
         mt_data.plugin_params.usb_params.usb_sink = NULL;
    }
    if(audio_sink != NULL)
    {
        connect_parameters->app_task = mt_data.app_task;
        connect_parameters->audio_plugin = CVSD1MIC_EXAMPLE;
        connect_parameters->audio_sink = audio_sink;
        connect_parameters->features = sinkAudioGetPluginFeatures();
        connect_parameters->mode = AUDIO_MODE_CONNECTED;
        connect_parameters->params = &mt_data.plugin_params;
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
    uint16 sink_all = (uint16)mt_data.mt_device[MT_PARENT].audio_sink + (uint16)mt_data.mt_device[MT_CHILD].audio_sink;
    return (Sink) sink_all;
}

bool isMTVoiceSink(Sink sink)
{
    uint16 sink_all = (uint16)mt_data.mt_device[MT_PARENT].audio_sink + (uint16)mt_data.mt_device[MT_CHILD].audio_sink;
    if(sink == (Sink)sink_all)
    {
        return TRUE;
    }
    return FALSE;
}

int mtGetConnectDevices(void)
{
    int count = (mt_data.mt_device[MT_PARENT].state >= MT_L2CAP_Connected) + (mt_data.mt_device[MT_CHILD].state >= MT_L2CAP_Connected);
    MT_DEBUG(("MT: Connected Device %d\n", count));    
    return count;
}

bool mtCanPair(void)
{
    if(mt_data.status == MT_ST_RECONNECTING || mt_data.status == MT_ST_LINKLOSS)
    {
        return TRUE;
    }
    return FALSE;
}

