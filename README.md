# sim900MiniLib

sim900MiniLib is an Arduino library for dealing with a sim900 module via the AT commands.\
It will give you the possibility to send and receive SMS, call someone and registrating to a GSM provider

## Installation

Clone the repository and import the library using your IDE

## Usage

\- Import the library and dependencies
```C++
#include <sim900MiniLib.h>
#include <SoftwareSerial.h>
```

\- Initialize the library
```C++
boolean debug = true;
SoftwareSerial SIM900Serial(7, 8); // I'm using a software to preserve the hardware serial for the serial console (replace the pin number 7(rx), 8(tx) with yours)
sim900MiniLib sim900MiniLib(
    &SIM900Serial, // pass a pointer to the var SIM900Serial (SoftwareSerial)
    &Serial,       // pass a pointer to the var Serial (HardwareSerial)
    debug
);

// Don't forget to set the serials speeds in the setup function
Serial.begin(115200);
SIM900Serial.begin(19200);
```

\- Start the GSM Sim900 module
```C++
// Input : int pin
sim900MiniLib.startORstop(your_control_pin);
```

\- Check if the Sim900 is alive `(cmd: AT)`
```C++
sim900MiniLib.status();

// Result {type: boolean}, true if it's ok
```

\- Set the Sim900 in text mode `(cmd: AT+CMGF?)` or check if it's the case `(cmd: AT+CMGF=1)`
```C++
// Input : boolean check (default: false), set it to true to check
sim900MiniLib.textMode(false);

// Result {type: boolean}, true if it's ok
```

\- Set the Sim900 in automatic timezone mode `(cmd: AT+CTZU?)` or check it `(cmd: AT+CTZU=1)`
```C++
// Input : boolean check (default: false), set it to true to check
sim900MiniLib.autoTimezone(false);

// Result {type: boolean}, true if it's ok
```

\- Check if the Sim900 is registrated `(cmd: AT+CREG?)`
```C++
sim900MiniLib.checkRegistration();

/*
Result {type: int},
  0: is not registrated
  1: is registrated
  2: is asking for registration (you must recheck soon)
*/
```

\- Enable/Disable/Check the Sms Receive mode

actions:\
  check: to check the state of the mode `(cmd: AT+CNMI?)`\
  enable: enable  `(cmd: AT+CNMI=2,2,0,0,0)`\
  disable: disable `(cmd: AT+CNMI=0,0,0,0,0)`

```C++
// Input : String action (list above)
sim900MiniLib.receiveSMSMode(F("check")); // don't forget to include the action in a F() statement [doc PROGMEM](https://www.arduino.cc/reference/en/language/variables/utilities/progmem/)

// Result {type: boolean}, true if it's ok
```

\- Read an incoming Sms
```C++
// Input : String smsData[4], empty array of 4 elements
sim900MiniLib.readSMS(smsData[4]);

/*
Result {the given array in input, updated with the Sms values}
  smsData[0]: Sender Phone
  smsData[1]: Index
  smsData[2]: Date
  smsData[3]: Message
*/
```

\- Send a Sms `(cmd: "AT + CMGS = \"" + phoneNumber + "\"")`
```C++
// Input : String phoneNumber, recipient’s mobile number (in international format),
//         String message
sim900MiniLib.sendSMS("+XXXXXXXXXXXX", "ping");

// Result {type: boolean}, true if it's ok
// Note : the code can wait 30s, to let the Sim900 send the Sms without !!STREeESS!!
```

\- Get time from the GSM provider `(cmd: AT+CCLK?)`
```C++
// Input : String timeInfos[4], empty array of 7 elements
sim900MiniLib.time(String timeInfos[7]);

/*
Result {the given array in input, updated with the date&time infos}
  timeInfos[0]: Year    (2 digits)
  timeInfos[1]: Month       "
  timeInfos[2]: Day         "
  timeInfos[3]: Hour        "
  timeInfos[4]: Minutes     "
  timeInfos[5]: Seconds     "
  timeInfos[6]: TimeZone    "
*/
```

\- Call someone `(cmd: "ATD" + phoneNumber + ";")`

Note : to hang up the call the command used is `(cmd: ATH)`,\
  and to check the call status (to know if the recipient has hung up) the command used is `(cmd: AT+CPAS)`
```C++
// Input : String phoneNumber, recipient’s mobile number (in international format),
//         long int delayBeforeHangUp, delay before hang up
void callSomeone(const String phoneNumber, long int delayBeforeHangUp = 30000);
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.