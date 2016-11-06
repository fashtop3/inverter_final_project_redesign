/*
 * uart_sim800.cpp
 *
 * Created: 11/5/2016 11:45:25 AM
 * Author : Ayodeji
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
//#include "usart.h"
#include <string.h>
#include "serial.h"

#define F_CPU 16000000UL // Clock Speed

unsigned volatile char blink = 0;


ISR(TIMER1_COMPA_vect)
{
	blink=1;
	//if(*rx_buffer != 0)
	//USART_Transmit_string((unsigned char*)rx_buffer);
}

int getstring(char *data, char size, bool *ok = 0)
{
	char i = 0;
	volatile char c;
	if(serialHasChar(0))
	{
		if (i==0) memset(data, 0x00 , size); 
		
		while (i < (size-2))
		{
			if(serialHasChar(0))
			{
				c = serialGet(0);
				if (c == '\r') continue;
				if (c == '\0' || c == '\n') { *ok = true; break; }
				data[i++] = c;
			}
		}
		return i;
	}
	
	return 0;
}


int main(void)
{
	DDRB |= 1<<PINB3 | 1<<PINB4;
	PORTB |= 1<<PINB3 | 1<<PINB4;

	sei();
	
	TCCR1B |= (1<<WGM12) | (1<<CS12) | (1<<CS10);
	TIMSK |= (1<<OCIE1A);
	OCR1A = 15624; //timer overflow value set to 1sec

	
	serialInit(0, BAUD(9600, F_CPU));
	serialWriteString(0, "Welcome to IoT Inverter Config:\n");
	//
	uint8_t read = 30;
	char match[] = "Ayodeji";
	char data[read] = {0};
		char *ret = 0;
		
    while (true) 
    {
		if(blink == 1)
		{
			blink = 0;
			PORTB ^= 1<<PINB3;
		}
		
		if (getstring(data, 20))
		{				
			serialWriteString(0, "\nBefore\n");
			serialWriteString(0, "Returning: ");
			serialWriteString(0, data);
			serialWriteString(0, "\nAfter\n");
			PORTB ^= 1<<PINB4;
			if (ret = strstr(data, match))
			{
				serialWriteString(0, "Match: ");
				serialWriteString(0, ret);
				serialWriteString(0, "\n");
				ret = 0;
			}
			//int8_t cmp = strcmp(data, match);
			//if (cmp == 0)
			//{
				//serialWriteString(0, "\nFound a match\n");
			//}
			//else if(cmp > 0)
			//{
				//serialWriteString(0, "Data sent is less than Match");
			//}
			//else if(cmp < 0)
			//{
				//serialWriteString(0, "Match is greater than data sent");
			//}
		}
				
    }
}