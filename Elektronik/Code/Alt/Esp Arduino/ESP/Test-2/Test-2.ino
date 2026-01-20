#include <HardwareSerial.h>

HardwareSerial CRSFSerial(1); // UART1 für CRSF
uint8_t crsfBuffer[64];
uint8_t crsfIndex = 0;
uint16_t channels[16] = {0};

#define START_BYTE 0xAA
unsigned long lastPrint = 0;

void setup() {
  Serial.begin(115200);                   // USB Monitor
  CRSFSerial.begin(420000, SERIAL_8N1, 16, 17); // RX1=16, TX1=17 für Arduino TX
  Serial.println("ESP32 CRSF Bridge gestartet...");
}

void loop() {
  // CRSF Bytes einlesen
  while (CRSFSerial.available()) {
    uint8_t b = CRSFSerial.read();
    crsfBuffer[crsfIndex++] = b;

    if (crsfIndex >= 2) {
      uint8_t len = crsfBuffer[1];
      if (crsfIndex == len + 2) {
        parseCRSFFrame(crsfBuffer, len + 2);
        crsfIndex = 0;

        // Debug-Ausgabe alle 500ms
        if (millis() - lastPrint >= 500) {
          lastPrint = millis();
          Serial.print("Frame empfangen: ");
          Serial.print("Ch1="); Serial.print(channels[0]);
          Serial.print(" Ch2="); Serial.print(channels[1]);
          Serial.print(" Ch3="); Serial.print(channels[2]);
          Serial.print(" Ch4="); Serial.println(channels[3]);
        }

        // Channels 1-4 an Arduino senden
        sendChannels();
      }
    }

    if (crsfIndex >= sizeof(crsfBuffer)) crsfIndex = 0;
  }
}

// Channels 1-4 senden mit Startbyte
void sendChannels() {
  CRSFSerial.write(START_BYTE);
  for (int i = 0; i < 4; i++) {
    CRSFSerial.write((channels[i] >> 8) & 0xFF); // High Byte
    CRSFSerial.write(channels[i] & 0xFF);        // Low Byte
  }
}

// CRSF Frame parsen (nur RC Channels)
void parseCRSFFrame(uint8_t* frame, uint8_t length) {
  if (length < 14) return;
  if (frame[2] != 0x16) return; // nur RC-Kanäle

  channels[0]  = ((frame[3]     | frame[4]<<8) & 0x07FF);
  channels[1]  = ((frame[4]>>3 | frame[5]<<5) & 0x07FF);
  channels[2]  = ((frame[5]>>6 | frame[6]<<2 | frame[7]<<10) & 0x07FF);
  channels[3]  = ((frame[7]>>1 | frame[8]<<7) & 0x07FF);
}
