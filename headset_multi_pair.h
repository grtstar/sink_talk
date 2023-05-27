#ifndef _HEADSET_MULTI_PAIR_
#define _HEADSET_MULTI_PAIR_

#include "sink_inquiry.h"
bool mtIsInquirySession(void);
bool mtInquiryPair(inquiry_session session, bool remind);
void mtInquiryStop(void);
uint8* mtGetAdvData(void);
uint8 mtGetAdvDataLen(void);
bool sinkMultiTalkPairServiceEnabled(void);
uint16 mtGetPairServiceUuid(void);
uint8* mtGetAdvertisingFilter(void);
#endif
