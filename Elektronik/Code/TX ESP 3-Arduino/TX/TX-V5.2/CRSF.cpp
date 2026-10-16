#include "CRSF.h"

// CRSF-Frame-Aufbau:  [Sync/Adresse] [Länge] [Typ] [Nutzdaten ...] [CRC8]
// "Länge" zählt Typ + Nutzdaten + CRC, also Gesamtlänge = Länge + 2.
// Die CRC8 (Polynom 0xD5) läuft über Typ + Nutzdaten.

static const uint8_t CRSF_SYNC_FC      = 0xC8;  // Empfänger -> Flight Controller (normaler Fall)
static const uint8_t CRSF_SYNC_RADIO   = 0xEA;
static const uint8_t CRSF_SYNC_RX      = 0xEC;
static const uint8_t CRSF_SYNC_TX      = 0xEE;

static const uint8_t CRSF_TYPE_LINK_STATS = 0x14;
static const uint8_t CRSF_TYPE_CHANNELS   = 0x16;

static const uint8_t CRSF_MAX_FRAME   = 64;     // laut Spezifikation max. 64 Byte inkl. Header

static bool isSyncByte(uint8_t b) {
    return b == CRSF_SYNC_FC || b == CRSF_SYNC_RADIO || b == CRSF_SYNC_RX ||
           b == CRSF_SYNC_TX;
}

CRSF::CRSF(int rxPin, int txPin, uint8_t uartNum)
    : _serial(uartNum), _rxPin(rxPin), _txPin(txPin), crsfIndex(0),
      _hasStats(false), _lastChannelsMs(0), _lastStatsMs(0), _everReceived(false),
      _goodFrames(0), _crcErrors(0)
{
    for (int i = 0; i < 16; i++) channels[i] = 0;
    memset(&_stats, 0, sizeof(_stats));
}

void CRSF::begin(uint32_t baud) {
    _serial.begin(baud, SERIAL_8N1, _rxPin, _txPin);
}

void CRSF::update() {
    // Begrenzt, damit ein Byte-Sturm den loop() nicht blockiert
    int n = _serial.available();
    while (n-- > 0) feed(_serial.read());
}

void CRSF::feed(uint8_t b) {
    // 1) Auf Sync-Byte warten. Ohne diesen Schritt kann der Parser nach einem
    //    einzigen verlorenen Byte dauerhaft "verrutschen".
    if (crsfIndex == 0) {
        if (!isSyncByte(b)) return;
        crsfBuffer[crsfIndex++] = b;
        return;
    }

    // 2) Längen-Byte prüfen: mindestens Typ + CRC (2), höchstens 62
    if (crsfIndex == 1) {
        if (b < 2 || b > CRSF_MAX_FRAME - 2) {
            // Ungültig -> neu synchronisieren. Das Byte könnte selbst ein Sync sein.
            crsfIndex = 0;
            if (isSyncByte(b)) crsfBuffer[crsfIndex++] = b;
            return;
        }
        crsfBuffer[crsfIndex++] = b;
        return;
    }

    crsfBuffer[crsfIndex++] = b;

    uint8_t total = crsfBuffer[1] + 2;
    if (crsfIndex >= total) {
        parseCRSFFrame(crsfBuffer, total);
        crsfIndex = 0;
    }
}

uint8_t CRSF::crc8(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0xD5) : (uint8_t)(crc << 1);
    }
    return crc;
}

void CRSF::parseCRSFFrame(const uint8_t* frame, uint8_t length) {
    uint8_t lenField = frame[1];
    // CRC über [Typ .. letztes Nutzdatenbyte] = lenField - 1 Bytes ab frame[2]
    uint8_t crc = crc8(&frame[2], lenField - 1);
    if (crc != frame[length - 1]) {
        _crcErrors++;
        return;   // kaputter Frame -> verwerfen, KEINE Kanalwerte übernehmen
    }
    _goodFrames++;

    uint8_t type = frame[2];
    const uint8_t* payload = &frame[3];
    uint8_t payloadLen = lenField - 2;

    if (type == CRSF_TYPE_CHANNELS && payloadLen == 22) {
        parseChannels(payload);
    } else if (type == CRSF_TYPE_LINK_STATS && payloadLen >= 10) {
        parseLinkStats(payload);
    }
}

void CRSF::parseChannels(const uint8_t* p) {
    // 16 Kanäle à 11 Bit, LSB zuerst, dicht gepackt in 22 Bytes
    uint32_t bitBuf = 0;
    uint8_t  bits = 0;
    uint8_t  ch = 0;
    for (uint8_t i = 0; i < 22 && ch < 16; i++) {
        bitBuf |= (uint32_t)p[i] << bits;
        bits += 8;
        while (bits >= 11 && ch < 16) {
            channels[ch++] = bitBuf & 0x07FF;
            bitBuf >>= 11;
            bits -= 11;
        }
    }
    _lastChannelsMs = millis();
    _everReceived = true;
}

void CRSF::parseLinkStats(const uint8_t* p) {
    _stats.uplinkRssi1   = p[0];
    _stats.uplinkRssi2   = p[1];
    _stats.uplinkLQ      = p[2];
    _stats.uplinkSNR     = (int8_t)p[3];
    _stats.activeAntenna = p[4];
    _stats.rfMode        = p[5];
    _stats.uplinkTxPower = p[6];
    _stats.downlinkRssi  = p[7];
    _stats.downlinkLQ    = p[8];
    _stats.downlinkSNR   = (int8_t)p[9];
    _hasStats = true;
    _lastStatsMs = millis();
}

uint16_t CRSF::read(uint8_t channel) {
    if (channel < 1 || channel > 16) return 0;
    return channels[channel - 1];
}

bool CRSF::isLinkUp(uint32_t timeoutMs) {
    if (!_everReceived) return false;
    if (millis() - _lastChannelsMs > timeoutMs) return false;
    // Wenn der Empfänger selbst LQ 0 meldet (frisch), Verbindung als weg werten
    if (_hasStats && millis() - _lastStatsMs < 500 && _stats.uplinkLQ == 0) return false;
    return true;
}

uint32_t CRSF::msSinceLastChannels() {
    if (!_everReceived) return UINT32_MAX;
    return millis() - _lastChannelsMs;
}

bool CRSF::hasLinkStats() { return _hasStats; }

uint8_t CRSF::linkQuality() { return _hasStats ? _stats.uplinkLQ : 0; }

int CRSF::rssi() {
    if (!_hasStats) return 0;
    uint8_t v = _stats.activeAntenna ? _stats.uplinkRssi2 : _stats.uplinkRssi1;
    return -(int)v;
}

int8_t CRSF::snr() { return _hasStats ? _stats.uplinkSNR : 0; }

const CRSFLinkStats& CRSF::linkStats() { return _stats; }
