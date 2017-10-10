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
void power_button_ctrl();
void DQR();
size_t readline(char *buffer, size_t max, uint16_t timeout=SIM800_SERIAL_TIMEOUT);

unsigned short int internet = 0;
short unsigned int ref_power_state = 0;
short unsigned int ref_load_protect;
volatile int press_debounce = 0;
volatile int release_debounce = 0;
volatile int pressed = 0;
volatile bool received_data = false;

bool is_urc(const char *line, size_t len);
bool expect(const char *expected, uint16_t timeout=SIM800_SERIAL_TIMEOUT);
bool expect_scan(const char *pattern, void *ref, void *ref1, uint16_t timeout = SIM800_SERIAL_TIMEOUT);

int main(void)
{		
	inverter.setServerResponse((const uint8_t *)&ref_power_state);
	
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
					
		//power_button_ctrl();

		if(expect_scan(F("DATA:%c:%s"), &request, data, 2000)){
			received_data = true;
			//serialWriteString(0, data);
			if (request == 'Q') //query
			{
				DQR();
			}
			else if (request == 'D')
			{
				if (inverter.isModuleAvailable())
				{
					if (sscanf(data, "%hu,%hu,%hu", &internet, &ref_power_state, &ref_load_protect) == 3) //0,1,70
					{
						
						if (!internet) {
							_delay_ms(200);
							serialWriteString(0, "Out of service\n");
							continue;
						}

						inverter.setOverload(&ref_load_protect);
						inverter.setServerResponse((const uint8_t *)&ref_power_state);
					}
				}
				DQR();				
			}
			*data = '\0';
		}
		else {
			received_data = false;
		}
    }
	
}

void DQR()
{
	_delay_ms(200);
	serialWriteString(0, inverter.data());
	serialWrite(0, '\n');
}

void power_button_ctrl()
{
	if (bit_is_clear(PIND, POWER_BUTTON) && bit_is_clear(PIND, SIM_MODULE))
	{
		++press_debounce;
		if (press_debounce > 3)
		{
			if (pressed == 0)
			{
				ref_power_state ^= 1;
				inverter.setServerResponse((const uint8_t *)&ref_power_state);
				pressed = 1;
			}
			press_debounce = 0;
		}
	}
	else {
		pressed = 0;
		press_debounce = 0;
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
			if(c != 'D' && !idx) continue;
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
		_delay_us(10);
	}
	
	buffer[idx] = 0;
	idx = 0;
	return rx;
}

ISR(TIMER1_COMPA_vect)
{
	//this ISR runs every 100ms;
	static volatile uint8_t count = 0;
	static volatile uint16_t data_check = 0;
	static volatile uint16_t internet_delay_check = 1;
	count++;

	power_button_ctrl();

	inverter.monitor();

	if (count > 9) //do this every 1sec
	{
		INV_CTR |= (1<<MODULE_HARD_RESET); //set logic high
		
		if (inverter.isModuleAvailable())
		{
			//HARD RESET SECTION
			if (!received_data) //when SIM MODULE CONTROLLER HANGS
			{
				data_check++;
				if (data_check >= 610)
				{
					INV_CTR &= ~(1<<MODULE_HARD_RESET); //issue reset
					data_check = 0; //set a constant check
					
					ref_power_state = 0;
					inverter.setServerResponse((const uint8_t *)&internet); // internet is 0
					inverter.setOverload(inverter.getOverloadDefault());
				}
			}
			else // SIM MODULE IS STILL SENDING
			{
				data_check = 0;
				////INTERNET VISIBLE SECTION
				if (!internet)
				{ 
					internet_delay_check++; 
					if (internet_delay_check >= 310) //5min check
					{
						ref_power_state = 0;
						inverter.setServerResponse((const uint8_t *)&internet); // internet is 0
						inverter.setOverload(inverter.getOverloadDefault());
						internet_delay_check = 1;
					}
				}
				else
				{
					internet_delay_check = 1;
				}
			}
		}
		else
		{
			inverter.setOverload(inverter.getOverloadDefault());
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
	////////OCR1A = 15624; //timer overflow value set to 1sec ---> for 16MHz
	
	OCR1A = 1562; //timer overflow value set to 1sec ---> for 16MHz
	
	//OCR1A = 1953; //timer overflow value set to 1sec ---> for 20MHz
}