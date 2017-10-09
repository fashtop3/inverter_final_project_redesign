#include "wdt.h"
#include "Arduino.h"

void WDT_rst(void)
{
  WDTCSR = 0x18;
  WDTCSR = 0x08;
  Serial.println("SOFT.RST");
  while (1);
}
