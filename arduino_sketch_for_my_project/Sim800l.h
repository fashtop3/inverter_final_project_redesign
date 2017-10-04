/* this library is writing by  Cristian Steib.

      PINOUT:
          _____________________________
         |  ARDUINO UNO >>>   SIM800L  |
          -----------------------------
              GND      >>>   GND
          RX  10       >>>   TX
          TX  11       >>>   RX
         RESET 2       >>>   RST

     POWER SOURCE 4.2V >>> VCC


*/
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

#ifdef F
#undef F
#define F(s) (s)
#else
#define F(s) (s)
#endif

#define SIM800_SERIAL_TIMEOUT 4000

enum EventSync {
  SYNC_WAKEUP = 0x0,
  SYNC_REG_NTWK,
  SYNC_SET_APN,
  SYNC_ENABLE_GPRS,
  SYNC_HTTP_REQUEST
};



class Sim800l
{

  public:
    init();

    bool reset();
    bool expect_AT_OK(const char *cmd, uint16_t timeout);
    bool expect_AT(const char *cmd, const char *expected, uint16_t timeout);
    bool expect(const char *expected, uint16_t timeout);
    
    void setAPN(const char *apn, const char *user, const char *pass);
    bool wakeup();

    void print(uint8_t s);
    void println(uint8_t s);
    void print(const char *s);
    void println(const char *s);

    void setup();

  protected:
  void _eat_echo();
    bool _isModuleTimeSet;
    bool _is_ntwk_reg;
    volatile bool _sim_is_waked;
    volatile bool _is_connected;
    volatile static uint8_t _reset_delay;
    volatile static uint8_t _request_delay;
    uint8_t eventsync;
    uint8_t urc_status;
    const char *_apn;
    const char *_user;
    const char *_pass;
    char url[100];
    char *hostname;
    char *param;
    mutable char serverResponse;

    static uint8_t __called_reg_ntwk__;
    static uint8_t __called_apn__;
    static uint8_t __called_grps__;

  private:
    int _timeout;
    String _buffer;
    String _readSerial(uint16_t timeout = SIM800_SERIAL_TIMEOUT);


  public:

    void begin();

    // Methods for calling || Funciones de llamadas.
    bool answerCall();
    void callNumber(char* number);
    bool hangoffCall();
    uint8_t getCallStatus();
    //Methods for sms || Funciones de SMS.
    bool sendSms(char* number, char* text);
    String readSms(uint8_t index); //return all the content of sms
    String getNumberSms(uint8_t index); //return the number of the sms..
    bool delAllSms();     // return :  OK or ERROR ..

    void signalQuality();
    void setPhoneFunctionality();
    void activateBearerProfile();
    void deactivateBearerProfile();
    //get time with the variables by reference
    void RTCtime(int *day, int *month, int *year, int *hour, int *minute, int *second);
    String dateNet(); //return date,time, of the network
    bool updateRtc(int utc);  //Update the RTC Clock with de Time AND Date of red-.
};

/* access point configurations */
const char apn_u_p[] PROGMEM = "web";
const char mtn_apn[] PROGMEM = "web.gprs.mtnnigeria.net";
const char eti_apn[] PROGMEM = "etisalat";
const char air_apn[] PROGMEM = "internet.ng.airtel.com";
PGM_P const apn[] PROGMEM = { mtn_apn, eti_apn, air_apn };

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
