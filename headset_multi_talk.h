#ifndef _HEADSET_MULTI_TALK_
#define _HEADSET_MULTI_TALK_

void mtInit(Task task);
bool mtEnterPairing(void);
void mtInquiry(void);
void mtACLConnect(bdaddr *bd_addr);
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
void mtTailConnect(void);
int mtGetConnectDevices(void);

bool mtCanPair(void);
bool mtIsOnlyChildConnect(void);

bool processEventMultiTalk(Task task, MessageId id, Message message);
#endif
