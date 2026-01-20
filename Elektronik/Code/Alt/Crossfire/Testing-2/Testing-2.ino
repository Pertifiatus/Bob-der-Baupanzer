#include <HardwareSerial.h>

HardwareSerial CRSFSerial(1);
uint8_t crsfBuffer[64];
uint8_t crsfIndex = 0;
uint16_t channels[16] = {0};

unsigned long lastPrint = 0;

void setup() {
  Serial.begin(115200);
  CRSFSerial.begin(420000, SERIAL_8N1, 16, 17);
}

void loop() {
  // Alle Bytes verarbeiten
  while (CRSFSerial.available()) {
    uint8_t b = CRSFSerial.read();
    crsfBuffer[crsfIndex++] = b;

    if (crsfIndex >= 2) {
      uint8_t len = crsfBuffer[1];
      if (crsfIndex == len + 2) {
        parseCRSFFrame(crsfBuffer, len + 2);
        crsfIndex = 0;
      }
    }

    if (crsfIndex >= sizeof(crsfBuffer)) crsfIndex = 0;
  }

  // Alle 20ms die aktuellen Kanalwerte ausgeben
  if (millis() - lastPrint >= 5) {
    lastPrint = millis();
    Serial.print("RC: ");
    for (int i=0; i<16; i++) {
      Serial.print(channels[i]);
      if (i < 15) Serial.print(" | ");
    }
    Serial.println();
  }
}

void parseCRSFFrame(uint8_t* frame, uint8_t length) {
  if (length < 14) return;
  if (frame[2] != 0x16) return; // nur RC-Kanäle

  // 16 Kanäle decodieren
  channels[0]  = ((frame[3]     | frame[4]<<8) & 0x07FF);
  channels[1]  = ((frame[4]>>3 | frame[5]<<5) & 0x07FF);
  channels[2]  = ((frame[5]>>6 | frame[6]<<2 | frame[7]<<10) & 0x07FF);
  channels[3]  = ((frame[7]>>1 | frame[8]<<7) & 0x07FF);
  channels[4]  = ((frame[8]>>4 | frame[9]<<4) & 0x07FF);
  channels[5]  = ((frame[9]>>7 | frame[10]<<1 | frame[11]<<9) & 0x07FF);
  channels[6]  = ((frame[11]>>2 | frame[12]<<6) & 0x07FF);
  channels[7]  = ((frame[12]>>5 | frame[13]<<3) & 0x07FF);
  channels[8]  = ((frame[14]    | frame[15]<<8) & 0x07FF);
  channels[9]  = ((frame[15]>>3 | frame[16]<<5) & 0x07FF);
  channels[10] = ((frame[16]>>6 | frame[17]<<2 | frame[18]<<10) & 0x07FF);
  channels[11] = ((frame[18]>>1 | frame[19]<<7) & 0x07FF);
  channels[12] = ((frame[19]>>4 | frame[20]<<4) & 0x07FF);
  channels[13] = ((frame[20]>>7 | frame[21]<<1 | frame[22]<<9) & 0x07FF);
  channels[14] = ((frame[22]>>2 | frame[23]<<6) & 0x07FF);
  channels[15] = ((frame[23]>>5 | frame[24]<<3) & 0x07FF);
}