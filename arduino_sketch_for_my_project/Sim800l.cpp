/* this library is writing by  Cristian Steib.
        steibkhriz@gmail.com
    Designed to work with the GSM Sim800l,maybe work with SIM900L

       This library use SoftwareSerial, you can define de RX and TX pin
       in the header "Sim800l.h" ,by default the pin is RX=10 TX=11..
       be sure that gnd is attached to arduino too.
       You can also change the other preferred RESET_PIN

       Esta libreria usa SoftwareSerial, se pueden cambiar los pines de RX y TX
       en el archivo header, "Sim800l.h", por defecto los pines vienen configurado en
       RX=10 TX=11.
       Tambien se puede cambiar el RESET_PIN por otro que prefiera

      PINOUT:
          _____________________________
         |  ARDUINO UNO >>>   SIM800L  |
          -----------------------------
              GND      >>>   GND
          RX  10       >>>   TX
          TX  11       >>>   RX
         RESET 2       >>>   RST

     POWER SOURCE 4.2V >>> VCC

      Created on: April 20, 2016
          Author: Cristian Steib


*/
#include "Arduino.h"
#include "Sim800l.h"
#include <SoftwareSerial.h>

SoftwareSerial SIM(SIM_RX_PIN, SIM_TX_PIN);
SoftwareSerial INV(INV_RX_PIN, INV_TX_PIN);


volatile uint8_t Sim800l::_reset_delay = 0;
volatile uint8_t Sim800l::_request_delay = 0;

uint8_t Sim800l::__called_reg_ntwk__ = 0;
uint8_t Sim800l::__called_apn__ = 0;
uint8_t Sim800l::__called_grps__ = 0;


Sim800l::init()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Initializing sim module....");

  SIM.begin(9600); // INITIALIZE UART
  //  INV.begin(57600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);

  urc_status = 0xff;
  hostname = 0;
  param = 0;
  _isModuleTimeSet = false;
  _sim_is_waked = false;
  _is_connected = false;
  _is_ntwk_reg = 0;
  eventsync = SYNC_WAKEUP;
  serverResponse = 0;
}


void Sim800l::setAPN(const char *apn, const char *user, const char *pass)
{
  _apn = apn;
  _user = user;
  _pass = pass;
}


bool Sim800l::wakeup()
{
#if (LED)
  digitalWrite(LED_PIN, 1);
#endif
  //TODO: CHECK WAIT FOR OK IF NOT .... SHUTDOWN AND RESTART
  //// check if the chip is already awake, otherwise start wakeup
  if (!expect_AT_OK(F(""), 2000))
  {
    Serial.println("!!! SIM800 Unavailable.... Restarting....");
    _sim_is_waked = false;
    if (!reset())
    {
      _sim_is_waked = false;
      Serial.println("!!! SIM800 Reset Error");
    } else {
      _sim_is_waked = true;
      Serial.println("SIM800 Restarted!!!");
    }
  }
  else
  {
    _sim_is_waked = true;
    Serial.println("!!! SIM800 already awake");
    expect_AT_OK(F("E0"), 2000); //TODO: PUT RETRIES HERE
    _eat_echo();
  }

#if (LED)
  digitalWrite(LED_PIN, 0);
#endif
  return _sim_is_waked;
}


bool Sim800l::reset() {
  bool ok = false;

#if (LED)
  digitalWrite(LED_PIN, 1);
#endif

  digitalWrite(RESET_PIN, 0);
  delay(1000);
  digitalWrite(RESET_PIN, 1);
  // wait for the module response

  _eat_echo();
  expect_AT_OK(F(""), 2000);
  expect_AT_OK(F(""), 2000);
  expect_AT_OK(F(""), 2000);
  ok = expect_AT_OK(F("E0"), 2000); //TODO: PUT RETRIES HERE
  _eat_echo();


#if (LED)
  digitalWrite(LED_PIN, 0);
#endif

  return ok;
}


