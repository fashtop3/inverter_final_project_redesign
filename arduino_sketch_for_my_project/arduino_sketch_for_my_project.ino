
#include "Sim800l.h"

Sim800l sim800;

void setup() {
  //STATE:1,1,70
    sim800.init();
    sim800.setHostname("52.170.211.220/api/report/1");
    sim800.setup();
}

void loop() {
}
