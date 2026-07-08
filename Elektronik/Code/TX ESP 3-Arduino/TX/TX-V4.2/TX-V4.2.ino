#include <CRSF.h>

// RX = GPIO6, TX = GPIO5
CRSF crossfire(6, 5);
HardwareSerial TXSerial(0);

unsigned long lastSendESP = 0;
unsigned long lastSendA1 = 0;
unsigned long lastSendA2 = 0;
unsigned long lastSendA3 = 0;

bool receiverConnected() {
  return !(crossfire.read(1) == 0 &&  //Gibt die ausgabe der ==0 funktion aus. Wenn alle 0 sind, true
           crossfire.read(2) == 0 && crossfire.read(3) == 0 && crossfire.read(4) == 0 && crossfire.read(5) == 0 && crossfire.read(6) == 0);
}

const byte ST_ESP = 0xA0;  // Start-Token für ESP32
const byte ST_A1 = 0xA1;   // Start-Token für Arduino Nr. 1
const byte ST_A2 = 0xA2;   // Start-Token für Arduino Nr. 2
const byte ST_A3 = 0xA3;   // Start-Token für Arduino Nr. 3
bool DebugMode = false;

void setup() {

  Serial.begin(250000);
  crossfire.begin();

  TXSerial.begin(250000, SERIAL_8N1, 20, 21);  // RX = 20 , TX = 21

  unsigned long jetzt = millis();
  lastSendESP = jetzt;
  lastSendA1 = jetzt - 20;
  lastSendA2 = jetzt - 28;
  lastSendA3 = jetzt - 36;
}

void loop() {
  crossfire.update();

  if (!receiverConnected()) return;
  if (!DebugMode) {
    if (millis() - lastSendESP >= 50 && TXSerial.availableForWrite() > 80) {
      lastSendESP = millis();
      sendESP();
    }
    if (millis() - lastSendA1 >= 50 && TXSerial.availableForWrite() > 40) {
      lastSendA1 = millis();
      sendA1();
    }
    if (millis() - lastSendA2 >= 50 && TXSerial.availableForWrite() > 40) {
      lastSendA2 = millis();
      sendA2();
    }
    if (millis() - lastSendA3 >= 50 && TXSerial.availableForWrite() > 40) {
      lastSendA3 = millis();
      sendA3();
    }
  }
  if (DebugMode) {
    for (int i = 1; i < 17; i++) {
      Serial.print(" CH");
      Serial.print(i);
      Serial.print(":");
      Serial.print(crossfire.read(i));
    }
    Serial.println("");
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

  for (int i = 1; i <= 6; i++) {
    int C = map(crossfire.read(i), 172, 1810, 0, 1000);
    C = constrain(C, 0, 1000);
    TXSerial.print(C);
    if (i < 6) TXSerial.print(",");
  }
  TXSerial.println();
}
void sendA2() {

  TXSerial.write(ST_A2);

  for (int i = 1; i <= 8; i++) {
    int C = map(crossfire.read(i), 172, 1810, 0, 1000);
    C = constrain(C, 0, 1000);
    TXSerial.print(C);
    if (i < 8) TXSerial.print(",");
  }
  TXSerial.println();
}
void sendA3() {

  TXSerial.write(ST_A3);

  for (int i = 1; i <= 8; i++) {
    int C = map(crossfire.read(i), 172, 1810, 0, 1000);
    C = constrain(C, 0, 1000);
    TXSerial.print(C);
    if (i < 8) TXSerial.print(",");
  }
  TXSerial.println();
}