bool Sim800l::expect_AT_OK(const char *cmd, uint16_t timeout)
{
  return expect_AT(cmd, F("OK"), timeout);
}

bool Sim800l::expect_AT(const char *cmd, const char *expected, uint16_t timeout)
{
  print(F("AT"));
  println(cmd);
  return expect(expected, timeout);
}

bool Sim800l::expect(const char *expected, uint16_t timeout)
{
  return _readSerial(timeout).indexOf(expected) != -1 ? true : false;
}


void Sim800l::print(uint8_t s)
{
  SIM.print(s);
}

void Sim800l::println(uint8_t s)
{
  SIM.print(s);
  _eat_echo();
  SIM.print('\n');
}

void Sim800l::print(const char *s)
{
  SIM.print(s);
}

void Sim800l::println(const char *s)
{
  SIM.print(s);
  _eat_echo();
  SIM.print('\n');
}

void Sim800l::_eat_echo()
{
  while (SIM.available())
  {
    SIM.read();
    delay(1);
  }
}


void Sim800l::setup()
{
  if (wakeup()) {
    delay(2000);
    registerNetwork(30000);
  }
}

bool Sim800l::registerNetwork(uint16_t timeout)
{
  _is_ntwk_reg = false;
  Serial.println("Check Registered Network?...");
  _eat_echo();
  unsigned short int n = 0;
  println(F("AT+CREG?"));
  if (expect_scan(F("+CREG: 0,%hu"), &n), timeout) {
    if ((n == 1 || n == 5))
    {
      _is_ntwk_reg = true;
      Serial.println("Registered Network");
      return true;
    }
  }

  Serial.println("No Registered Network Found?...");
  return false;
}

bool Sim800l::expect_scan(const char *pattern, void *ref, uint16_t timeout)
{
  String resp = _readSerial(timeout);
  Serial.println(resp);
  char buf[resp.length()];
  resp.toCharArray(buf, resp.length());
  return sscanf(buf, (const char *) pattern, ref) == 1;
}

bool Sim800l::expect_scan(const char *pattern, void *ref, void *ref1, uint16_t timeout)
{
  String resp = _readSerial(timeout);
  char buf[resp.length()];
  resp.toCharArray(buf, resp.length());
  return sscanf(buf, pattern, ref, ref1) == 2;
}

