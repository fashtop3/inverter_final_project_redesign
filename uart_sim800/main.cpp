/*
 * uart_sim800.cpp
 *
 * Created: 11/5/2016 11:45:25 AM
 * Author : Ayodeji
 */ 

//AT
//AT
//AT
//ATE0
//AT+IFC=0,0
//AT+CIURC=0

#define F_CPU 16000000UL // Clock Speed


#include <avr/io.h>
#include <avr/interrupt.h>
//#include "usart.h"
#include <string.h>
#include <util/delay.h>
#include <stdlib.h>
#include "serial.h"
#include "InvSIM800.h"
#include "Lcd.h"

unsigned volatile char blink = 0;

#ifdef F
#undef F
#endif
#define F(s) (s)


ISR(TIMER1_COMPA_vect)
{
	blink=1;
}


int main(void)
{
	Lcd lcd;//= Lcd();
	lcd.send_A_String("hELLO");
	_delay_ms(3000);
	lcd.clScr();
	
	DDRB |= 1<<PINB3 | 1<<PINB4;
	
	sei();
	
	TCCR1B |= (1<<WGM12) | (1<<CS12) | (1<<CS10);
	TIMSK |= (1<<OCIE1A);
	OCR1A = 15624; //timer overflow value set to 1sec
	serialInit(0, BAUD(9600, F_CPU));

	_delay_ms(3000);
	PORTB |= 1<<PINB3 | 1<<PINB4;
		
	//serialWriteString(0, F("Welcome to IoT Inverter Config:\n"));
	
	InvSIM800 sim800 = InvSIM800();
	
	if (sim800.reset(true))
	{
		sim800.setAPN(F("web.gprs.mtnnigeria.net"), F("web"), F("web"));
		//sim800.println(F("SIM800 waiting for network registration..."));
		
		//sim800.println(F("SIM800 enabling GPRS..."));
		if (!sim800.enableGPRS(5000)) {
			//sim800.println("SIM800 can't enable GPRS");
			while (1);
		}
		
		
		static const char* const url = "iot.rockcityfmradio.com/api/report/1?type=project&ac_in=190&battery_level=78&charging=1&load=40";
		static uint32_t length = 0;
		uint16_t status = sim800.HTTP_get(url, length);
		if (status == 200)
		{
			char buffer[20] = {0};
			uint16_t idx = sim800.HTTP_read(buffer, 0, 4);
			if (idx)
			{
				lcd.send_A_String(buffer);
				char control = buffer[0];
				if (control == '1')
				{
					PORTB ^= 1<<PINB4;
				}	
			}
		}
	}
	
	////NOW WORKING
	//if(sim800.expect_AT_OK(F("")))
	//{
		//PORTB ^= 1<<PINB4;
	//}
	
	//////tested ok
	////char str1[10];
	////char str2[10];
	////
	////unsigned long int length;
	////unsigned short int status;
	//////+HTTPACTION: 0,200,1
	////if(sim800.expect_scan(F("+HTTPACTION: 0,%hu,%lu"), &status, &length, 60000))
	////{
		////itoa(length, str1, 10);
		////itoa(status, str2, 10);
		////serialWriteString(0, "Length: ");
		////serialWriteString(0, str1);
		////
		////serialWriteString(0, "\nStatus: ");
		////serialWriteString(0, str2);
		////
		////PORTB ^= 1<<PINB4;
	////}
	
	//sim800.println("SIM800 initialized");
	
		
    while (true) 
    {
		if(blink == 1)
		{
			blink = 0;
			PORTB ^= 1<<PINB3;
		}	
    }
}