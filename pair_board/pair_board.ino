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
    "AT+BIND?"};

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
    "AT+ROLE=1",
    "AT+RESET",
    "AT+INIT",
    "AT+RMAAD"};

const String _pair2Commands[] = {
    "AT",
    "AT+ORGL",
    "AT+RESET",
    "AT+INIT",
    "AT+RMAAD"};

void pair() {
  String failed = "";
  Serial.println("board  : PAIR");
  for (const String command : _pair1Commands) {
    sendToMaster(command);
    while (true) {
      if (master.available()) {
        String receive = readFromMaster();
        if (receive == "OK") {
          break;
        } else if (receive == "ERROR:[0]") {
          failed += " master:" + command;
          break;
        }
      }
      if (Serial.available()) {
        String message = readFromSerial();
        message.trim();
        if (message == "q") {
          return;
        } else if (message == "c") {
          failed += " master:passed:" + command;
          break;
        }
      }
    }
  }
  for (const String command : _pair2Commands) {
    sendToSlave(command);
    while (true) {
      if (slave.available()) {
        String receive = readFromSlave();
        if (receive == "OK") {
          break;
        } else if (receive == "ERROR:[0]") {
          failed += " slave:" + command;
          break;
        }
      }
      if (Serial.available()) {
        String message = readFromSerial();
        message.trim();
        if (message == "q") {
          return;
        } else if (message == "c") {
          failed += " slave:passed:" + command;
          break;
        }
      }
    }
  }
  sendToSlave("AT+ADDR?");
  String address;
  while (address.substring(0, 6) != "+ADDR:") {
    if (slave.available()) {
      address = readFromSlave();
      address.trim();
      if (address == "ERROR:[0]") {
        Serial.println("FAILED : AT+ADDR");
        return;
      }
    }
    if (Serial.available()) {
      String message = readFromSerial();
      message.trim();
      if (message == "q") {
        return;
      }
    }
  }
  String output = address.substring(6);
  output.replace(":", ",");
  Serial.println(output);
  if (failed != "") {
    Serial.println("FAILED :" + failed);
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
  } else if (command == "BC+INFO" || command == "bp") {
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