/* 
* InvSIM800.cpp
*
* Created: 11/8/2016 3:03:29 PM
* Author: Ayodeji
*/

#define F_CPU 16000000UL // Clock Speed

#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "InvSIM800.h"
#include <stdlib.h>

#define println_param(prefix, p) print(F(prefix)); print(F(",\"")); print(p); println(F("\""));

// default constructor
InvSIM800::InvSIM800()
{
	urc_status = 0xff;
	hostname = 0;
	param = 0;
	eventsync = 0;
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
	
	return strcmp(buf, (const char *) expected) == 0;
	//return strcmp_P(buf, (const char PROGMEM *) expected) == 0;
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
	return sscanf(buf, (const char *) pattern, ref) == 1;
	//return sscanf_P(buf, (const char PROGMEM *) pattern, ref) == 1;
}

bool InvSIM800::expect_scan(const char *pattern, void *ref, void *ref1, uint16_t timeout)
{
	char buf[SIM800_BUFSIZE];
	size_t len;
	do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
	return sscanf(buf, (const char *) pattern, ref, ref1) == 2;
	//return sscanf_P(buf, (const char PROGMEM *) pattern, ref, ref1) == 2;
}

bool InvSIM800::expect_scan(const char *pattern, void *ref, void *ref1, void *ref2, uint16_t timeout)
{
	char buf[SIM800_BUFSIZE];
	size_t len;
	do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
	return sscanf(buf, (const char *) pattern, ref, ref1, ref2) == 3;
	//return sscanf_P(buf, (const char PROGMEM *) pattern, ref, ref1, ref2) == 3;
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


void InvSIM800::httpRequest()
{
	throw std::logic_error("The method or operation is not implemented.");
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

void InvSIM800::print(uint32_t s)
{
	serialWrite(0, s);
}

void InvSIM800::println(uint32_t s)
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
	_delay_ms(200);
	PORTD |= 1<<PIND2;
	_delay_ms(25000);

	// RST high keeps the chip in reset without a diode, so put to low
	if (!fona) PORTD &= ~1<<PIND2;

	while (serialHasChar(0)) serialGet(0);

	expect_AT_OK(F(""), 2000);
	expect_AT_OK(F(""), 2000);
	expect_AT_OK(F(""), 2000);
	bool ok = expect_AT_OK(F("E0"), 2000);

	expect_AT_OK(F("+IFC=0,0"), 2000); // No hardware flow control
	expect_AT_OK(F("+CIURC=0"), 2000); // No "Call Ready"

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
	//print(F("AT+HTTPREAD="));
	//print(start);
	//print(F(","));
	//println(length);
	
	println(F("AT+HTTPREAD=0,1"));

	unsigned long int available;
	expect_scan(F("+HTTPREAD: %lu"), &available);
	
	size_t idx = read(buffer, (size_t) available);
	if (!expect_OK()) return 0;
	return idx;
}

bool InvSIM800::shutdown()
{
	disableGPRS();
	expect_AT_OK(F("+CPOWD=1"));
	if (expect(F("NORMAL POWER DOWN"), 5000))
	{
		return true;
	}
	

	//if (urc_status != 12 && digitalRead(SIM800_PS) == HIGH) {
		//PRINTLN("!!! SIM800 shutdown using PWRKEY");
		//pinMode(SIM800_KEY, OUTPUT);
		//pinMode(SIM800_PS, INPUT);
		//digitalWrite(SIM800_KEY, LOW);
		//for (uint8_t s = 30; s > 0 && digitalRead(SIM800_PS) != LOW; --s) delay(1000);
		//digitalWrite(SIM800_KEY, HIGH);
		//pinMode(SIM800_KEY, INPUT);
		//pinMode(SIM800_KEY, INPUT_PULLUP);
	//}
	return false;
}

bool InvSIM800::wakeup()
{
	expect_AT_OK(F(""));
	// check if the chip is already awake, otherwise start wakeup
	if (!expect_AT_OK(F(""), 5000))
	{
		// SIM800 using PWRKEY wakeup procedure"
		//pinMode(SIM800_KEY, OUTPUT);
		//pinMode(SIM800_PS, INPUT);
		//do {
			//digitalWrite(SIM800_KEY, HIGH);
			//delay(10);
			//digitalWrite(SIM800_KEY, LOW);
			//delay(1100);
			//digitalWrite(SIM800_KEY, HIGH);
			//delay(2000);
		//} while (digitalRead(SIM800_PS) == LOW);
		//// make pin unused (do not leak)
		//pinMode(SIM800_KEY, INPUT_PULLUP);
		//PRINTLN("!!! SIM800 ok");
	} 
// 	else 
// 	{
// 		PRINTLN("!!! SIM800 already awake");
// 	}

	return reset(true);
}

bool InvSIM800::registerNetwork(uint16_t timeout)
{
	expect_AT_OK(F(""));
	while (timeout -= 1000) {
		unsigned short int n = 0;
		println(F("AT+CREG?"));
		expect_scan(F("+CREG: 0,%hu"), &n);
		if ((n == 1 || n == 5)) 
		{
			return true;
		}
		_delay_ms(1000);
	}
	return false;
}

void InvSIM800::setHostname(const char *h)
{
	hostname = (char*)h;
}

void InvSIM800::setParam(const char* p)
{
	param = (char*)p;
}

char* InvSIM800::getUrl()
{
	uint16_t h = (hostname)?strlen(hostname):0;
	uint16_t p = (param)?strlen(param):0;
	char *dest = (char *)malloc((h+p)+1);
	
	if(!h) return "Invalid Hostname";
	strcpy(dest, hostname);
	if (param) strcat(dest, param);
	/*serialWriteString(0, dest);*/
	strcpy(url, dest);
	free(dest);
	return url;
}