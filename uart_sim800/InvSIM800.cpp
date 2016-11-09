/* 
* InvSIM800.cpp
*
* Created: 11/8/2016 3:03:29 PM
* Author: Ayodeji
*/

#include <stdio.h>
#include <string.h>
#include "InvSIM800.h"

#define println_param(prefix, p) print(F(prefix)); print(F(",\"")); print(p); println(F("\""));

// default constructor
InvSIM800::InvSIM800()
{
} //InvSIM800

void InvSIM800::setAPN(const char *apn, const char *user, const char *pass)
{
	_apn = apn;
	_user = user;
	_pass = pass;
}

void InvSIM800::eat_echo()
{
	 while (serialHasChar(0))
	 {
		serialGet(0);
		// don't be too quick or we might not have anything available
		// when there actually is...
		_delay_ms(1);
	 }
}

bool InvSIM800::expect(const char *expected, uint16_t timeout)
{
	char buf[SIM800_BUFSIZE];
	size_t len;
	do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
	return strcmp_P(buf, (const char PROGMEM *) expected) == 0;
}

bool InvSIM800::expect_AT(const char *cmd, const char *expected, uint16_t timeout)
{
	print(F("AT"));
	println(cmd);
	return expect(expected, timeout);
}

bool InvSIM800::expect_scan(const char *pattern, void *ref, uint16_t timeout)
{
	char buf[SIM800_BUFSIZE];
	size_t len;
	do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
	return sscanf_P(buf, (const char PROGMEM *) pattern, ref) == 1;
}

bool InvSIM800::expect_scan(const char *pattern, void *ref, void *ref1, uint16_t timeout)
{
	char buf[SIM800_BUFSIZE];
	size_t len;
	do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
	return sscanf_P(buf, (const char PROGMEM *) pattern, ref, ref1) == 2;
}

bool InvSIM800::expect_scan(const char *pattern, void *ref, void *ref1, void *ref2, uint16_t timeout)
{
	char buf[SIM800_BUFSIZE];
	size_t len;
	do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
	return sscanf_P(buf, (const char PROGMEM *) pattern, ref, ref1, ref2) == 3;
}

bool InvSIM800::is_urc(const char *line, size_t len)
{
	urc_status = 0xff;

	for (uint8_t i = 0; i < 17; i++)
	{
		const char *urc = (const char *) pgm_read_word(&(_urc_messages[i]));
		size_t urc_len = strlen(urc);
		if (len >= urc_len && !strncmp(urc, line, urc_len))
		{
			urc_status = i;
			return true;
		}
	}

	return false;
}


bool InvSIM800::time(char *date, char *time, char *tz)
{
	println(F("AT+CCLK?"));
	return expect_scan(F("+CCLK: \"%8s,%8s%3s\""), date, time, tz) == 3;
}

bool InvSIM800::enableGPRS(uint16_t timeout)
{
	expect_AT(F("+CIPSHUT"), F("SHUT OK"), 5000);
	expect_AT_OK(F("+CIPMUX=1")); // enable multiplex mode
	expect_AT_OK(F("+CIPRXGET=1")); // we will receive manually

	bool attached = false;
	while (!attached && timeout > 0) {
		attached = expect_AT_OK(F("+CGATT=1"), 10000);
		_delay_ms(1000);
		timeout -= 1000;
	}
	
	if (!attached) return false;

	if (!expect_AT_OK(F("+SAPBR=3,1,\"CONTYPE\",\"GPRS\""), 10000)) return false;

	// set bearer profile access point name
	if (_apn) {
		print(F("AT+SAPBR=3,1,\"APN\",\""));
		print(_apn);
		println(F("\""));
		if (!expect_OK()) return false;

		if (_user) {
			print(F("AT+SAPBR=3,1,\"USER\",\""));
			print(_user);
			println(F("\""));
			if (!expect_OK()) return false;
		}
		if (_pass) {
			print(F("AT+SAPBR=3,1,\"PWD\",\""));
			print(_pass);
			println(F("\""));
			if (!expect_OK()) return false;
		}
	}

	// open GPRS context
	expect_AT_OK(F("+SAPBR=1,1"), 30000);

	do {
		println(F("AT+CGATT?"));
		attached = expect(F("+CGATT: 1"));
		_delay_ms(10);
	} while (--timeout && !attached);

	return attached;
}

