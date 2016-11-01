/*
 * FinalProjectFromScratch.cpp
 *
 * Created: 11/1/2016 8:25:30 AM
 * Author : Ayodeji
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "Inverter.h"
#include "serial.h"

Inverter inverter(); //initialize inverter

//function prototype
void my_timer_setup();

int main(void)
{
	cli(); //clear global interrupt
	sei(); //Enable Global Interrupt
	my_timer_setup();
	
    /* Replace with your application code */
    while (1) 
    {
    }
}


ISR(TIMER1_COMPA_vect)
{
	
	//when mains is balance for use
	//mainsBalanceMonitor();
	
	//monitoring overloading
	//overloadMonitoring();
	
	//monitoring battery charging
	//chargingSelection();
	
	//battery level monitoring
	//batteryMonitoring();
	
	//display on LCD
	//displayInverterState();
}


ISR(ADC_vect)
{
	switch(ADMUX)
	{
		case 0x60: //check the pin on Analog Multiplexer Pin layout
		//conversion for AC input
		//ad_ac_readings = ADCH;
			inverter.setAcAnalogValue(ADCH);
			ADMUX = 0x61; //set to enable next pin for analog conversion
		break;
		case 0x61:
		//Conversion for Battery level
		//ad_batt_readings = ADCH;
			inverter.setBattAnalogValue(ADCH);
			ADMUX = 0x62;  //set to enable next pin for analog conversion
		break;
		case 0x62:
		//conversion for Overload
		//ad_overload_readings = ADCH;
			inverter.setOverloadAnalogValue(ADCH);
			ADMUX = 0x60;  //set to enable next pin for analog conversion
		break;
		
		default:
		// default
			ADMUX = 0x60; //set to first 
		break;
		
	}
	
	//start new conversion
	ADCSRA |= 1<<ADSC;
}

void my_timer_setup()
{
	//1000000/64 = 15625
	TCCR1B |= (1<<WGM12) | (1<<CS11) | (1<<CS10);
	TIMSK |= (1<<OCIE1A);
	OCR1A = 15624; //timer overflow value set to 1sec
}
