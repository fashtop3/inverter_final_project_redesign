#include "Arduino.h"
#include "Sim800l.h"
#include "wdt.h"

SoftwareSerial SIM(SIM_RX_PIN, SIM_TX_PIN);
SoftwareSerial INV(INV_RX_PIN, INV_TX_PIN);

Sim800l::init()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("INIT.");

  INV.begin(57600);
  SIM.begin(9600); // INITIALIZE UART


  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  pinMode(DQR_LED, OUTPUT);
  pinMode(RDS_LED, OUTPUT);
  digitalWrite(DQR_LED, LOW);
  digitalWrite(RDS_LED, LOW);
  digitalWrite(RESET_PIN, HIGH);

  urc_status = 0xff;

  _is_connected = false;
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
  blink_reset_led();

  expect_AT_OK(F(""), 2000);
  expect_AT_OK(F("E0"), 3000);
  if (expect_AT_OK(F("E0"), 2000))
  {
    Serial.println("SIM UP");
    delay(1000);
    _eat_echo();
  }

  if (!registerNetwork()) {
    reset();
    delay(5000);
    return wakeup();
  }

  //  blink_reset_led();

  return true;
}


bool Sim800l::reset() {
  bool ok = false;

  Serial.println("RST"); // REset

  blink_reset_led();
  // wait for the module response

  digitalWrite(RESET_PIN, 0);
  delay(1000);
  digitalWrite(RESET_PIN, 1);
  // wait for the module response

  delay(5000);
  _eat_echo();
  expect_AT_OK(F(""), 6000);
  _eat_echo();


  blink_reset_led();

  return expect_AT_OK(F("E0"), 2000); //TODO: PUT RETRIES HERE
}


void Sim800l::blink_led(unsigned char PIN)
{
  uint8_t level = 0;
  for (int i = 0; i < 10; i++) {
    digitalWrite(PIN, level);
    delay(200);
    level ^= 1;
  }
}

void Sim800l::blink_reset_led()
{
  uint8_t level = 0;
  for (int i = 0; i < 14; i++) {
    level ^= 1;
    digitalWrite(LED_PIN, level);
    digitalWrite(DQR_LED, level);
    digitalWrite(RDS_LED, level);
    delay(200);
  }
}

void Sim800l::http_error_led()
{
  uint8_t level = 0;
  for (int i = 0; i < 6; i++) {
    level ^= 1;
    digitalWrite(DQR_LED, level);
    digitalWrite(RDS_LED, level);
    delay(200);
  }
}

bool Sim800l::expect_AT_OK(const char *cmd, uint16_t timeout)
{
  return expect_AT(cmd, F("OK"), timeout);
}

bool Sim800l::expect_AT(const char *cmd, const char *expected, uint16_t timeout)
{
  print(F("AT"));
  println(cmd);
  return expect(expected, SIM, timeout);
}

bool Sim800l::expect_OK(uint16_t timeout)
{
  return expect(F("OK"), SIM, timeout);
}

bool Sim800l::expect(const char *expected, SoftwareSerial &s, uint16_t timeout)
{
  do len = readline(buf, SIM800_BUFSIZE, s, timeout); while (is_urc(buf, len));
  return strcmp(buf, (const char *) expected) == 0;
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
        if (sendInverterReq('Q')) {
          delay(1000);
          //          httpRequest();
        }
        else {
          setup();
        }
        //        __hard_reset__();
      }
    }
  }
}

bool Sim800l::registerNetwork()
{
  _eat_echo();
  //  delay(30000);
  network_led();
  unsigned short int n = 0;
  println(F("AT+CREG?"));
  if (expect_scan(F("+CREG: 0,%hu"), &n, SIM, 2000)) {
    if (n == 1 || n == 5)
    {
      Serial.println("NR"); //network regsitered
      network_led_found(true);
      return true;
    }
  }
  Serial.println("NE?"); //no reg netwwrk found
  network_led_found(false);
  delay(2000);
  if (millis() > 40000) {
    WDT_rst();
  }
  return false;
}

void Sim800l::network_led()
{
  //30 sec delay
  for (int i = 0; i < 100; i++) {
    digitalWrite(LED_PIN, 1);
    delay(100);
    digitalWrite(LED_PIN, 0);
    digitalWrite(DQR_LED, 1);
    delay(100);
    digitalWrite(DQR_LED, 0);
    digitalWrite(RDS_LED, 1);
    delay(100);
    digitalWrite(RDS_LED, 0);
  }
}

void Sim800l::network_led_found(bool found)
{
  if (found) {
    digitalWrite(RDS_LED, 1); return;
  }
  digitalWrite(DQR_LED, 1);
}

