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


int main(void)
{
	
    /* Replace with your application code */
    while (1) 
    {
    }
}


//ISR(ADC_vect)
//{
	//switch(ADMUX)
	//{
		//case 0x60:
		////conversion for AC input
		//ad_ac_readings = ADCH;
		//ADMUX = 0x61;
		//break;
		//case 0x61:
		////Conversion for Battery level
		//ad_batt_readings = ADCH;
		//ADMUX = 0x62;
		//break;
		//case 0x62:
		////conversion for Overload
		//ad_overload_readings = ADCH;
		//ADMUX = 0x60;
		//break;
		//
		//default:
		//// default
		//
		//break;
		//
	//}
	//
	////start new conversion
	//ADCSRA |= 1<<ADSC;
//}

