/* 
* Inverter.h
*
* Created: 11/1/2016 8:28:02 AM
* Author: Ayodeji
*/


#ifndef __INVERTER_H__
#define __INVERTER_H__

#include <stdbool.h>
#include <avr/io.h>
#include <stdio.h>
#include "Lcd.h"

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


enum Event {
	NO_INT_vect = 0x0,
	CLEAR_SCR_INT_vect = 0x12,
	INT_BATTERY_LOW_vect = 0x23,
	MAINS_INT_vect = 0x24,
};

class Inverter
{

public:

//functions
public:
	Inverter(Lcd &lcd);
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
	
	void emitMessage();
	
protected:

	Inverter* __setLoad(bool attach);
	Inverter* __setChargeEnable(bool enable);
	Inverter* __chargingMode(bool upgrade = false);
	
	bool __isBattLow();
	bool __isBattFull();
	
	bool __AcInputVoltageCheck();
	bool __isOverload();
	
	Inverter* __setAcAnalogValue(uint16_t value);
	Inverter* __setBattAnalogValue(uint16_t value);
	Inverter* __setOverloadAnalogValue(uint16_t value);
	
	void __messageBattLow();
	void __mainInformation();
	
	void __text_load();
	void __text_battery();
	void __text_mains();

	void __saveCurrentEventFor(Event e);
	void __restoreEventFor(Event e);
	void __resetEventFor(Event e);
private:
	Inverter( const Inverter &c );
	Inverter& operator=( const Inverter &c );
	void __resetChargeSelectionControl();

private:
	Lcd &_lcd;
	int __OVERLOAD_VAL__;
	int __INVERTER_INT__;
	int __INVERTER_INT_CP__;
	const double __BATT_LOW_LEVEL__;
	const double __BATT_FULL_LEVEL__;
	volatile bool _chargeUpgrade = true;
	volatile bool _isUpgraded = true;
	volatile bool _isOverloaded = false;
	volatile bool _isLoadEnabled = false;
	volatile bool _hasLowBatt = false;
	volatile bool _isFullyCharged = false;
	volatile bool _isModeSet;
	volatile bool _isMains;
	volatile bool _isCharging;
	volatile int _entryCounter1;
	volatile int _entryCounter2;
	volatile int _entryCounter4;
	volatile int _entryCounter3;
	volatile int _entryCounter5;

	//analog readings
	volatile static uint16_t __analog_ac_value__;
	volatile static uint16_t __analog_batt_value__;
	volatile static uint16_t __analog_overload_value__;
	
	int mainsStandby;
}; //Inverter

#endif //__INVERTER_H__
