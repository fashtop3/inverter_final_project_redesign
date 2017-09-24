/* 
* Lcd.h
*
* Created: 11/1/2016 12:26:32 PM
* Author: Ayodeji
*/


//NOTE THAT THE CLOCK FREQ WITH THIS LCD IS 270KHz......
//PLEASE PERFORM NECESSARY CONVERTIONS FOR DIFFERENT CRYSTAL OSCILATOR

#ifndef __LCD_H__
#define __LCD_H__

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#define RS						PINB0
#define RW						PINB1
#define ENABLE					PINB2

#define LCDsControl				PORTB
#define LCDsControlDirection	DDRB

#define LCDsCrib				PORTC
#define LCDsCribDirection		DDRC


class Lcd
{
//variables
public:
	Lcd();
	~Lcd();
	
	Lcd* send_A_Command(unsigned char command);
	Lcd* send_A_Character(unsigned char character);
	Lcd* send_A_String(const char *str);
	Lcd* printStringToLCD(uint8_t x, uint8_t y, char *str);
	Lcd* printIntToLCD(uint8_t x, uint8_t y, int val, uint8_t len);
	Lcd* printDoubleToLCD(uint8_t x, uint8_t y, double val, uint8_t len, char prec);
	Lcd* gotoLocation(uint8_t x, uint8_t y);
	Lcd* clScr();
	
	Lcd* check_If_Busy();
	Lcd* peek_A_Boo(void);

protected:
	int firstColumnPositions[2] = {0, 64};
		
private:
	Lcd( const Lcd &c );
	Lcd& operator=( const Lcd &c );

}; //Lcd

#endif //__LCD_H__
