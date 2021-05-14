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
boolean sim900MiniLib::_execCmd(String cmd, boolean (*function)(int), boolean returnResult, String dataToReturn[1], String resultMustBe, String prefix)
{
  String dataS;

  if (prefix != "") {this->_printDebug(prefix, false);}

  _serial->println(cmd);
  delay(150);

  for (int i=0; i <= 5; i++) {
    while (_serial->available() > 0) {
      char data = _serial->read();
      if ((function)(data)) {
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
  
  // if returnResult is true and resultMustBe is "" then assign dataToReturn + return true
  // if returnResult is true and resultMustBe is not "" then assing dataToReturn + continue the function
  if (returnResult) {
    dataToReturn[0] = dataS;
    if (resultMustBe == "") {
      return true;
    }
  }

  // resultMustBe is mandatory if returnResult is false
  if (!returnResult && resultMustBe == ""){
    this->_printDebug("please provide resultMustBe");
    return false;
  }

  // check the result
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
  String trash[1];
  return this->_execCmd("AT", isAlphaNumeric, false, trash, "ATOK", "[S900] check is started :");
}

// AT command to set _serial->to TEXT mode
boolean sim900MiniLib::textMode(boolean check) {
  String trash[1];
  if (check) {
    return this->_execCmd("AT+CMGF?", isAlphaNumeric, false, trash, "CMGF1OK", "[S900] check txt mode :");
  }
  else {
    return this->_execCmd("AT+CMGF=1", isAlphaNumeric, false, trash, "OK", "[S900] enable txt mode :");
  }
}

// AT command to set _serial->to automatic timezone mode
boolean sim900MiniLib::autoTimezone(boolean check) {
  String trash[1];
  if (check) {
    return this->_execCmd("AT+CTZU?", isAlphaNumeric, false, trash, "CTZU1OK", "[S900] check autoTimezone :");
  }
  else {
    return this->_execCmd("AT+CTZU=1", isAlphaNumeric, false, trash, "OK", "[S900] enable autoTimezone :");
  }
}
  
// Enable SMS Received mode
// Set module to send SMS data to serial out upon receipt 
boolean sim900MiniLib::receiveSMSMode(String action) {
  String cmd, resultMustBe, preffix, trash[1];

  if (action == "check") {
    return this->_execCmd("AT+CNMI?", isAlphaNumeric, false, trash, "CNMI22000OK", "[S900] check rcvdSmsMode :");
  }
  else if (action == "enable") {
    cmd = "AT+CNMI=2,2,0,0,0";
    resultMustBe = "CNMI22000OK";
    preffix = "[S900] en rcvdSmsMode :";
  }
  else if (action == "disable") {
    cmd = "AT+CNMI=0,0,0,0,0";
    resultMustBe = "CNMI00000OK";
    preffix = "[S900] dis rcvdSmsMode :";
  }

  // for enable disable allow 5 retry
  for (int i=0; i <= 5; i++) {
    if (this->_execCmd(cmd, isAlphaNumeric, false, trash, resultMustBe, preffix)) {
      return true;
    } else {
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
  String dataS[1];

  this->_execCmd("AT+CREG?", isDigit, true, dataS, "", "[S900] check GSM : ");

  if (dataS[0] == "01" || dataS[0] == "05") {
    this->_printDebug("is registrated");
    return 1;
  }
  else if (dataS[0] == "02") {
    this->_printDebug("is asking for registration");
    return 2;
  } 
  else {
    this->_printDebug("is not registrated");
    return 0;
  } 
}

// Extract phone number, index, date and message from the SMS
boolean sim900MiniLib::readSMS(String smsData[4]) {
  int ligneDelim = 0; // delimiter = '\n'
  int SMSInfoDelim = 0; //delimiter = '"'
  int rcvdCheck = 0;
  boolean rcvdIsSMS = false;
  String SMSSender = "";
  String SMSIndex = "";
  String SMSDate = "";
  String SMSMessage = "";

  while (_serial->available() > 0) {
    char data = _serial->read();

    if (data == '\n') {ligneDelim++;}
    if (data == '"') {SMSInfoDelim++;}
  
    if (ligneDelim == 1) {
      if (!rcvdIsSMS) {
        if (data == '+') {rcvdCheck++;}
        if (data == 'C') {rcvdCheck++;}
        if (data == 'M') {rcvdCheck++;}
        if (data == 'T') {rcvdCheck++;}
        if (data == ':') {rcvdCheck++;}
        if (rcvdCheck == 5) {rcvdIsSMS = true;}
      }
    
      if (rcvdIsSMS == 1 && data != '"') {
        if (SMSInfoDelim == 1) {SMSSender.concat(data);}
        if (SMSInfoDelim == 3) {SMSIndex.concat(data);}
        if (SMSInfoDelim == 5) {SMSDate.concat(data);}
      }
    }
    if (ligneDelim == 2 && rcvdIsSMS && data != '\n' && data != ' ') {
        SMSMessage.concat(data);
    }
    if (ligneDelim == 3 && rcvdIsSMS) {
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
      return true;
    }
  }

  return false;
}

// Send SMS
boolean sim900MiniLib::sendSMS(String phoneNumber, String textSMS) {
  // send sms
  this->_printDebug("[S900] reply sms :");
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
      return true;
    }
    else {
      this->_printDebug("waiting");
    }
    delay(250);
  }
  this->_printDebug("is NOTOK");
  return false;
}

// Get time
boolean sim900MiniLib::time(String timeInfos[7]) {
  String dataToReturn[1];

  if(!this->_execCmd("AT+CCLK?", isAlphaNumeric, true, dataToReturn, "OK", "[S900] time :")) {
    return false;
  }

  char charArr[26];
  dataToReturn[0].toCharArray(charArr, 26);

  String digit;
  int x=1;
  int y=0;
  for (int i=0; i < 26; i++) {
    if (isDigit(charArr[i])) {
      if (x == 3) {
        x = 1; y++;
        if (y == 6) { break; }
      }
      if (x <= 2) {
        timeInfos[y].concat(charArr[i]);
        x++;
      }
    }
  }

  return true;
}

// Call someone
void sim900MiniLib::callSomeone(String phoneNumber, long int delayBeforeHangUp) {
  delay(500); // wait to avoid freeze from the sim900 module
  String trash[1];
  boolean terminated = false;

  long initialTime = millis();
  long callTimeout = initialTime + delayBeforeHangUp;
  long actualTime = millis();

  // start the call
  this->_execCmd("ATD" + phoneNumber + ";", isAlphaNumeric, false, trash, "OK", "[S900] calling :");

  while (actualTime < callTimeout) {
    actualTime = millis();

    // security for avoid bugs with arduino uptime reset
    if ( actualTime < initialTime) {
      break;
    }

    String callStatus[1];
    this->_execCmd("AT+CPAS", isDigit, true, callStatus, "", "");

    if (callStatus[0] == "0") {
      this->_printDebug("terminated");
      terminated = true;
      break;
    }
    this->_printDebug("progress");
    delay(500);

  }
  if(!terminated) {
    this->_execCmd("ATH", isAlphaNumeric, false, trash, "ATH", "[S900] hang up :");
  }
}