bool Sim800l::expect_scan(const char *pattern, void *ref, void *ref1, void *ref2, uint16_t timeout)
{
  String resp = _readSerial(timeout);
  char buf[resp.length()];
  resp.toCharArray(buf, resp.length());
  return sscanf(buf, (const char *) pattern, ref, ref1, ref2) == 3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

//String _buffer;

void Sim800l::begin() {
  SIM.begin(9600);
#if (LED)
  pinMode(OUTPUT, LED_PIN);
#endif
  _buffer.reserve(255); //reserve memory to prevent intern fragmention
}


//
//PRIVATE METHODS
//
String Sim800l::_readSerial(uint16_t timeout) {
  while  (!SIM.available() && --timeout)
  {
    delay(1);
    //    Serial.println(timeout);
  }
  if (SIM.available()) {
    return SIM.readString();
  }

  return "";
}


//
//PUBLIC METHODS
//

//void Sim800l::reset() {
//#if (LED)
//  digitalWrite(LED_PIN, 1);
//#endif
//  digitalWrite(RESET_PIN, 1);
//  delay(1000);
//  digitalWrite(RESET_PIN, 0);
//  delay(1000);
//  // wait for the module response
//
//  SIM.print(F("AT\r\n"));
//  while (_readSerial().indexOf("OK") == -1 ) {
//    SIM.print(F("AT\r\n"));
//  }
//
//  //wait for sms ready
//  while (_readSerial().indexOf("SMS") == -1 ) {
//  }
//#if (LED)
//  digitalWrite(LED_PIN, 0);
//#endif
//}

void Sim800l::setPhoneFunctionality() {
  /*AT+CFUN=<fun>[,<rst>]
    Parameters
    <fun> 0 Minimum functionality
    1 Full functionality (Default)
    4 Disable phone both transmit and receive RF circuits.
    <rst> 1 Reset the MT before setting it to <fun> power level.
  */
  SIM.print (F("AT+CFUN=1\r\n"));
}


void Sim800l::signalQuality() {
  /*Response
    +CSQ: <rssi>,<ber>Parameters
    <rssi>
    0 -115 dBm or less
    1 -111 dBm
    2...30 -110... -54 dBm
    31 -52 dBm or greater
    99 not known or not detectable
    <ber> (in percent):
    0...7 As RXQUAL values in the table in GSM 05.08 [20]
    subclause 7.2.4
    99 Not known or not detectable
  */
  SIM.print (F("AT+CSQ\r\n"));
  Serial.println(_readSerial());
}


void Sim800l::activateBearerProfile() {
  SIM.print (F(" AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\" \r\n" )); _buffer = _readSerial(); // set bearer parameter
  SIM.print (F(" AT+SAPBR=3,1,\"APN\",\"internet\" \r\n" )); _buffer = _readSerial(); // set apn
  SIM.print (F(" AT+SAPBR=1,1 \r\n")); delay(1200); _buffer = _readSerial(); // activate bearer context
  SIM.print (F(" AT+SAPBR=2,1\r\n ")); delay(3000); _buffer = _readSerial(); // get context ip address
}


void Sim800l::deactivateBearerProfile() {
  SIM.print (F("AT+SAPBR=0,1\r\n "));
  delay(1500);
}



bool Sim800l::answerCall() {
  SIM.print (F("ATA\r\n"));
  _buffer = _readSerial();
  //Response in case of data call, if successfully connected
  if ( (_buffer.indexOf("OK") ) != -1 ) return true;
  else return false;
}


void  Sim800l::callNumber(char* number) {
  SIM.print (F("ATD"));
  SIM.print (number);
  SIM.print (F("\r\n"));
}



uint8_t Sim800l::getCallStatus() {
  /*
    values of return:

    0 Ready (MT allows commands from TA/TE)
    2 Unknown (MT is not guaranteed to respond to tructions)
    3 Ringing (MT is ready for commands from TA/TE, but the ringer is active)
    4 Call in progress

  */
  SIM.print (F("AT+CPAS\r\n"));
  _buffer = _readSerial();
  return _buffer.substring(_buffer.indexOf("+CPAS: ") + 7, _buffer.indexOf("+CPAS: ") + 9).toInt();

}



bool Sim800l::hangoffCall() {
  SIM.print (F("ATH\r\n"));
  _buffer = _readSerial();
  if ( (_buffer.indexOf("OK") ) != -1) return true;
  else return false;
}






bool Sim800l::sendSms(char* number, char* text) {

  SIM.print (F("AT+CMGF=1\r")); //set sms to text mode
  _buffer = _readSerial();
  SIM.print (F("AT+CMGS=\""));  // command to send sms
  SIM.print (number);
  SIM.print(F("\"\r"));
  _buffer = _readSerial();
  SIM.print (text);
  SIM.print ("\r");
  //change delay 100 to readserial
  _buffer = _readSerial();
  SIM.print((char)26);
  _buffer = _readSerial();
  //expect CMGS:xxx   , where xxx is a number,for the sending sms.
  if (((_buffer.indexOf("CMGS") ) != -1 ) ) {
    return true;
  }
  else {
    return false;
  }
}


String Sim800l::getNumberSms(uint8_t index) {
  _buffer = readSms(index);
  Serial.println(_buffer.length());
  if (_buffer.length() > 10) //avoid empty sms
  {
    uint8_t _idx1 = _buffer.indexOf("+CMGR:");
    _idx1 = _buffer.indexOf("\",\"", _idx1 + 1);
    return _buffer.substring(_idx1 + 3, _buffer.indexOf("\",\"", _idx1 + 4));
  } else {
    return "";
  }
}



String Sim800l::readSms(uint8_t index) {
  SIM.print (F("AT+CMGF=1\r"));
  if (( _readSerial().indexOf("ER")) == -1) {
    SIM.print (F("AT+CMGR="));
    SIM.print (index);
    SIM.print("\r");
    _buffer = _readSerial();
    if (_buffer.indexOf("CMGR:") != -1) {
      return _buffer;
    }
    else return "";
  }
  else
    return "";
}


bool Sim800l::delAllSms() {
  SIM.print(F("at+cmgda=\"del all\"\n\r"));
  _buffer = _readSerial();
  if (_buffer.indexOf("OK") != -1) {
    return true;
  } else {
    return false;
  }

}


void Sim800l::RTCtime(int *day, int *month, int *year, int *hour, int *minute, int *second) {
  SIM.print(F("at+cclk?\r\n"));
  // if respond with ERROR try one more time.
  _buffer = _readSerial();
  if ((_buffer.indexOf("ERR")) != -1) {
    delay(50);
    SIM.print(F("at+cclk?\r\n"));
  }
  if ((_buffer.indexOf("ERR")) == -1) {
    _buffer = _buffer.substring(_buffer.indexOf("\"") + 1, _buffer.lastIndexOf("\"") - 1);
    *year = _buffer.substring(0, 2).toInt();
    *month = _buffer.substring(3, 5).toInt();
    *day = _buffer.substring(6, 8).toInt();
    *hour = _buffer.substring(9, 11).toInt();
    *minute = _buffer.substring(12, 14).toInt();
    *second = _buffer.substring(15, 17).toInt();
  }
}

//Get the time  of the base of GSM
String Sim800l::dateNet() {
  SIM.print(F("AT+CIPGSMLOC=2,1\r\n "));
  _buffer = _readSerial();

  if (_buffer.indexOf("OK") != -1 ) {
    return _buffer.substring(_buffer.indexOf(":") + 2, (_buffer.indexOf("OK") - 4));
  } else
    return "0";
}

// Update the RTC of the module with the date of GSM.
bool Sim800l::updateRtc(int utc) {

  activateBearerProfile();
  _buffer = dateNet();
  deactivateBearerProfile();

  _buffer = _buffer.substring(_buffer.indexOf(",") + 1, _buffer.length());
  String dt = _buffer.substring(0, _buffer.indexOf(","));
  String tm = _buffer.substring(_buffer.indexOf(",") + 1, _buffer.length()) ;

  int hour = tm.substring(0, 2).toInt();
  int day = dt.substring(8, 10).toInt();

  hour = hour + utc;

  String tmp_hour;
  String tmp_day;
  //TODO : fix if the day is 0, this occur when day is 1 then decrement to 1,
  //       will need to check the last month what is the last day .
  if (hour < 0) {
    hour += 24;
    day -= 1;
  }
  if (hour < 10) {

    tmp_hour = "0" + String(hour);
  } else {
    tmp_hour = String(hour);
  }
  if (day < 10) {
    tmp_day = "0" + String(day);
  } else {
    tmp_day = String(day);
  }
  //for debugging
  //Serial.println("at+cclk=\""+dt.substring(2,4)+"/"+dt.substring(5,7)+"/"+tmp_day+","+tmp_hour+":"+tm.substring(3,5)+":"+tm.substring(6,8)+"-03\"\r\n");
  SIM.print("at+cclk=\"" + dt.substring(2, 4) + "/" + dt.substring(5, 7) + "/" + tmp_day + "," + tmp_hour + ":" + tm.substring(3, 5) + ":" + tm.substring(6, 8) + "-03\"\r\n");
  if ( (_readSerial().indexOf("ER")) != -1) {
    return false;
  } else return true;


}
