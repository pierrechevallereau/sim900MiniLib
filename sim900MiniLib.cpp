// sim900MiniLib.cpp

#include "sim900MiniLib.h"

// Constructor /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////
/////////////////////////////

// Init function
sim900MiniLib::sim900MiniLib(SoftwareSerial * SIM900Serial, HardwareSerial * hwSerial, boolean debug) {
  _serial = SIM900Serial;
  _hwSerial = hwSerial;
  _debug = debug;
}

// Private Methods /////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////
/////////////////////////////

// Forward what Software Serial received to Serial Port
void sim900MiniLib::_updateSerial() {
  if (_debug) {
    delay(500);
    while (_serial->available() > 0) {
      _hwSerial->write(_serial->read());
    }
  }
}

// Print debug logs when debug is enable
void sim900MiniLib::_printDebug(String message, boolean newLine) {
  if (_debug) {
    if (newLine) {
      _hwSerial->println(message);
    }
    else {
      _hwSerial->print(message);
    }
  }
}

// Check return of the previous cmd, DEFAULT ARG = OK
boolean sim900MiniLib::_execCmd(String cmd, String resultMustBe, String prefix) {
  String dataS;

  _serial->println(cmd);
  delay(100);

  for (int i=0; i <= 5; i++) {
    while (_serial->available() > 0) {
      char data = _serial->read();
      if (isAlphaNumeric(data)) {
        dataS.concat(data);
      }
    }
    if (dataS != "") {
      break;
    }
    else {
      delay(20);
    }
  }

  this->_printDebug(prefix, false);
  if (dataS.endsWith(resultMustBe)) {
    this->_printDebug(" is ok");
    return true;
  }
  else {
    this->_printDebug(" is NOTOK " + dataS);
    return false;
  }
}

// Public Methods //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////
/////////////////////////////

// Print debug mode (enabled/disabled)
void sim900MiniLib::checkDebug() {
  if (_debug) {
    _hwSerial->println("Debug enabled"); 
  }
  else {
    _hwSerial->println("Debug disabled"); 
  }
}

// Start the GSM _serial->module
void sim900MiniLib::startORstop(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delay(1000);
  digitalWrite(pin, LOW);
  delay(5000);
}

// Check status of SIM900 module
boolean sim900MiniLib::status() {
  return this->_execCmd("AT", "ATOK", "[Sim900] check is started :");
}

// AT command to set _serial->to TEXT mode
boolean sim900MiniLib::textMode(boolean check) {
  if (check) {
    return this->_execCmd("AT+CMGF?", "CMGF1OK", "[Sim900] check txt mode :");
  }
  else {
    return this->_execCmd("AT+CMGF=1", "OK", "[Sim900] enable txt mode :");
  }
}

// AT command to set _serial->to automatic timezone mode
boolean sim900MiniLib::autoTimezone(boolean check) {
  if (check) {
    return this->_execCmd("AT+CTZU?", "CTZU1OK", "[Sim900] check autoTimezone :");
  }
  else {
    return this->_execCmd("AT+CTZU=1", "OK", "[Sim900] enable autoTimezone :");
  }
}
  
// Enable SMS Received mode
// Set module to send SMS data to serial out upon receipt 
boolean sim900MiniLib::receiveSMSMode(String action) {
  String cmd, resultMustBe, preffix;

  if (action == "check") {
    return this->_execCmd("AT+CNMI?", "CNMI22000OK", "[Sim900] check rcvdSmsMode :");
  }
  else if (action == "enable") {
    cmd = "AT+CNMI=2,2,0,0,0";
    resultMustBe = "CNMI22000OK";
    preffix = "[Sim900] en rcvdSmsMode :";
  }
  else if (action == "disable") {
    cmd = "AT+CNMI=0,0,0,0,0";
    resultMustBe = "CNMI00000OK";
    preffix = "[Sim900] dis rcvdSmsMode :";
  }

  // for enable disable allow 5 retry
  for (int i=0; i <= 5; i++) {
    if (this->_execCmd(cmd, resultMustBe, preffix)) {
      return true;
    }
    else {
      delay(250);
    }
  }

  return false;
}

// Check GMS registration
// 0 is false
// 1 is true (registrated)
// 2 is asking for registrated (you can retry your check)
boolean sim900MiniLib::checkRegistration() {
  String dataS;

  this->_printDebug("[Sim900] check GSM : ", false);
  _serial->println("AT+CREG?");
  delay(100);

  for (int i=0; i <= 5; i++) {
    while (_serial->available() > 0) {
      char data = _serial->read();
      if (isDigit(data)) {
        dataS.concat(data);
      }
    }
    if (dataS != "") {
      break;
    }
    else {
      delay(20);
    }
  }

  if (dataS == "01" || dataS == "05") {
    this->_printDebug("is registrated");
    return 1;
  }
  else if (dataS == "02") {
    this->_printDebug("is asking for registration");
    return 2;
  } 
  else {
    this->_printDebug("is not registrated");
    return 0;
  } 
}

