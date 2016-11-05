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
//#define BAUD 9600
//#define MYUBRR F_CPU/16/BAUD-1

//answer located in FLASH memory.
unsigned volatile char blink = 0;

// USART Receiver interrupt service routine


ISR(TIMER1_COMPA_vect)
{
	blink=1;
	//if(*rx_buffer != 0)
	//USART_Transmit_string((unsigned char*)rx_buffer);
}

//int getstring(char *data, char size, bool *ok = false)
//{
	//memset(data, 0x00 , sizeof(data));
	//char i = 0;
	//volatile char c;
	//while (i < (size-2))
	//{
		//if(serialHasChar(0))
		//{
			//c = serialGet(0);
			//if (c == '\0' || c == '\n') {
				 //break;
				 //*ok = true;
			//}
			//data[i++] = c;
		//}
	//}
	//return i == 0 ? i : i + 1;
//}

//char *getstring(char size, bool *ok = false)
//{
	//char data[size] = {0};
	//char i = 0;
	//volatile char c;
	//while (i < (size-2))
	//{
		//if(serialHasChar(0))
		//{
			//c = serialGet(0);
			//if (c == '\0' || c == '\n')
			//{
				//*ok = true;
				 //break;
			//}
			//data[i++] = c;
		//}
		////continue;
	//}
	//return data;
//}

int main(void)
{
	DDRB |= 1<<PINB3 | 1<<PINB4;
	PORTB |= 1<<PINB3 | 1<<PINB4;

	sei();
	
	TCCR1B |= (1<<WGM12) | (1<<CS12) | (1<<CS10);
	TIMSK |= (1<<OCIE1A);
	OCR1A = 15624; //timer overflow value set to 1sec
	
	//USART_Init(MYUBRR);
	//USART_Transmit('A');
	
	//serialInit(0, BAUD(9600, F_CPU));
	//serialWriteString(0, "Welcome to IoT Inverter Config:\n");
	//
	//uint8_t read = 30;
	//volatile int i = 0;
	//char match[] = "1234567890123456789012345678";
	//char data[read] = {0};
	//char *data2;
	//volatile char c;
	//bool ok = false;
		
    while (true) 
    {
		//if(blink == 1)
		//{
			//blink = 0;
			//PORTB ^= 1<<PINB3;
		//}
		
		//if(!ok) data2 = getstring(50, &ok);
		
		//if (ok)
		//{
			//serialWriteString(0, "\nBefore\n");
			//serialWriteString(0, "Returning: ");
			//serialWriteString(0, data2);
			//serialWriteString(0, "\nAfter\n");
			//
		//}
		
		//if (getstring(data, 20))
		//{
			//serialWriteString(0, "\nBefore\n");
			//serialWriteString(0, "Returning: ");
			//serialWriteString(0, data);
			//serialWriteString(0, "\nAfter\n");
			//
			//memset(data, 0x00 , sizeof(data));
			//
			////if (strcmp(match, data) == 0)
			////{
				////serialWriteString(0, "\nFound a match\n");
			////}
			//
			//i = 0;
			//PORTB ^= 1<<PINB4;
		//}
				
    }
}