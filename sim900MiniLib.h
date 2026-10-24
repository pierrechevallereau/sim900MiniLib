// sim900MiniLib.h
//
// Minimalist library for SIM900 module (AT commands) — v2
// - No Strings: fixed-size char[] buffers (no heap fragmentation)
// - Line-by-line parsing: no more reliance on echo, no lingering "OK" messages
// - Incoming SMS messages (+CMT) are captured wherever they arrive (even during a command)
// - Works with any Stream (SoftwareSerial, Serial1, ...)

#ifndef SIM900MINILIB_H
#define SIM900MINILIB_H

#include <Arduino.h>

#ifndef SIM900_LINE_MAX
#define SIM900_LINE_MAX 80       // Maximum size of a response line (including header and CMT)
#endif
#ifndef SIM900_SMS_MAX
#define SIM900_SMS_MAX 160       // Maximum text size for a saved SMS
#endif
#define SIM900_SMS_TIMEOUT_MS 1000  // safety: abort if the final \n never comes

struct Sim900Sms {
  char sender[22];
  char date[24];                 // ex: 26/10/24,14:30:00+08
  char message[SIM900_SMS_MAX + 1];
};

struct Sim900Time {
  uint8_t year, month, day, hour, minute, second;   // year in 2 digits
  int8_t tzQuarters;                                // Time zone in quarter hours (+08 => 8)
};

class sim900MiniLib {
public:
  // debug = nullptr to disable logs
  sim900MiniLib(Stream* gsm, Stream* debug = nullptr);

  // Synchronizes autobaud (AT x N) and cuts echo (ATE0)
  bool begin();

  // Pressing PWRKEY: Turns ON or OFF depending on the current state (toggle)
  void startORstop(uint8_t pin);
  // Only turns on if the module is not responding, then calls begin()
  bool powerOn(uint8_t pin);

  bool status();
  bool textMode(bool check = false);
  bool autoTimezone(bool check = false);

  bool receiveSMSMode(bool enable);
  bool isReceiveSMSMode();

  // -1: No response | 0: Not registered | 1: Registered (home/roaming) | 2: Searching
  int8_t checkRegistration();

  // Non-blocking: to be called frequently in loop().
  // Returns a pointer to the received SMS (valid until the next call
  // to a method in the library), or nullptr if there is nothing new.
  const Sim900Sms* readSMS();

  bool sendSMS(const char* phoneNumber, const char* text);
  bool time(Sim900Time& t);
  bool callSomeone(const char* phoneNumber, uint32_t maxDurationMs = 30000);

  // Clears the serial port by processing incoming SMS messages (called by readSMS)
  void service();

private:
  enum State : uint8_t { IDLE, BODY };

  Stream* _gsm;
  Stream* _dbg;

  char     _line[SIM900_LINE_MAX];
  uint8_t  _lineLen = 0;
  State    _state = IDLE;
  uint32_t _lastRx = 0;

  Sim900Sms _sms;
  uint8_t   _msgLen = 0;
  bool      _smsReady = false;

  bool _waitingPrompt = false;
  bool _gotPrompt = false;

  bool _pollLine();
  bool _handleUrc(const char* line);
  void _bodyChar(char c);
  void _checkSmsEnd();
  void _finishSms();
  void _drain();
  void _idle(uint32_t ms);
  bool _waitOk(uint32_t timeoutMs, const char* prefix = nullptr,
               char* out = nullptr, size_t outLen = 0);
  bool _cmd(const __FlashStringHelper* cmd, uint32_t timeoutMs = 1000,
            const char* prefix = nullptr, char* out = nullptr, size_t outLen = 0);
};

#endif
