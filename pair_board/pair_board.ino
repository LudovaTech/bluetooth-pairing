#include <SoftwareSerial.h>

SoftwareSerial master(11, 12);
SoftwareSerial slave(9, 10);

bool speak_with_master = true;
String receiveBuffer = "";
bool activeInteractiveMode = true;

void setup() {
    Serial.begin(38400);
    Serial.println("configuring...");
    master.begin(38400);
    slave.begin(38400);
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    Serial.println("speaking with master...");
    master.listen();
}

void loop() {
    transmitInfos();
    if (activeInteractiveMode) {
        interactiveMode();
    }
}

void transmitInfos() {
    // Lire depuis le master
    if (master.available()) {
        String receive = "";
        while (master.available()) {
            char receiveChar = master.read();
            if (receiveChar == '\r' || receiveChar == '\n') {
                if (receiveChar == '\r') master.read(); // Lire le '\n' suivant
                break;
            } else {
                receive += receiveChar;
            }
        }
        Serial.println("master : " + receive);
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
}


void interactiveMode() {
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
    if (command == "BC+WHICH?" || command == "bw") {
        if (speak_with_master) {
            Serial.println("board  : speaking with master");
        } else {
            Serial.println("board  : speaking with slave");
        }
    } else if (command == "BC+WHICH=MASTER" || command == "bwm") {
        speak_with_master = true;
        master.listen();
        Serial.println("board  : now speaking with master");
    } else if (command == "BC+WHICH=SLAVE" || command == "bws") {
        speak_with_master = false;
        slave.listen();
        Serial.println("board  : now speaking with slave");
    } else {
        if (!command.startsWith("AT")) {
            Serial.println("board  : unknown command : '" + command + "'");
        } else {
            if (speak_with_master) {
                master.print(command + "\r\n");
            } else {
                slave.print(command + "\r\n");
            }
        }
    }
}
