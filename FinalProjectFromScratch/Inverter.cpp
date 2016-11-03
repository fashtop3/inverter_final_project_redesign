/* 
* Inverter.cpp
*
* Created: 11/1/2016 8:28:02 AM
* Author: Ayodeji
*/

#include <avr/interrupt.h>
#include "Inverter.h"

volatile uint16_t Inverter::analog_ac_value = 0;
volatile uint16_t Inverter::analog_batt_value = 0;
volatile uint16_t Inverter::analog_overload_value = 0;

int Inverter::getEntryCounter()
{
	return entryCounter1;
}

void Inverter::incrementEntryCounter()
{
	if (entryCounter1 < 5)
	{
		if(!isMains)
		{
			++entryCounter1;
		}
	} 
	
	//entryCounter2 for switching charging
	if (entryCounter2 < 8)
	{
		if(!isCharging)
		{
			++entryCounter2;
		}
	} 
	
	//entryCounter3 for switching charging mode
	if (entryCounter3 < 10)
	{
		if(!isModeSet)
		{
			++entryCounter3;
		}
	}
	
	//entryCounter4 for switching overload
	if (entryCounter4 < 5)
	{
		if(isOverloaded)
		{
			++entryCounter4;
		}
	}	
	
	//entryCounter5 for switching low batt
	if (entryCounter5 < 5)
	{
		if(hasLowBatt)
		{
			++entryCounter5;
		}
	}
}

