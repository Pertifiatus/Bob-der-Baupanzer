#include <HardwareSerial.h>

HardwareSerial CRSFSerial(1);    // UART1 für CRSF

uint8_t crsfBuffer[64];
uint8_t crsfIndex = 0;
uint16_t channels[16] = {0};

#define START_BYTE 0xAA
unsigned long lastPrint = 0;

void setup() {
  Serial.begin(115200);                        // USB Monitor
  CRSFSerial.begin(420000, SERIAL_8N1, 16, 17); // CRSF: RX=16, TX=17
  Serial.println("ESP32 CRSF Bridge gestartet...");
}

void loop() {
  // Nur ein Byte pro Loop lesen → Watchdog-freundlich
  if (CRSFSerial.available()) {
    uint8_t b = CRSFSerial.read();
    if (crsfIndex < sizeof(crsfBuffer)) {
      crsfBuffer[crsfIndex++] = b;
    } else {
      crsfIndex = 0; // Overflow verhindern
    }

    // Prüfen, ob Frame komplett
    if (crsfIndex >= 2) {
      uint8_t len = crsfBuffer[1];
      if (crsfIndex == len + 2) {
        parseCRSFFrame(crsfBuffer, len + 2);
        crsfIndex = 0;

        // Debug-Ausgabe alle 500ms
        if (millis() - lastPrint >= 500) {
          lastPrint = millis();
          printChannels(); // Nur die Kanäle über Serial anzeigen
        }
      }
    }
  }
}

// Empfange und zeige die Kanäle über die serielle Konsole
void printChannels() {
  Serial.print("Frame empfangen: ");
  for (int i = 0; i < 16; i++) {
    Serial.print("Ch"); Serial.print(i + 1); Serial.print("=");
    Serial.print(channels[i]);
    if (i < 15) Serial.print(" | "); // Trenner zwischen den Kanälen
  }
  Serial.println();
}

// CRSF Frame parsen (alle RC Channels)
void parseCRSFFrame(uint8_t* frame, uint8_t length) {
  if (length < 14) return;         // Frame zu kurz
  if (frame[2] != 0x16) return;    // Nur RC-Kanäle (Frame Type 0x16)

  // Dekodieren der Kanäle (0-15)
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