void InvSIM800::print(uint8_t s)
{
	serialWrite(0, s);
}

void InvSIM800::println(uint8_t s)
{
	serialWrite(0, s);
	eat_echo();
	serialWrite(0, '\n');
}

void InvSIM800::print(const char *s)
{
	serialWriteString(0, s);
}

void InvSIM800::println(const char *s) 
{
	serialWriteString(0, s);
	eat_echo();
	serialWrite(0, '\n');
}

bool InvSIM800::expect_AT_OK(const char *cmd, uint16_t timeout)
{
	return expect_AT(cmd, F("OK"), timeout);
}

bool InvSIM800::expect_OK(uint16_t timeout /*= SIM800_SERIAL_TIMEOUT*/)
{
	return expect(F("OK"), timeout);
}

bool InvSIM800::reset(bool fona) {

	DDRD |= 1<<PIND2;
	
	PORTD |= 1<<PIND2;
	_delay_ms(10);
	PORTD &= ~1<<PIND2;
	_delay_ms(100);
	PORTD |= 1<<PIND2;
	_delay_ms(7000);

	// RST high keeps the chip in reset without a diode, so put to low
	if (!fona) PORTD &= ~1<<PIND2;

	while (serialHasChar(0)) serialGet(0);

	expect_AT_OK(F(""));
	expect_AT_OK(F(""));
	expect_AT_OK(F(""));
	bool ok = expect_AT_OK(F("E0"));

	expect_AT_OK(F("+IFC=0,0")); // No hardware flow control
	expect_AT_OK(F("+CIURC=0")); // No "Call Ready"

	while (serialHasChar(0)) serialGet(0);

	return ok;
}

size_t InvSIM800::read(char *buffer, size_t length)
{
	uint32_t idx = 0;
	while (length) {
		while (length && serialHasChar(0)) {
			buffer[idx++] = (char) serialGet(0);
			length--;
		}
	}
	return idx;
}

size_t InvSIM800::readline(char *buffer, size_t max, uint16_t timeout)
{
	 uint16_t idx = 0;
	 while (--timeout) {
		 while (serialHasChar(0)) {
			 char c = (char) serialGet(0);
			 if (c == '\r') continue;
			 if (c == '\n') {
				 if (!idx) continue;
				 timeout = 0;
				 break;
			 }
			 if (max - idx) buffer[idx++] = c;
		 }

		 if (timeout == 0) break;
		 _delay_ms(1);
	 }
	 buffer[idx] = 0;
	 return idx;
}

bool InvSIM800::disableGPRS()
{
	expect_AT(F("+CIPSHUT"), F("SHUT OK"));
	if (!expect_AT_OK(F("+SAPBR=0,1"), 30000)) return false;

	return expect_AT_OK(F("+CGATT=0"));
}

unsigned short int InvSIM800::HTTP_get(const char *url, unsigned long int &length)
{
	expect_AT_OK(F("+HTTPTERM"));
	_delay_ms(100);

	if (!expect_AT_OK(F("+HTTPINIT"))) return 1000;
	if (!expect_AT_OK(F("+HTTPPARA=\"CID\",1"))) return 1101;
	if (!expect_AT_OK(F("+HTTPPARA=\"UA\",\"IOTINV#1 r0.1\""))) return 1102;
	if (!expect_AT_OK(F("+HTTPPARA=\"REDIR\",1"))) return 1103; //1 allows reirect , o means no redirection
	println_param("AT+HTTPPARA=\"URL\"", url);
	if (!expect_OK()) return 1110;

	if (!expect_AT_OK(F("+HTTPACTION=0"))) return 1004;

	unsigned short int status;
	expect_scan(F("+HTTPACTION: 0,%hu,%lu"), &status, &length, 60000);

	return status;
}

size_t InvSIM800::HTTP_read(char *buffer, uint32_t start, size_t length)
{
	print(F("AT+HTTPREAD="));
	print(start);
	print(F(","));
	println((uint32_t) length);

	unsigned long int available;
	expect_scan(F("+HTTPREAD: %lu"), &available);
	
	size_t idx = read(buffer, (size_t) available);
	if (!expect_OK()) return 0;
	return idx;
}

