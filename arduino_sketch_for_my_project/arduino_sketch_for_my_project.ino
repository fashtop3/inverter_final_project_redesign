
#include "Sim800l.h"

Sim800l sim800;

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
