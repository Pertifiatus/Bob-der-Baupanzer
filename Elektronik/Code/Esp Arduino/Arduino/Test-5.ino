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
      int Channel1 = map(lastChannels[0], 170, 1811, 0, 100);
      int Channel2 = map(lastChannels[1], 170, 1811, 0, 100);
      int Channel3 = map(lastChannels[2], 170, 1811, 0, 100);
      int Channel4 = map(lastChannels[3], 170, 1811, 0, 100);

      
      Serial.print("Ch1="); Serial.print(Channel1);
      Serial.print(" Ch2="); Serial.print(Channel2);
      Serial.print(" Ch3="); Serial.print(Channel3);
      Serial.print(" Ch4="); Serial.println(Channel4);
    } else {
      mySerial.read(); // falsches Byte verwerfen
    }
  }
}
