// sim900MiniLib.cpp

#include "sim900MiniLib.h"
#include <string.h>

// Helpers /////////////////////////////////////////////////////////////////////

static void copyRange(char* dst, size_t dstLen, const char* from, const char* to) {
  size_t n = (size_t)(to - from);
  if (n >= dstLen) n = dstLen - 1;
  memcpy(dst, from, n);
  dst[n] = '\0';
}

static bool num2(const char* p, uint8_t& out) {
  if (p[0] < '0' || p[0] > '9' || p[1] < '0' || p[1] > '9') return false;
  out = (uint8_t)((p[0] - '0') * 10 + (p[1] - '0'));
  return true;
}

// Number: digits, '+' at the beginning only (avoids any injection into the AT command)
static bool validNumber(const char* s) {
  if (!s || !*s) return false;
  uint8_t n = 0;
  if (*s == '+') ++s;
  if (!*s) return false;
  for (; *s; ++s, ++n) {
    if (*s < '0' || *s > '9' || n > 20) return false;
  }
  return true;
}

static bool isErrorLine(const char* l) {
  return strcmp(l, "ERROR") == 0 ||
         strncmp(l, "+CME ERROR", 10) == 0 ||
         strncmp(l, "+CMS ERROR", 10) == 0;
}

// Constructor /////////////////////////////////////////////////////////////////

sim900MiniLib::sim900MiniLib(Stream* gsm, Stream* debug) : _gsm(gsm), _dbg(debug) {
  _sms.sender[0] = _sms.date[0] = _sms.message[0] = '\0';
}

// Reception: non-blocking line assembler ///////////////////////////////


// Reads what is available. Returns true as soon as a "normal" (non-URC)
// line is complete in _line; false when there is nothing left to read.
bool sim900MiniLib::_pollLine() {
  while (_gsm->available() > 0) {
    int c = _gsm->read();
    if (c < 0) break;
    _lastRx = millis();

    if (_state == BODY) { _bodyChar((char)c); continue; }

    if (c == '\r') continue;
    if (c == '\n') {
      if (_lineLen == 0) continue;           // empty lines ignored
      _line[_lineLen] = '\0';
      _lineLen = 0;
      if (_dbg) { _dbg->print(F("< ")); _dbg->println(_line); }
      if (_handleUrc(_line)) continue;
      return true;
    }
    // the CMGS prompt "> " is not followed by a \n
    if (c == '>' && _waitingPrompt && _lineLen == 0) { _gotPrompt = true; continue; }

    if (_lineLen < SIM900_LINE_MAX - 1) _line[_lineLen++] = (char)c;
  }
  _checkSmsEnd();
  return false;
}

//  SMS received in CNMI=2,2 mode: +CMT: "<sender>","<name>","<date>"\r\n<text>\r\n
bool sim900MiniLib::_handleUrc(const char* line) {
  if (strncmp(line, "+CMT:", 5) != 0) return false;

  const char* q[6];
  uint8_t n = 0;
  for (const char* p = line; *p && n < 6; ++p) if (*p == '"') q[n++] = p;

  _sms.sender[0] = _sms.date[0] = _sms.message[0] = '\0';
  if (n >= 2) copyRange(_sms.sender, sizeof(_sms.sender), q[0] + 1, q[1]);
  if (n >= 6) copyRange(_sms.date, sizeof(_sms.date), q[4] + 1, q[5]);

  _msgLen = 0;
  _smsReady = false;          // an unread SMS is overwritten (buffered for a single message)
  _state = BODY;
  return true;
}

// The SMS text ends at the first \n. A multi-line SMS is therefore truncated at its
// first line (the following lines are ignored): this is the price of a deterministic
// end to a message, which cannot "swallow" the response of an AT command.
void sim900MiniLib::_bodyChar(char c) {
  if (c == '\r') return;
  if (c == '\n') { _finishSms(); return; }
  if (_msgLen < SIM900_SMS_MAX) {
    _sms.message[_msgLen++] = c;
    _sms.message[_msgLen] = '\0';
  }
}

void sim900MiniLib::_finishSms() {
  _state = IDLE;
  _smsReady = true;
  if (_dbg) { _dbg->print(F("[S900] sms from ")); _dbg->println(_sms.sender); }
}

// Security: If the final \n is lost (missing byte), we deliver what we have.
void sim900MiniLib::_checkSmsEnd() {
  if (_state == BODY && millis() - _lastRx > SIM900_SMS_TIMEOUT_MS) _finishSms();
}

void sim900MiniLib::service() {
  while (_pollLine()) { /* Unexpected lines: ignored */ }
}

// Clears what has already arrived WITHOUT losing incoming SMS messages.
void sim900MiniLib::_drain() { service(); }

// Active wait that continues to process SMS messages (replaces delay()).
void sim900MiniLib::_idle(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) service();
}

// Waits for "OK" (true) or an error/timeout (false).
// If a prefix is ​​provided, the value of the line "<prefix> xxx" is copied to out.
bool sim900MiniLib::_waitOk(uint32_t timeoutMs, const char* prefix, char* out, size_t outLen) {
  if (out && outLen) out[0] = '\0';
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    if (!_pollLine()) continue;
    if (strcmp(_line, "OK") == 0) return true;
    if (isErrorLine(_line)) return false;
    if (prefix && out && outLen) {
      size_t pl = strlen(prefix);
      if (strncmp(_line, prefix, pl) == 0) {
        const char* v = _line + pl;
        while (*v == ' ') ++v;
        strncpy(out, v, outLen - 1);
        out[outLen - 1] = '\0';
      }
    }
  }
  return false;
}

