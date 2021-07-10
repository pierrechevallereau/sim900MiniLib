//sim900MiniLib.h

#ifndef SIM900MINILIB_H
#define SIM900MINILIB_H

#include "Arduino.h"
#include <SoftwareSerial.h>

class sim900MiniLib {
  private:
    SoftwareSerial * _serial;
    HardwareSerial * _hwSerial;
    boolean _debug;
    boolean _receiveSMSmode = false;

    void _updateSerial();
    void _printDebug(const __FlashStringHelper* message, boolean newLine = true);
    boolean _execCmd(const String cmd, boolean (*function)(int), boolean returnResult, String dataToReturn[1], const String resultMustBe, const __FlashStringHelper* prefix);

  public:
    sim900MiniLib(SoftwareSerial * SIM900Serial, HardwareSerial * hwSerial, boolean debug);

    // begin serial
    void checkDebug();

    // Status of the GSM SIM900 module
    boolean status();

    // Start the GSM SIM900 module
    void startORstop(const int pin);

    // AT command to set SIM900 to TEXT mode
    boolean textMode(boolean check = false);

    // AT command to set _serial->to automatic timezone mode
    boolean autoTimezone(boolean check = false);
  
    // Enable SMS Received mode
    boolean receiveSMSMode(const String action);

    // check GSM registration
    boolean checkRegistration();

    // Extract phone number, index, date and message from the SMS
    boolean readSMS(String smsData[4]);

    // Send SMS
    boolean sendSMS(const String phoneNumber, const String textSMS);

    // Get time
    boolean time(String timeInfos[7]);

    // Call someone
    void callSomeone(const String phoneNumber, long int delayBeforeHangUp = 30000);
};

#endif
