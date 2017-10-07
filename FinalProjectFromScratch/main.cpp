/*
 * FinalProjectFromScratch.cpp
 *
 * Created: 11/1/2016 8:25:30 AM
 * Author : Ayodeji
 */ 

//#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>
#include <string.h>
#include "Lcd.h"
#include "Inverter.h"
#include "serial.h"
#include "InvSIM800.h"
#include "f_cpu.h"

#define BAUD_RATE 57600
//Lcd lcd;
Inverter inverter; //initialize inverter
//InvSIM800 sim800 = InvSIM800();

uint8_t urc_status;
//char buf[SIM800_BUFSIZE];
//size_t len = 0;

//function prototype
void myTimerSetup();
void checkModuleToggled();
size_t readline(char *buffer, size_t max, uint16_t timeout=SIM800_SERIAL_TIMEOUT);

unsigned short int internet = 0;
short unsigned int ref_power_state;
short unsigned int ref_load_protect;

bool is_urc(const char *line, size_t len);
bool expect(const char *expected, uint16_t timeout=SIM800_SERIAL_TIMEOUT);
bool expect_scan(const char *pattern, void *ref, void *ref1, uint16_t timeout = SIM800_SERIAL_TIMEOUT);

int main(void)
{		
	//inverter.setServerResponse(1);
	
	//lcd.send_A_String("Hello World2");
	//sim800.setHostname("52.170.211.220/api/report/1");
	
	cli(); //clear global interrupt
	sei(); //Enable Global Interrupt
	myTimerSetup();
	_delay_ms(500); //allow boot time
	inverter.setSwitch(true); //power on the inverter
	serialInit(0, BAUD(BAUD_RATE, F_CPU));

	char request = 0;
	char data[30];

		
    while (1) 
    {	
		//len = readline(buf, SIM800_BUFSIZE);
		//serialWriteString(0, "hellol\n");
		if(expect_scan(F("DATA:%c:%s"), &request, data, 3000)){
			//serialWriteString(0, data);
			if (request == 'Q') //query
			{
				_delay_ms(200);
				serialWriteString(0, inverter.data());
				serialWrite(0, '\n');
			}
			else if (request == 'D')
			{
				if (sscanf(data, "%hu,%hu,%hu", &internet, &ref_power_state, &ref_load_protect) == 3) //0,1,70
				{ 
					
					if (!internet) {
						_delay_ms(200);
						serialWriteString(0, "Out of service\n");
						continue;
					}

					inverter.setOverload(ref_load_protect);
					inverter.setServerResponse(ref_power_state);
					_delay_ms(200);
					serialWriteString(0, inverter.data());
					serialWrite(0, '\n');
				}
				
			}
		}
    }
	
}

bool expect(const char *expected, uint16_t timeout)
{
	char buf[SIM800_BUFSIZE];
	size_t len;
	do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));	
	return strcmp(buf, (const char *) expected) == 0;
}

bool expect_scan(const char *pattern, void *ref, void *ref1, uint16_t timeout)
{
	char buf[SIM800_BUFSIZE];
	size_t len;
	do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
	return sscanf(buf, (const char *) pattern, ref, ref1) == 2;
}

bool is_urc(const char *line, size_t len)
{
	urc_status = 0xff;
	
	for (uint8_t i = 0; i < 17; i++)
	{
		char urc[30];
		strcpy(urc, (PGM_P)pgm_read_word(&(_urc_messages[i])));
		uint8_t urc_len = strlen(urc);
		if (len >= urc_len && !strncmp(urc, line, urc_len))
		{
			urc_status = i;
			return true;
		}
	}

	return false;
}


size_t readline(char *buffer, size_t max, uint16_t timeout)
{
	static volatile uint16_t idx = 0;
	volatile char c;
	uint16_t rx = 0;
	
	while (--timeout) {
		while (serialHasChar(0)) {
			c = serialGet(0);
			if (c == '\r') continue;
			if (c == '\n') {
				if (!idx) continue;
				timeout = 0;
				rx = idx;
				break;
			}
			if (max - idx) buffer[idx++] = c;
		}

		if (timeout == 0) break;
		_delay_ms(1);
	}
	
	buffer[idx] = 0;
	idx = 0;
	return rx;
}


ISR(TIMER1_COMPA_vect)
{
	//this ISR runs every 100ms;
	static volatile uint8_t count = 0;
	static volatile uint16_t internet_delay_check = 1;
	count++;

	inverter.monitor();
	
	if (count > 10) //do this every 1sec
	{
		if (!internet) {
			if (internet_delay_check >= 1800) //equals 30 mins delay
			{
				ref_power_state = 0;
				inverter.setServerResponse(ref_power_state); // intenet is 0
				inverter.setOverload(inverter.getOverloadDefault());
				internet_delay_check = 1;
			}
			internet_delay_check++;
		} else {
			internet_delay_check = 1;
		}
		
		inverter.incrementEntryCounter();
		inverter.emitMessage();
		count = 0;
	}
}


ISR(ADC_vect)
{
 	inverter.analogPinSwitching(); //allow all MCU to read from all enabled analog pin
	ADCSRA |= 1<<ADSC; //start new conversion
}

void myTimerSetup()
{
	//16000000/1024 = 15625 //FOR 1sec
	TCCR1B |= (1<<WGM12) | (1<<CS12) | (1<<CS10);
	TIMSK |= (1<<OCIE1A);
	//OCR1A = 15624; //timer overflow value set to 1sec
	OCR1A = 1562; //timer overflow value set to 1sec
}