bool Sim800l::setSwitchAPN()
{
  static bool *set = 0;

  if (*set) return *set;

  //TODO:  ADD RETRIES
  char cs[15];
  println(F("AT+CSCA?"));
  //+CSCA: "+234803000000",145
  if (expect_scan(F("+CSCA: \"%13s\",%*lu"), &cs, SIM, 5000))
  {
    _eat_echo();

    if (strcmp(cs, (char*)"+234803000000") == 0) //mtn
    {
      setAPN("web.gprs.mtnnigeria.net", "web", "web"); return *set = true;
    }
    else if (strcmp(cs, (char*)"+234809000151") == 0) //etisalat correct is +2348090001518
    {
      setAPN("etisalat", "web", "web"); return *set = true;
    }
    else if (strcmp(cs, (char*)"+234802000000") == 0) //airtel correct is +2348020000009
    {
      setAPN("internet.ng.airtel.com", "web", "web"); return *set = true;
    }
    else if (strcmp(cs, (char*)"+234805000150") == 0) //glo  correct is +2348050001501
    {
      setAPN("gloflat", "web", "web"); return *set = true;
    }
    else
    {
      //__hard_reset__();
    }
  }

  return *set;
}

bool Sim800l::enableGPRS(uint16_t timeout)
{

  setSwitchAPN();

#if (LED)
  digitalWrite(LED_PIN, 0);
#endif
  _is_connected = false;

  //  if (_is_connected)
  disableGPRS();

  expect_AT(F("+CIPSHUT"), F("SHUT OK"), 5000);
  expect_AT_OK(F("+CIPMUX=1")); // enable multiplex mode
  expect_AT_OK(F("+CIPRXGET=1")); // we will receive manually

  while (!_is_connected && timeout > 0) {
    _is_connected = expect_AT_OK(F("+CGATT=1"), 5000);
    delay(1000);
    timeout -= 1000;
  }

  if (!_is_connected) return false;
  _is_connected = false;

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

  do {
    println(F("AT+CGATT?"));
    _is_connected = expect(F("+CGATT: 1"), SIM);
    _delay_ms(10);
  } while (--timeout && !_is_connected);

  //CONNECTED or CONNECT ERROR
  if (_is_connected) {
    delay(2000);
    Serial.println("CT");
#if (LED)
    digitalWrite(LED_PIN, 1);
#endif
  } else {
    Serial.println("CE");
  }
  return _is_connected;
}

bool Sim800l::disableGPRS()
{
  expect_AT(F("+CIPSHUT"), F("SHUT OK"));
  if (!expect_AT_OK(F("+SAPBR=0,1"), 30000)) return false;

  return expect_AT_OK(F("+CGATT=0"));
}




bool Sim800l::expect_scan(const char *pattern, void *ref, SoftwareSerial &s, uint16_t timeout)
{
  do len = readline(buf, SIM800_BUFSIZE, s, timeout); while (is_urc(buf, len));
  return sscanf(buf, (const char *) pattern, ref) == 1;
}

bool Sim800l::expect_scan(const char *pattern, void *ref, void *ref1, SoftwareSerial &s, uint16_t timeout)
{
  do len = readline(buf, SIM800_BUFSIZE, s, timeout); while (is_urc(buf, len));
  return sscanf(buf, pattern, ref, ref1) == 2;
}

bool Sim800l::expect_scan(const char *pattern, void *ref, void *ref1, void *ref2, SoftwareSerial &s, uint16_t timeout)
{
  do len = readline(buf, SIM800_BUFSIZE, s, timeout); while (is_urc(buf, len));
  return sscanf(buf, (const char *) pattern, ref, ref1, ref2) == 3;
}

