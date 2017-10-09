/* 
* Inverter.cpp
*
* Created: 11/1/2016 8:28:02 AM
* Author: Ayodeji
*/

//#define F_CPU 16000000UL
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Inverter.h"
#include <string.h>


#ifndef LCD_INIT
#define LCD_INIT
	Lcd lcd;
#endif

volatile uint16_t Inverter::__analog_ac_value__ = 0;
volatile uint16_t Inverter::__analog_batt_value__ = 0;
volatile uint16_t Inverter::__analog_overload_value__ = 0;


void Inverter::setServerResponse(const uint8_t *serverSwitch)
{
		_serverPort = (volatile char *)serverSwitch;
}

uint8_t Inverter::getEntryCounter()
{
	return _entryCounter1;
}

void Inverter::incrementEntryCounter()
{
	if (_entryCounter1 < 5)
	{
		if(!_isMains)
		{
			++_entryCounter1;
		}
	} 
	
	//entryCounter2 for switching charging
	if (_entryCounter2 < 8)
	{
		if(!_isCharging)
		{
			++_entryCounter2;
		}
	} 
	
	//entryCounter3 for switching charging mode
	if (_entryCounter3 < 10)
	{
		if(!_isModeSet)
		{
			++_entryCounter3;
		}
	}
	
	//entryCounter4 for switching overload
	if (_entryCounter4 < 5)
	{
		if(_isOverloaded)
		{
			++_entryCounter4;
		}
	}	
	
	//entryCounter5 for switching low batt
	if (_entryCounter5 < 5)
	{
		if(_hasLowBatt)
		{
			++_entryCounter5;
		}
	}
	
	if (!_isMains && getSwitchSet() && _load_delay < 6) _load_delay++;
}

// default constructor
Inverter::Inverter()
	:__BATT_LOW_LEVEL__(10.5),
	 __BATT_FULL_LEVEL__(14.5),
	 __OVERLOAD_VAL__(OVERLOAD_DEFAULT),
	 //lcd(lcd),
	 _isCharging(false), 
	 _isModeSet(false),
	 _entryCounter1(1),
	 _entryCounter2(1),
	 _entryCounter3(1),
	 _entryCounter4(1),
	 _entryCounter5(1),
	 _load_delay(1)
{
	//*_serverPort = 0;
	//initialize analog holding variables
	this->__setAcAnalogValue(0)
		->__setBattAnalogValue(0)
		->__setOverloadAnalogValue(0);
	
	__INVERTER_INT__ = MAINS_INT_vect;
	
	cli();
		
	//configure inverter mode
	INV_MODE_DIR |= (1<<CHANGE_OVER) | (1<<CHARGE_MODE) | (1<<CHARGE_SELECT);
		
	//enable pins for output
	INV_DIR |= (1<<POWER) | (1<<LOAD);
		
	//enable pins for input
	//INV_DIR &= ~( (1<<BATT_LOW) | (1<<BATT_FULL) | (1<<AC_PIN) | (1<<OVERLOAD) );
	
	//module control
	INV_DIR &= ~(1<<SIM_MODULE | 1<<POWER_BUTTON);
	INV_CTR &= ~(1<<SIM_MODULE); //set it to low
	
	INV_CTR |= (1<<POWER_BUTTON);
	//INV_CTR &= ~(1<<SIM_MODULE);
		
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
// 		
	ADCSRA |= 1<<ADSC; //start first conversion
// 		
// 		
	//powerOn(); //power the
	//changeToInverter();
		
	_delay_ms(50); //delays for 500ms for stabilize
	//outputLoad(); //put load on by default

} //Inverter


void Inverter::__resetChargeSelectionControl()
{
	if(_isModeSet && _entryCounter3 == 10)
	{
		_entryCounter3 = 5;
		_isModeSet = false;
	}
}

// default destructor
Inverter::~Inverter()
{
} //~Inverter

/**
 * \Inverter power switch control 
 * 
 * \param on boolean
 * 
 * \return Inverter::Inverter*
 */
