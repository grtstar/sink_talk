#ifndef __HEADSET_UART_H__
#define __HEADSET_UART_H__

void UartInit(Task task);
void UartSend(const uint8_t* d, uint16 len);
void UartSendEvent(uint16 event);
uint8 UartGetState(void);
#endif
