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

#include "sink_debug.h"
#include "headset_uart.h"

#define UART_DEBUG(x) DEBUG(x)
/*
The setting in the xIDE Project Properties Transport field configures the Host transport on boot up and must
be set to raw to use these functions.
*/

static void UartMessageHandler( Task pTask, MessageId pId, Message pMessage);
static void UartProcessData(const uint8 *data, int size);

static TaskData message_task = {UartMessageHandler};

Sink uart_sink;
Source uart_source;

static void UartMessageHandler( Task pTask, MessageId pId, Message pMessage)
{
    (void)pTask;
    (void)(pMessage);

    switch (pId)
    {
    case MESSAGE_MORE_DATA:
        {
            uint16 packet_size;
            MessageMoreData *msg = (MessageMoreData*)pMessage;
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
    case 0x4321:
        UartSend((const uint8*)"56789", 5);
        MessageSendLater(&message_task, 0x4321, NULL, 3000);
        break;
    }
}

void UartInit(void)
{
    VmDeepSleepEnable(FALSE);
    StreamUartConfigure(VM_UART_RATE_9K6, VM_UART_STOP_ONE, VM_UART_PARITY_NONE);
    uart_sink = StreamUartSink();
    PanicNull(uart_sink); 
    /*SourceConfigure(StreamSourceFromSink(uart_sink), VM_SOURCE_MESSAGES, VM_MESSAGES_SOME);*/
    /*SinkConfigure(uart_sink, VM_SINK_MESSAGES, VM_MESSAGES_SOME);*/
    uart_source = StreamUartSource();
    PanicNull(uart_source); 
    MessageSinkTask(StreamSinkFromSource(uart_source), &message_task);
    /*MessageSinkTask(uart_sink, &message_task);*/

    /*ProtoHexInit(&_protoUart);*/
}

void UartSend(const uint8_t *data, uint16 packet_size)
{
    uint16 sink_offset=0;
    if (SinkIsValid(uart_sink))
    {
        if((sink_offset=SinkClaim(uart_sink, packet_size)) != 0xFFFF)
        {
            memmove(SinkMap(uart_sink)+sink_offset, data, packet_size);
            PanicFalse(SinkFlush(uart_sink, packet_size));
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
    for(i=0; i<size; i++)
    {
        UART_DEBUG(("%02X ", data[i]));
        if((i & 0x7) == 0x7)
        {
            UART_DEBUG(("\n"));
        }
    }
}
