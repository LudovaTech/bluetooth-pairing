#include <SoftwareSerial.h>

SoftwareSerial module(6, 7);

void setup() {
  Serial.begin(38400);
  Serial.println("starting...");
  module.begin(9600);
}


void loop() {
  if (module.available()) {
    Serial.print(module.read());
  }
}