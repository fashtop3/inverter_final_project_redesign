/* 
* InvSIM800.cpp
*
* Created: 11/8/2016 3:03:29 PM
* Author: Ayodeji
*/


#include "invSIM800.h"

// default constructor
InvSIM800::InvSIM800()
{
} //InvSIM800

// default destructor
InvSIM800::~InvSIM800()
{
} //~InvSIM800

size_t InvSIM800::readline(char *buffer, size_t max, uint16_t timeout)
{
	 uint16_t idx = 0;
	 while (--timeout) {
		 while (_serial.available()) {
			 char c = (char) _serial.read();
			 if (c == '\r') continue;
			 if (c == '\n') {
				 if (!idx) continue;
				 timeout = 0;
				 break;
			 }
			 if (max - idx) buffer[idx++] = c;
		 }

		 if (timeout == 0) break;
		 delay(1);
	 }
	 buffer[idx] = 0;
	 return idx;
}