Inverter* Inverter::setSwitch(bool on)
{
	if(on) 
	{
		if (__isBattLow())
		{			
			if (_entryCounter5 == 5)
			{
				setSwitch(false);
			}
		}
		else
		{
			//__restoreEventFor(INT_BATTERY_LOW_vect);
			if (_load_delay == 6) _load_delay = 1;
			INV_CTR &= ~(1<<POWER); //negative logic to Power ON
			_isPowerSet = true;
		}
		
	}
	else
	{
		if (_hasLowBatt) {
			__saveCurrentEventFor(INT_CHARGE_REQ_vect);
		} else {
			__restoreEventFor(INT_CHARGE_REQ_vect);
		}
		INV_CTR |= 1<<POWER; //negative logic to Power Off
		_isPowerSet = false;
	}
	
	return this;
} 


/**
 * \brief 
 *  Get if inverter switch 
 * is enabled
 * \return bool
 */
bool Inverter::getSwitchSet()
{
	return _isPowerSet;
}

/**
 * \Inverter Loading control 
 * 
 * \param load boolean
 * 
 * \return Inverter::Inverter*
 */
Inverter* Inverter::__setLoad(bool load)
{
	if (load)
	{		
		if (!_isMains && __isOverload())
		{
			//emit message here
			//Todo: work around delay here
			if (_entryCounter4 == 5)
			{
				__setLoad(false);
			}
		} 
		else
		{   
			if (_load_delay == 5)
			{
				INV_CTR |= 1<<LOAD;
				_isLoadSet = true;
			}
		}
	} 
	else
	{
		_load_delay = 1;
		INV_CTR &= ~(1<<LOAD);
		_isLoadSet = false;
	}
	
	return this;
}


/**
 * \brief 
 * Get if load is enabled
 * \return bool
 */
bool Inverter::getLoadSet()
{
	return _isLoadSet;
}

/**
 * \Inverter source switching control 
 * \if true it switches to mains
 * \else it switches to inverter
 * \this also prevents bridging 
 * \so neccesities are considered for switching
 *
 * \param mainsOrInverter boolean
 * 
 * \return Inverter::Inverter*
 */
Inverter* Inverter::switchToMains(bool mainsOrInverter)
{
	if (mainsOrInverter)
	{
		_isMains = true;
		_hasLowBatt = false; //clear tag
		setSwitch(false); //switch off inverter
		INV_MODE_CTR |=	(1<<CHANGE_OVER);
		if (_entryCounter2 == 8)
		{
			//__setChargeEnable((bool *)true); 
			__setChargeEnable(&mainsOrInverter); 
			_isCharging = mainsOrInverter;
		}
		_load_delay = 5;
		//__setLoad(true); //allowing automatic backup::: effective if inverter is powered on
	} 
	else
	{
		_isMains = false;
		_isFullyCharged = false; //battery charge no longer pristine because it has switched to inverter
		INV_MODE_CTR &= ~(1<<CHANGE_OVER); //change over to inverter
		__setChargeEnable(false);
		_isCharging = false;
	}
	
	return this;
}

/**
 * \Inverter charging control 
 * 
 * \param enable boolean
 * 
 * \return Inverter::Inverter*
 */
Inverter* Inverter::__setChargeEnable(bool enable)
{
	//Todo: check the last fully charge state to prevent recursive charge
	
	if(enable && __isBattFull())
	{
		//emit message battery full
		_isFullyCharged = true;
		__setChargeEnable(!enable); //recall
		return this;
	}
		
	if (enable)
	{
		if (!_isFullyCharged) //recheck
		{
			INV_MODE_CTR |= (1<<CHARGE_MODE);
		}
		//then set which mode to use 
		if (getAcInputReadings() < 200)
		{
			__resetChargeSelectionControl();
			__chargingMode(_chargeUpgrade);
		} 
		else
		{
			__resetChargeSelectionControl();
			__chargingMode(!_chargeUpgrade);
		}
	} 
	else
	{
		INV_MODE_CTR &= ~(1<<CHARGE_MODE);
	}
	
	return this;
}

/**
 * \Charging selection mode if it needs
 * \charging upgrade for low input voltage 
 * 
 * \param upgrade boolean
 * 
 * \return Inverter::Inverter*
 */
