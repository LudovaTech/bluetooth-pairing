#include <SoftwareSerial.h>

SoftwareSerial master(11, 12);
SoftwareSerial slave(9, 10);
const int alimPin = 13;

String receiveBuffer = "";
bool activeInteractiveMode = true;
bool _speaking_with_master = true;

void sendToMaster(String message) {
  if (!_speaking_with_master) {
    master.listen();
    _speaking_with_master = true;
  }
  Serial.println("board  -> master : " + message);
  master.print(message + "\r\n");
}

void sendToSlave(String message) {
  if (_speaking_with_master) {
    slave.listen();
    _speaking_with_master = false;
  }
  Serial.println("board  ->  slave : " + message);
  slave.print(message + "\r\n");
}

String _readFrom(SoftwareSerial *ser) {
  String receive = "";
  while (ser->available()) {
    char receiveChar = ser->read();
    if (receiveChar == '\r' || receiveChar == '\n') {
      if (receiveChar == '\r') ser->read();  // Lire le '\n' suivant
      break;
    } else {
      receive += receiveChar;
    }
  }
  return receive;
}

String readFromMaster() {
  return _readFrom(&master);
}

String readFromSlave() {
  return _readFrom(&slave);
}

void setup() {
  Serial.begin(38400);
  Serial.println("configuring...");
  master.begin(38400);
  slave.begin(38400);
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
  if (master.available()) {
    Serial.println("master : " + readFromMaster());
  }

  if (slave.available()) {
    Serial.println("slave  : " + readFromSlave());
  }
}

void interactiveMode() {
  // Lire depuis Serial
  if (Serial.available()) {
    while (Serial.available()) {
      receiveBuffer += (char)Serial.read();
      if (receiveBuffer.endsWith("\r\n")) {
        processSerialCommand(receiveBuffer);
        receiveBuffer = "";  // Réinitialiser le buffer après traitement
      }
    }
  }
}

void processSerialCommand(String command) {
  command.trim();  // Supprimer les espaces en début et en fin de la commande
  if (command == "BC+WHICH?" || command == "bw") {
    if (_speaking_with_master) {
      Serial.println("board  : speaking with master");
    } else {
      Serial.println("board  : speaking with slave");
    }
  } else if (command == "BC+WHICH=MASTER" || command == "bwm") {
    _speaking_with_master = true;
    master.listen();
    Serial.println("board  : now speaking with master");
  } else if (command == "BC+WHICH=SLAVE" || command == "bws") {
    _speaking_with_master = false;
    slave.listen();
    Serial.println("board  : now speaking with slave");
  } else {
    if (!command.startsWith("AT")) {
      Serial.println("board  : unknown command : '" + command + "'");
    } else {
      if (_speaking_with_master) {
        sendToMaster(command);
      } else {
        sendToSlave(command);
      }
    }
  }
}
