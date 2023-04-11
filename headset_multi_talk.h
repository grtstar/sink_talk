#ifndef _HEADSET_MULTI_TALK_
#define _HEADSET_MULTI_TALK_

#include "sink_volume.h"

enum
{
    MT_LEFT,
    MT_RIGHT
};

void mtInit(Task task);
bool mtEnterPairing(void);
void mtInquiry(void);
void mtACLConnect(bdaddr *bd_addr, uint16 psm);
void mtACLDisconnect(int device);
void mtScoConnect(Sink sink);
void mtScoDisconnect(int device);

bool mtConnect(bdaddr *bd_addr);
void mtReconnect(void);
void mtDisconnect(void);

void handleMTL2capRegisterCfm(CL_L2CAP_REGISTER_CFM_T *msg);
void handleMTL2capConnectInd(CL_L2CAP_CONNECT_IND_T *msg);
void handleMTL2capConnectCfm(CL_L2CAP_CONNECT_CFM_T *msg);
void handleMTL2capDisconInd(CL_L2CAP_DISCONNECT_IND_T *msg);
void handleMTL2capDisconCfm(CL_L2CAP_DISCONNECT_CFM_T *msg);
void handleMTSynRegisterCfm(CL_DM_SYNC_REGISTER_CFM_T *msg);
void handleMTSynConnInd(CL_DM_SYNC_CONNECT_IND_T *msg);
void handleMTSynConnCfm(CL_DM_SYNC_CONNECT_CFM_T *msg);
void handleMTSynDisconInd(CL_DM_SYNC_DISCONNECT_IND_T *msg);
void mt_handler(Task task, MessageId id, Message message);

void acl_parent_handler(Task task, MessageId id, Message message);
void acl_child_handler(Task task, MessageId id, Message message);
void ACLProcessParentData(const uint8_t *data, int size);
void ACLProcessChildData(const uint8_t *data, int size);


bool mtVoicePopulateConnectParameters(audio_connect_parameters *connect_parameters);
Sink mtGetActiveScoSink(void);
bool isMTVoiceSink(Sink sink);

bool mtRssiConnect(void);

bool mtCanPair(void);
bool mtIsOnlyChildConnect(void);
bool mtIsOnlyParentConnect(void);
int mtGetConnectDevices(void);

Sink mtGetSink(int type);

bool mtHeadConnected(void);
bool mtNodeConnected(void);

bool processEventMultiTalk(Task task, MessageId id, Message message);

typedef enum MTType
{
    MT_HEAD,
    MT_NODE,
    MT_TAIL,
    MT_PAIRHEAD
} MTType;

typedef struct RouteTable
{
    uint8 count;
    uint8 nop;
    uint8 reconnect_index;
    uint8 nop2;
    bdaddr item[8];
} RouteTable;


typedef enum DevcieSate
{
    MT_L2CAP_Disconnected,
    MT_L2CAP_Connecting,
    MT_L2CAP_WaitConnect,
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
    MT_ST_CHECKLOOP,
    MT_ST_CONNECTED,
    MT_ST_LINKLOSS,
    MT_ST_WAITING_CONNECT,
    MT_ST_RECONNECTING,
    MT_ST_STAY_DISCONNET,
    MT_ST_SEARCHING
} MTStatus;

typedef enum ACLMsg
{
    ACLMSG_EVENT,
    ACLMSG_CURRENT_CONNECTED,
    ACLMSG_ACK,
    ACLMSG_FIND_TAIL,
    ACLMSG_CHECK_TTL,
    ACLMSG_DEVICE_COUNT,
    ACLMSG_ROUTE_TABLE,
    ACLMSG_REPORT_TABLE,
    ACLMSG_HEAD_ADDR,
    ACLMSG_CHECK_TAIL,
    ACLMSG_CHECK_HEAD,
    ACLMSG_NEARBY_DISCONNECT,
    ACLMSG_PEER_ADDR,
    ACLMSG_CHECK_LOOP
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

typedef enum MTMode
{
    CLOSE_MODE,
    NEARBY_MODE,
    FREIEND_MODE,
    COUPLE_MODE,
    FREIEND_MODE_PAIRING,
    COUPLE_MODE_PRE_PAIRING,
    COUPLE_MODE_PAIRING
}MTMode;

enum
{
    COUPLE_MT_WITH_PEER,
    COUPLE_MT_NO_PEER,
    COUPLE_AG
};

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
    uint8 connected_device;
    RouteTable route_table;
    bdaddr addr;
    bdaddr head_addr;
    MTMode mt_mode;
    uint8   nearby_connected;
    bdaddr couple_addr;
    bdaddr headset_addr;
    uint8_t couple_type;

    bdaddr header_addr[2];
    uint8 sco_expend_dev;
    uint8 connect_token;
    uint8 prepare_paring;
} MTData;

#define MULTITALK_FRIEND_PSM 0x0055
#define MULTITALK_NEARBY_PSM 0x0057
#define MULTITALK_COUPLE_PSM 0x0059


#ifdef ENABLE_MT_DEBUG
#define MT_DEBUG(x) DEBUG(x)
#define MT_DEBUG_ADDR(x) DEBUG(("bdaddr = %x:%x:%lx\n", x.nap, x.uap, x.lap))
#else
#define MT_DEBUG(X)
#define MT_DEBUG_ADDR(x)
#endif


#define PowerAmpOn() PioSetPio(14, pio_drive, TRUE)
#define PowerAmpOff() PioSetPio(14, pio_drive, FALSE)


