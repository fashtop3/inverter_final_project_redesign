
#include <SoftwareSerial.h>

char inByte, inByte2 = 0;

// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
SoftwareSerial mySerial(9, 10);

void setup() {
  //STATE:1,1,"70"
  // initialize both serial ports:
  Serial.begin(9600);
  Serial.println("Module testing");

  mySerial.begin(57600);
}

void loop() {
  if(Serial.available()){
    inByte = Serial.read();
    //Serial.print(inByte);
    mySerial.print(inByte);
  }

  if(mySerial.available()) {
    //Serial.println("Sending from AT32");
    inByte2 = mySerial.read();
    Serial.print(inByte2);
  }
}
