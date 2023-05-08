#include <stdio.h>
#include <string.h>
#include <vm.h>
#include <panic.h>
#include <connection.h>
#include <ps.h>
#include <panic.h>
#include <stream.h>
#include <source.h>
#include <sink.h>
#include <message.h>
#include <boot.h>
#include <pio.h>

#include "protocol_hex.h"
#include "sink_events.h"
#include "sink_debug.h"
#include "headset_uart.h"
#include "sink_statemanager.h"

#define UART_DEBUG(x) DEBUG(x)
/*
The setting in the xIDE Project Properties Transport field configures the Host transport on boot up and must
be set to raw to use these functions.
*/

enum
{
    UTYPE_EVENT,
    UTYPE_STATE,
    UTYPE_PROMPT,
    UTYPE_PEER_ADDR,
    UTYPE_HEADSET_ADDR,
    UTYPE_PEER_STATE,
    UTYPE_GET_HEADSET_ADDR,
    UTYPE_TONE
};

enum
{
    P_HEAD1,
    P_HEAD2,
    P_LEN,
    P_DATA,
    P_CRC
};

extern void BdaddrToArray(bdaddr *addr, uint8 *d);
extern bdaddr ArrayToBdaddr(const uint8 *data);
extern void mtSetHeadsetAddr(bdaddr *addr);
extern void mtPeerStateCoupleMode(uint8 state);

static void UartMessageHandler(Task pTask, MessageId pId, Message pMessage);
static void UartProcessData(const uint8 *data, int size);
static uint8 UartCalcCRC(uint8 *d, int len);
static void UartTxData(const uint8 *data, uint16 packet_size);

static TaskData message_task = {UartMessageHandler};

typedef struct UartData
{
    Sink uart_sink;
    Task app_task;
    uint8 uart_buff[16];
    uint8 uart_index;
    uint8 uart_stage;
    uint8 uart_data_len;
    uint8 uart_state;
}UartData;

UartData *ud;

/* transmits data to UART */
void UartTxData(const uint8 *data, uint16 packet_size)
{
    uint16 sink_offset = 0;
    uint16 i = 0;
    UART_DEBUG(("Uart send:\n"));
    for (i = 0; i < packet_size; i++)
    {
        UART_DEBUG(("%02X ", data[i]));
        if ((i & 0x7) == 0x7)
        {
            UART_DEBUG(("\n"));
        }
    }
    UART_DEBUG(("\n"));

    if (SinkIsValid(ud->uart_sink))
    {
        if ((sink_offset = SinkClaim(ud->uart_sink, packet_size)) != 0xFFFF)
        {
            memmove(SinkMap(ud->uart_sink) + sink_offset, data, packet_size);
            PanicFalse(SinkFlush(ud->uart_sink, packet_size));
        }
        else
        {
            Panic();
        }
    }
    else
    {
        Panic();
    }
}

static void UartMessageHandler(Task pTask, MessageId pId, Message pMessage)
{
    (void)pTask;
    (void)(pMessage);

    switch (pId)
    {
    case MESSAGE_MORE_DATA:
    {
        uint16 packet_size;
        MessageMoreData *msg = (MessageMoreData *)pMessage;
        while ((packet_size = SourceBoundary(msg->source)) != 0)
        {
            const uint8 *in = SourceMap(msg->source);
            /* recevie uart data */
            UartProcessData(in, packet_size);
            SourceDrop(msg->source, packet_size);
        }
    }
    break;
    case MESSAGE_MORE_SPACE:

        break;
    }
}

void UartInit(Task task)
{
    Source uart_source;
    ud = (UartData *)PanicUnlessMalloc(sizeof(UartData));
    VmDeepSleepEnable(FALSE);
    StreamUartConfigure(VM_UART_RATE_9K6, VM_UART_STOP_ONE, VM_UART_PARITY_NONE);
    ud->uart_sink = StreamUartSink();
    PanicNull(ud->uart_sink);
    /*SourceConfigure(StreamSourceFromSink(uart_sink), VM_SOURCE_MESSAGES, VM_MESSAGES_SOME);*/
    /*SinkConfigure(uart_sink, VM_SINK_MESSAGES, VM_MESSAGES_SOME);*/
    uart_source = StreamUartSource();
    PanicNull(uart_source);
    MessageSinkTask(StreamSinkFromSource(uart_source), &message_task);
    /*MessageSinkTask(uart_sink, &message_task);*/
    ud->app_task = task;
    ud->uart_data_len = 0;
    ud->uart_index = 0;
    ud->uart_stage= 0;
    ud->uart_state = 0;
}

void UartSend(const uint8_t *data, uint16 packet_size)
{
    uint16 sink_offset = 0;
    if (SinkIsValid(ud->uart_sink))
    {
        if ((sink_offset = SinkClaim(ud->uart_sink, packet_size)) != 0xFFFF)
        {
            memmove(SinkMap(ud->uart_sink) + sink_offset, data, packet_size);
            PanicFalse(SinkFlush(ud->uart_sink, packet_size));
        }
        else
        {
            Panic();
        }
    }
    else
    {
        Panic();
    }
}

