
#include "Sim800l.h"

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

 int *control  = 0;

void setup() {
  //STATE:1,1,70
    sim800.init();
//    sim800.reset();
    delay(5000);
    if(sim800.sendInverterReq('Q')) {
      uint8_t len = 10;
      sim800.HTTP_get(len);
    }
//    sim800.setup();
}

void loop() {
}
