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

const int CHANNELS_ESP = 14;
const int CHANNELS_A1 = 6;
const int CHANNELS_A2 = 8;
const int CHANNELS_A3 = 8;

// Frame-Länge im Binärprotokoll: 1 Start-Byte + 2 Bytes je Kanal + 1 Prüfsummen-Byte.
// Anders als bei der alten ASCII-Kodierung ist diese Länge konstant, egal welchen
// Wert (0-1000) die Kanäle gerade haben - siehe sendChannelFrame() weiter unten.
const int FRAME_LEN_ESP = 1 + CHANNELS_ESP * 2 + 1;  // 30 Byte
const int FRAME_LEN_A1 = 1 + CHANNELS_A1 * 2 + 1;    // 14 Byte
const int FRAME_LEN_A2 = 1 + CHANNELS_A2 * 2 + 1;    // 18 Byte
const int FRAME_LEN_A3 = 1 + CHANNELS_A3 * 2 + 1;    // 18 Byte

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
    if (millis() - lastSendESP >= 50 && TXSerial.availableForWrite() > FRAME_LEN_ESP) {
      lastSendESP = millis();
      sendESP();
    }
    if (millis() - lastSendA1 >= 50 && TXSerial.availableForWrite() > FRAME_LEN_A1) {
      lastSendA1 = millis();
      sendA1();
    }
    if (millis() - lastSendA2 >= 50 && TXSerial.availableForWrite() > FRAME_LEN_A2) {
      lastSendA2 = millis();
      sendA2();
    }
    if (millis() - lastSendA3 >= 50 && TXSerial.availableForWrite() > FRAME_LEN_A3) {
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

// Kodiert und sendet einen Kanal-Frame im Binärprotokoll.
// Format: [Start-Byte][2 Bytes je Kanal][1 Prüfsummen-Byte]
//
// Jeder Kanalwert (0-1000) wird in zwei 7-Bit-Bytes zerlegt (low/high). Dadurch
// bleiben alle Nutzdaten-Bytes < 0x80 - nur die Start-Bytes (0xA0-0xA3) haben
// Bit 7 gesetzt und sind so unabhängig vom Kanalwert immer eindeutig als
// Frame-Anfang erkennbar (siehe ReadSerial() in den Empfänger-Sketches).
// Die Prüfsumme (XOR aller Nutzdaten-Bytes) lässt Empfänger korrupte Frames
// verwerfen, statt falsche Kanalwerte zu übernehmen.
void sendChannelFrame(byte startToken, int channelCount) {
  TXSerial.write(startToken);

  byte checksum = 0;
  for (int i = 1; i <= channelCount; i++) {
    int value = map(crossfire.read(i), 172, 1810, 0, 1000);
    value = constrain(value, 0, 1000);

    byte lowBits = value & 0x7F;
    byte highBits = (value >> 7) & 0x7F;

    TXSerial.write(lowBits);
    TXSerial.write(highBits);
    checksum ^= lowBits ^ highBits;
  }

  TXSerial.write(checksum & 0x7F);
}

void sendESP() {
  sendChannelFrame(ST_ESP, CHANNELS_ESP);
}
void sendA1() {
  sendChannelFrame(ST_A1, CHANNELS_A1);
}
void sendA2() {
  sendChannelFrame(ST_A2, CHANNELS_A2);
}
void sendA3() {
  sendChannelFrame(ST_A3, CHANNELS_A3);
}
