#include <SoftwareSerial.h>

#define RX_PIN 0  // RX vom ESP32 TX
#define TX_PIN 1  // TX zum ESP32 RX (optional)
#define START_BYTE 0xAA

SoftwareSerial mySerial(RX_PIN, TX_PIN); // RX, TX

// Variablen für die letzten empfangenen Kanäle
uint16_t lastChannels[4] = {0};

void setup() {
  Serial.begin(115200);       // USB-Serial-Monitor
  mySerial.begin(9600);       // Verbindung zum ESP32
  Serial.println("Arduino Debug gestartet...");
}

void loop() {
  // Prüfen, ob mindestens 9 Bytes verfügbar sind (Startbyte + 4*2 Bytes)
  if (mySerial.available() >= 9) {
    if (mySerial.peek() == START_BYTE) {
      mySerial.read(); // Startbyte entfernen

      // 4 Kanäle auslesen und in lastChannels speichern
      for (int i = 0; i < 4; i++) {
        uint8_t highByte = mySerial.read();
        uint8_t lowByte = mySerial.read();
        lastChannels[i] = (highByte << 8) | lowByte;
      }

      // Ausgabe der letzten empfangenen Kanäle über USB-Serial
      Serial.print("Ch1="); Serial.print(lastChannels[0]);
      Serial.print(" Ch2="); Serial.print(lastChannels[1]);
      Serial.print(" Ch3="); Serial.print(lastChannels[2]);
      Serial.print(" Ch4="); Serial.println(lastChannels[3]);
    } else {
      mySerial.read(); // falsches Byte verwerfen
    }
  }
}
