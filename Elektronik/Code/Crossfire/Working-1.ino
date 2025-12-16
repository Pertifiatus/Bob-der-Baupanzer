#include <HardwareSerial.h>

HardwareSerial CRSFSerial(1);

uint8_t crsfBuffer[64];
uint8_t crsfIndex = 0;

void setup() {
  Serial.begin(115200);
  CRSFSerial.begin(420000, SERIAL_8N1, 16, 17);
  Serial.println("CRSF Listener gestartet...");
}

void loop() {
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
}

void parseCRSFFrame(uint8_t* frame, uint8_t length) {
  uint8_t type = frame[2]; // Frame-Typ

  if (type != 0x16) return; // nur RC-Kanäle

  uint16_t channels[16];

  // 8 Kanäle decodieren (11-bit)
  channels[0]  = ((frame[3]     | frame[4]<<8) & 0x07FF);
  channels[1]  = ((frame[4]>>3 | frame[5]<<5) & 0x07FF);
  channels[2]  = ((frame[5]>>6 | frame[6]<<2 | frame[7]<<10) & 0x07FF);
  channels[3]  = ((frame[7]>>1 | frame[8]<<7) & 0x07FF);
  channels[4]  = ((frame[8]>>4 | frame[9]<<4) & 0x07FF);
  channels[5]  = ((frame[9]>>7 | frame[10]<<1 | frame[11]<<9) & 0x07FF);
  channels[6]  = ((frame[11]>>2 | frame[12]<<6) & 0x07FF);
  channels[7]  = ((frame[12]>>5 | frame[13]<<3) & 0x07FF);

  Serial.println("RC Kanäle:");
  for (int i=0; i<8; i++) {
    Serial.printf("CH%02d: %d\n", i+1, channels[i]);
  }
}
