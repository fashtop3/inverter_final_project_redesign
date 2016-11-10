/* 
* Lcd.cpp
*
* Created: 11/1/2016 12:26:32 PM
* Author: Ayodeji
*/

#define F_CPU 16000000UL
#include <util/delay.h>
#include "Lcd.h"

//NOTE THAT THE CLOCK FREQ WITH THIS LCD IS 270KHz......
//PLEASE PERFORM NECESSARY CONVERTIONS FOR DIFFERENT CRYSTAL OSCILATOR
//Also make sure the F_CPU is defined in the main.cpp

//int Lcd::firstColumnPositions[2] = {0, 64};

// default constructor
Lcd::Lcd()
{
	//firstColumnPositions[0] = 0;
	//firstColumnPositions[1] = 64;
		
	LCDsControlDirection |= (1<<RS) | (1<<RW) | (1<<ENABLE);
	
	_delay_ms(100); 
	
	send_A_Command(0x01); 	// 0x01 clears the screen
	_delay_ms(2.5); //2 for 1MHz
	
	send_A_Command(0x38);		//8 bit mood
	_delay_ms(2.5); //50 for 1MHz
	
	send_A_Command(0b00001100);
	_delay_ms(2.5); //50 for 1MHz
} //Lcd

// default destructor
Lcd::~Lcd()
{
} //~Lcd

/** Send a command to the LCD
* \param command BYTE to inform LCD what to do
*/
Lcd* Lcd::send_A_Command(unsigned char command)
{
	check_If_Busy();
	LCDsCrib = command;
	LCDsControl &= ~(1<<RW | 1<<RS);
	peek_A_Boo();
	LCDsCrib = 0;

	return this;
}

/** Send a character to the LCD
* \param character BYTE to be displayed on the LCD
*/
Lcd* Lcd::send_A_Character(unsigned char character)
{
	check_If_Busy();
	LCDsCrib = character;
	LCDsControl &= ~(1<<RW);
	LCDsControl |= 1<<RS;
	peek_A_Boo();
	LCDsCrib = 0;

	return this;
}

/** Write  string on the LCD
* \param str String of bytes to LCD
*/
Lcd* Lcd::send_A_String(char *str)
{
	while(*str != '\0')
	{
		send_A_Character(*str++);
	}

	return this;
}

/**
* \Print string on a particular location on the LCD
*
* \param x X-axis ranging 1-16 for 16x2 LCD
* \param y Y-axis ranging 1-2 for 16x2 LCD
* \param str Strings  to write on the LCD
*/
Lcd* Lcd::printStringToLCD(uint8_t x, uint8_t y, char *str)
{
	gotoLocation(x, y);
	send_A_String(str);

	return this;
}

/**
* \This converts integer value to string for LCD
*
* \param x X-axis ranging 1-16 for 16x2 LCD
* \param y Y-axis ranging 1-2 for 16x2 LCD
* \param val Int value to display
* \param len length of the integer value
*/
Lcd* Lcd::printIntToLCD(uint8_t x, uint8_t y, int val, uint8_t len)
{
	char str[len];
	itoa(val, str, 10);
	gotoLocation(x, y);
	send_A_String(str);
	//send_A_String(" ");

	return this;
}

/**
* \Converts double to string for LCD to display
*
* \param x X-axis ranging 1-16 for 16x2 LCD
* \param y Y-axis ranging 1-2 for 16x2 LCD
* \param val Double value to display
* \param len length of double number e.g 123.62 (len = 6)
* \param prec the number of precisions  e.g 123.62 (prec = 2)
*/
Lcd* Lcd::printDoubleToLCD(uint8_t x, uint8_t y, double val, uint8_t len, char prec)
{
	char str[len];
	dtostrf(val, len, prec, str); //conversion from double to string
	
	gotoLocation(x, y);
	send_A_String(str);
	//send_A_String(" ");

	return this;
}

/**
* \brief
*
* \param x X-axis ranging 1-16 for 16x2 LCD
* \param y Y-axis ranging 1-2 for 16x2 LCD
*/
Lcd* Lcd::gotoLocation(uint8_t x, uint8_t y)
{
	send_A_Command(0x80 + firstColumnPositions[y-1] + (x-1));

	return this;
}

/**
* \Clear LCD screen
*/
Lcd* Lcd::clScr()
{
	send_A_Command(0x01);
	_delay_ms(2.5); //2 for 1MHz
	//send_A_Command(0b00001110);
	send_A_Command(0b00001100); //hiding cursor
	_delay_ms(2.5); //100 for 1MHz //allow the command to execute to avoid screen flicker

	return this;
}

/**
* Check if Mr LCD is busy with its own task
*/
Lcd* Lcd::check_If_Busy()
{
	LCDsCribDirection = 0; //input mode
	LCDsControl |= 1<<RW;
	LCDsControl &= ~(1<<RS);
	
	while(LCDsCrib >= 0x80) //is busy if greater or equal to
	{
		peek_A_Boo();
	}
	
	LCDsCribDirection = 0xFF; //reset it back to output

	return this;
}

/**
* Place a hold by flashing the enable light for LCD to read a byte
*/
Lcd* Lcd::peek_A_Boo(void)
{
	LCDsControl |= 1<<ENABLE;
	asm volatile ("nop");
	asm volatile ("nop");
	_delay_ms(2.5);
	LCDsControl &= ~(1<<ENABLE);

	return this;
}

