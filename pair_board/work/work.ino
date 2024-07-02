#include <SoftwareSerial.h>

SoftwareSerial master(11, 12); // RX, TX

void setup() {
    Serial.begin(38400);
    master.begin(38400);
    
    Serial.println("Entering AT mode...");
    delay(1000); // Wait for the module to initialize
    
    Serial.println("Sending AT command...");
    master.print("AT\r\n"); // Send the AT command
    
    delay(1000); // Wait for response
    if (master.available()) {
        while (master.available()) {
            char c = master.read();
            Serial.print(c); // Print the response from HC-05
        }
    } else {
        Serial.println("No response from HC-05");
    }
}

void loop() {
    // Nothing needed here
}
