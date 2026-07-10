#include <Servo.h>

#define ENABLE_PIN 8
#define SPEED 180  //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning
int EN = 1;        // 0=Enabled, 1=Disabled
int Drive = 0;     // Drive Status
long speedX = 0;
long speedY = 0;

Servo ESC1;       // create Servo object to control a servo
int ESC1pos = 0;  // variable to store the servo position

int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

const byte ST = 0xA1;
const int NUM_CHANNELS = 6;
int channels[NUM_CHANNELS];

unsigned long lastPacketTime = 0;
const unsigned long TimeoutFailsafe = 500;
bool failsafeActive = false;

// Binärprotokoll: pro Kanal 2 Bytes (low/high, je auf 7 Bit begrenzt), damit
// alle Nutzdaten-Bytes < 0x80 bleiben - nur die Start-Bytes der Boards
// (0xA0-0xA3) haben Bit 7 gesetzt und sind so immer eindeutig vom Kanalwert
// zu unterscheiden. Dadurch ist die Frame-Länge konstant, egal ob die Kanäle
// bei 0 oder bei 1000 stehen.
const int FRAME_PAYLOAD_LEN = NUM_CHANNELS * 2;
byte frameBuffer[FRAME_PAYLOAD_LEN];
int frameIndex = 0;
bool receivingFrame = false;

void setup() {
  Serial.begin(115200);

  ESC1.attach(9);  // attaches the servo on pin 9 to the Servo object
  ESC1.write(90);
  Serial.println("Waiting 2s");
  delay(2000);
  Serial.println("Waited");

  activateFailsafe();
  lastPacketTime = millis();
}

void loop() {
  ReadSerial();
  Channellogic();
  ESC();
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 100) {  // Nur alle 100ms Text ausgeben (schont die CPU)
    lastDebugTime = millis();
  printDebugInfo();
  }
}


void ReadSerial() {
  while (Serial.available() > 0) {  //Wenn Serielle Daten verfügbar sind
    byte incomingByte = Serial.read();

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

  //  Failsafe Check
  if (millis() - lastPacketTime > TimeoutFailsafe) {
    if (!failsafeActive) {
      activateFailsafe();
      failsafeActive = true;
    }
  }
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

  for (int i = 0; i < NUM_CHANNELS; i++) {
    byte lowBits = frameBuffer[i * 2];
    byte highBits = frameBuffer[i * 2 + 1];
    channels[i] = lowBits | (highBits << 7);
  }

  // Wenn Daten korrekt ankommen -> Failsafe beenden
  if (failsafeActive) {
    failsafeActive = false;
  }
  lastPacketTime = millis();  // Zeitstempel für Timeout-Timer erneuern
}

void activateFailsafe() {
  // Sicherheitswerte definieren (z.B. alles auf 0)
  channels[0] = 500;
  channels[1] = 500;
  channels[2] = 500;
  channels[3] = 500;
  channels[4] = 0;
  channels[5] = 1000;
}

void printDebugInfo() {
  if (failsafeActive) {
    Serial.print(" [!] - Fallback: ");
  } else {
    Serial.print(" [OK] - Werte: ");
  }

  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.print(channels[i]);
    if (i < NUM_CHANNELS - 1) Serial.print(",");
  }
  Serial.println();
}

void Channellogic() {
  speedX = map(channels[0], 0, 1000, 0, SPEED);
  speedX = applyDeadzone(speedX, 100);

  speedY = map(channels[1], 0, 1000, 0, SPEED);
  speedY = applyDeadzone(speedY, 100);

  // Logik für Enable (EN)
  if (channels[5] < 500) {
    EN = 0;  // Motor AN
  } else {
    EN = 1;  // Motor AUS
  }

  if (channels[4] < 500) {
    Drive = 1;
  } else {
    Drive = 0;
  }
  if (Drive == 1) {  // Arming in Drive Action
    if (speedX != 0 || speedY != 0) {
      EN = 0;  // Wenn die Sticks nicht center sind, Motor AN
    }
  }
  if (channels[5] > 500 && channels[4] > 500) {
    EN = 1;
  }
}

void ESC() {

  if (Drive == 1) {
    ESC1.write(speedX);
  }
  if (Drive == 0) {
    ESC1.write(90);
  }
}
