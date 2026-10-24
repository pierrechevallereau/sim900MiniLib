#include <SoftwareSerial.h>
#include <sim900MiniLib.h>

const uint8_t PWRKEY_PIN = 9;               // change it if necessary

SoftwareSerial SIM900Serial(7, 8);          // RX, TX on the Arduino side
sim900MiniLib gsm(&SIM900Serial, &Serial);  // 2nd argument: debug stream, nullptr to disable logs

void setup() {
  Serial.begin(115200);
  SIM900Serial.begin(19200);

  if (!gsm.powerOn(PWRKEY_PIN)) { Serial.println(F("SIM900 does not respond")); while (true); }

  gsm.textMode();
  gsm.autoTimezone();
  gsm.receiveSMSMode(true);

  while (gsm.checkRegistration() != 1) delay(1000);   // -1/0/2 : we wait
  Serial.println(F("Enregistré sur le réseau"));
}

void loop() {
  const Sim900Sms* sms = gsm.readSMS();     // non blocking, loop to read incoming SMS
  if (sms) {
    Serial.print(sms->sender); Serial.print(F(" : ")); Serial.println(sms->message);
    if (strcmp(sms->message, "ping") == 0) gsm.sendSMS(sms->sender, "pong");
  }
}
