#ifndef __HEADSET_UART_H__
#define __HEADSET_UART_H__

void UartInit(Task task);
void UartSend(const uint8_t* d, uint16 len);
void UartSendEvent(uint16 event);
void UartSendPeerAddr(bdaddr *addr);
uint8 UartGetState(void);
void UartSendPrompt(int prompt, uint8 queue);
void UartGetHeadsetAddr(void);
#endif