Inverter* Inverter::__chargingMode(bool upgrade /*= false*/)
{
	if (_entryCounter3 == 10)
	{
		if (upgrade)
		{
			INV_MODE_CTR &= ~(1<<CHARGE_SELECT); //upgrade when mains is < 200
			_isUpgraded = upgrade;
		}
		else
		{
			INV_MODE_CTR |= (1<<CHARGE_SELECT); //default charging >= 200
			_isUpgraded = upgrade;
		}
		_isModeSet = true;
	}
	
	return this;
}

Inverter* Inverter::__remoteSourceOrBypass()
{
	//if (isModuleAvailable())
	//{
		if (*_serverPort == 1)
		{
			return setSwitch(!_hasLowBatt);
		}
		else if (*_serverPort == 0)
		{
			__setLoad(false);
			return setSwitch(false);
		}
		
		return this;
	//}

	//return setSwitch(!_hasLowBatt);
}

bool Inverter::isModuleAvailable()
{
	//return true;
	return (INV_CTR_READ & (1<<SIM_MODULE));
}

//char ac_str[4];
//char batt_str[6];
//char load_str[4];
//itoa(getAcInputReadings(), ac_str, 10);
//dtostrf(getBattInputReadings(), 5, 2, batt_str);
//itoa(getOverloadInputReadings(), load_str, 10);
//
//char paradata[100] = "?t=p"; // &a=190&b=78&c=1&l=40";
//strcat(paradata, "&a=");
//strcat(paradata, ac_str);

//strcat(paradata, "&b=");
//strcat(paradata, batt_str);
//
//strcat(paradata, "&c=");
//strcat(paradata, (_isCharging)?"1":"0");
//
//strcat(paradata, "&l=");
//strcat(paradata, load_str);


char * Inverter::data()
{
	char ac_str[4];
	char batt_str[6];
	char load_str[4];
	itoa(getAcInputReadings(), ac_str, 10);
	dtostrf(getBattInputReadings(), 5, 2, batt_str);
	itoa(getOverloadInputReadings(), load_str, 10);
	
	char paradata[100] = "DATA:"; // &a=190&b=78&c=1&l=40";
	strcat(paradata, ac_str);

	strcat(paradata, ",");
	strcat(paradata, batt_str);

	strcat(paradata, ",");
	strcat(paradata, load_str);
		
	strcat(paradata, ",");
	strcat(paradata, (_isCharging)?"1":"0");
	
	strcat(paradata, ",");
	strcat(paradata, getSwitchSet()?"1":"0");	
	
	strcat(paradata, ",");
	strcat(paradata, (getLoadSet())?"1":"0");
	
	//char ac_str[4];
	//char batt_str[6];
	//char load_str[4];
	//itoa(getAcInputReadings(), ac_str, 10); 
	//dtostrf(getBattInputReadings(), 5, 2, batt_str);
	//itoa(getOverloadInputReadings(), load_str, 10);
	//
	//char paradata[100] = "DATA:"; // &a=190&b=78&c=1&l=40"; //DATA:190,15.5,35,1 
	//strcat(paradata, ac_str);
	//strcat(paradata, ",");
	//strcat(paradata, batt_str);
	//strcat(paradata, ",");
	//strcat(paradata, load_str);
	//strcat(paradata, ",");
	//strcat(paradata, (_isCharging)?"1":"0");
	//strcat(paradata, ",");
	//strcat(paradata, getSwitchSet()?"1":"0");
	//strcat(paradata, ",");
	//strcat(paradata, getLoadSet()?"1":"0");
	
	
	return paradata;	
}

bool Inverter::__isBattLow()
{
	if (getBattInputReadings() < __BATT_LOW_LEVEL__)
	{
		_hasLowBatt = true;
		return true;
	} 
	else
	{
		_entryCounter5 = 1;
		_hasLowBatt = false;
		return false;
	}
}

bool Inverter::__isBattFull()
{
	return (getBattInputReadings() > __BATT_FULL_LEVEL__);
}

bool Inverter::__AcInputVoltageCheck()
{
	//switch source to inverter if input ac is less or if too high
	return ( (getAcInputReadings() <= 160) || (getAcInputReadings() >= 240) );
}

bool Inverter::__isOverload()
{
	if (getOverloadInputReadings() > __OVERLOAD_VAL__)
	{
		_isOverloaded = true;
		return true;
	} 
	else
	{
		_isOverloaded = false;
		_entryCounter4 = 1;
		return false;
	}
}

