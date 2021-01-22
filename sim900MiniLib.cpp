// sim900MiniLib.cpp

#include "sim900MiniLib.h"

// Constructor /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////
/////////////////////////////

// Init function
sim900MiniLib::sim900MiniLib(SoftwareSerial * SIM900Serial, HardwareSerial * hwSerial, boolean debug){
  _serial = SIM900Serial;
  _hwSerial = hwSerial;
  _debug = debug;
}

// Private Methods /////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////
/////////////////////////////

// Forward what Software Serial received to Serial Port
void sim900MiniLib::_updateSerial(){
  if(_debug){
    delay(100);
    while(_serial->available()) 
    {
      _hwSerial->write(_serial->read());
    }
  }
}

// Print debug logs when debug is enable
void sim900MiniLib::_printDebug(String message){
  if(_debug){
    _hwSerial->println(message);
  }
}

// Check return of the previous cmd, DEFAULT ARG = OK
boolean sim900MiniLib::_checkReturnOfTheCMD(String mustBe, String prefix){
  String dataS = "";

  for (int i=0; i <= 5; i++){
    while(_serial->available()) {
      char data = _serial->read();
      if(isAlphaNumeric(data)){
        dataS.concat(data);
      }
    }
    if(dataS == ""){
      delay(10);
    }
    else{
      break;
    }
  }

  if(dataS.endsWith(mustBe)){
    this->_printDebug(prefix + "is OK"); //(result : " + dataS + ")");
    return true;
  }
  else{
    this->_printDebug(prefix + "is NOTOK"); //(result : " + dataS + ")");
    return false;
  }

}

// Public Methods //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////
/////////////////////////////

