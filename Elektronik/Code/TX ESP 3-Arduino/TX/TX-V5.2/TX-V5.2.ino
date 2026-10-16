#include "CRSF.h"

// RX = GPIO6, TX = GPIO5
CRSF crossfire(6, 5);
HardwareSerial TXSerial(0);

unsigned long lastSendESP = 0;
unsigned long lastSendA1 = 0;
unsigned long lastSendA2 = 0;
unsigned long lastSendA3 = 0;

// --- Failsafe-Einstellungen ---
// Kommt so lange kein gültiger Kanal-Frame vom ELRS-Empfänger, gilt die
// Verbindung als weg. ELRS schickt je nach Paketrate 50-500 Frames/s,
// 250 ms sind also viele verpasste Pakete am Stück.
const unsigned long LINK_TIMEOUT_MS = 250;

// Unterhalb dieser LQ (in %) wird ebenfalls Failsafe ausgelöst.
// 0 = nur bei komplettem Verbindungsverlust. Ein Wert wie 20 macht den
// Panzer vorsichtiger, kann bei grenzwertiger Reichweite aber "ruckeln".
const uint8_t LQ_MIN = 0;

bool failsafe = true;

const byte ST_ESP = 0xA0;  // Start-Token für ESP32
const byte ST_A1 = 0xA1;   // Start-Token für Arduino Nr. 1
const byte ST_A2 = 0xA2;   // Start-Token für Arduino Nr. 2
const byte ST_A3 = 0xA3;   // Start-Token für Arduino Nr. 3
bool DebugMode = false;

const int CHANNELS_ESP = 14;
const int CHANNELS_A1 = 6;
const int CHANNELS_A2 = 8;
const int CHANNELS_A3 = 8;

// Frame-Länge im Binärprotokoll: 1 Start-Byte + 2 Bytes je Kanal + 1 Prüfsummen-Byte.
const int FRAME_LEN_ESP = 1 + CHANNELS_ESP * 2 + 1;  // 30 Byte
const int FRAME_LEN_A1 = 1 + CHANNELS_A1 * 2 + 1;    // 14 Byte
const int FRAME_LEN_A2 = 1 + CHANNELS_A2 * 2 + 1;    // 18 Byte
const int FRAME_LEN_A3 = 1 + CHANNELS_A3 * 2 + 1;    // 18 Byte

bool receiverConnected() {
  if (!crossfire.isLinkUp(LINK_TIMEOUT_MS)) return false;
  if (LQ_MIN > 0 && crossfire.hasLinkStats() && crossfire.linkQuality() < LQ_MIN) return false;
  return true;
}

void setup() {
  Serial.begin(250000);
  crossfire.begin();

  TXSerial.begin(250000, SERIAL_8N1, 20, 21);  // RX = 20 , TX = 21

  unsigned long jetzt = millis();
  lastSendESP = jetzt;
  lastSendA1 = jetzt - 20;
  lastSendA2 = jetzt - 28;
  lastSendA3 = jetzt - 36;
}

void loop() {
  crossfire.update();

  bool connected = receiverConnected();
  if (connected != !failsafe) {
    failsafe = !connected;
    Serial.println(failsafe ? "[!] FAILSAFE - keine Daten mehr an die Boards"
                            : "[OK] Verbindung da - sende wieder");
  }

  if (DebugMode) {
    printDebug();   // auch im Failsafe, damit man sieht WARUM
    return;
  }

  // Im Failsafe nichts senden -> die Boards gehen nach ihrem eigenen
  // Timeout (500 ms) in ihre activateFailsafe()-Werte.
  if (failsafe) return;

  if (millis() - lastSendESP >= 50 && TXSerial.availableForWrite() > FRAME_LEN_ESP) {
    lastSendESP = millis();
    sendESP();
  }
  if (millis() - lastSendA1 >= 50 && TXSerial.availableForWrite() > FRAME_LEN_A1) {
    lastSendA1 = millis();
    sendA1();
  }
  if (millis() - lastSendA2 >= 50 && TXSerial.availableForWrite() > FRAME_LEN_A2) {
    lastSendA2 = millis();
    sendA2();
  }
  if (millis() - lastSendA3 >= 50 && TXSerial.availableForWrite() > FRAME_LEN_A3) {
    lastSendA3 = millis();
    sendA3();
  }
}

// Debug-Ausgabe nur alle 200 ms, sonst flutet sie den Serial-Port und
// crossfire.update() kommt nicht mehr hinterher.
void printDebug() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint < 200) return;
  lastPrint = millis();

  Serial.print(failsafe ? "[FS] " : "[OK] ");
  Serial.print("LQ:");
  Serial.print(crossfire.linkQuality());
  Serial.print("% RSSI:");
  Serial.print(crossfire.rssi());
  Serial.print("dBm SNR:");
  Serial.print(crossfire.snr());
  Serial.print(" age:");
  Serial.print(crossfire.msSinceLastChannels());
  Serial.print("ms CRCerr:");
  Serial.print(crossfire.crcErrors());
  for (int i = 1; i <= 16; i++) {
    Serial.print(" CH");
    Serial.print(i);
    Serial.print(":");
    Serial.print(crossfire.read(i));
  }
  Serial.println();
}

// Kodiert und sendet einen Kanal-Frame im Binärprotokoll.
// Format: [Start-Byte][2 Bytes je Kanal][1 Prüfsummen-Byte]
// (unverändert zu V5.1 - die Boards brauchen kein Update)
void sendChannelFrame(byte startToken, int channelCount) {
  TXSerial.write(startToken);

  byte checksum = 0;
  for (int i = 1; i <= channelCount; i++) {
    int value = map(crossfire.read(i), 172, 1811, 0, 1000);
    value = constrain(value, 0, 1000);

    byte lowBits = value & 0x7F;
    byte highBits = (value >> 7) & 0x7F;

    TXSerial.write(lowBits);
    TXSerial.write(highBits);
    checksum ^= lowBits ^ highBits;
  }

  TXSerial.write(checksum & 0x7F);
}

void sendESP() {
  sendChannelFrame(ST_ESP, CHANNELS_ESP);
}
void sendA1() {
  sendChannelFrame(ST_A1, CHANNELS_A1);
}
void sendA2() {
  sendChannelFrame(ST_A2, CHANNELS_A2);
}
void sendA3() {
  sendChannelFrame(ST_A3, CHANNELS_A3);
}
