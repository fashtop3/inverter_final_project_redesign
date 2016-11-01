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

Inverter inverter; //initialize inverter
Lcd lcd;

//function prototype
void my_timer_setup();

int main(void)
{
	//cli(); //clear global interrupt
	//sei(); //Enable Global Interrupt
	my_timer_setup();
	
	//testing inverter api
	inverter.setSwitch(true);
	//->setLoad(true)
	//->switchToMains(true)
	//->chargingMode(true);
	
	
    /* Replace with your application code */
    while (1) 
    {		
		lcd.printIntToLCD(1, 1, inverter.getAcInputReadings(), 4);
		lcd.printDoubleToLCD(1, 2, inverter.getBattInputReadings(), 4, 2);
		lcd.printIntToLCD(11, 2, inverter.getOverloadInputReadings(), 3);
		//_delay_ms(500);
		//lcd->clScr();
    }
	
	//delete inverter, lcd;
}


ISR(TIMER1_COMPA_vect)
{
	inverter.monitor();
	
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
	//allow all MCU to read from all enabled analog pin
	inverter.analogPinSwitching();
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
