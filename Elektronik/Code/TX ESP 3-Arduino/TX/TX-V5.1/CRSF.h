#ifndef CRSF_H
#define CRSF_H

#include <Arduino.h>

class CRSF {
public:
    CRSF(int rxPin, int txPin);  // RX/TX Pins beim Erstellen angeben

    void begin(uint32_t baud = 420000);
    void update();               // Muss im loop() aufgerufen werden
    uint16_t read(uint8_t channel);  // 1–16

private:
    HardwareSerial _serial;
    int _rxPin;
    int _txPin;

    uint8_t crsfBuffer[64];
    uint8_t crsfIndex;
    uint16_t channels[16];

    void parseCRSFFrame(uint8_t* frame, uint8_t length);
};

#endif