// Print debug mode (enabled/disabled)
void sim900MiniLib::checkDebug(){
  if (_debug) {
    _hwSerial->println("Debug enabled"); 
  } else {
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
boolean sim900MiniLib::status(){
  _serial->println("AT");
  delay(70);
  return this->_checkReturnOfTheCMD("ATOK", "[Sim900] check is started : ");
}

// AT command to set _serial->to TEXT mode
boolean sim900MiniLib::textMode(boolean check) {
  if(check)
  {
    _serial->println("AT+CMGF?"); 
    delay(70);
    return this->_checkReturnOfTheCMD("CMGF1OK", "[Sim900] check txt mode : ");
  }
  else
  {
    _serial->println("AT+CMGF=1"); 
    delay(70);
    return this->_checkReturnOfTheCMD("OK", "[Sim900] enable txt mode : ");
  }

}

// AT command to set _serial->to automatic timezone mode
boolean sim900MiniLib::autoTimezone(boolean check) {
  if(check)
  {
    _serial->println("AT+CTZU?"); 
    delay(70);
    return this->_checkReturnOfTheCMD("CTZU1OK", "[Sim900] check autoTimezone : ");
  }
  else
  {
    _serial->println("AT+CTZU=1"); 
    delay(70);
    return this->_checkReturnOfTheCMD("OK", "[Sim900] enable autoTimezone : ");
  }
}
  
// Enable SMS Received mode
// Set module to send SMS data to serial out upon receipt 
boolean sim900MiniLib::receiveSMSMode(String action) {
  if(action == "check"){
    _serial->println("AT+CNMI?");
    delay(70);

    if(this->_checkReturnOfTheCMD("CNMI22000OK", "[Sim900] check sms rcvd mode : ")){
      return true;
    }
  }
  else if(action == "enable"){
    _serial->println("AT+CNMI=2,2,0,0,0");
    delay(70);
    if(this->_checkReturnOfTheCMD("OK", "[Sim900] enable sms rcvd mode : ")){
      return true;
    }
  }
  else if(action == "disable"){
    _serial->println("AT+CNMI=0,0,0,0,0");
    delay(70);
    if(this->_checkReturnOfTheCMD("OK", "[Sim900] disable sms rcvd mode : ")){
      return true;
    }
  }

  // Always return false if not returned to true before
  return false;
}

// Check GMS registration
// 0 is false
// 1 is true (registrated)
// 2 is asking for registrated (you can retry your check)
boolean sim900MiniLib::checkRegistration() {
  String dataS = "";

  _serial->println("AT+CREG?");
  delay(70);

  for (int i=0; i <= 5; i++){
    while(_serial->available()) {
      char data = _serial->read();
      if(isDigit(data)){
        dataS.concat(data);
      }
    }
    if(dataS == ""){
      delay(10);
    }
    else{
      break;
    }
  }

  if(dataS == "01" || dataS == "05"){
    this->_printDebug("[Sim900] check GSM : is registrated");
    return 1;
  }
  else if(dataS == "02"){
    this->_printDebug("[Sim900] check GSM : is asking for registration");
    return 2;
  } 
  else{
    this->_printDebug("[Sim900] check GSM : is not registrated " + dataS);
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
  
  while(_serial->available()) {
    char data = _serial->read();

    if(data == '\n'){ligneDelim++;}
    if(data == '"'){SMSInfoDelim++;}
  
    if(ligneDelim == 1){
      if(rcvdIsSMS == 0){
        if(data == '+') {rcvdCheck = 1;}
        if((data == 'C') && (rcvdCheck == 1)) {rcvdCheck = 2;}
        if((data == 'M') && (rcvdCheck == 2)) {rcvdCheck = 3;}
        if((data == 'T') && (rcvdCheck == 3)) {rcvdCheck = 4;}
        if((data == ':') && (rcvdCheck == 4)) {rcvdCheck = 5;}
        if(rcvdCheck == 5){rcvdIsSMS = 1;}
      }
    
      if(rcvdIsSMS == 1 && data != '"') {
        if(SMSInfoDelim == 1){SMSSender.concat(data);}
        if(SMSInfoDelim == 3){SMSIndex.concat(data);}
        if(SMSInfoDelim == 5){SMSDate.concat(data);}
      }
    }
    if(ligneDelim == 2 && rcvdIsSMS == 1 && data != '\n' && data != ' '){
        SMSMessage.concat(data);
    }
    if(ligneDelim == 3 && rcvdIsSMS == 1){
      SMSSender.trim();
      SMSIndex.trim();
      SMSDate.trim();
      SMSMessage.trim();

      smsData[0] = SMSSender;
      smsData[1] = SMSIndex;
      smsData[2] = SMSDate;
      smsData[3] = SMSMessage;

      this->_printDebug("[Sim900] SMS received : " + smsData[3]);
    }
  }
}

// Send SMS
void sim900MiniLib::sendSMS(String phoneNumber, String textSMS) {
  // stop receiving sms
  if(this->receiveSMSMode("check")){this->receiveSMSMode("disable");}

  // send sms
  _serial->println("AT + CMGS = \"" + phoneNumber + "\"");
  delay(70);
  _serial->print(textSMS);  // End AT command with a ^Z, ASCII code 26
  delay(70);
  _serial->write(26);

  // wait 30s for sms send
  for (int i = 0; i <= 60; i++){
    String dataS = "";

    while(_serial->available()) {
      char data = _serial->read();
      if(isAlpha(data)){
        dataS.concat(data);
      }
    }

    if(dataS.endsWith("CMGSOK")){
      this->_printDebug("is OK (result : " + dataS + ")" + i);
      break;
    }
    else{
      this->_printDebug("is NOTOK (result : " + dataS + ")");
      //this->_printDebug("[SIM900] sms send : is NOTOK (result : " + dataS + ")");
    }
    delay(500);
  }

  // start receiving sms
  if(!this->receiveSMSMode("check")){this->receiveSMSMode("enable");}
}

// Call someone
void sim900MiniLib::callSomeone(String phoneNumber) {
  _serial->println("ATD+ " + phoneNumber + ";");
  delay(1000);
  //_serial->println();
  
  // In this example, the call only last 30 seconds
  // You can edit the phone call duration in the delay time
  //delay(30000);
  // AT command to hang up
  //_serial->println("ATH"); // hang up

  // UNLIMITED IF CALL IS REQUESTED
}

// Get time
void sim900MiniLib::time(String timeInfos[7]) {
  String dataS = "";
  String dataNum = "";

  _serial->println("AT+CCLK?");
  delay(70);

  for (int i=0; i <= 5; i++){
    while(_serial->available()) {
    char data = _serial->read();
      if(isAlpha(data)){
        dataS.concat(data);
      }
      else if(isDigit(data)){
        dataNum.concat(data);
      }
    }
    if(dataS == ""){
      delay(10);
    }
    else{
      break;
    }
  }

  if(dataS.endsWith("CCLKOK")){
    this->_printDebug("[Sim900] time : is OK");

    char dataNumCharArr[dataNum.length()];
    dataNum.toCharArray(dataNumCharArr, dataNum.length());
    timeInfos[0].concat(dataNum[0]); timeInfos[0].concat(dataNum[1]); // Year
    timeInfos[1].concat(dataNum[2]); timeInfos[1].concat(dataNum[3]); // Month
    timeInfos[2].concat(dataNum[4]); timeInfos[2].concat(dataNum[5]); // Day
    timeInfos[3].concat(dataNum[6]); timeInfos[3].concat(dataNum[7]); // Hour
    timeInfos[4].concat(dataNum[8]); timeInfos[4].concat(dataNum[9]); // Minute
    timeInfos[5].concat(dataNum[10]); timeInfos[5].concat(dataNum[11]); // Seconde
    timeInfos[6].concat(dataNum[12]); timeInfos[6].concat(dataNum[13]); // Timezone

    this->_printDebug("[SIM900] time : " +
      timeInfos[0] + "-" + timeInfos[1] + "-" + timeInfos[2] + "," +
      timeInfos[3] + ":" + timeInfos[4] + ":" + timeInfos[5] + "," +
      timeInfos[6]
    );
  }
  else{
    this->_printDebug("[Sim900] time : is NOTOK");
  }
}
