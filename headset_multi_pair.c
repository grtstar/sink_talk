#include "sink_main_task.h"
#include "sink_events.h"
#include "sink_ble.h"
#include "sink_statemanager.h"

#include "headset_multi_pair.h"

uint8 bleAdv[8] = "M6S";
inquiry_session inqSesson = inquiry_session_normal;

bool mtIsInquirySession(void)
{
    return inqSesson == inquiry_session_multi_talk;
}

bool mtIs2TalkPairingSession(void)
{
    return inqSesson == inquiry_session_2talk_pairing || inqSesson == inquiry_session_ag;
}

bool mtInquiryPair(inquiry_session session, bool remind)
{
    /* start ble broadcast */
    if(inqSesson == session)
    {
        return FALSE;
    }
    inqSesson = session;
    if(session == inquiry_session_friend)
    {
        strcpy((char*)bleAdv, "M6SF");
    }
    if(session == inquiry_session_multi_talk)
    {
        strcpy((char*)bleAdv, "M6SM");
    }
    if(session == inquiry_session_nearby)
    {
        strcpy((char*)bleAdv, "M6SN");
    }
    if(session == inquiry_session_2talk)
    {
        strcpy((char*)bleAdv, "M6SC");
    }
    if(session == inquiry_session_2talk_pairing)
    {
        strcpy((char*)bleAdv, "M6SA");
    }
    if(session == inquiry_session_ag)
    {
        return inquiryPair(session, remind);
    }
    if(remind)
    {
        MessageSendLater(&theSink.task, EventSysRssiPairReminder, 0, D_SEC(INQUIRY_REMINDER_TIMEOUT_SECS));
    }
    stateManagerEnterConnectableState(FALSE);
    sinkBlePowerOffEvent();    
    sinkBlePowerOnEvent();
    return TRUE;
}

void mtInquiryStop(void)
{
    if(inqSesson == inquiry_session_ag || inqSesson == inquiry_session_2talk_pairing)
    {
        inquiryStop();
    }
    inqSesson = inquiry_session_normal;
    MessageCancelAll(&theSink.task, EventSysRssiPairReminder);
    sinkBlePowerOffEvent();
}

uint8* mtGetAdvData(void)
{
    return bleAdv;
}

uint8 mtGetAdvDataLen(void)
{
    return strlen((char*)bleAdv);
}

bool sinkMultiTalkPairServiceEnabled(void)
{
    return TRUE;
}

#define UUID_M6S_FRIEND 0x820A
#define UUID_M6S_MULTI_TALK 0x820B
#define UUID_M6S_NEARBY 0x820C
#define UUID_M6S_2TALK 0x820D
#define UUID_M6S_2TALK_PAIRING 0x820E

static const uint8 friend_pair_ble_advertising_filter[] = {UUID_M6S_FRIEND & 0xFF, UUID_M6S_FRIEND >> 8};
static const uint8 multi_pair_ble_advertising_filter[] = {UUID_M6S_MULTI_TALK & 0xFF, UUID_M6S_MULTI_TALK >> 8};
static const uint8 nearby_pair_ble_advertising_filter[] = {UUID_M6S_NEARBY & 0xFF, UUID_M6S_NEARBY >> 8};
static const uint8 talk2_pair_ble_advertising_filter[] = {UUID_M6S_2TALK & 0xFF, UUID_M6S_2TALK >> 8};
static const uint8 talk2_pairing_ble_advertising_filter[] = {UUID_M6S_2TALK_PAIRING & 0xFF, UUID_M6S_2TALK_PAIRING >> 8};

uint16 mtGetPairServiceUuid(void)
{
    switch (inqSesson)
    {
        case inquiry_session_friend:
            return UUID_M6S_FRIEND;
        case inquiry_session_multi_talk:
            return UUID_M6S_MULTI_TALK;
        case inquiry_session_nearby:
            return UUID_M6S_NEARBY;
        case inquiry_session_2talk:
            return UUID_M6S_2TALK;
        case inquiry_session_2talk_pairing:
            return UUID_M6S_2TALK_PAIRING;
        default:
            return UUID_M6S_FRIEND;
    }
}

uint8* mtGetAdvertisingFilter(void)
{
    switch (inqSesson)
    {
        case inquiry_session_friend:
            return (uint8*)friend_pair_ble_advertising_filter;
        case inquiry_session_multi_talk:
            return (uint8*)multi_pair_ble_advertising_filter;
        case inquiry_session_nearby:
            return (uint8*)nearby_pair_ble_advertising_filter;
        case inquiry_session_2talk:
            return (uint8*)talk2_pair_ble_advertising_filter;
        case inquiry_session_2talk_pairing:
            return (uint8*)talk2_pairing_ble_advertising_filter;
        default:
            return (uint8*)friend_pair_ble_advertising_filter;
    }
}
