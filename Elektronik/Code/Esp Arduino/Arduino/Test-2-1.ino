#include <SoftwareSerial.h>

#define RX_PIN 8
#define START_BYTE 0xAA

SoftwareSerial espSerial(RX_PIN, 255); // TX nicht benötigt
uint16_t channels[4];

void setup() {
  Serial.begin(115200);
  Serial.println("Läuft");
  espSerial.begin(115200); //Softseriall auf 115200 Baud rate
}

void loop() {
  if (espSerial.available() >= 9) { // Startbyte + 8 Bytes
    if (espSerial.read() == START_BYTE) {
      for (int i = 0; i < 4; i++) {
        uint8_t highByte = espSerial.read();
        uint8_t lowByte  = espSerial.read();
        channels[i] = (highByte << 8) | lowByte;
      }

      Serial.print("Channels 1-4: ");
      for (int i = 0; i < 4; i++) {
        Serial.print(channels[i]);
        if (i < 3) Serial.print(" | ");
      }
      Serial.println();
    }
  }
}
