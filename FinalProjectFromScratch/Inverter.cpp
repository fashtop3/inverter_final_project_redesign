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

Inverter* Inverter::setSwitch(bool on)
{
	if(on) 
	{
		INV_CTR |= 1<<POWER;
	}
	else 
	{
		INV_CTR &= ~(1<<POWER);		
	}
	
	return this;
}

Inverter* Inverter::setLoad(bool load)
{
	if (load)
	{
		INV_CTR |= 1<<LOAD;
	} 
	else
	{
		INV_CTR &= ~(1<<LOAD);
	}
	
	return this;
}

Inverter* Inverter::switchToMains(bool mainsOrInverter)
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
	
	return this;
}

Inverter* Inverter::setChargeEnable(bool enable)
{
	if (enable)
	{
		INV_MODE_CTR |= (1<<CHARGE_MODE);
	} 
	else
	{
		INV_MODE_CTR &= ~(1<<CHARGE_MODE);
	}
	
	return this;
}

Inverter* Inverter::chargingMode(bool upgrade /*= false*/)
{
	if (upgrade)
	{
		INV_MODE_CTR &= ~(1<<CHARGE_SELECT); //upgrade when mains is < 200 
	} 
	else
	{
		INV_MODE_CTR |= (1<<CHARGE_SELECT); //default charging >= 200
	}
	
	return this;
}

bool Inverter::isBattLow()
{
	return true;
}

bool Inverter::isBattFull()
{
	return true;
}

Inverter* Inverter::batteryMonitor()
{

	return this;
}

bool Inverter::AcInputVoltageCheck()
{
	return true;
}

Inverter* Inverter::surgeProtect()
{

	return this;
}

bool Inverter::isOverload()
{
	return true;
}

Inverter* Inverter::mainsBalanceMonitor()
{
	
	return this;
}

Inverter* Inverter::overloadMonitor()
{

	return this;
}

Inverter* Inverter::setAcAnalogValue(uint16_t value)
{
	this->analog_ac_value = value;
	return this;
}

Inverter* Inverter::setBattAnalogValue(uint16_t value)
{
	this->analog_batt_value = value;
	return this;
}

Inverter* Inverter::setOverloadAnalogValue(uint16_t value)
{
	this->analog_overload_value = value;
	return this;
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

Inverter* Inverter::analogPinSwitching()
{
	switch(ADMUX)
	{
		case 0x60: //check the pin on Analog Multiplexer Pin layout
			//conversion for AC input
			//ad_ac_readings = ADCH;
			this->setAcAnalogValue(ADCH);
			ADMUX = 0x61; //set to enable next pin for analog conversion
		break;
		
		case 0x61:
			//Conversion for Battery level
			//ad_batt_readings = ADCH;
			this->setBattAnalogValue(ADCH);
			ADMUX = 0x62;  //set to enable next pin for analog conversion
		break;
		
		case 0x62:
			//conversion for Overload
			//ad_overload_readings = ADCH;
			this->setOverloadAnalogValue(ADCH);
			ADMUX = 0x60;  //set to enable next pin for analog conversion
		break;
		
		default:
			// default
			ADMUX = 0x60; //set to first
		break;
		
	}

	return this;
}

