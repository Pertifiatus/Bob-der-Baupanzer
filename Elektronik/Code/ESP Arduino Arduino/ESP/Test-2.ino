#include <HardwareSerial.h>

HardwareSerial CRSFSerial(1);    // UART1 für CRSF
HardwareSerial ArduinoMotorSerial(2); // UART2 für Arduino
HardwareSerial ArduinoGreifSerial(3); // UART3 für Arduino

uint8_t crsfBuffer[64];
uint8_t crsfIndex = 0;
uint16_t channels[16] = {0};

#define START_BYTE 0xAA
unsigned long lastPrint = 0;
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);                        
  CRSFSerial.begin(420000, SERIAL_8N1, 15, 17); // CRSF: RX=16, TX=17
  ArduinoMotorSerial.begin(9600, SERIAL_8N1, 25, 26); // TX=26, RX=25
  ArduinoGreifSerial.begin(9600, SERIAL_8N1, 12, 14); // TX=12, RX=14
  Serial.println("ESP32 CRSF Bridge gestartet...");
}

void loop() {
  if (CRSFSerial.available()) {
    uint8_t b = CRSFSerial.read();
    if (crsfIndex < sizeof(crsfBuffer)) {
      crsfBuffer[crsfIndex++] = b;
    } else {
      crsfIndex = 0;
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

          Serial.print("Channels: ");
          for (int i = 0; i < 16; i++) {
            Serial.print(channels[i]);
            Serial.print(i < 15 ? ", " : "\n");
          }
        }
      }
    }
  }
  // Channels alle 20ms an Arduino senden
  if (millis() - lastSend >= 20) {
    lastSend = millis();
    sendChannelsToArduinoGreif();
    sendChannelsToArduinoMotor();
  }
}
// Channels 1-4 an Arduino senden
void sendChannelsToArduinoGreif() {
  ArduinoMotorSerial.write(START_BYTE);
  for (int i = 0; i < 3; i++) {
    ArduinoMotorSerial.write((channels[i] >> 8) & 0xFF);
    ArduinoMotorSerial.write(channels[i] & 0xFF);
  }
}

void sendChannelsToArduinoMotor() {
  ArduinoMotorSerial.write(START_BYTE);
  for (int i = 3; i < 163; i++) {
    ArduinoMotorSerial.write((channels[i] >> 8) & 0xFF);
    ArduinoMotorSerial.write(channels[i] & 0xFF);
  }
}

// CRSF Frame parsen (nur RC Channels)
void parseCRSFFrame(uint8_t* frame, uint8_t length) {
  if (length < 26) return;     // 16 Kanäle brauchen 22 payload-bytes
  if (frame[2] != 0x16) return; 

  // CRSF 16 Channel Mapping
  channels[0]  = (frame[3] | frame[4] << 8) & 0x07FF;
  channels[1]  = (frame[4] >> 3 | frame[5] << 5) & 0x07FF;
  channels[2]  = (frame[5] >> 6 | frame[6] << 2 | frame[7] << 10) & 0x07FF;
  channels[3]  = (frame[7] >> 1 | frame[8] << 7) & 0x07FF;
  channels[4]  = (frame[8] >> 4 | frame[9] << 4) & 0x07FF;
  channels[5]  = (frame[9] >> 7 | frame[10] << 1 | frame[11] << 9) & 0x07FF;
  channels[6]  = (frame[11] >> 2 | frame[12] << 6) & 0x07FF;
  channels[7]  = (frame[12] >> 5 | frame[13] << 3) & 0x07FF;
  channels[8]  = (frame[14] | frame[15] << 8) & 0x07FF;
  channels[9]  = (frame[15] >> 3 | frame[16] << 5) & 0x07FF;
  channels[10] = (frame[16] >> 6 | frame[17] << 2 | frame[18] << 10) & 0x07FF;
  channels[11] = (frame[18] >> 1 | frame[19] << 7) & 0x07FF;
  channels[12] = (frame[19] >> 4 | frame[20] << 4) & 0x07FF;
  channels[13] = (frame[20] >> 7 | frame[21] << 1 | frame[22] << 9) & 0x07FF;
  channels[14] = (frame[22] >> 2 | frame[23] << 6) & 0x07FF;
  channels[15] = (frame[23] >> 5 | frame[24] << 3) & 0x07FF;
}
