

//inverter serial response format
//DATA:<AC IN>,<BATTERY LEVEL>,<LOAD RANGE>,<CHARGING>,<CURRENT POWER STATE>,<CURRENT BACKUP STATE>

#ifndef Sim800l_h
#define Sim800l_h
#include <SoftwareSerial.h>
#include "Arduino.h"


#define SIM_RX_PIN 8
#define SIM_TX_PIN 9
#define INV_RX_PIN 10
#define INV_TX_PIN 11
#define RESET_PIN 2   // pin to the reset pin sim800l

#define LED true // used for indicator led, in case that you don want set to false . 
#define LED_PIN 13 //pin to indicate states. 
#define DQR_LED 4
#define RDS_LED 3
#define MODULE_AVAILABLE 7


//#define DEBUG_MODE

#ifdef F
#undef F
#define F(s) (s)
#else
#define F(s) (s)
#endif

#define println_param(prefix, p) print(F(prefix)); print(F(",\"")); print(p); println(F("\""));

#define SIM800_CMD_TIMEOUT 30000
#define SIM800_SERIAL_TIMEOUT 4000
#define SIM800_BUFSIZE 64
#define hostname  "52.170.211.220/api/r/1"

class Sim800l
{

  public:

    init();

    bool reset();
    bool expect_AT_OK(const char *cmd, uint16_t timeout = SIM800_SERIAL_TIMEOUT);
    bool expect_AT(const char *cmd, const char *expected, uint16_t timeout = SIM800_SERIAL_TIMEOUT);
    bool expect(const char *expected, SoftwareSerial &s, uint16_t timeout = SIM800_SERIAL_TIMEOUT);
    bool expect_OK(uint16_t timeout = SIM800_SERIAL_TIMEOUT);

    void setAPN(const char *apn, const char *user, const char *pass);
    bool wakeup();
    bool registerNetwork();
    bool expect_scan(const char *pattern, void *ref, SoftwareSerial &s, uint16_t timeout = SIM800_SERIAL_TIMEOUT);
    bool expect_scan(const char *pattern, void *ref, void *ref1, SoftwareSerial &s, uint16_t timeout = SIM800_SERIAL_TIMEOUT);
    bool expect_scan(const char *pattern, void *ref, void *ref1, void *ref2, SoftwareSerial &s, uint16_t timeout = SIM800_SERIAL_TIMEOUT);

    void print(uint8_t s);
    void println(uint8_t s);
    void print(const char *s);
    void println(const char *s);

    void setup();
    bool setSwitchAPN();
    bool enableGPRS(uint16_t timeout = SIM800_CMD_TIMEOUT);
    bool disableGPRS();
    bool checkConnected();

    //    size_t readline(char *buf, uint8_t maxIdx, uint16_t timeout);
    size_t readline(char *buf, uint8_t maxIdx, SoftwareSerial &s, uint16_t timeout);
    size_t read(char *buf, uint8_t len, SoftwareSerial &s);
    bool is_urc(const char *line, size_t len);

    bool shutdown();
    bool sendInverterReq(const char req);
    void httpRequest(uint8_t &len);
    unsigned short int HTTP_get(uint8_t &len);
    bool HTTP_read(uint8_t start, uint8_t len);
    size_t HTTP_read(char *b, uint8_t start, uint8_t len);

    void RTCtime(int *day, int *month, int *year, int *hour, int *minute, int *second);
    String dateNet(); //return date,time, of the network
    bool updateRtc(int utc);  //Update the RTC Clock with de Time AND Date of red-.

    void blink_led(unsigned char PIN);
    void blink_reset_led();
    void http_error_led();
    void network_led();
    void network_led_found(bool found);

  protected:
    void _eat_echo();
    void _eat_echo(SoftwareSerial &serial);
    bool __hard_reset__();
    volatile bool _is_connected;
    mutable uint8_t power;
    mutable uint8_t load_max;
    mutable uint8_t output;

    short unsigned int a, l, c, p, k;
    char b[6];

    uint8_t urc_status;
    const char *_apn;
    const char *_user;
    const char *_pass;
    //    String hostname;

    char buf[SIM800_BUFSIZE];
    size_t len;


  private:
    int _timeout;
    String _buffer;
    String _readSerial(uint16_t timeout = SIM800_SERIAL_TIMEOUT);
    String _readSerial(const SoftwareSerial &serial, uint16_t timeout = SIM800_SERIAL_TIMEOUT);
};

/* access point configurations */
//const char apn_u_p[] PROGMEM = "web";
//const char mtn_apn[] PROGMEM = "web.gprs.mtnnigeria.net";
//const char eti_apn[] PROGMEM = "etisalat";
//const char air_apn[] PROGMEM = "internet.ng.airtel.com";
//PGM_P const apn[] PROGMEM = { mtn_apn, eti_apn, air_apn };

///* incoming socket data notification */
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
//
const char * const _urc_messages[] PROGMEM = {
  urc_01, urc_02, urc_03, urc_04, urc_06, urc_07, urc_08, urc_09, urc_10,
  urc_11, urc_12, urc_13, urc_14, urc_15, urc_16, urc_17, urc_18
};

#endif
