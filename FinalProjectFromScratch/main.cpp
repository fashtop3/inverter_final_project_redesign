/*
 * FinalProjectFromScratch.cpp
 *
 * Created: 11/1/2016 8:25:30 AM
 * Author : Ayodeji
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>
#include "Inverter.h"
#include "Lcd.h"
#include "serial.h"
#include "InvSIM800.h"

 Lcd lcd;
 Inverter inverter(lcd); //initialize inverter
 InvSIM800 sim800 = InvSIM800();

//function prototype
void myTimerSetup();
void checkModuleToggled();

int main(void)
{		
	lcd.clScr(); // clear screen
	sim800.setHostname("iot.rockcityfmradio.com/api/report/1");
	
	//cli(); //clear global interrupt
	sei(); //Enable Global Interrupt
	myTimerSetup();
	_delay_ms(500); //allow boot time
	//inverter.setSwitch(true); //power on the inverter
		
    while (1) 
    {		
		if (inverter.isModuleAvailable())
		{
			sim800.setup();
		}
    }
	
}

ISR(TIMER1_COMPA_vect)
{
	//this ISR runs every 100ms;
	static volatile uint8_t count = 0;
	count++;
	
	//PORTB ^= 1<<PINB3; //do this every 100ms;	
	inverter.monitor(sim800.getServerResponse());
	checkModuleToggled();
	
	if (count > 10) //do this every 1sec
	{
		sim800.incrementInSec();
		inverter.incrementEntryCounter();
		inverter.emitMessage();
		sim800.setParam(inverter.data());
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

void checkModuleToggled()
{
	static bool cur = false;
	if (inverter.isModuleAvailable())
	{
		if (!cur) sim800.externalReset();
		cur = true;
	}
	else
	{
		cur = false;
	}
}