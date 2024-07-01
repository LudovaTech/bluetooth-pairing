#include <SoftwareSerial.h>

SoftwareSerial master(11, 12);
SoftwareSerial slave(9, 10);

bool speak_with_master = true;

void setup() {
    Serial.begin(38400);
    Serial.println("configuring...");
    master.begin(38400);
    slave.begin(38400);
    Serial.println("speaking with master...");
    master.listen();
    master.print("AT\r\n");
}

String receiveBuffer = "";

void loop() {
    // Lire depuis le master
    while (master.available()) {
        char c = master.read();
        Serial.print(c);
    }

    // Lire depuis le slave
    if (slave.available()) {
        String receive = "";
        while (slave.available()) {
            char receiveChar = slave.read();
            if (receiveChar == '\r' || receiveChar == '\n') {
                if (receiveChar == '\r') slave.read(); // Lire le '\n' suivant
                break;
            } else {
                receive += receiveChar;
            }
        }
        Serial.println("slave  : " + receive);
    }

    // Lire depuis Serial
    if (Serial.available()) {
        while (Serial.available()) {
            receiveBuffer += (char)Serial.read();
            if (receiveBuffer.endsWith("\r\n")) {
                processSerialCommand(receiveBuffer);
                receiveBuffer = ""; // Réinitialiser le buffer après traitement
            }
        }
    }
}

void processSerialCommand(String command) {
    command.trim(); // Supprimer les espaces en début et en fin de la commande
    if (command == "BC+WHICH?") {
        if (speak_with_master) {
            Serial.println("board  : speaking with master");
        } else {
            Serial.println("board  : speaking with slave");
        }
    } else if (command == "BC+WHICH=MASTER") {
        speak_with_master = true;
        master.listen();
        Serial.println("board  : now speaking with master");
    } else if (command == "BC+WHICH=SLAVE") {
        speak_with_master = false;
        slave.listen();
        Serial.println("board  : now speaking with slave");
    } else {
        if (!command.startsWith("AT")) {
            Serial.println("board  : unknown command : '" + command + "'");
        } else {
            if (speak_with_master) {
                Serial.println("send '" + command + "'");
                master.print(command + "\r\n");
                delay(1000);
            } else {
                slave.print(command + "\r\n");
            }
        }
    }
}