void UartProcessData(const uint8_t *data, int size)
{
    int i = 0;
    UART_DEBUG(("Uart recv:\n"));
    for (i = 0; i < size; i++)
    {
        UART_DEBUG(("%02X ", data[i]));
        if ((i & 0x7) == 0x7)
        {
            UART_DEBUG(("\n"));
        }
    }
    UART_DEBUG(("\n"));
    for (i = 0; i < size; i++)
    {
        switch (ud->uart_stage)
        {
        case P_HEAD1:
            if (data[i] == 0xAA)
            {
                ud->uart_stage = P_HEAD2;
                ud->uart_index = 0;
                ud->uart_buff[ud->uart_index++] = data[i];
            }
            break;
        case P_HEAD2:
            if (data[i] == 0x55)
            {
                ud->uart_stage = P_LEN;
                ud->uart_buff[ud->uart_index++] = data[i];
            }
            else if(data[i] != 0xAA)
            {
                ud->uart_stage = P_HEAD1;
            }
            break;
        case P_LEN:
            ud->uart_data_len = data[i];
            ud->uart_stage = P_DATA;
            ud->uart_buff[ud->uart_index++] = data[i];
            break;
        case P_DATA:
            if(ud->uart_index >= 16)
            {
                ud->uart_stage = P_HEAD1;
                break;
            }
            ud->uart_buff[ud->uart_index++] = data[i];
            if (ud->uart_index == ud->uart_data_len + 3)
            {
                ud->uart_stage = P_CRC;
            }
            break;
        case P_CRC:
        {
            if (ud->uart_buff[3] == UTYPE_EVENT)
            {
                uint16 event = ud->uart_buff[4] << 8 | ud->uart_buff[5];
                UART_DEBUG(("UART: evnet = 0x%x\n", event));
                MessageSend(ud->app_task, event, NULL);
            }
            if (ud->uart_buff[3] == UTYPE_STATE)
            {
                ud->uart_state = ud->uart_buff[4];
                UART_DEBUG(("UART: state = 0x%x\n", ud->uart_state));
                stateManagerUpdateState();
            }
            if (ud->uart_buff[3] == UTYPE_HEADSET_ADDR)
            {
                bdaddr addr = ArrayToBdaddr(&ud->uart_buff[4]);
                mtSetHeadsetAddr(&addr);
                UART_DEBUG(("UART: headset addr: %x:%x:%lx\n", addr.nap, addr.uap, addr.lap));
            }
            if (ud->uart_buff[3] == UTYPE_PEER_STATE) /* 3024 peer connect state */
            {
                mtPeerStateCoupleMode(ud->uart_buff[4]);
            }
        }
            ud->uart_stage = P_HEAD1;
            break;
        }
    }
}

uint8 UartCalcCRC(uint8 *d, int len)
{
    uint8 sum = 0;
    int i = 0;
    for (i = 0; i < len; i++)
    { 
        sum += d[i];
    }
    return sum;
}

void UartSendEvent(uint16 event)
{
    switch (event)
    {
    case EventUsrPowerOff:
    case EventUsrResetPairedDeviceList:
    {
        uint8 d[7] = {0xAA, 0x55, 3, UTYPE_EVENT};
        d[4] = event >> 8;
        d[5] = event;
        d[6] = UartCalcCRC(d, 6);
        UartTxData(d, 7);
    }
    break;
    default:
        break;
    }
}

void UartSendTone(uint16 event)
{
    switch (event)
    {
    case EventUsrMainOutVolumeDown:
    case EventUsrMainOutVolumeUp:
    {
        uint8 d[7] = {0xAA, 0x55, 3, UTYPE_TONE};
        d[4] = event >> 8;
        d[5] = event;
        d[6] = UartCalcCRC(d, 6);
        UartTxData(d, 7);
    }
    break;
    default:
        break;
    }
}

void UartGetHeadsetAddr(void)
{
    uint8 d[3 + 2] = {0xAA, 0x55, 1, UTYPE_GET_HEADSET_ADDR};
    d[4] = UartCalcCRC(d, 4);
    UartTxData(d, sizeof(d));
}

void UartSendPeerAddr(bdaddr *addr)
{
    uint8 d[3 + 9] = {0xAA, 0x55, 8, UTYPE_PEER_ADDR};
    BdaddrToArray(addr, &d[4]);
    d[11] = UartCalcCRC(d, 11);
    UartTxData(d, 12);
}

void UartSendPrompt(int prompt, uint8 queue)
{
    uint8 d[3 + 4] = {0xAA, 0x55, 3, UTYPE_PROMPT};
    d[4] = prompt;
    d[5] = queue;
    d[6] = UartCalcCRC(d, 6);
    UartTxData(d, sizeof(d));
}

uint8 UartGetState(void)
{
    return ud->uart_state;
}

void UartSendState(int state)
{
    switch (state)
    {
    case sink_ext_state_multiTalkSlow3:
    case sink_ext_state_multiTalkSlow2:
    case sink_ext_state_multiTalkFast:
    {
        uint8 d[6] = {0xAA, 0x55, 2, UTYPE_STATE};
        d[4] = state;
        d[5] = UartCalcCRC(d, 5);
        UartTxData(d, 6);
        UartTxData(d, 6);
    }
    break;
    default:
    {
        uint8 d[6] = {0xAA, 0x55, 2, UTYPE_STATE};
        d[4] = 0;
        d[5] = UartCalcCRC(d, 5);
        UartTxData(d, 6);
    }
    break;
    }
}
