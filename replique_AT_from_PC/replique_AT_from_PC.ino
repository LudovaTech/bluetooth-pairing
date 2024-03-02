#include <SoftwareSerial.h>
#define DebugSerial Serial

#define RX_pin 10
#define TX_pin 11


SoftwareSerial BluetoothSerial(RX_pin, TX_pin);


void setup() {
  pinMode(RX_pin, INPUT);
  pinMode(TX_pin, OUTPUT);
  DebugSerial.begin(9600);
  BluetoothSerial.begin(9600);  // default speed in AT command mode
}

void loop() {
  // Bluetooth -> PC
  if (BluetoothSerial.available()) {
    DebugSerial.println("in");
    DebugSerial.write(BluetoothSerial.read());
  }
  // PC -> Bluetooth
  if (DebugSerial.available()) {
    auto info = DebugSerial.read();
    BluetoothSerial.write(info);
    DebugSerial.write(info);
  }
}
