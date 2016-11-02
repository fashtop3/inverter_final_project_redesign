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
#define AC_OP			PINA0 // main input
#define AC_IP			PINA1 // inverter input
#define INV_LOAD		PINA2 // overload input



class Inverter
{

//functions
public:
	Inverter();
	~Inverter();
	
	Inverter* setSwitch(bool on);
	Inverter* switchToMains(bool mainsOrInverter);
	Inverter* monitor();
	
	int getAcInputReadings();
	double getBattInputReadings();
	int getOverloadInputReadings();
	Inverter* analogPinSwitching();
	
	int getEntryCounter();
	void incrementEntryCounter();
	
protected:

	Inverter* setLoad(bool attach);
	Inverter* setChargeEnable(bool enable);
	Inverter* chargingMode(bool upgrade = false);
	
	bool isBattLow();
	bool isBattFull();
	
	bool AcInputVoltageCheck();
	bool isOverload();
	
	Inverter* setAcAnalogValue(uint16_t value);
	Inverter* setBattAnalogValue(uint16_t value);
	Inverter* setOverloadAnalogValue(uint16_t value);
	
private:
	Inverter( const Inverter &c );
	Inverter& operator=( const Inverter &c );
	void resetChargeSelectionControl();
	const double BATT_LOW_LEVEL;
	const double BATT_FULL_LEVEL;
	bool chargeUpgrade = true;
	bool isUpgraded = true;
	bool isOverloaded = false;
	bool isLoadEnabled = false;
	bool hasLowBatt = false;
	bool isModeSet;
	bool isMains;
	bool isCharging;
	int entryCounter1;
	int entryCounter2;
	int entryCounter3;
	int entryCounter4;
	int entryCounter5;

	//analog readings
private:
	volatile static uint16_t analog_ac_value;
	volatile static uint16_t analog_batt_value;
	volatile static uint16_t analog_overload_value;
	
	int mainsStandby;
}; //Inverter

#endif //__INVERTER_H__