// default constructor
Inverter::Inverter(Lcd &lcd)
	:BATT_LOW_LEVEL(10.5),
	 BATT_FULL_LEVEL(14.5),
	 lcd(lcd),
	 isCharging(false), 
	 isModeSet(false),
	 entryCounter1(1),
	 entryCounter2(1),
	 entryCounter3(1),
	 entryCounter4(1),
	 entryCounter5(1)
{
	//initialize analog holding variables
	this->setAcAnalogValue(0)
		->setBattAnalogValue(0)
		->setOverloadAnalogValue(0);
	
	INVERTER_INT = MAINS_INT_vect;
	
	cli();
		
	//configure inverter mode
	INV_MODE_DIR |= (1<<CHANGE_OVER) | (1<<CHARGE_MODE) | (1<<CHARGE_SELECT);
		
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


void Inverter::resetChargeSelectionControl()
{
	if(isModeSet && entryCounter3 == 10)
	{
		entryCounter3 = 5;
		isModeSet = false;
	}
}

// default destructor
Inverter::~Inverter()
{
} //~Inverter

Inverter* Inverter::setSwitch(bool on)
{
	if(on) 
	{		
		if (isBattLow())
		{
			//Todo: emit message here
			if(!isMains)
			{
				saveCurrentEventFor(INT_BATTERY_LOW_vect);
			}
			if (entryCounter5 == 5)
			{
				setSwitch(false);
			}
		} 
		else
		{
			restoreEventFor(INT_BATTERY_LOW_vect);
			INV_CTR |= 1<<POWER;
		}
		
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
		if (!isMains && isOverload())
		{
			//emit message here
			//Todo: work around delay here
			if (entryCounter4 == 5)
			{
				setLoad(false);
			}
		} 
		else
		{
			INV_CTR |= 1<<LOAD;
		}
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
		isMains = true;
		setSwitch(false); //switch off inverter
		INV_MODE_CTR |=	(1<<CHANGE_OVER);
		if (entryCounter2 == 8)
		{
			setChargeEnable(true); 
			isCharging = true;
		}
	} 
	else
	{
		isMains = false;
		INV_MODE_CTR &= ~(1<<CHANGE_OVER); //change over to inverter
		setChargeEnable(false);
		isCharging = false;
	}
	
	return this;
}

Inverter* Inverter::setChargeEnable(bool enable)
{
	//Todo: check the last fully charge state to prevent recursive charge
	
	if(enable && isBattFull())
	{
		//emit message battery full and init a var and can only be track back when ac is off
		setChargeEnable(!enable); //recall
		return this;
	}
	
	if (enable)
	{
		INV_MODE_CTR |= (1<<CHARGE_MODE);
		//then set which mode to use 
		if (getAcInputReadings() < 200)
		{
			resetChargeSelectionControl();
			chargingMode(chargeUpgrade);
		} 
		else
		{
			resetChargeSelectionControl();
			chargingMode(!chargeUpgrade);
		}
	} 
	else
	{
		INV_MODE_CTR &= ~(1<<CHARGE_MODE);
	}
	
	return this;
}

Inverter* Inverter::chargingMode(bool upgrade /*= false*/)
{
	if (entryCounter3 == 10)
	{
		if (upgrade)
		{
			INV_MODE_CTR &= ~(1<<CHARGE_SELECT); //upgrade when mains is < 200
			isUpgraded = upgrade;
		}
		else
		{
			INV_MODE_CTR |= (1<<CHARGE_SELECT); //default charging >= 200
			isUpgraded = upgrade;
		}
		isModeSet = true;
	}
	
	return this;
}

bool Inverter::isBattLow()
{
	if (getBattInputReadings() < BATT_LOW_LEVEL)
	{
		hasLowBatt = true;
		return true;
	} 
	else
	{
		entryCounter5 = 1;
		hasLowBatt = false;
		return false;
	}
}

bool Inverter::isBattFull()
{
	return (getBattInputReadings() > BATT_FULL_LEVEL);
}

bool Inverter::AcInputVoltageCheck()
{
	//switch source to inverter if input ac is less or if too high
	return ( (getAcInputReadings() <= 160) || (getAcInputReadings() >= 240) );
}

bool Inverter::isOverload()
{
	if (getOverloadInputReadings() > 75)
	{
		isOverloaded = true;
		return true;
	} 
	else
	{
		isOverloaded = false;
		entryCounter4 = 1;
		return false;
	}
}

Inverter* Inverter::setAcAnalogValue(uint16_t value)
{
	Inverter::analog_ac_value = value;
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

void Inverter::emitMessage()
{
	switch(INVERTER_INT) 
	{
		case CLEAR_SCR_INT_vect:
			lcd.clScr();
			break;
		case INT_BATTERY_LOW_vect:
			messageBattLow();
			break;
		case MAINS_INT_vect:
			mainInformation();
			break;
		
		default:
			//lcd.clScr();
			break;
	}
}

void Inverter::messageBattLow()
{
	if(!isMains) //only emit message when source is inverter
	{
		lcd.printStringToLCD(1, 1, "Battery Low!!!");
		lcd.printStringToLCD(1, 2, "BATT:");
		
		//Todo: if value flickers try introducing a variable to delay
		lcd.printDoubleToLCD(7, 2, getBattInputReadings(), 4, 1);
		lcd.send_A_String("v");
		lcd.send_A_String(" ");
	}
}

void Inverter::mainInformation()
{		
	text_mains();
	text_load();
	text_battery();
}

void Inverter::text_load()
{
	if (isOverloaded)
	{
		lcd.printStringToLCD(13, 1, "OVER");
		lcd.printStringToLCD(13, 2, "LOAD");
	} 
	else
	{
		lcd.printStringToLCD(13, 1, "LOAD");
		lcd.printIntToLCD(13, 2, getOverloadInputReadings(), 3);
		lcd.send_A_String("%");
		lcd.send_A_String(" ");
	}
}

void Inverter::text_battery()
{
	static int styleCounter = 0;
	lcd.printStringToLCD(7, 1, "BATT ");
	if (isMains)
	{
		if (isBattFull())
		{
			lcd.printStringToLCD(7, 2, "FULL ");
		} 
		else
		{
			switch(styleCounter)
			{
				case 0: lcd.printStringToLCD(7, 1, ".    "); break;
				case 1: lcd.printStringToLCD(7, 1, "..   "); break;
				case 2: lcd.printStringToLCD(7, 1, "...  "); break;
				case 3: lcd.printStringToLCD(7, 1, ".... "); break;
				case 4: lcd.printStringToLCD(7, 1, "....."); break;
				default:
				break;
			}
						
			lcd.printDoubleToLCD(7, 2, getBattInputReadings(), 4, 1);
			lcd.send_A_String("v");
			lcd.send_A_String(" ");
						
			(styleCounter > 4) ? styleCounter = 0 : styleCounter++;
		}
	}
	else
	{
		lcd.printDoubleToLCD(7, 2, getBattInputReadings(), 4, 1);
		lcd.send_A_String("v");
		lcd.send_A_String(" ");
	}
}

void Inverter::text_mains()
{
	lcd.printStringToLCD(1, 1, "MAINS");
	lcd.printIntToLCD(1, 2, getAcInputReadings(), 3);
	lcd.send_A_String("v");
	lcd.send_A_String(" ");
}

void Inverter::saveCurrentEventFor(Event e)
{
	//copy and save interrupt
	if (e != INVERTER_INT)
	{
		//lcd.printIntToLCD(1, 2, INVERTER_INT, 2);
		INVERTER_INT_CP = INVERTER_INT;
		INVERTER_INT = e;
		//lcd.printIntToLCD(4, 2, INVERTER_INT, 2);
		lcd.clScr();
	}
}

void Inverter::restoreEventFor(Event e)
{
	if (e == INVERTER_INT)
	{
		//lcd.printIntToLCD(1, 2, INVERTER_INT, 2);
		INVERTER_INT = INVERTER_INT_CP;
		//lcd.printIntToLCD(4, 2, INVERTER_INT, 2);
		lcd.clScr();
	}
}

void Inverter::resetEventFor(Event e)
{
	if(e != INVERTER_INT)
	{
		lcd.clScr();
	}
	INVERTER_INT = INVERTER_INT_CP = e;
}

int Inverter::getAcInputReadings()
{
	double readings = static_cast<double> (this->analog_ac_value);
	return ( ( readings / 51 ) * 100 );
}

double Inverter::getBattInputReadings()
{
	double readings = static_cast<double> (this->analog_batt_value);
	return ( ( readings / 51.0 ) * 10 );
}

int Inverter::getOverloadInputReadings()
{
	double readings = static_cast<double> (this->analog_overload_value);
	return ( ( readings / 51 ) * 20 );
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
		break;
		
	}

	return this;
}

Inverter* Inverter::monitor()
{
		
	if (AcInputVoltageCheck()) //check low or high voltage
	{
		switchToMains(false)
			->setSwitch(true)
			->setLoad(true);
				
		entryCounter1 = 1;
		entryCounter2 = 1;
		entryCounter3 = 1;
	} 
	else
	{
		saveCurrentEventFor(MAINS_INT_vect);
		//workaround a delay count here
		if (entryCounter1 == 5)
		{
			switchToMains(true)
			->setLoad(true);
						
			entryCounter4 = 1;
		}
	}
	
	return this;
}

