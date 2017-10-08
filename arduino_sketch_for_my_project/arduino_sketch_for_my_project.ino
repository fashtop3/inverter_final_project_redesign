
#include "Sim800l.h"
#include "wdt.h"

Sim800l sim800;

/**
 * 1. check if sim module has registered newtwork
 *    else Goto reset
 * 2. if is registered network set apn
 * 3. attempt disconnect/connect to internet
 * 4. first query for data from inveter
 * 5. send data to server and get response from server
 * 6. send data to inverter
 * 7. repeat 4.
 *    :if STEP 5 fails retry 3x 
 *    else go to STEP 3 if attempt 3x if Fail goto STEP 1 
 */

 uint8_t len = 10;
 char req = 'Q';

void setup() {
  //STATE:1,1,70
    sim800.init();
    sim800.wakeup();
    sim800.enableGPRS();
    delay(5000);
    WDT_rst();
    while(sim800.sendInverterReq(req)) {
      sim800.httpRequest(len);
      req = 'D';
    }
}

void loop() {
}
