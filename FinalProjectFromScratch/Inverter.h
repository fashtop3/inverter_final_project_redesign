/* 
* Inverter.h
*
* Created: 11/1/2016 8:28:02 AM
* Author: Ayodeji
*/

#ifndef __INVERTER_H__
#define __INVERTER_H__

#define F_CPU 1000000UL

#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

//inverter io controls pin configurations
#define BATT_LOW		PIND2
#define BATT_FULL		PIND3
#define AC_PIN			PIND4
#define OVERLOAD		PIND5
#define POWER			PIND6
#define LOAD			PIND7
#define INV_DIR			DDRD	//Data Direction Register controlling inverter functions
#define INV_CTR			PORTD	//Voltage level IO Register

#define CHANGE_OVER		PINB3
#define CHARGE_MODE		PINB4
#define CHARGE_SELECT	PINB5
#define INV_MODE_DIR	DDRB
#define INV_MODE_CTR	PORTB

//Analog conversion pins
#define AC_OP			PINA0
#define AC_IP			PINA1
#define INV_LOAD		PINA2



class Inverter
{
//variables
public:
protected:
private:

//functions
public:
	Inverter();
	~Inverter();
	
	void setSwitch(bool on);
	void setLoad(bool attach);
	
	void switchToMains(bool mainsOrInverter);
	void setChargeEnable(bool enable);
	void chargingMode(bool upgrade = false);
	
	bool isBattLow();
	bool isBattFull();
	
	void batteryMonitor();
	bool AcInputVoltageCheck();
	void surgeProtect();
	
	bool isOverload();
	void mainsBalanceMonitor();
	
	void overloadMonitor();
	
	void setAcAnalogValue(uint16_t value);
	void setBattAnalogValue(uint16_t value);
	void setOverloadAnalogValue(uint16_t value);
	
	int getAcInputReadings();
	int getBattInputReadings();
	int getOverloadInputReadings();


protected:
private:
	Inverter( const Inverter &c );
	Inverter& operator=( const Inverter &c );

	//analog readings
	volatile uint16_t analog_ac_value;
	volatile uint16_t analog_batt_value;
	volatile uint16_t analog_overload_value;
}; //Inverter

#endif //__INVERTER_H__
