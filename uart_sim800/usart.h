#ifndef USARTLIB
#define USARTLIB

void USART_Init(unsigned int ubrr);
void USART_Init_Transmit(unsigned int ubrr);
void USART_Init_Receive(unsigned int ubrr);
void USART_Transmit(unsigned int data);
void USART_Transmit_string(unsigned char * str);
unsigned char USART_Receive(void);

#endif