// Extract phone number, index, date and message from the SMS
void sim900MiniLib::readSMS(String smsData[4]) {
  int ligneDelim = 0; // delimiter = '\n'
  int SMSInfoDelim = 0; //delimiter = '"'
  int rcvdCheck = 0;
  int rcvdIsSMS = 0;
  String SMSSender = "";
  String SMSIndex = "";
  String SMSDate = "";
  String SMSMessage = "";

  while (_serial->available() > 0) {
    char data = _serial->read();

    if (data == '\n') {ligneDelim++;}
    if (data == '"') {SMSInfoDelim++;}
  
    if (ligneDelim == 1) {
      if (rcvdIsSMS == 0) {
        if (data == '+') {rcvdCheck = 1;}
        if ((data == 'C') && (rcvdCheck == 1)) {rcvdCheck = 2;}
        if ((data == 'M') && (rcvdCheck == 2)) {rcvdCheck = 3;}
        if ((data == 'T') && (rcvdCheck == 3)) {rcvdCheck = 4;}
        if ((data == ':') && (rcvdCheck == 4)) {rcvdCheck = 5;}
        if (rcvdCheck == 5) {rcvdIsSMS = 1;}
      }
    
      if (rcvdIsSMS == 1 && data != '"') {
        if (SMSInfoDelim == 1) {SMSSender.concat(data);}
        if (SMSInfoDelim == 3) {SMSIndex.concat(data);}
        if (SMSInfoDelim == 5) {SMSDate.concat(data);}
      }
    }
    if (ligneDelim == 2 && rcvdIsSMS == 1 && data != '\n' && data != ' ') {
        SMSMessage.concat(data);
    }
    if (ligneDelim == 3 && rcvdIsSMS == 1) {
      SMSSender.trim();
      SMSIndex.trim();
      SMSDate.trim();
      SMSMessage.trim();
      SMSMessage.toLowerCase();

      smsData[0] = SMSSender;
      smsData[1] = SMSIndex;
      smsData[2] = SMSDate;
      smsData[3] = SMSMessage;

      this->_printDebug("received");
    }
  }
}

// Send SMS
void sim900MiniLib::sendSMS(String phoneNumber, String textSMS) {
  // send sms
  this->_printDebug("[Sim900] sending sms :");
  _serial->println("AT + CMGS = \"" + phoneNumber + "\"");
  delay(70);
  _serial->print(textSMS);  // End AT command with ASCII code 26
  delay(70);
  _serial->write(26);

  // wait 30s for sms send
  for (int i = 0; i <= 120; i++) {
    String dataS;

    while (_serial->available() > 0) {
      char data = _serial->read();
      if (isAlpha(data)) {
        dataS.concat(data);
      }
    }

    if (dataS.endsWith("CMGSOK")) {
      this->_printDebug("is ok");
      break;
    }
    else {
      this->_printDebug("waiting");
    }
    delay(250);
  }
}

// Call someone
void sim900MiniLib::callSomeone(String phoneNumber, boolean hangUp) {
  // call
  this->_execCmd("ATD+ " + phoneNumber + ";", "OK", "[Sim900] calling :");

  // wait for hang up
  if (hangUp) {
    delay(30000);
  }
  else {
    delay(10000); // to increase
  }
  this->_execCmd("ATH", "ATH", "[Sim900] hang up call :");
}

// Get time
void sim900MiniLib::time(String timeInfos[7]) {
  String dataS, dataNum;

  this->_printDebug("[Sim900] get time : ", false);
  _serial->println("AT+CCLK?");
  delay(100);

  for (int i=0; i <= 5; i++) {
    while (_serial->available() > 0) {
      char data = _serial->read();
      if (isAlpha(data)) {
        dataS.concat(data);
      }
      else if (isDigit(data)) {
        dataNum.concat(data);
      }
    }
    if (dataS != "") {
      break;
    }
    else {
      delay(20);
    }
  }

  if (dataS.endsWith("CCLKOK")) {
    this->_printDebug("is ok");

    char dataNumCharArr[dataNum.length()];
    dataNum.toCharArray(dataNumCharArr, dataNum.length());
    timeInfos[0].concat(dataNum[0]); timeInfos[0].concat(dataNum[1]); // Year
    timeInfos[1].concat(dataNum[2]); timeInfos[1].concat(dataNum[3]); // Month
    timeInfos[2].concat(dataNum[4]); timeInfos[2].concat(dataNum[5]); // Day
    timeInfos[3].concat(dataNum[6]); timeInfos[3].concat(dataNum[7]); // Hour
    timeInfos[4].concat(dataNum[8]); timeInfos[4].concat(dataNum[9]); // Minute
    timeInfos[5].concat(dataNum[10]); timeInfos[5].concat(dataNum[11]); // Second
    timeInfos[6].concat(dataNum[12]); timeInfos[6].concat(dataNum[13]); // Timezone
  }
  else {
    this->_printDebug("is notok");
  }
}
