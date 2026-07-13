#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

const byte ST = 0xA0;
const int NUM_CHANNELS = 14;
int channels[NUM_CHANNELS];

#define RX 14
#define TX 27


#define LED_PIN 33
#define LED_COUNT 10
Servo Pitch;
Servo Sweep;
Servo video_switcher;
ESP32PWM pwm;

int LED_HT = 5;
int CAM;

unsigned int packetCount = 0;
unsigned int packetsPerSecond = 0;

LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 20 chars and 4 line display

// Binärprotokoll: pro Kanal 2 Bytes (low/high, je auf 7 Bit begrenzt), damit
// alle Nutzdaten-Bytes < 0x80 bleiben - nur die Start-Bytes der Boards
// (0xA0-0xA3) haben Bit 7 gesetzt und sind so immer eindeutig vom Kanalwert
// zu unterscheiden. Dadurch ist die Frame-Länge konstant, egal ob die Kanäle
// bei 0 oder bei 1000 stehen (vorher variierte die ASCII-Frame-Länge stark
// mit der Anzahl Ziffern, was bei hohen Kanalwerten Frames verschluckt hat).
const int FRAME_PAYLOAD_LEN = NUM_CHANNELS * 2;
byte frameBuffer[FRAME_PAYLOAD_LEN];
int frameIndex = 0;
bool receivingFrame = false;

void setup() {
  Serial.begin(250000);
  ESP32PWM::allocateTimer(0);

  Serial2.begin(250000, SERIAL_8N1, RX, TX);

  lcd.init();  // initialize the lcd
  lcd.backlight();

  Pitch.attach(25, 500, 2500);
  Sweep.attach(26, 500, 2500);
  video_switcher.attach(32);
}

void loop() {
  ReadSerial();

  Video();

  static unsigned long lastDisplayTime = 0;
  if (millis() - lastDisplayTime > 200) {
    lastDisplayTime = millis();
    updateDisplay();
  }

  static unsigned long lastPacketRateTime = 0;
  if (millis() - lastPacketRateTime >= 1000) {
    lastPacketRateTime = millis();
    packetsPerSecond = packetCount;
    packetCount = 0;
  }

  /*
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 200) {
    lastDebugTime = millis();
    printDebugInfo();
  }
  */
}





void Video() {
  //Head Tracker
  Pitch.write(map(channels[9], 0, 1000, 0, 180));
  Sweep.write(map(channels[10], 0, 1000, 0, 180));

  //Video Switcher
  CAM = map(channels[8], 0, 1000, 1, 3);
  if (CAM == 1) {
    video_switcher.writeMicroseconds(1000);
  }
  if (CAM == 2) {
    video_switcher.writeMicroseconds(1500);
  }
  if (CAM == 3) {
    video_switcher.writeMicroseconds(2000);
  }
}

void updateDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("Ch:");

  // Paket-Rate rechts in der ersten Zeile anzeigen
  char pktBuf[9];
  snprintf(pktBuf, sizeof(pktBuf), "Pk/s:%3u", packetsPerSecond > 999 ? 999 : packetsPerSecond);
  lcd.setCursor(20 - (sizeof(pktBuf) - 1), 0);
  lcd.print(pktBuf);

  // Erste Zeile mit Channels
  for (int I = 0; I <= 6; I++) {
    lcd.setCursor(I * 3, 1);
    lcd.print(map(channels[I], 0, 1000, 0, 9));
  }

  // Zweite Zeile mit Channels
  for (int I = 7; I <= 12; I++) {
    lcd.setCursor((I-7) * 3, 2);
    lcd.print(map(channels[I], 0, 1000, 0, 9));
  }

  // Dritte Zeile mit Channels
  for (int I = 13; I <= 13; I++) {
    lcd.setCursor((I-13) * 3, 3);
    lcd.print(map(channels[I], 0, 1000, 0, 9));
  }
}

void printDebugInfo() {

  Serial.print(" [OK] - Werte: ");

  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.print(channels[i]);
    if (i < NUM_CHANNELS - 1) Serial.print(",");
  }
  Serial.println();
}

// Prüft die Prüfsumme und übernimmt bei Erfolg die Kanalwerte aus frameBuffer.
void decodeFrame(byte receivedChecksum) {
  byte checksum = 0;
  for (int i = 0; i < FRAME_PAYLOAD_LEN; i++) checksum ^= frameBuffer[i];
  checksum &= 0x7F;

  if (checksum != receivedChecksum) {
    // Frame verwerfen, da Prüfsumme nicht passt (z.B. durch Bitfehler)
    return;
  }

  // Die XOR-Prüfsumme erkennt nicht jeden Bitfehler (z.B. zwei Bitkipper an
  // derselben Position in unterschiedlichen Bytes können sich gegenseitig
  // aufheben). Als zweite Verteidigungslinie werden die dekodierten Werte
  // zusätzlich auf den gültigen Bereich 0-1000 begrenzt, damit ein durch die
  // Prüfsumme rutschender Fehler keinen Kanalwert weit außerhalb des
  // erwarteten Bereichs an die Servos/den Videoswitcher weiterreichen kann.
  for (int i = 0; i < NUM_CHANNELS; i++) {
    byte lowBits = frameBuffer[i * 2];
    byte highBits = frameBuffer[i * 2 + 1];
    channels[i] = constrain(lowBits | (highBits << 7), 0, 1000);
  }

  packetCount++;
}

void ReadSerial() {
  while (Serial2.available() > 0) {  //Wenn Serielle Daten verfügbar sind
    byte incomingByte = Serial2.read();

    if (incomingByte == ST) {  //Falls unser Startbyte erkannt wird: neuen Frame beginnen
      receivingFrame = true;
      frameIndex = 0;
      continue;
    }

    if (incomingByte & 0x80) {
      // Startbyte eines anderen Boards -> unser Frame ist ungültig/beendet
      receivingFrame = false;
      continue;
    }

    if (!receivingFrame) continue;

    if (frameIndex < FRAME_PAYLOAD_LEN) {
      frameBuffer[frameIndex++] = incomingByte;
    } else {
      // Dieses Byte ist die Prüfsumme -> Frame ist komplett
      decodeFrame(incomingByte);
      receivingFrame = false;
    }
  }
}
