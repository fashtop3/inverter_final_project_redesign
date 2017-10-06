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

SoftwareSerial SIM(SIM_RX_PIN, SIM_TX_PIN);
SoftwareSerial INV(INV_RX_PIN, INV_TX_PIN);

Sim800l::init()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Initializing sim module....");

  INV.begin(57600);
  SIM.begin(9600); // INITIALIZE UART


  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);

  urc_status = 0xff;
  param = ""; //.reserve(100);
  _isModuleTimeSet = false;
  _awake_ = false;
  _is_connected = false;
  _is_ntwk_reg = 0;

  power = 0;
  load_max = 75;
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

  _eat_echo();
  _awake_ = false;
  if (!expect_AT_OK(F(""), 2000))
  {
    Serial.println("!!! SIM800 Unavailable.... Restarting....");
    if (reset()) {
      Serial.println("SIM800 Restarted!!!");
      delay(5000);
    }
    wakeup();
  }

  _awake_ = true;
  Serial.println("!!! SIM800 awake");
  delay(1000);
  expect_AT_OK(F("E0"), 2000);
  _eat_echo();
  if (!registerNetwork(2000)) {
    reset();
    delay(5000);
    wakeup();
  }

#if (LED)
  digitalWrite(LED_PIN, 0);
#endif

  return _awake_;
}