Inverter* Inverter::__setAcAnalogValue(uint16_t value)
{
	Inverter::__analog_ac_value__ = value;
	return this;
}

Inverter* Inverter::__setBattAnalogValue(uint16_t value)
{
	this->__analog_batt_value__ = value;
	return this;
}

Inverter* Inverter::__setOverloadAnalogValue(uint16_t value)
{
	this->__analog_overload_value__ = value;
	return this;
}


/**
 * \This method is called every second 
 * \ as configured in the main.cpp ISR(TIMER1_COMPA_vect)
 * \ It controls how messages are displayed on LCD
 *
 * \return void
 */
void Inverter::emitMessage()
{
	switch(__INVERTER_INT__) 
	{
		case CLEAR_SCR_INT_vect:
			lcd.clScr();
			break;
		case INT_BATTERY_LOW_vect:
			__messageBattLow();
			break;
		case MAINS_INT_vect:
			__mainInformation();
			break;
		case INT_CHARGE_REQ_vect:
			__chargeRequired();
			break;
		
		default:
			//lcd.clScr();
			break;
	}
}


/**
 * \Battery message is designed here 
 * 
 * \return void
 */
void Inverter::__messageBattLow()
{
	if(!_isMains) //only emit message when source is inverter
	{
		lcd.printStringToLCD(1, 1, "BATTERY LOW!!!");
		lcd.printStringToLCD(1, 2, "BATT:");
		
		//Todo: if value flickers try introducing a variable to delay
		lcd.printDoubleToLCD(7, 2, getBattInputReadings(), 4, 1);
		lcd.send_A_String("v");
		lcd.send_A_String(" ");
	}
}

void Inverter::__chargeRequired()
{
	if(!_isMains) //only emit message when source is inverter
	{
		lcd.printStringToLCD(1, 1, "BATTERY LOW!!!");
		lcd.printStringToLCD(1, 2, "CHARGE REQUIRED");
	}
}

/**
 * \This method is called from  emitMessage()
 * It writes various messages on the LCD
 * 
 * \return void
 */
void Inverter::__mainInformation()
{		
	__text_mains();
	__text_load();
	__text_battery();
}

