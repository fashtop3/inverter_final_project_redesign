
#include "Sim800l.h"

Sim800l sim800;

void setup() {
  //STATE:1,1,70
    sim800.init();
    sim800.reset();
    delay(10000);
    sim800.sendInverterReq();
//    sim800.setup();
}

void loop() {
}
