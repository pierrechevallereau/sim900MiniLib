# sim900MiniLib

sim900MiniLib is an Arduino library for dealing with a SIM900 module via AT commands.
It lets you send and receive SMS, call someone, read the network time and register to a GSM provider.

Design notes (v2):

- no `String`: fixed-size `char[]` buffers, so no heap fragmentation on small AVRs
- responses are parsed line by line (`OK` / `ERROR` / `+CME ERROR` / `+CMS ERROR`), with or without echo
- incoming SMS (`+CMT:`) are captured wherever they arrive, even while a command is running
- works with any `Stream` (`SoftwareSerial`, `Serial1`, ...)

## Installation

Clone the repository and import the library using your IDE.

## Usage

```cpp
#include <SoftwareSerial.h>
#include <sim900MiniLib.h>

SoftwareSerial SIM900Serial(7, 8);           // RX, TX on the Arduino side
sim900MiniLib gsm(&SIM900Serial, &Serial);   // 2nd argument: debug stream, nullptr to disable logs

void setup() {
  Serial.begin(115200);
  SIM900Serial.begin(19200);
}
```

See `examples/ReceiveSms` for a complete sketch.

### API

All functions are blocking except `readSMS()` and `service()`. Avoid long `delay()` calls in `loop()`:
the `SoftwareSerial` RX buffer only holds 64 bytes.

| Function | Description |
|---|---|
| `bool begin()` | sync autobaud (`AT`) and disable echo (`ATE0`) |
| `void startORstop(uint8_t pin)` | pulse PWRKEY: **toggles** the module power |
| `bool powerOn(uint8_t pin)` | power on only if the module does not answer, then `begin()` |
| `bool status()` | `AT`, true if the module answers |
| `bool textMode(bool check = false)` | `AT+CMGF=1`, or with `check = true` verify it is set (`AT+CMGF?`) |
| `bool autoTimezone(bool check = false)` | `AT+CTZU=1`, or with `check = true` verify it is set (`AT+CTZU?`) |
| `bool receiveSMSMode(bool enable)` | `AT+CNMI=2,2,0,0,0` (enable) or `AT+CNMI=0,0,0,0,0` (disable) |
| `bool isReceiveSMSMode()` | check the state with `AT+CNMI?` |
| `int8_t checkRegistration()` | `-1`: no answer, `0`: not registered, `1`: registered (home or roaming), `2`: searching (recheck soon) |
| `const Sim900Sms* readSMS()` | non-blocking, call it often; returns `nullptr` if there is no new SMS |
| `bool sendSMS(const char* number, const char* text)` | `AT+CMGS`, waits for the `>` prompt, up to 30 s for the result |
| `bool time(Sim900Time& t)` | `AT+CCLK?` |
| `bool callSomeone(const char* number, uint32_t maxDurationMs = 30000)` | `ATD`, polls `AT+CPAS`, hangs up (`ATH`) after the delay |

### Reading an SMS

```cpp
const Sim900Sms* sms = gsm.readSMS();
if (sms) {
  Serial.println(sms->sender);    // e.g. +33612345678
  Serial.println(sms->date);      // e.g. 26/10/24,14:30:00+08
  Serial.println(sms->message);
}
```

The returned pointer is only valid until the next call to a library function. Only one SMS is buffered:
an unread SMS is overwritten by the next one. Multi-line SMS are truncated to their first line.
The text is kept as received (no lower-casing, spaces are kept).

### Reading the network time

```cpp
Sim900Time t;
if (gsm.time(t)) {
  // t.year (2 digits), t.month, t.day, t.hour, t.minute, t.second
  // t.tzQuarters: timezone in quarters of an hour (+08 => 8)
}
```

### Buffer sizes

Override before including the library if you need to save RAM:

```cpp
#define SIM900_SMS_MAX 80
#define SIM900_LINE_MAX 64
```

## Migrating from v1

| v1 | v2 |
|---|---|
| `sim900MiniLib(SoftwareSerial*, HardwareSerial*, bool debug)` | `sim900MiniLib(Stream* gsm, Stream* debug = nullptr)` |
| `readSMS(String smsData[4])` | `const Sim900Sms* readSMS()` |
| `receiveSMSMode(F("enable"))` | `receiveSMSMode(true)` |
| `time(String timeInfos[7])` | `time(Sim900Time&)` |
| `checkRegistration()` returned a `boolean` (so `2` was read as `true`) | returns `int8_t` |
| `callSomeone()` returned `void` | returns `bool` |

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.