size_t Sim800l::readline(char *buf, uint8_t maxIdx, SoftwareSerial &s, uint16_t timeout)
{
  uint16_t idx = 0;
  while (--timeout) {
    while (s.available()) {
      char c = (char) s.read();
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
  expect_scan(F("+SAPBR: 1,%hu,\"%*hu.%*hu.%*hu.%*hu\""), &status, SIM, 3000); //+SAPBR: 1,1,"10.96.42.184"

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
  println(F("AT+CPOWD=1"));
  _delay_ms(20);
  //expect_AT_OK(F("+CPOWD=1"));
  if (expect(F("NORMAL POWER DOWN"), SIM)) {
#ifdef DEBUG_MODE
    Serial.println("NPD"); //NORMAL POWER DOWN
#endif
  }

  return true;
}


bool Sim800l::sendInverterReq(const char req)
{
  //  DATA:D:1,0,75
  //  DATA:Q:1,0,75
#ifdef DEBUG_MODE
  Serial.println("PUSHING DATA TO INVERTER...");
#endif
#if (LED)
  digitalWrite(LED_PIN, 1);
#endif
  INV.listen();
  delay(1000);
  INV.print("DATA:");
  INV.print(req);
  INV.print(':');
  INV.print(_is_connected);
  INV.print(',');
  INV.print(power);
  INV.print(',');
  INV.print(load_max);
  INV.print('\n');
  //  INV.println("DATA:<MODE:[Q|D]>:1,1,70");
  delay(1000);
  // Response format DATA:0,13.14,38,0,1,1 => DATA:<AC IN>,<BATTERY LEVEL>,<LOAD RANGE>,<CHARGING>,<CURRENT POWER STATE>,<CURRENT BACKUP STATE>

  char data[22];
  Serial.println("DQR"); //DATA QUERY
  if (expect_scan(F("DATA:%s"), data, INV, 5000)) {
    //    Serial.println(data); //, &b, &l, &c, &p, &k //,%hu,%hu,%hu,%hu,%hu
    if (sscanf(data, "%hu,%[^,],%hu,%hu,%hu,%hu", &a, b, &l, &c, &p, &k) == 6) {
      Serial.println("+DSET");
      blink_led(DQR_LED);
      SIM.listen();
      delay(1000);
      return true;
    }
  }
#if (LED)
  digitalWrite(LED_PIN, 0);
#endif
  return sendInverterReq(req);
}


void Sim800l::httpRequest(uint8_t &len)
{
  Serial.println("REQ.");
#if (LED)
  digitalWrite(LED_PIN, 1);
#endif

  uint8_t status = HTTP_get(len);
  if (status == 200)
  {
    if (HTTP_read(0, len)) {
      //DATA:1,56,1
      if (expect_scan(F("DATA:%hu,%hu,%hu"), &power, &load_max, &output, SIM, 3000)) {
        Serial.println("RDS");//REQUEST DATA SET
#ifdef DEBUG_MODE
        Serial.println(power);
        Serial.println(load_max);
        Serial.println(output);
#endif
        blink_led(RDS_LED);
      }
      _eat_echo();
    }
  } else {
    http_error_led();
  }

#if (LED)
  digitalWrite(LED_PIN, 0);
#endif

}

unsigned short int Sim800l::HTTP_get(uint8_t &len)
{
  //serialWriteString(0, "Debugging\n");
  expect_AT_OK(F("+HTTPTERM"));
  delay(3000);

  if (!expect_AT_OK(F("+HTTPINIT"))) return 100;
  if (!expect_AT_OK(F("+HTTPPARA=\"CID\",1"))) return 110;
  if (!expect_AT_OK(F("+HTTPPARA=\"UA\",\"IOTINV#1 r0.1\""))) return 102;
  //if (!expect_AT_OK(F("+HTTPPARA=\"REDIR\",1"))) return 1103; //1 allows reirect , o means no redirection
  Serial.println("GET");
  print("AT+HTTPPARA=\"URL\"");
  print(F(",\""));
  print(hostname);
  print("?t=p&a=");
  print(a);
  print("&b=");
  print(b);
  print("&c=");
  print(c);
  print("&l=");
  print(l);
  print("&p=");
  print(p);
  print("&k=");
  print(k);
  println(F(",\""));
  if (!expect_OK()) return 111;

  if (!expect_AT_OK(F("+HTTPACTION=0"))) return 104;

  uint16_t stat;
  expect_scan(F("+HTTPACTION: 0,%hu,%su"), &stat, &len, SIM, 30000);
  delay(100);
  Serial.println(stat);
  return stat;
}

bool Sim800l::HTTP_read(uint8_t start, uint8_t len)
{

  print("AT+HTTPREAD=");
  print(start);
  print(",");
  println(len);

  //println(F("AT+HTTPREAD=0,1"));
  delay(13);
#ifdef DEBUG_MODE
  Serial.println("HTTP_read");
#endif
  uint16_t avail;
  return expect_scan(F("+HTTPREAD: %hu"), &avail, SIM);
}


size_t Sim800l::HTTP_read(char *b, uint8_t start, uint8_t len)
{
  print("AT+HTTPREAD=");
  print(start);
  print(",");
  println(len);

  //println(F("AT+HTTPREAD=0,1"));
  delay(13);
#ifdef DEBUG_MODE
  Serial.println("HTTP_read");
#endif
  uint16_t avail;
  expect_scan(F("+HTTPREAD: %hu"), &avail, SIM);
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
