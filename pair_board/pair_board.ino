#include <SoftwareSerial.h>

SoftwareSerial master(11, 12);
SoftwareSerial slave(9, 10);

String receiveBuffer = "";
bool activeInteractiveMode = true;
bool _speaking_with_master = true;

void sendToMaster(String message, bool print = true) {
  if (!_speaking_with_master) {
    master.listen();
    _speaking_with_master = true;
  }
  if (print) {
    Serial.println("board  -> master : " + message);
  }
  master.print(message + "\r\n");
}

void sendToSlave(String message, bool print = true) {
  if (_speaking_with_master) {
    slave.listen();
    _speaking_with_master = false;
  }
  if (print) {
    Serial.println("board  ->  slave : " + message);
  }
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

String readFromMaster(bool print = true) {
  String receive = _readFrom(&master);
  if (print) {
    Serial.println("master : " + receive);
  }
  return receive;
}

String readFromSlave(bool print = true) {
  String receive = _readFrom(&slave);
  if (print) {
    Serial.println("slave  : " + receive);
  }
  return receive;
}

void transmitInfos(bool print = true) {
  if (master.available()) {
    readFromMaster(print);
  }

  if (slave.available()) {
    readFromSlave(print);
  }
}

const String _infosCommands[] = {
    "AT",
    "AT+NAME?",
    "AT+ADDR?",
    "AT+ROLE?",
    "AT+PSWD?",
    "AT+CMODE?",
    "AT+BIND?",
    "AT+UART?"};

void infosFrom(bool toMaster) {
  if (toMaster) {
    Serial.println("board  : Infos from master :");
  } else {
    Serial.println("board  : Infos from slave :");
  }
  for (const String command : _infosCommands) {
    if (toMaster) {
      sendToMaster(command, false);
    } else {
      sendToSlave(command, false);
    }
    delay(50);
    transmitInfos();
    transmitInfos(false);
  }
}

String readFromSerial() {
  while (Serial.available()) {
    receiveBuffer += (char)Serial.read();
    if (receiveBuffer.endsWith("\r\n")) {
      String receive = receiveBuffer;
      receiveBuffer = "";  // Réinitialiser le buffer après traitement
      return receive;
    }
  }
  return "";
}

const String _pair1Commands[] = {
    "AT",
    "AT+ORGL",
    "AT", //La commande après AT+ORGL échoue systématiquement, permet de passer la commande sans causer de problèmes
    "AT+ROLE=1",
    "AT+RESET",
    "AT",
    "AT+RMAAD",
    "AT+CMODE=0",
    "AT+UART=115200,0,0",
    "AT+NAME=LUDOMASTER",
    "AT+PSWD=\"VATECH\""};

const String _pair2Commands[] = {
    "AT",
    "AT+ORGL",
    "AT",
    "AT+RMAAD",
    "AT+CMODE=0",
    "AT+UART=115200,0,0",
    "AT+NAME=LUDOSLAVE",
    "AT+PSWD=\"VATECH\""};

bool waitForOK(String *failed, String command, bool waitMaster) {
  while (true) {
    if (
        waitMaster
            ? master.available()
            : slave.available()) {
      String receive = waitMaster ? readFromMaster() : readFromSlave();
      if (receive == "OK") {
        break;
      } else if (receive == "ERROR:[0]") {
        *failed += waitMaster ? " master:" : " slave:";
        *failed += command;
        break;
      } else {
        Serial.println("RECEIVE " + receive);
      }
    }
    if (Serial.available()) {
      String message = readFromSerial();
      message.trim();
      if (message == "q") {
        return true;
      } else if (message == "c") {
        *failed += waitMaster ? " master:passed:" : " slave:passed:";
        *failed += command;
        break;
      }
    }
  }
  return false;
}

String getAdress(bool fromMaster) {
  if (fromMaster) {
    sendToMaster("AT+ADDR?");
  } else {
    sendToSlave("AT+ADDR?");
  }
  String message_address;
  while (message_address.substring(0, 6) != "+ADDR:") {
    if (fromMaster ? master.available() : slave.available()) {
      if (fromMaster) {
        message_address = readFromMaster();
      } else {
        message_address = readFromSlave();
      }
      message_address.trim();
      if (message_address == "ERROR:[0]") {
        Serial.println("FAILED : AT+ADDR");
        return "";
      }
    }
    if (Serial.available()) {
      String message = readFromSerial();
      message.trim();
      if (message == "q") {
        return "";
      }
    }
  }
  String address = message_address.substring(6);
  address.replace(":", ",");
  if (fromMaster) { // trap OK
    if (master.available()) {
      readFromMaster();
    }
  } else {
    if (slave.available()) {
      readFromSlave();
    }
  }
  return address;
}

void pair() {
  String failed = "";
  Serial.println("board  : PAIR");
  for (const String command : _pair1Commands) {
    sendToMaster(command);
    if (waitForOK(&failed, command, true)) return;
  }
  for (const String command : _pair2Commands) {
    sendToSlave(command);
    if (waitForOK(&failed, command, false)) return;
  }
  String addressSlave = getAdress(false);
  String addressMaster = getAdress(true);
  if (addressSlave == "") return;
  if (addressMaster == "") return;
  sendToMaster("AT+PAIR=" + addressSlave + ",20");
  sendToSlave("AT+BIND=" + addressMaster);
  // AT+PAIR don't send OK I don't know why
  sendToMaster("AT+BIND=" + addressSlave);
  if (waitForOK(&failed, "AT+BIND", true)) return;
  sendToMaster("AT+LINK=" + addressSlave);
  if (waitForOK(&failed, "AT+LINK", true)) return;

  if (failed != "") {
    Serial.println("FAILED :" + failed);
  } else {
    Serial.println("DONE");
  }
}

void interactiveMode() {
  // Lire depuis Serial
  if (Serial.available()) {
    processSerialCommand(readFromSerial());
  }
}

void processSerialCommand(String command) {
  if (command == "") return;
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
  } else if (command == "BC+INFO" || command == "bi") {
    infosFrom(_speaking_with_master);
  } else if (command == "BC+PAIR" || command == "bp") {
    pair();
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

/*
Commandes :
bw : BOARD+WHICH? : Vers qui s'addresse les commandes
bwm : BOARD+WHICH=MASTER : sélectionne master comme receveur des commandes
bws : BOARD+WHICH=SLAVE : sélectionne slave comme receveur des commandes
bi : BOARD+INFO : affiche les informations du receveur des commandes
bp : BOARD+PAIR : appareillage automatique

Dans bp:
c : continuer à executer
q : quitter la fonction
*/