void BdaddrToArray(bdaddr *addr, uint8 *array);
bdaddr ArrayToBdaddr(const uint8 *array);

void RouteTablePushChild(RouteTable *rt, bdaddr addr);
bdaddr RouteTableGet(RouteTable *rt, uint8 index);
bool RouteTableIsNotSaved(RouteTable *rt);
uint8 RouteTableIsContain(RouteTable *rt, bdaddr *addr);
void RouteTableSort(RouteTable *rt);
uint8 RouteTableGetIndex(RouteTable *rt, bdaddr *addr);

bool mtReportRouteTable(int ch, RouteTable *rt);
bool mtSendRouteTable(int ch, RouteTable *rt);
void mtClearRouteTable(RouteTable *rt);
void mtSaveRouteTable(RouteTable *rt, uint8 mt_type);
void mtLoadRouteTable(RouteTable *rt);
void mtResetPairList(void);
void mtRouteTableAdjust(bdaddr child_addr);

void mtSaveCoupleAddr(bdaddr *addr, uint8 couple_type);
bool mtLoadCoupleAddr(bdaddr *addr, uint8 *couple_type);
void mtResetCoupleAddr(void);

void mtSetHeadsetAddr(bdaddr *addr);

bool ACLSend(Sink sink, const uint8_t *data, uint16 packet_size);
void ACLBroadcastEvent(Sink sink, uint16 event);

bool mtBroadcastCurrentCount(int count);
bool mtSendFindTail(int ch, uint8 type);
bool mtSendCheckTTL(uint8 ttl);
bool mtBroadcastConnectedCount(uint8 count);
bool mtCheckLoop(uint8 ttl ,bdaddr *addr);
bool mtSendAck(int ch, int id);

int BdaddrCompare(bdaddr *_1, bdaddr *_2);
int GetOnes(uint8 v);


/* -------------- Friend Mode ------------------*/
bool handleMTL2capConnectIndFriendMode(CL_L2CAP_CONNECT_IND_T *msg);
void handleMTL2capConnectCfmFriendMode(CL_L2CAP_CONNECT_CFM_T *msg);
void handleMTL2capDisconIndFriendMode(CL_L2CAP_DISCONNECT_IND_T *msg);
void handleMTL2capDisconCfmFriendMode(CL_L2CAP_DISCONNECT_CFM_T *msg);
void handleMTSynConnIndFriendMode(CL_DM_SYNC_CONNECT_IND_T *msg);
void handleMTSynConnCfmFriendMode(CL_DM_SYNC_CONNECT_CFM_T *msg);
void handleMTSynDisconIndFriendMode(CL_DM_SYNC_DISCONNECT_IND_T *msg);
bool processEventMultiTalkFriendMode(Task task, MessageId id, Message message);

/* -------------- Nearby Mode ------------------*/
bool handleMTL2capConnectIndNearbyMode(CL_L2CAP_CONNECT_IND_T *msg);
void handleMTL2capConnectCfmNearbyMode(CL_L2CAP_CONNECT_CFM_T *msg);
void handleMTL2capDisconIndNearbyMode(CL_L2CAP_DISCONNECT_IND_T *msg);
void handleMTL2capDisconCfmNearbyMode(CL_L2CAP_DISCONNECT_CFM_T *msg);
void handleMTSynConnIndNearbyMode(CL_DM_SYNC_CONNECT_IND_T *msg);
void handleMTSynConnCfmNearbyMode(CL_DM_SYNC_CONNECT_CFM_T *msg);
void handleMTSynDisconIndNearbyMode(CL_DM_SYNC_DISCONNECT_IND_T *msg);
bool processEventMultiTalkNearbyMode(Task task, MessageId id, Message message);

bool mtSendDisconnect(void);
bool mtSendCheckTail(void);
bool mtSendCheckHead(uint8 count);
bool mtBroadcastHeadAddr(uint8 count);
void ACLProcessParentDataNearby(const uint8_t *data, int size);
void ACLProcessChildDataNearby(const uint8_t *data, int size);

/* -------------- Couple Mode -------------------*/
bool handleMTL2capConnectIndCoupleMode(CL_L2CAP_CONNECT_IND_T *msg);
void handleMTL2capConnectCfmCoupleMode(CL_L2CAP_CONNECT_CFM_T *msg);
void handleMTL2capDisconIndCoupleMode(CL_L2CAP_DISCONNECT_IND_T *msg);
void handleMTL2capDisconCfmCoupleMode(CL_L2CAP_DISCONNECT_CFM_T *msg);
void handleMTSynConnIndCoupleMode(CL_DM_SYNC_CONNECT_IND_T *msg);
void handleMTSynConnCfmCoupleMode(CL_DM_SYNC_CONNECT_CFM_T *msg);
void handleMTSynDisconIndCoupleMode(CL_DM_SYNC_DISCONNECT_IND_T *msg);
bool processEventMultiTalkCoupleMode(Task task, MessageId id, Message message);
void ACLProcessParentDataCouple(const uint8_t *data, int size);

bool mtSendPeerAddr(int ch, bdaddr *addr);
void mtPeerStateCoupleMode(uint8 state);
void MTResetDeviceList(void);
bool mtCheckPeerDevice(bdaddr *addr);
void mtMicMute(uint8 mic_gain);

void sinkGetMTVolume(volume_info * volume);
void sinkSetMTVolume(volume_info * volume);
#endif
