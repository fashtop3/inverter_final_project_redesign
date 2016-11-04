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
//#include "Inverter.h"
//#include "Lcd.h"
#include "serial.h"

//Lcd lcd;
//Inverter inverter(lcd); //initialize inverter

//function prototype
void my_timer_setup();

int main(void)
{		
	DDRB |= 1<<PINB3;
	
	//cli(); //clear global interrupt
	sei(); //Enable Global Interrupt
	//my_timer_setup();
	_delay_ms(500); //allow boot time
	PORTB |= 1<<PINB3;
	//inverter.setSwitch(true); //power on the inverter
	
	// Initialize UART modules
	for (int i = 0; i < serialAvailable(); i++) {
		serialInit(i, BAUD(9600, F_CPU));
	}
	
	// Print Welcome Message
	
	for (int i = 0; i < serialAvailable(); i++) {
		serialWriteString(i, "Hello from UART");
		serialWrite(i, i + '0');
		serialWriteString(i, "... :)\n");
	}
	
    while (1) 
    {		
		//inverter.monitor();
			
		for (int i = 0; i < serialAvailable(); i++) {
			if (serialHasChar(i)) {
				serialWrite(i, serialGet(i));
			}
		}
		
		//serialWrite(0, 'A');
		//PORTB ^= 1<<PINB3;
		//_delay_ms(2000);
    }
}

//ISR(TIMER1_COMPA_vect)
//{
	//inverter.incrementEntryCounter();
	//inverter.emitMessage();	
//}


//ISR(ADC_vect)
//{
	//inverter.analogPinSwitching(); //allow all MCU to read from all enabled analog pin
	//ADCSRA |= 1<<ADSC; //start new conversion
//}

void my_timer_setup()
{
	//16000000/1024 = 15625
	TCCR1B |= (1<<WGM12) | (1<<CS12) | (1<<CS10);
	TIMSK |= (1<<OCIE1A);
	OCR1A = 15624; //timer overflow value set to 1sec
}
