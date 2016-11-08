/* 
* InvSIM800.h
*
* Created: 11/8/2016 3:03:29 PM
* Author: Ayodeji
*/


#ifndef __INVSIM800_H__
#define __INVSIM800_H__
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "serial.h"
#include <stdbool.h>

#ifdef F
#undef F
#define F(s) (s)
#else
#define F(s) (s)
#endif

#define SIM800_BAUD 9600 //115200
#define SIM800_CMD_TIMEOUT 30000
#define SIM800_SERIAL_TIMEOUT 1000
#define SIM800_BUFSIZE 64

class InvSIM800
{

//functions
public:
	InvSIM800();
	
	void setAPN(const char *apn, const char *user, const char *pass);
	bool expect_AT(const char *cmd, const char *expected, uint16_t timeout = SIM800_SERIAL_TIMEOUT);
	bool expect_AT_OK(const char *cmd, uint16_t timeout = SIM800_SERIAL_TIMEOUT);
	bool expect_OK(uint16_t timeout = SIM800_SERIAL_TIMEOUT);
	bool expect_scan(const char *pattern, void *ref, uint16_t timeout = SIM800_SERIAL_TIMEOUT);
	bool expect_scan(const char *pattern, void *ref, void *ref1, uint16_t timeout = SIM800_SERIAL_TIMEOUT);
	bool expect_scan(const char *pattern, void *ref, void *ref1, void *ref2, uint16_t timeout = SIM800_SERIAL_TIMEOUT);

	bool reset(bool fona);
	
	size_t read(char *buffer, size_t length);
	size_t readline(char *buffer, size_t max, uint16_t timeout);
	
	void print(uint8_t s);
	void println(uint8_t s);
	void print(const char *s);
	void println(const char *s);
	
	bool time(char *date, char *time, char *tz);
	
	bool enableGPRS(uint16_t timeout);
	bool disableGPRS();
	
	// HTTP GET request, returns the status and puts length in the referenced variable
	unsigned short int HTTP_get(const char *url, unsigned long int &length);
	// manually read the payload after a request, returns the amount read, call multiple times to read whole
	size_t HTTP_read(char *buffer, uint32_t start, size_t length);

protected:
	void eat_echo();
	bool expect(const char *expected, uint16_t timeout  = SIM800_SERIAL_TIMEOUT);
	bool is_urc(const char *line, size_t len);

protected:
	uint8_t urc_status = 0xff;
	const uint32_t _serialSpeed = SIM800_BAUD;
	const char *_apn;
	const char *_user;
	const char *_pass;

}; //InvSIM800

/* access point configurations */
const char apn_u_p[] PROGMEM = "web";
const char mtn_apn[] PROGMEM = "web.gprs.mtnnigeria.net";
const char eti_apn[] PROGMEM = "etisalat";
const char air_apn[] PROGMEM = "internet.ng.airtel.com";
PGM_P const apn[] PROGMEM = { mtn_apn, eti_apn, air_apn };

/* incoming socket data notification */
const char * const urc_01 PROGMEM = "+CIPRXGET: 1,";
/* FTP state change notification */
const char * const urc_02 PROGMEM = "+FTPGET: 1,";
/* PDP disconnected */
const char * const urc_03 PROGMEM = "+PDP: DEACT";
/* PDP disconnected (for SAPBR apps) */
const char * const urc_04 PROGMEM = "+SAPBR 1: DEACT";
/* AT+CLTS network name */
const char * const urc_05 PROGMEM = "*PSNWID:";
/* AT+CLTS time */
const char * const urc_06 PROGMEM = "*PSUTTZ:";
/* AT+CLTS timezone */
const char * const urc_07 PROGMEM = "+CTZV:";
/* AT+CLTS dst information */
const char * const urc_08 PROGMEM = "DST:";
/* AT+CLTS undocumented indicator */
const char * const urc_09 PROGMEM = "+CIEV:";
/* Assorted crap on newer firmware releases. */
const char * const urc_10 PROGMEM = "RDY";
const char * const urc_11 PROGMEM = "+CPIN: READY";
const char * const urc_12 PROGMEM = "Call Ready";
const char * const urc_13 PROGMEM = "SMS Ready";
const char * const urc_14 PROGMEM = "NORMAL POWER DOWN";
const char * const urc_15 PROGMEM = "UNDER-VOLTAGE POWER DOWN";
const char * const urc_16 PROGMEM = "UNDER-VOLTAGE WARNNING";
const char * const urc_17 PROGMEM = "OVER-VOLTAGE POWER DOWN";
const char * const urc_18 PROGMEM = "OVER-VOLTAGE WARNNING";

const char * const _urc_messages[] PROGMEM = {
	urc_01, urc_02, urc_03, urc_04, urc_06, urc_07, urc_08, urc_09, urc_10,
	urc_11, urc_12, urc_13, urc_14, urc_15, urc_16, urc_17, urc_18
};

#endif //__INVSIM800_H__
