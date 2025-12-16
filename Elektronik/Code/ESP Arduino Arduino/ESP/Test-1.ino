#include <HardwareSerial.h>

HardwareSerial CRSFSerial(1);    // UART1 für CRSF
HardwareSerial ArduinoMotorSerial(2); // UART2 für Arduino
HardwareSerial ArduinoGreifSerial(3); // UART2 für Arduino

uint8_t crsfBuffer[64];
uint8_t crsfIndex = 0;
uint16_t channels[16] = {0};

#define START_BYTE 0xAA
unsigned long lastPrint = 0;
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);                        // USB Monitor
  CRSFSerial.begin(420000, SERIAL_8N1, 16, 17); // CRSF: RX=16, TX=17
  ArduinoGreifSerial.begin(9600, SERIAL_8N1, 25, 26); // TX=26, RX=25
  ArduinoMotorSerial.begin(9600, SERIAL_8N1, 32, 33); // TX=33, RX=32
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
          Serial.print("Frame empfangen: ");
          Serial.print("Ch1="); Serial.print(channels[0]);
          Serial.print(" Ch2="); Serial.print(channels[1]);
          Serial.print(" Ch3="); Serial.print(channels[2]);
          Serial.print(" Ch4="); Serial.println(channels[3]);
        }
      }
    }
  }

  // Channels alle 20ms an Arduino senden
  if (millis() - lastSend >= 20) {
    lastSend = millis();
    sendChannelsToArduino();
  }
}

// Channels 1-4 an Arduino senden
void sendChannelsToArduino() {
  ArduinoGreifSerial.write(START_BYTE);
  for (int i = 0; i < 4; i++) {
    ArduinoGreifSerial.write((channels[i] >> 8) & 0xFF); // High Byte
    ArduinoGreifSerial.write(channels[i] & 0xFF);        // Low Byte
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