bool sim900MiniLib::_cmd(const __FlashStringHelper* cmd, uint32_t timeoutMs,
                         const char* prefix, char* out, size_t outLen) {
  _drain();
  if (_dbg) { _dbg->print(F("> ")); _dbg->println(cmd); }
  _gsm->println(cmd);
  return _waitOk(timeoutMs, prefix, out, outLen);
}

// Public Methods //////////////////////////////////////////////////////////////

bool sim900MiniLib::begin() {
  for (uint8_t i = 0; i < 10; i++) {
    if (_cmd(F("AT"), 500)) return _cmd(F("ATE0"), 1000);
  }
  return false;
}

void sim900MiniLib::startORstop(uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delay(1000);
  digitalWrite(pin, LOW);
  delay(5000);
}

bool sim900MiniLib::powerOn(uint8_t pin) {
  if (status()) return begin();
  startORstop(pin);
  return begin();
}

bool sim900MiniLib::status() { return _cmd(F("AT"), 500); }

bool sim900MiniLib::textMode(bool check) {
  if (!check) return _cmd(F("AT+CMGF=1"));
  char v[4];
  return _cmd(F("AT+CMGF?"), 1000, "+CMGF:", v, sizeof(v)) && v[0] == '1';
}

bool sim900MiniLib::autoTimezone(bool check) {
  if (!check) return _cmd(F("AT+CTZU=1"));
  char v[4];
  return _cmd(F("AT+CTZU?"), 1000, "+CTZU:", v, sizeof(v)) && v[0] == '1';
}

bool sim900MiniLib::receiveSMSMode(bool enable) {
  for (uint8_t i = 0; i < 3; i++) {
    if (_cmd(enable ? F("AT+CNMI=2,2,0,0,0") : F("AT+CNMI=0,0,0,0,0"))) return true;
    _idle(250);
  }
  return false;
}

bool sim900MiniLib::isReceiveSMSMode() {
  char v[16];
  return _cmd(F("AT+CNMI?"), 1000, "+CNMI:", v, sizeof(v)) && strcmp(v, "2,2,0,0,0") == 0;
}

int8_t sim900MiniLib::checkRegistration() {
  char v[16];                                   // "<n>,<stat>[,<lac>,<ci>]"
  if (!_cmd(F("AT+CREG?"), 2000, "+CREG:", v, sizeof(v))) return -1;
  const char* comma = strchr(v, ',');
  if (!comma) return -1;
  switch (comma[1]) {
    case '1': case '5': return 1;
    case '2':           return 2;
    default:            return 0;
  }
}

const Sim900Sms* sim900MiniLib::readSMS() {
  service();
  if (!_smsReady) return nullptr;
  _smsReady = false;
  return &_sms;
}

bool sim900MiniLib::sendSMS(const char* number, const char* text) {
  if (!validNumber(number) || !text) return false;
  for (const char* p = text; *p; ++p) if (*p == 26 || *p == 27) return false;

  _drain();
  _gotPrompt = false;
  _waitingPrompt = true;
  _gsm->print(F("AT+CMGS=\""));
  _gsm->print(number);
  _gsm->println('"');

  uint32_t start = millis();
  while (!_gotPrompt && millis() - start < 5000) {
    if (_pollLine() && isErrorLine(_line)) break;
  }
  _waitingPrompt = false;

  if (!_gotPrompt) {
    _gsm->write((uint8_t)27);                    // ESC: cancels input on the module side
    if (_dbg) _dbg->println(F("[S900] no prompt"));
    return false;
  }

  _gsm->print(text);
  _gsm->write((uint8_t)26);                      // Ctrl+Z: send
  return _waitOk(30000);                         // "+CMGS: n" then "OK"
}

bool sim900MiniLib::time(Sim900Time& t) {
  char v[28];                                    // "26/10/24,14:30:00+08" (quotes included)
  if (!_cmd(F("AT+CCLK?"), 2000, "+CCLK:", v, sizeof(v))) return false;

  const char* p = v;
  if (*p == '"') ++p;
  if (strlen(p) < 20) return false;
  if (p[2] != '/' || p[5] != '/' || p[8] != ',' || p[11] != ':' || p[14] != ':') return false;
  if (p[17] != '+' && p[17] != '-') return false;

  uint8_t tz;
  if (!num2(p, t.year) || !num2(p + 3, t.month) || !num2(p + 6, t.day) ||
      !num2(p + 9, t.hour) || !num2(p + 12, t.minute) || !num2(p + 15, t.second) ||
      !num2(p + 18, tz)) return false;
  t.tzQuarters = (p[17] == '-') ? -(int8_t)tz : (int8_t)tz;
  return true;
}

bool sim900MiniLib::callSomeone(const char* number, uint32_t maxMs) {
  if (!validNumber(number)) return false;

  _drain();
  _gsm->print(F("ATD"));
  _gsm->print(number);
  _gsm->println(';');
  if (!_waitOk(10000)) return false;

  uint32_t start = millis();                     // unsigned arithmetic: rollover OK
  bool ended = false;
  while (millis() - start < maxMs) {
    char v[4];
    if (_cmd(F("AT+CPAS"), 1000, "+CPAS:", v, sizeof(v)) && v[0] == '0') { ended = true; break; }
    _idle(500);
  }
  if (!ended) _cmd(F("ATH"), 3000);
  return true;
}
