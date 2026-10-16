#ifndef CRSF_H
#define CRSF_H

#include <Arduino.h>

// Link-Statistik eines ELRS/Crossfire-Empfängers (CRSF Frame-Typ 0x14)
struct CRSFLinkStats {
    uint8_t uplinkRssi1;      // Antenne 1, Betrag in dBm (z.B. 65 = -65 dBm)
    uint8_t uplinkRssi2;      // Antenne 2, Betrag in dBm
    uint8_t uplinkLQ;         // Link Quality 0-100 %  (das ist "LQ" im Handsender)
    int8_t  uplinkSNR;        // dB
    uint8_t activeAntenna;    // 0 oder 1
    uint8_t rfMode;           // Paketraten-Index (ELRS-spezifisch)
    uint8_t uplinkTxPower;    // Index (0=0mW, 1=10mW, 2=25mW, 3=100mW, ...)
    uint8_t downlinkRssi;     // Betrag in dBm (Telemetrie zurück zum Sender)
    uint8_t downlinkLQ;       // 0-100 %
    int8_t  downlinkSNR;      // dB
};

class CRSF {
public:
    // uartNum: welche Hardware-UART benutzt wird (ESP32-C3 hat 0 und 1)
    CRSF(int rxPin, int txPin, uint8_t uartNum = 1);

    void begin(uint32_t baud = 420000);
    void update();                    // Muss im loop() aufgerufen werden
    void feed(uint8_t b);             // Ein einzelnes Byte verarbeiten (wird von update() benutzt)

    uint16_t read(uint8_t channel);   // 1-16, Rohwert 172-1811 (0 = noch nie empfangen)

    // --- Verbindungs-Überwachung ---
    // true, wenn innerhalb von timeoutMs ein gültiger (CRC-geprüfter) Kanal-Frame kam.
    // ELRS-Empfänger hören bei Verbindungsverlust auf, Kanal-Frames zu schicken,
    // die alten Kanalwerte bleiben in der Library aber stehen - deshalb IMMER
    // diese Funktion statt "alle Kanäle == 0" für den Failsafe benutzen.
    bool isLinkUp(uint32_t timeoutMs = 250);
    uint32_t msSinceLastChannels();

    uint8_t linkQuality();            // Uplink-LQ 0-100 %, 0 wenn noch keine Statistik kam
    int     rssi();                   // RSSI der aktiven Antenne in dBm (negativ), 0 = unbekannt
    int8_t  snr();
    bool    hasLinkStats();
    const CRSFLinkStats& linkStats(); // alle Werte

    // Diagnose-Zähler
    uint32_t goodFrames()  { return _goodFrames; }
    uint32_t crcErrors()   { return _crcErrors; }

private:
    HardwareSerial _serial;
    int _rxPin;
    int _txPin;

    uint8_t  crsfBuffer[64];
    uint8_t  crsfIndex;
    uint16_t channels[16];

    CRSFLinkStats _stats;
    bool     _hasStats;
    uint32_t _lastChannelsMs;
    uint32_t _lastStatsMs;
    bool     _everReceived;

    uint32_t _goodFrames;
    uint32_t _crcErrors;

    static uint8_t crc8(const uint8_t* data, uint8_t len);
    void parseCRSFFrame(const uint8_t* frame, uint8_t length);
    void parseChannels(const uint8_t* payload);
    void parseLinkStats(const uint8_t* payload);
};

#endif
