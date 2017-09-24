/* 
* InvSIM800.cpp
*
* Created: 11/8/2016 3:03:29 PM
* Author: Ayodeji
*/

//#define F_CPU 16000000UL // Clock Speed

#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "InvSIM800.h"
#include <stdlib.h>

#define println_param(prefix, p) print(F(prefix)); print(F(",\"")); print(p); println(F("\""));

volatile uint8_t InvSIM800::_reset_delay = 0;
volatile uint8_t InvSIM800::_request_delay = 0;

uint8_t InvSIM800::__called_reg_ntwk__ = 0;
uint8_t InvSIM800::__called_apn__ = 0;
uint8_t InvSIM800::__called_grps__ = 0;

// default constructor
InvSIM800::InvSIM800()
{
	SIM_RST_DDR |= 1<<SIM800_RS; //SIM800_RS // reset pin -> output
	SIM_RST_CTRL &= ~(1<<SIM800_RS);
	
	serialInit(0, BAUD(SIM800_BAUD, F_CPU)); // INITIALIZE UART
	
	urc_status = 0xff;
	hostname = 0;
	param = 0;
	_isModuleTimeSet = false;
	_sim_is_waked = false;
	_is_connected = false;
	_is_ntwk_reg = 0;
	eventsync = SYNC_WAKEUP;
	serverResponse = 0;
	

	
} //InvSIM800

void InvSIM800::setAPN(const char *apn, const char *user, const char *pass)
{
	_apn = apn;
	_user = user;
	_pass = pass;
}

void InvSIM800::_switchEventSync(EventSync sync)
{
	if (sync == SYNC_REG_NTWK)
	{
		_is_ntwk_reg = false;
		//_reg_ntwk_delay = 0;
	}
	else if (sync == SYNC_WAKEUP)
	{
		_reset_delay = 0;
		//_reg_ntwk_delay = 0;
		_is_ntwk_reg = false;
		_sim_is_waked = false;
	}
	
	eventsync = sync;
}

bool InvSIM800::_checkConnected()
{
	uint8_t status = 0;
	println(F("AT+SAPBR=2,1")); //check if module is connected to Internet
	expect_scan(F("+SAPBR: 1,%hu,\"%*hu.%*hu.%*hu.%*hu\""), &status, 3000); //+SAPBR: 1,1,"10.96.42.184"
	if (status == 0)
	{
		__hard_reset__();
		return false;
	}
	return true;
}

void InvSIM800::_eat_echo()
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
	
	//for (uint8_t i = 0; i < 17; i++)
	//{
		//char urc[30];
		//strcpy(urc, (PGM_P)pgm_read_word(&(_urc_messages[i])));
		//uint8_t urc_len = strlen(urc);
		////const char *urc = (const char *) pgm_read_word(&(_urc_messages[i]));
		////size_t urc_len = strlen(urc);
		//if (len >= urc_len && !strncmp(urc, line, urc_len))
		//{
			//urc_status = i;
			//return true;
		//}
	//}

	return false;
}


void InvSIM800::_syncServerTime()
{	
	if (!_isModuleTimeSet && _sim_is_waked)
	{
		static uint32_t length = 0;
		uint8_t status = HTTP_get("", length); // 16/11/12,21:02:00+01
		
		if (status == 200)
		{
			char buffer[21] = {0};
			uint16_t idx = HTTP_read(buffer, 0, 20);
			if (idx)
			{
				print(F("AT+CCLK=\""));
				print(buffer);
				println(F("\""));
				if (expect_OK(2000)) _isModuleTimeSet =  true;
			}
		}
	}
	
}

bool InvSIM800::__hard_reset__()
{
	shutdown();
	return true;
}

bool InvSIM800::__soft_reset__(uint8_t &called, uint8_t limits)
{
	uint16_t preset = limits - ((0.5)*limits);
	
	if (called == preset)
	{
		//_gprs_enable_delay = 0;
		_request_delay = 0;
		//_reg_ntwk_delay = 0;
		_reset_delay = 15; //jump hardware reset
		_is_ntwk_reg = false;
		_isModuleTimeSet = false;
		_sim_is_waked = false;
		_is_connected = false;
		_switchEventSync(SYNC_WAKEUP);	
	}
	
	if (called > limits)
	{
		called = 0;
		//_gprs_enable_delay = 0;
		_request_delay = 0;
		//_reg_ntwk_delay = 0;
		_reset_delay = 0;
		_is_ntwk_reg = false;
		_isModuleTimeSet = false;
		_sim_is_waked = false;
		_is_connected = false;
		__hard_reset__();
		_switchEventSync(SYNC_WAKEUP);	
	}
	
	return true;
}

bool InvSIM800::time(char *date, char *time, char *tz)
{
	println(F("AT+CCLK?"));
	return expect_scan(F("+CCLK: \"%8s,%8s%3s\""), date, time, tz) == 3;
}

