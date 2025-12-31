#include <CRSF.h>

// RX = GPIO6, TX = GPIO5
CRSF crossfire(6, 5);
HardwareSerial TXSerial(0);

unsigned long lastSendESP = 0;
unsigned long lastSendA1 = 0;
unsigned long lastSendA2 = 0;
unsigned long lastSendA3 = 0;

const byte ST_ESP = 0x01;  // Start-Token für ESP32
const byte ST_A1 = 0xA1;   // Start-Token für Arduino Nr. 1
const byte ST_A2 = 0xA2;   // Start-Token für Arduino Nr. 2
const byte ST_A3 = 0xA3;   // Start-Token für Arduino Nr. 1

void setup() {

  Serial.begin(115200);
  crossfire.begin();

  TXSerial.begin(115200, SERIAL_8N1, 20, 21);  // RX = 20 , TX = 21
}

void loop() {
  crossfire.update();

  if (millis() - lastSendESP >= 50) {
    lastSendESP = millis();
    sendESP();
  }
  if (millis() - lastSendA1 >= 50) {
    lastSendA1 = millis();
    sendA1();
  }
  if (millis() - lastSendA2 >= 50) {
    lastSendA2 = millis();
    sendA2();
  }
  if (millis() - lastSendA3 >= 50) {
    lastSendA3 = millis();
    sendA3();
  }
}

void sendESP() {

  TXSerial.write(ST_ESP);

  for (int i = 1; i <= 14; i++) {
    int C = map(crossfire.read(i), 172, 1810, 0, 1000);
    C = constrain(C, 0, 1000);
    TXSerial.print(C);
    if (i < 14) TXSerial.print(",");
  }
  TXSerial.println();
}

void sendA1() {

  TXSerial.write(ST_A1);

  for (int i = 1; i <= 7; i++) {
    int C = map(crossfire.read(i), 172, 1810, 0, 1000);
    C = constrain(C, 0, 1000);
    TXSerial.print(C);
    if (i < 7) TXSerial.print(",");
  }
  TXSerial.println();
}
void sendA2() {

  TXSerial.write(ST_A2);

  for (int i = 1; i <= 6; i++) {
    int C = map(crossfire.read(i), 172, 1810, 0, 1000);
    C = constrain(C, 0, 1000);
    TXSerial.print(C);
    if (i < 6) TXSerial.print(",");
  }
  TXSerial.println();
}
void sendA3() {

  TXSerial.write(ST_A3);

  for (int i = 1; i <= 3; i++) {
    int C = map(crossfire.read(i), 172, 1810, 0, 1000);
    C = constrain(C, 0, 1000);
    TXSerial.print(C);
    if (i < 3) TXSerial.print(",");
  }
  TXSerial.println();
}