#include "usart.h"
#include <avr/io.h>

void USART_Init(unsigned int ubrr)
{
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	/*Enable receiver and transmitter */
	/*Enable tx and rx interrupt*/
	UCSRB |= (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
	
	/* Set frame format: 8data, 2stop bit */
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
}

void USART_Transmit(unsigned int data)
{
	while(! (UCSRA & (1<<UDRE)));
	UDR = data;
}

//Transmit string through UART
void USART_Transmit_string(unsigned char * str)
{
	while(*str)
	{
		UDR = *str++;
		while(!(UCSRA&(1<<UDRE)));
	}
}

unsigned char USART_Receive()
{
	while(! (UCSRA & (1<<RXC)));
	return UDR;
}

void USART_Init_Transmit(unsigned int ubrr)
{
	UBRRH = (unsigned char) (ubrr >> 8);	//Setting the high byte of ubrr into UBRRH
	UBRRL = (unsigned char) ubrr;			//Setting the low byte of ubrr
	UCSRB = (1<<TXEN);		//Enabling Tx and Rx
	UCSRC = (1<<USBS) | (3<<UCSZ0);		//Setting 2-stop bit and enabling 8 bit data
}

void USART_Init_Receive(unsigned int ubrr)
{
	UBRRH = (unsigned char) (ubrr >> 8);	//Setting the high byte of ubrr into UBRRH
	UBRRL = (unsigned char) ubrr;			//Setting the low byte of ubrr
	UCSRB = (1<<RXEN);		//Enabling Tx and Rx
	UCSRC = (1<<USBS) | (3<<UCSZ0);		//Setting 2-stop bit and enabling 8 bit data
}