bool InvSIM800::enableGPRS(uint16_t timeout)
{
	++__called_grps__; __called_apn__ = __called_reg_ntwk__ = 0;
	disableGPRS();
	expect_AT(F("+CIPSHUT"), F("SHUT OK"), 5000);
	expect_AT_OK(F("+CIPMUX=1")); // enable multiplex mode
	expect_AT_OK(F("+CIPRXGET=1")); // we will receive manually

	bool attached = false;
	while (!attached && timeout > 0) {
		attached = expect_AT_OK(F("+CGATT=1"), 3000);
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
	if(!expect_AT_OK(F("+SAPBR=1,1"), 30000)) return false;
	_is_connected = true;

	do {
		println(F("AT+CGATT?"));
		attached = expect(F("+CGATT: 1"));
		_delay_ms(10);
	} while (--timeout && !attached);

	//_gprs_enable_delay = 0;
	
	return attached;
}

void InvSIM800::print(uint8_t s)
{
	serialWrite(0, s);
}

void InvSIM800::println(uint8_t s)
{
	serialWrite(0, s);
	_eat_echo();
	serialWrite(0, '\n');
}

void InvSIM800::print(const char *s)
{
	serialWriteString(0, s);
}

void InvSIM800::println(const char *s) 
{
	serialWriteString(0, s);
	_eat_echo();
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
	
	serialInit(0, BAUD(SIM800_BAUD, F_CPU));
		
	bool ok = false;
	
	if (!_sim_is_waked && _reset_delay == 5)
	{
		SIM_RST_DDR &= ~1<<SIM800_RS;
		_delay_ms(500);
		SIM_RST_CTRL |= 1<<SIM800_RS;
	}

	// RST high keeps the chip in reset without a diode, so put to low
	//if (!fona) PORTD &= ~1<<PIND2;
//
	if (!_sim_is_waked && _reset_delay == 30)
	{
		while (serialHasChar(0)) serialGet(0);
		
		expect_AT_OK(F(""), 2000);
		expect_AT_OK(F(""), 2000);
		expect_AT_OK(F(""), 2000);
		ok = expect_AT_OK(F("E0"), 2000); //TODO: PUT RETRIES HERE

		expect_AT_OK(F("+IFC=0,0"), 2000); // No hardware flow control
		expect_AT_OK(F("+CIURC=0"), 2000); // No "Call Ready"

		while (serialHasChar(0)) serialGet(0);
		_reset_delay = 25;
	}

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
	//serialWriteString(0, "Debugging\n");
		
	expect_AT_OK(F("+HTTPTERM"));
	_delay_ms(100);
	
	if (!expect_AT_OK(F("+HTTPINIT"))) return 1000;
	if (!expect_AT_OK(F("+HTTPPARA=\"CID\",1"))) return 1101;
	if (!expect_AT_OK(F("+HTTPPARA=\"UA\",\"IOTINV#1 r0.1\""))) return 1102;
	//if (!expect_AT_OK(F("+HTTPPARA=\"REDIR\",1"))) return 1103; //1 allows reirect , o means no redirection
	println_param("AT+HTTPPARA=\"URL\"", url);
	if (!expect_OK()) return 1110;

	if (!expect_AT_OK(F("+HTTPACTION=0"))) return 1004;

	unsigned short int status;
	expect_scan(F("+HTTPACTION: 0,%hu,%lu"), &status, &length, 30000);

	return status;
}

size_t InvSIM800::HTTP_read(char *buffer, uint16_t start, uint16_t length)
{
	char cmd[50];
	char str[5];
	char len[5];
	itoa(start, str, 10);
	itoa(length, len, 10);
	
	strcpy(cmd, "AT+HTTPREAD=");
	strcat(cmd, str);
	strcat(cmd, ",");
	strcat(cmd, len);
	
	println(cmd);
	
	//println(F("AT+HTTPREAD=0,1"));

	unsigned long int available;
	expect_scan(F("+HTTPREAD: %lu"), &available);
	
	size_t idx = read(buffer, (size_t) available);
	if (!expect_OK()) return 0;
	return idx;
}

bool InvSIM800::shutdown()
{
	disableGPRS();
	while(serialHasChar(0)) serialGet(0);
	println(F("AT+CPOWD=1"));
	_delay_ms(20);
	//expect_AT_OK(F("+CPOWD=1"));
	//expect(F("NORMAL POWER DOWN"));
	_sim_is_waked = false;
		
	return true;
	
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
}

bool InvSIM800::wakeup()
{	
	//TODO: CHECK WAIT FOR OK IF NOT .... SHUTDOWN AND RESTART
	

	//// check if the chip is already awake, otherwise start wakeup
	//if (!expect_AT_OK(F(""), 1000))
	//{
		//// SIM800 using PWRKEY wakeup procedure"
		////pinMode(SIM800_KEY, OUTPUT);
		////pinMode(SIM800_PS, INPUT);
		////do {
			////digitalWrite(SIM800_KEY, HIGH);
			////delay(10);
			////digitalWrite(SIM800_KEY, LOW);
			////delay(1100);
			////digitalWrite(SIM800_KEY, HIGH);
			////delay(2000);
		////} while (digitalRead(SIM800_PS) == LOW);
		////// make pin unused (do not leak)
		////pinMode(SIM800_KEY, INPUT_PULLUP);
		////PRINTLN("!!! SIM800 ok");
	//} 
// 	else 
// 	{
// 		PRINTLN("!!! SIM800 already awake");
// 	}


	if(reset(true))
	{
		_sim_is_waked = true;
		_reset_delay = 0;
		_switchEventSync(SYNC_REG_NTWK);
		return true;
	}
	return false;
}

bool InvSIM800::registerNetwork(uint16_t timeout)
{
	++__called_reg_ntwk__;
	//Todo: monitor retries
	
	expect_AT_OK(F(""));
	while (timeout -= 1000) {
		unsigned short int n = 0;
		println(F("AT+CREG?"));
		expect_scan(F("+CREG: 0,%hu"), &n);
		if ((n == 1 || n == 5))
		{
			//_reg_ntwk_delay = 0;
			__called_reg_ntwk__ = 0;
			_is_ntwk_reg = true;
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
	char dest[(h+p)+1]; // = (char *)malloc((h+p)+1);
	////
	if(!h) return (char *)"Invalid Hostname";
	strcpy(dest, hostname);
	if (param) strcat(dest, param);
	/////*serialWriteString(0, dest);*/
	strcpy(url, dest);
	////free(dest);
	return url;
}

void InvSIM800::setup()
{
	switch(eventsync)
	{
		case SYNC_WAKEUP:
			wakeup();
			break;
		case SYNC_REG_NTWK:
			if(registerNetwork(3000)) { _switchEventSync(SYNC_SET_APN); } else { __soft_reset__(__called_reg_ntwk__, 10); }
			break;
		case SYNC_SET_APN:
			if(setSwitchAPN()) { _switchEventSync(SYNC_ENABLE_GPRS); } else { __soft_reset__(__called_apn__, 15); }
			break;
		case SYNC_ENABLE_GPRS:
			if(enableGPRS())  { _switchEventSync(SYNC_HTTP_REQUEST);} else { __soft_reset__(__called_grps__, 8); };
			break;
		case SYNC_HTTP_REQUEST:
			__called_grps__ = __called_apn__ = __called_reg_ntwk__ = 0;
			//syncServerTime();
			httpRequest();
			break;
	}
}

bool InvSIM800::setSwitchAPN()
{
	++__called_apn__; __called_reg_ntwk__ = 0;
	
	//TODO:  ADD RETRIES
	char cs[15];
	println(F("AT+CSCA?"));
	//+CSCA: "+234803000000",145	if (expect_scan(F("+CSCA: \"%13s\",%*lu"), &cs), 3000)
	{
		_eat_echo();
		
		if(strcmp(cs, (char*)"+234803000000") == 0) //mtn
		{
			setAPN("web.gprs.mtnnigeria.net", "web", "web"); return true;
		}
		else if (strcmp(cs, (char*)"+234809000151") == 0) //etisalat correct is +2348090001518
		{
			setAPN("etisalat", "web", "web"); return true;
		}
		else if (strcmp(cs, (char*)"+234802000000") == 0) //airtel correct is +2348020000009
		{
			setAPN("internet.ng.airtel.com", "web", "web"); return true;
		}
		else if (strcmp(cs, (char*)"+234805000150") == 0) //glo  correct is +2348050001501
		{
			setAPN("gloflat", "web", "web"); return true;
		}
		else
		{
			//__hard_reset__();
		}
	}
	
	return false;
}

void InvSIM800::httpRequest()
{
	static uint8_t called = 0;

	if (_request_delay > 15)
	{
		++called;
		
		static uint32_t length = 0;
		uint16_t status = HTTP_get(getUrl(), length);
		if (status == 200 && length==1)
		{
			char buffer[20] = {0};
			uint16_t idx = HTTP_read(buffer, 0, 1);
			if (idx)
			{
				char control = buffer[0];
				serverResponse = control;
			}
			
			_request_delay = 10;
			called = 0;
			return;
		}
		else if (status >= 1000)
		{
			_request_delay = 14;
			__soft_reset__(called, 5);
			return;
		}
		else
		{
			_request_delay = 14;
			__soft_reset__(called, 10);
			return;
		}
	}
}

char InvSIM800::getServerResponse()
{
	return serverResponse;
}

void InvSIM800::nullServerResponse()
{
	serverResponse = 0;
}


void InvSIM800::incrementInSec()
{
	
	if (_reset_delay <= 60)
	{
		if(!_sim_is_waked)
		{
			++_reset_delay;
			if (_reset_delay == 60)
			{
				_reset_delay = 0;
			}
		}
	}
	
	if (_request_delay < 120)
	{
		++_request_delay;
	}
	
	//if(_reg_ntwk_delay < 20)
	//{
		//if (!_is_ntwk_reg)
		//{
			//++_reg_ntwk_delay;
		//}
	//}
}

void InvSIM800::externalReset()
{
	uint8_t x = 5, y = 10;
	__soft_reset__(x, y);
}
