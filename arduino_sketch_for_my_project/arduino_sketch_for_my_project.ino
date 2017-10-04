
#include "Sim800l.h"

Sim800l sim;

void setup() {
  //STATE:1,1,70
    sim.init();
    sim.setup();
}

void loop() {
}
