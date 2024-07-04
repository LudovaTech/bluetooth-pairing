#include <SoftwareSerial.h>

SoftwareSerial module(11, 12);

void setup() {
  Serial.begin(115200);
  Serial.println("starting...");
  module.begin(115200);
}


void loop() {
  if (module.available()) {
    char incoming = module.read();
    Serial.print(incoming);
  }
}