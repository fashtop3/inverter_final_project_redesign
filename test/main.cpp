/*
 * test.cpp
 *
 * Created: 11/27/2017 3:45:17 PM
 * Author : Ayodeji
 */ 

#include <avr/io.h>

#define POWER_BUTTON	PIND3
#define POWER			PIND6
#define LOAD			PIND7
#define INV_DIR			DDRD //module control pin	//Data Direction Register controlling inverter functions
#define INV_CTR			PORTD //module port	//Voltage level IO Register
#define INV_CTR_READ	PIND

#define CHANGE_OVER		PINB3
#define CHARGE_MODE		PINB4
#define CHARGE_SELECT	PINB5
#define INV_MODE_DIR	DDRB
#define INV_MODE_CTR	PORTB

//sim module
#define SIM_MODULE			PIND2 //to check if sim module is accessible
#define MODULE_HARD_RESET	PIND4 //Hardware Reset for ATMEGA328 controlling the sim module

//Analog conversion pins
#define AC_OP			PINA0 // main input
#define AC_IP			PINA1 // inverter input
#define INV_LOAD		PINA2 // overload input

#define  OVERLOAD_DEFAULT	75



int main(void)
{
		//configure inverter mode
		INV_MODE_DIR |= (1<<CHANGE_OVER) | (1<<CHARGE_MODE) | (1<<CHARGE_SELECT);
		
		//enable pins for output
		INV_DIR |= (1<<POWER) | (1<<LOAD);
		
		//module control
		INV_DIR &= ~(1<<SIM_MODULE | 1<<POWER_BUTTON);
		INV_DIR |= 1<<MODULE_HARD_RESET;
		
		INV_CTR &= ~(1<<SIM_MODULE); //set it to low
		INV_CTR |= 1<<MODULE_HARD_RESET;
		
		INV_CTR |= (1<<POWER_BUTTON);
		
		INV_DIR &= (1<<POWER) | (1<<LOAD);


    /* Replace with your application code */
    while (1) 
    {
    }
}

