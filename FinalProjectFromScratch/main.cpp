/*
 * FinalProjectFromScratch.cpp
 *
 * Created: 11/1/2016 8:25:30 AM
 * Author : Ayodeji
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>
#include "Inverter.h"
#include "Lcd.h"
#include "serial.h"

Lcd lcd;
Inverter inverter(lcd); //initialize inverter

//function prototype
void my_timer_setup();

int main(void)
{
	cli(); //clear global interrupt
	sei(); //Enable Global Interrupt
	my_timer_setup();
	inverter.setSwitch(true); //power on the inverter
	
    while (1) 
    {		
		inverter.monitor();
    }
}


ISR(TIMER1_COMPA_vect)
{
	inverter.incrementEntryCounter();
	inverter.emitMessage();	
}


ISR(ADC_vect)
{
	inverter.analogPinSwitching(); //allow all MCU to read from all enabled analog pin
	ADCSRA |= 1<<ADSC; //start new conversion
}

void my_timer_setup()
{
	//1000000/64 = 15625
	TCCR1B |= (1<<WGM12) | (1<<CS11) | (1<<CS10);
	TIMSK |= (1<<OCIE1A);
	OCR1A = 15624; //timer overflow value set to 1sec
}