void Inverter::__text_load()
{
	if (_isOverloaded)
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

void Inverter::__text_battery()
{
	static int styleCounter = 0;
	if (_isMains)
	{
		if (_isFullyCharged)
		{
			lcd.printStringToLCD(7, 1, "BATT ");
			lcd.printStringToLCD(7, 2, "FULL ");
		} 
		else
		{
			switch(styleCounter)
			{
				case 0: lcd.printStringToLCD(7, 1, "BATT "); break;
				case 1: lcd.printStringToLCD(7, 1, ".    "); break;
				case 2: lcd.printStringToLCD(7, 1, "..   "); break;
				case 3: lcd.printStringToLCD(7, 1, "...  "); break;
				case 4: lcd.printStringToLCD(7, 1, ".... "); break;
				case 5: lcd.printStringToLCD(7, 1, "....."); break;
				default:
				break;
			}
						
			lcd.printDoubleToLCD(7, 2, getBattInputReadings(), 4, 1);
			lcd.send_A_String("v");
			lcd.send_A_String(" ");
						
			(styleCounter == 5) ? styleCounter = 0 : styleCounter++;
		}
	}
	else
	{
		lcd.printStringToLCD(7, 1, "BATT ");
		lcd.printDoubleToLCD(7, 2, getBattInputReadings(), 4, 1);
		lcd.send_A_String("v");
		lcd.send_A_String(" ");
	}
}

void Inverter::__text_mains()
{
	lcd.printStringToLCD(1, 1, "MAINS");
	lcd.printIntToLCD(1, 2, getAcInputReadings(), 3);
	lcd.send_A_String("v");
	lcd.send_A_String(" ");
}

/**
 * \Checks if the event parsed is not the same  
 * \as the current inverter interrupt variable
 * \if not then it copy inverter interrupt 
 * \holder to a temp var set as required
 * \param e Event enumeration 
 * 
 * \return void
 */
void Inverter::__saveCurrentEventFor(Event e)
{
	//copy and save interrupt
	if (e != __INVERTER_INT__)
	{
		//lcd.printIntToLCD(1, 2, INVERTER_INT, 2);
		__INVERTER_INT_CP__ = __INVERTER_INT__;
		__INVERTER_INT__ = e;
		//lcd.printIntToLCD(4, 2, INVERTER_INT, 2);
		lcd.clScr();
	}
}

/**
 * \Restore previously stored event from  
 * \temporary holder if the e & inv_int matched
 *
 * \param e Event enum
 * 
 * \return void
 */
void Inverter::__restoreEventFor(Event e)
{
	if (e == __INVERTER_INT__)
	{
		//lcd.printIntToLCD(1, 2, INVERTER_INT, 2);
		__INVERTER_INT__ = __INVERTER_INT_CP__;
		//lcd.printIntToLCD(4, 2, INVERTER_INT, 2);
		lcd.clScr();
	}
}

/**
 * \Resets event 
 * 
 * \param e
 * 
 * \return void
 */
void Inverter::__resetEventFor(Event e)
{
	__INVERTER_INT__ = __INVERTER_INT_CP__ = e;
}

/**
 * \Read the analog input for 
 * \Alternation Current (mains)
 * 
 * \return int
 */
uint16_t Inverter::getAcInputReadings()
{
	double readings = static_cast<double> (this->__analog_ac_value__);
	return ( ( readings / 51 ) * 100 );
}

/**
 * \Read the analog input for  
 * \Battery level
 * 
 * \return double
 */
double Inverter::getBattInputReadings()
{
	double readings = static_cast<double> (this->__analog_batt_value__);
	return ( ( readings / 51.0 ) * 10 );
}

/**
 * \Read the analog input for  
 * \Inverter loading
 * 
 * \return int
 */
uint8_t Inverter::getOverloadInputReadings()
{
	double readings = static_cast<double> (this->__analog_overload_value__);
	return ( ( readings / 51 ) * 20 );
}


/**
 * \brief returns the default 
 *  Overload Level
 * \return uint8_t
 */
short unsigned int *Inverter::getOverloadDefault()
{
	return (short unsigned int *)OVERLOAD_DEFAULT;
}

void Inverter::setOverload(const short unsigned *val)
{
	__OVERLOAD_VAL__ = *val > 75 ? 75 : *val;
}

/**
 * \This method is called from main.cpp ISR(ADC_vect)
 * \Analog Digital Conversion 
 * \and switches the pin to read from
 * \after every analog data read
 *
 * \return Inverter::Inverter*
 */
Inverter* Inverter::analogPinSwitching()
{
	switch(ADMUX)
	{
		case 0x60: //check the pin on Analog Multiplexer Pin layout
			//conversion for AC input
			//ad_ac_readings = ADCH;
			this->__setAcAnalogValue(ADCH);
			ADMUX = 0x61; //set to enable next pin for analog conversion
		break;
		
		case 0x61:
			//Conversion for Battery level
			//ad_batt_readings = ADCH;
			this->__setBattAnalogValue(ADCH);
			ADMUX = 0x62;  //set to enable next pin for analog conversion
		break;
		
		case 0x62:
			//conversion for Overload
			//ad_overload_readings = ADCH;
			this->__setOverloadAnalogValue(ADCH);
			ADMUX = 0x60;  //set to enable next pin for analog conversion
		break;
		
		default:
			// default
		break;
		
	}

	return this;
}


/**
 * \This is the main inverter service method
 * \This method is called from main.cpp while(1)
 * \this controls all the inverter functions
 * \and also prevent surge
 *
 * \return Inverter::Inverter*
 */
Inverter* Inverter::monitor()
{		
	if (__AcInputVoltageCheck()) //check low or high voltage
	{
		switchToMains(false)
			->__remoteSourceOrBypass()
			->__setLoad(true);
				
		_entryCounter1 = 1;
		_entryCounter2 = 1;
		_entryCounter3 = 1;
	} 
	else
	{
		__saveCurrentEventFor(MAINS_INT_vect);
		//workaround a delay count here
		if (_entryCounter1 == 5)
		{
			switchToMains(true);
			__setLoad(true);
						
			_entryCounter4 = 1;
		}
	}
	
	return this;
}