bool Sim800l::reset() {
  bool ok = false;

  Serial.println("Reset Called...");
#if (LED)
  digitalWrite(LED_PIN, 1);
#endif

  digitalWrite(RESET_PIN, 0);
  delay(1000);
  digitalWrite(RESET_PIN, 1);
  // wait for the module response

  delay(3000);
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

bool Sim800l::expect_OK(uint16_t timeout)
{
  return expect(F("OK"), timeout);
}

bool Sim800l::expect(const char *expected, uint16_t timeout)
{
  char buf[SIM800_BUFSIZE];
  size_t len;
  do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
  //  Serial.println(buf);
  return strcmp(buf, (const char *) expected) == 0;
  //  return _readSerial(timeout).indexOf(expected) != -1 ? true : false;
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

void Sim800l::_eat_echo(SoftwareSerial &serial)
{
  while (serial.available())
  {
    serial.read();
    delay(1);
  }
}


void Sim800l::setup()
{
  if (wakeup()) {
    Serial.println("APN config.");
    if (setSwitchAPN()) {
#ifdef DEBUG_MODE
      Serial.println("APN Saved..");
      Serial.println(_apn);
      Serial.println("ENABLING GPRS...");
#endif
      if (enableGPRS()) {
#ifdef DEBUG_MODE
        Serial.println("GPRS ENABLED");
#endif
        delay(1000);
        //get initial internet state here
        sendInverterReq();
        delay(1000);
        httpRequest();
        //        __hard_reset__();
      }
    }
  }
}

bool Sim800l::registerNetwork(uint16_t timeout)
{
  _is_ntwk_reg = false;
#ifdef DEBUG_MODE
  Serial.println("Check Registered Network?...");
#endif
  _eat_echo();
  delay(20000);
  unsigned short int n = 0;
  println(F("AT+CREG?"));
  if (expect_scan(F("+CREG: 0,%hu"), &n), timeout) {
    if ((n == 1 || n == 5))
    {
      _is_ntwk_reg = true;
#ifdef DEBUG_MODE
      Serial.println("Registered Network");
#endif
      return true;
    }
  }

  Serial.println("No Registered Network Found?...");
  return false;
}

bool Sim800l::setSwitchAPN()
{
  //TODO:  ADD RETRIES
  char cs[15];
  println(F("AT+CSCA?"));
  //+CSCA: "+234803000000",145
  if (expect_scan(F("+CSCA: \"%13s\",%*lu"), &cs), 5000)
  {
    _eat_echo();

    if (strcmp(cs, (char*)"+234803000000") == 0) //mtn
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

bool Sim800l::enableGPRS(uint16_t timeout)
{
  bool attached = false;

  if (checkConnected())disableGPRS();

  expect_AT(F("+CIPSHUT"), F("SHUT OK"), 5000);
  expect_AT_OK(F("+CIPMUX=1")); // enable multiplex mode
  expect_AT_OK(F("+CIPRXGET=1")); // we will receive manually

  while (!attached && timeout > 0) {
    attached = expect_AT_OK(F("+CGATT=1"), 5000);
    delay(1000);
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
  if (!expect_AT_OK(F("+SAPBR=1,1"), 30000)) return false;
  _is_connected = true;

  do {
    println(F("AT+CGATT?"));
    attached = expect(F("+CGATT: 1"));
    _delay_ms(10);
  } while (--timeout && !attached);

  //_gprs_enable_delay = 0;

  return attached;
}

bool Sim800l::disableGPRS()
{
  expect_AT(F("+CIPSHUT"), F("SHUT OK"));
  if (!expect_AT_OK(F("+SAPBR=0,1"), 30000)) return false;

  return expect_AT_OK(F("+CGATT=0"));
}



bool Sim800l::expect_scan(const char *pattern, void *ref, uint16_t timeout)
{
  char buf[SIM800_BUFSIZE];
  size_t len;
  do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
#ifdef DEBUG_MODE
  Serial.println(buf);
#endif
  return sscanf(buf, (const char *) pattern, ref) == 1;
}

bool Sim800l::expect_scan(const char *pattern, void *ref, void *ref1, uint16_t timeout)
{
  char buf[SIM800_BUFSIZE];
  size_t len;
  do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
#ifdef DEBUG_MODE
  Serial.println(buf);
#endif
  return sscanf(buf, pattern, ref, ref1) == 2;
}

bool Sim800l::expect_scan(const char *pattern, void *ref, void *ref1, void *ref2, uint16_t timeout)
{
  char buf[SIM800_BUFSIZE];
  size_t len;
  do len = readline(buf, SIM800_BUFSIZE, timeout); while (is_urc(buf, len));
#ifdef DEBUG_MODE
  Serial.println(buf);
#endif
  return sscanf(buf, (const char *) pattern, ref, ref1, ref2) == 3;
}

size_t Sim800l::readline(char *buf, uint8_t maxIdx, uint16_t timeout)
{
  uint16_t idx = 0;
  while (--timeout) {
    while (SIM.available()) {
      char c = (char) SIM.read();
      if (c == '\r') continue;
      if (c == '\n') {
        if (!idx) continue;
        timeout = 0;
        break;
      }
      if (maxIdx - idx) {
        buf[idx++] = c;
      } else {
        timeout = 0;
        break;
      }
    }

    if (timeout == 0) break;
    delay(1);
  }
  buf[idx] = 0;
  return idx;
}

size_t Sim800l::read(char *buf, uint8_t len, SoftwareSerial &s)
{
  uint8_t idx = 0;
  while (s.available()) {
    char c = (char) s.read();
    if (len - idx) {
      buf[idx++] = c;
    } else {
      break;
    }
  }
  buf[idx] = 0;
  return idx;
}


bool Sim800l::is_urc(const char *line, size_t len)
{
  urc_status = 0xff;

  for (uint8_t i = 0; i < 17; i++)
  {
    char urc[30];
    strcpy(urc, (PGM_P)pgm_read_word(&(_urc_messages[i])));
    uint8_t urc_len = strlen(urc);
    if (len >= urc_len && !strncmp(urc, line, urc_len))
    {
      urc_status = i;
      return true;
    }
  }

  return false;
}

bool Sim800l::checkConnected()
{
  uint8_t status = 0;
  println(F("AT+SAPBR=2,1")); //check if module is connected to Internet
  expect_scan(F("+SAPBR: 1,%hu,\"%*hu.%*hu.%*hu.%*hu\""), &status, 3000); //+SAPBR: 1,1,"10.96.42.184"

  return  status == 0 ? false : true;
}

bool Sim800l::__hard_reset__()
{
  shutdown();
  return true;
}

bool Sim800l::shutdown()
{
  disableGPRS();
  _eat_echo();
  delay(2000);
#ifdef DEBUG_MODE
  Serial.println("SHUTTING DOWN");
#endif
  println(F("AT+CPOWD=1"));
  _delay_ms(20);
  //expect_AT_OK(F("+CPOWD=1"));
  if (expect(F("NORMAL POWER DOWN"))) {
#ifdef DEBUG_MODE
    Serial.println("NORMAL POWER DOWN");
#endif
  }
  _awake_ = false;

  return true;
}


bool Sim800l::sendInverterReq()
{
  //  STATE:1,1,70
#ifdef DEBUG_MODE
  Serial.println("PUSHING DATA TO INVERTER...");
#endif
  INV.listen();
  param = "";
  delay(1000);
  INV.print("DATA:Q:");
  INV.print(_is_connected);
  INV.print(',');
  INV.print(power);
  INV.print(',');
  INV.print(load_max);
  INV.print('\n');
  //  INV.println("STATE:1,1,70");
  delay(1000);
    param = _readSerial(INV, 3000); //DATA:0,13.14,38,0,1,1
  Serial.println("Checking..");
  
//  expect_scan(F("+HTTPACTION: 0,%hu,%su"), &stat, &len, 30000);
  
  if (param.indexOf("DATA") != -1) {
    Serial.println(param);
  } 
  else {
    sendInverterReq();
  }
  SIM.listen();
  delay(1000);
  //  _eat_echo(INV);
#ifdef DEBUG_MODE
  Serial.println("PUSHING DATA FINISHED...");
#endif
  return param != "";
}


String Sim800l::getUrl()
{
  return hostname + param;
}

void Sim800l::httpRequest()
{
  uint8_t len = 0;
#ifdef DEBUG_MODE
  Serial.print("SENDING HTTPREQUEST:.");
#endif
  String url = getUrl();
  uint8_t status = HTTP_get(url, len);
  if (status == 200)
  {
    if (HTTP_read(0, len)) {
      //DATA:1,56,1
      if (expect_scan(F("DATA:%hu,%hu,%hu"), &power, &load_max, &output, 3000)) {

        Serial.println("confirmed");
#ifdef DEBUG_MODE
        Serial.println(power);
        Serial.println(load_max);
        Serial.println(output);
#endif
        delay(100);
        sendInverterReq();
      }
      _eat_echo();
    }
    //    return;
  }
  delay(1000);
  httpRequest();

#ifdef DEBUG_MODE
  Serial.print("HTTPREQUEST RESPONSE CODE: ");
  Serial.println(stat);
#endif
}

unsigned short int Sim800l::HTTP_get(const String & url, uint8_t &len)
{
  //serialWriteString(0, "Debugging\n");
  expect_AT_OK(F("+HTTPTERM"));
  delay(2000);

  char bArr[url.length() + 1];
  url.toCharArray(bArr, url.length());

  if (!expect_AT_OK(F("+HTTPINIT"))) return 100;
  if (!expect_AT_OK(F("+HTTPPARA=\"CID\",1"))) return 110;
  if (!expect_AT_OK(F("+HTTPPARA=\"UA\",\"IOTINV#1 r0.1\""))) return 102;
  //if (!expect_AT_OK(F("+HTTPPARA=\"REDIR\",1"))) return 1103; //1 allows reirect , o means no redirection
  println_param("AT+HTTPPARA=\"URL\"", bArr);
  delay(100);
  Serial.println(bArr);
  if (!expect_OK()) return 111;

  if (!expect_AT_OK(F("+HTTPACTION=0"))) return 104;

  uint16_t stat;
  expect_scan(F("+HTTPACTION: 0,%hu,%su"), &stat, &len, 30000);
  delay(100);
  Serial.println(stat);   //printing status
  //  Serial.println(len); //printing data length
  return stat;
}

bool Sim800l::HTTP_read(uint8_t start, uint8_t len)
{
  char c[25];
  char s[3];
  char l[3];
  itoa(start, s, 10);
  itoa(len, l, 10);

  strcpy(c, "AT+HTTPREAD=");
  strcat(c, s);
  strcat(c, ",");
  strcat(c, l);

  println(c);
  //println(F("AT+HTTPREAD=0,1"));
  delay(13);
#ifdef DEBUG_MODE
  Serial.println("POINT OF READING DATA.");
#endif
  uint16_t avail;
  return expect_scan(F("+HTTPREAD: %hu"), &avail);
}


size_t Sim800l::HTTP_read(char *b, uint8_t start, uint8_t len)
{
  char c[25];
  char s[3];
  char l[3];
  itoa(start, s, 10);
  itoa(len, l, 10);

  strcpy(c, "AT+HTTPREAD=");
  strcat(c, s);
  strcat(c, ",");
  strcat(c, l);

  println(c);
  //println(F("AT+HTTPREAD=0,1"));
  delay(13);
#ifdef DEBUG_MODE
  Serial.println("POINT OF READING DATA.");
#endif
  uint16_t avail;
  expect_scan(F("+HTTPREAD: %hu"), &avail);
  size_t idx = read(b, len, SIM); //http://52.170.211.220/api/r/1?t=p&a=0&b=18.04&c=0&l=55
  if (!expect_OK()) return 0;
  return idx;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

//String _buffer;


//
//PRIVATE METHODS
//
String Sim800l::_readSerial(const SoftwareSerial & serial, uint16_t timeout) {
  while  (!serial.available() && --timeout)
  {
    delay(1);
  }
  if (serial.available()) {
    return serial.readString();
  }

  return "";
}


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
  _buffer = dateNet();
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
