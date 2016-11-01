/* 
* Inverter.cpp
*
* Created: 11/1/2016 8:28:02 AM
* Author: Ayodeji
*/

#include <avr/interrupt.h>
#include "Inverter.h"

//volatile uint16_t Inverter::analog_ac_value = 0;
//volatile uint16_t Inverter::analog_batt_value = 0;
//volatile uint16_t Inverter::analog_overload_value = 0;

// default constructor
Inverter::Inverter()
{
	//initialize analog holding variables
	this->setAcAnalogValue(0);
	this->setBattAnalogValue(0);
	this->setOverloadAnalogValue(0);
	
	cli();
		
	//configure inverter mode
	INV_MODE_DIR |= (1<<CHANGE_OVER) | (1<<CHARGE_MODE) | (1<<CHARGE_SELECT);
		
	//enable inverter mode pins
	//INV_MODE_CTR |= (1<<CHANGE_OVER) | (1<<CHARGE_MODE) | (1<<CHARGE_SELECT);
		
	//enable pins for output
	INV_DIR |= (1<<POWER) | (1<<LOAD);
		
	//enable pins for input
	INV_DIR &= ~( (1<<BATT_LOW) | (1<<BATT_FULL) | (1<<AC_PIN) | (1<<OVERLOAD) );
		
	//enable analog conversions
	//configure ADC
	//enable ADC interrupt flag
	//decide if 8-bit or 10-bit;
	//enable a prescaler
		
	//set ref Voltage to AVCC and Left Adjust Result to use ADCH data register
	ADMUX |= (1<<REFS0) | (1<<ADLAR); //if any changes here please correct the hexadecimal in the ISR switch statement
		
	//AD interrupt and setting prescaler based on inter oscillator or crystal oscillator
	ADCSRA |= (1<<ADIE) | (1<<ADPS2); // prescaler 16| (1<<ADPS1) | (1<<ADPS0); //turn on ADC
	ADCSRA |= (1<<ADEN); //enable ADC
		
	sei();	//Enable global interrupt
		
	ADCSRA |= 1<<ADSC; //start first conversion
		
		
	//powerOn(); //power the
	//changeToInverter();
		
	_delay_ms(500); //delays for 500ms for stabilize
	//outputLoad(); //put load on by default

} //Inverter

// default destructor
Inverter::~Inverter()
{
} //~Inverter

void Inverter::setSwitch(bool on)
{
	if(on) 
	{
		INV_CTR &= ~(1<<POWER);
	}
	else 
	{
		INV_CTR |= 1<<POWER;
	}
}

void Inverter::setLoad(bool load)
{
	if (load)
	{
		INV_CTR |= 1<<LOAD;
	} 
	else
	{
		INV_CTR &= ~(1<<LOAD);
	}
}

void Inverter::switchToMains(bool mainsOrInverter)
{
	if (mainsOrInverter)
	{
		//setSwitch(false); //switch off inverter
		INV_MODE_CTR |=	(1<<CHANGE_OVER);
		//setChargeEnable(true); 
	} 
	else
	{
		INV_MODE_CTR &= ~(1<<CHANGE_OVER); //change over to inverter
	}
}

void Inverter::setChargeEnable(bool enable)
{
	if (enable)
	{
		INV_MODE_CTR |= (1<<CHARGE_MODE);
	} 
	else
	{
		INV_MODE_CTR &= ~(1<<CHARGE_MODE);
	}
}

void Inverter::chargingMode(bool upgrade /*= false*/)
{
	if (upgrade)
	{
		INV_MODE_CTR |= (1<<CHARGE_SELECT);
	} 
	else
	{
		INV_MODE_CTR &= ~(1<<CHARGE_SELECT);
	}
}

bool Inverter::isBattLow()
{
	return true;
}

bool Inverter::isBattFull()
{
	return true;
}

void Inverter::batteryMonitor()
{

}

bool Inverter::AcInputVoltageCheck()
{
	return true;
}

void Inverter::surgeProtect()
{

}

bool Inverter::isOverload()
{
	return true;
}

void Inverter::mainsBalanceMonitor()
{
	
}

void Inverter::overloadMonitor()
{

}

void Inverter::setAcAnalogValue(uint16_t value)
{
	this->analog_ac_value = value;
}

void Inverter::setBattAnalogValue(uint16_t value)
{
	this->analog_batt_value = value;
}

void Inverter::setOverloadAnalogValue(uint16_t value)
{
	this->analog_overload_value = value;
}

int Inverter::getAcInputReadings()
{
	//double readings = static_cast<double> (this->analog_ac_value);
	return ( ( this->analog_ac_value / 51 ) * 100 );
}

double Inverter::getBattInputReadings()
{
	double readings = static_cast<double> (this->analog_batt_value);
	return ( ( readings / 51.0 ) * 10 );
}

int Inverter::getOverloadInputReadings()
{
	//double readings = static_cast<double> (this->analog_overload_value);
	return ( ( this->analog_overload_value / 51 ) * 20 );
}

