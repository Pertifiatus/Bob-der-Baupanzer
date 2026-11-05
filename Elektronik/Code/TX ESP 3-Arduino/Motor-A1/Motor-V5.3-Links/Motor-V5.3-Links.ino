#include <Wire.h>
#include <AS5600.h>

#define Maxrpm 180  //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning
#define PinDriverF 5
#define PinDriverR 6
#define intervall 100
#define Debug true

int Drive = 0;  // Drive/Enabled Status
long Speed = 0;
float rpm = 0;
float raw = 0;

const byte ST = 0xA1;  //Startbyte des Drive Arduinos
const int NUM_CHANNELS = 6;
int channels[NUM_CHANNELS];

unsigned long lastDebugTime = 0;
unsigned long lastPacketTime = 0;
const unsigned long TimeoutFailsafe = 500;
bool failsafeActive = true;

const int FRAME_PAYLOAD_LEN = NUM_CHANNELS * 2;
byte frameBuffer[FRAME_PAYLOAD_LEN];
int frameIndex = 0;
bool receivingFrame = false;

AS5600 as5600;

int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

void setup() {
  Serial.begin(250000);
  Wire.begin();  // muss bei neueren Versionen selbst aufgerufen werden
  Wire.setClock(400000);
  as5600.begin();
  as5600.setDirection(AS5600_COUNTERCLOCK_WISE);

  pinMode(PinDriverF, OUTPUT);
  pinMode(PinDriverR, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(PinDriverF, 0);
  digitalWrite(PinDriverR, 0);


  activateFailsafe();

  while (!as5600.isConnected()) {
    digitalWrite(LED_BUILTIN, 1);
  }
  digitalWrite(LED_BUILTIN, 0);
  while (as5600.magnetTooWeak() || as5600.magnetTooStrong()) {
    digitalWrite(LED_BUILTIN, 1);
    delay(500);
    digitalWrite(LED_BUILTIN, 0);
    delay(500);
  }

  lastPacketTime = millis();
}

void loop() {
  ReadSerial();
  Channellogic();
  Motor();
  ReadRPM();
  if(Debug)printDebugInfo();
}

void ReadRPM() {
  static unsigned long last = 0;
  if (millis() - last < 10) return;  // feste 10 ms Abtastung
  last = millis();
  raw = as5600.getAngularSpeed(AS5600_MODE_RPM);
  rpm = 0.8 * rpm + 0.2 * raw;  // Tiefpassfilter
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

  // Die XOR-Prüfsumme erkennt nicht jeden Bitfehler (z.B. zwei Bitkipper an
  // derselben Position in unterschiedlichen Bytes können sich gegenseitig
  // aufheben). Als zweite Verteidigungslinie werden die dekodierten Werte
  // zusätzlich auf den gültigen Bereich 0-1000 begrenzt, damit ein durch die
  // Prüfsumme rutschender Fehler keinen Kanalwert weit außerhalb des
  // erwarteten Bereichs an die Motorsteuerung weiterreichen kann.
  for (int i = 0; i < NUM_CHANNELS; i++) {
    byte lowBits = frameBuffer[i * 2];
    byte highBits = frameBuffer[i * 2 + 1];
    channels[i] = constrain(lowBits | (highBits << 7), 0, 1000);
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
  if (millis() - lastDebugTime > intervall) {
    lastDebugTime = millis();
    if (failsafeActive) {
      Serial.print(" [!] - Fallback: ");
    } else {
      Serial.print(" [OK] - Werte: ");
    }

    for (int i = 0; i < NUM_CHANNELS; i++) {
      Serial.print(channels[i]);
      if (i < NUM_CHANNELS - 1) Serial.print(",");
    }
    Serial.print("  RPM");
    Serial.print(rpm);
    Serial.println();
  }
}

void Channellogic() {
  Speed = map(channels[0], 0, 1000, -255, 255);
  Speed = applyDeadzone(Speed, 10);

  if (channels[4] < 500) {
    Drive = 1;
  } else {
    Drive = 0;
  }
}

void Motor() {

  if (Drive == 1) {
    if (Speed != 0) {
      if (Speed > 0) {
        analogWrite(PinDriverF, Speed);
        analogWrite(PinDriverR, 0);
      }
      if (Speed < 0) {
        analogWrite(PinDriverF, 0);
        analogWrite(PinDriverR, -Speed);
      }

    } else {
      analogWrite(PinDriverF, 0);
      analogWrite(PinDriverR, 0);
    }

  } else {
    analogWrite(PinDriverF, 0);
    analogWrite(PinDriverR, 0);
  }
}
