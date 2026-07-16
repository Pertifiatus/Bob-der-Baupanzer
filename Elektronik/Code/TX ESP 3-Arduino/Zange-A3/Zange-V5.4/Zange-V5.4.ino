#include <AccelStepper.h>

#define ENABLE_PIN 8
int EN = 1;    // 0=Enabled, 1=Disabled
int Grab = 0;  // Grab Status
long speedX = 0;
long speedY = 0;
long speedKipp = 0;
long speedGreif = 0;

const long MaxSpeedKipp = 600;    //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning
const long MaxSpeedGreif = 7000;  //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning
const long MaxSpeedMotor = MaxSpeedKipp + MaxSpeedGreif;  // Motor sieht im schlechtesten Fall die Summe beider Anteile

// Beide Motoren treiben zusammen ein Differenzial an: jeder Motorschritt ist eine
// Mischung aus Kipp- und Greif-Bewegung (X = Kipp+Greif, Y = -Kipp+Greif). Deshalb
// werden die Limits hier auf die zurueckgerechneten logischen Achsen angewendet und
// nicht mehr auf die rohen Motor-Positionen.
const long LIMIT_MIN_Kipp = -900;
const long LIMIT_MAX_Kipp = 900;
const long BREMSZONE_Kipp = 200;

const long LIMIT_MIN_Greif = -35000;
const long LIMIT_MAX_Greif = 10000;
const long BREMSZONE_Greif = 1000;

// Geschwindigkeiten fuers Homing (langsamer als der normale Betrieb) sowie die
// Toleranz, ab der eine Achse als "daheim" (Position 0) gilt.
const long HomingSpeedKipp = 300;
const long HomingSpeedGreif = 2000;
const long HomingToleranz = 5;

AccelStepper Gelenk5X(AccelStepper::DRIVER, 2, 5);
AccelStepper Greifer6Y(AccelStepper::DRIVER, 3, 6);

int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

const byte ST = 0xA3;
const int NUM_CHANNELS = 8;
int channels[NUM_CHANNELS];

unsigned long lastPacketTime = 0;
const unsigned long TimeoutFailsafe = 500;
bool failsafeActive = false;
bool Calibrated = false;

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
  pinMode(4, OUTPUT);  //Relais Pin
  digitalWrite(4, 0);
  pinMode(ENABLE_PIN, OUTPUT);
  Gelenk5X.setMaxSpeed(MaxSpeedMotor);
  Gelenk5X.setAcceleration(2000);
  Greifer6Y.setMaxSpeed(MaxSpeedMotor);
  Greifer6Y.setAcceleration(2000);

  Serial.begin(250000);

  activateFailsafe();
  lastPacketTime = millis();

  delay(5000);
  digitalWrite(4, 1);
  Serial.println("Relais AN");

  Calibration();
}

void Calibration() {
  Serial.print("Calibration-Initiated");
  while (!Calibrated) {
    ReadSerial();
    static unsigned long lastDebugTime = 0;

    if (millis() - lastDebugTime > 100) {  // Nur alle 100ms Text ausgeben
      lastDebugTime = millis();
      printDebugInfo();
    }

    if (channels[4] < 500 && channels[7] > 500) {
      Gelenk5X.setCurrentPosition(0);
      Greifer6Y.setCurrentPosition(0);
      Calibrated = true;
      digitalWrite(ENABLE_PIN, 0);
      vibrateMotor(Gelenk5X, 20, 2);
      digitalWrite(ENABLE_PIN, 1);
      Serial.print("Calib-SAVE");
    } else {
      Channellogic();
      StepperCalib();
    }
  }
}

void loop() {
  ReadSerial();
  Channellogic();
  if (!Homing()) {
    Stepper();
  }
  /*
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 100) {  // Nur alle 100ms Text ausgeben (schont die CPU)
    lastDebugTime = millis();
    printDebugInfo();
  }
  */
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
  lastPacketTime = millis();  //  Timeout-Timer erneuern
}

void activateFailsafe() {

  channels[0] = 500;
  channels[1] = 500;
  channels[2] = 500;
  channels[3] = 500;
  channels[4] = 0;
  channels[5] = 1000;
  channels[6] = 1000;
  channels[7] = 0;
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
  Serial.print(Grab);
  Serial.println();
}

void Channellogic() {
  long kippAnteil = map(channels[1], 0, 1000, MaxSpeedKipp, -MaxSpeedKipp);
  long greifAnteil = map(channels[2], 0, 1000, MaxSpeedGreif, -MaxSpeedGreif);

  kippAnteil = applyDeadzone(kippAnteil, MaxSpeedKipp * 0.1);
  greifAnteil = applyDeadzone(greifAnteil, MaxSpeedGreif * 0.1);

  speedKipp = kippAnteil;
  speedGreif = greifAnteil;

  speedX = kippAnteil + greifAnteil;
  speedY = -kippAnteil + greifAnteil;

  // Logik für Enable (EN)
  if (channels[6] < 500) {  //Falls Channel 6 kleiner 500 ist
    EN = 0;                 // Motor AN
  } else {
    EN = 1;  // Motor AUS
  }


  if (channels[7] > 500 && channels[4] > 500) {  //Falls Channel 7 großer 500, und Channel 4 großer 500. (Beide Switches gedrückt)
    Grab = 1;
  } else {
    Grab = 0;
  }

  if (Grab == 1) {  // Arming in Grab Action
    if (speedX != 0 || speedY != 0) {
      EN = 0;  // Wenn die Sticks nicht center sind, Motor AN
    }
  }

  if (channels[6] > 500 && channels[4] < 500) {  // Override, falls Channel gewechselt wird, während sich die Motoren drehen
    EN = 1;
  }
}

void Stepper() {
  if (Grab == 1) {
    long currentPosX = Gelenk5X.currentPosition();
    long currentPosY = Greifer6Y.currentPosition();

    // Differenzial-Kinematik umkehren (X = Kipp+Greif, Y = -Kipp+Greif), um die
    // tatsaechliche Position der beiden logischen Achsen zu bestimmen.
    long kippPos = (currentPosX - currentPosY) / 2;
    long greifPos = (currentPosX + currentPosY) / 2;

    long finalKipp = speedKipp;
    long finalGreif = speedGreif;

    //Bremszone Kipp
    if (finalKipp > 0 && (LIMIT_MAX_Kipp - kippPos) < BREMSZONE_Kipp) {
      // Bremse Richtung Plus
      finalKipp = finalKipp * (LIMIT_MAX_Kipp - kippPos) / BREMSZONE_Kipp;

    } else if (finalKipp < 0 && (kippPos - LIMIT_MIN_Kipp) < BREMSZONE_Kipp) {
      // Bremse Richtung Minus
      finalKipp = finalKipp * (kippPos - LIMIT_MIN_Kipp) / BREMSZONE_Kipp;
    }

    // Not-Stopp Sicherheitscheck
    if (kippPos >= LIMIT_MAX_Kipp && finalKipp > 0) finalKipp = 0;
    if (kippPos <= LIMIT_MIN_Kipp && finalKipp < 0) finalKipp = 0;

    //Bremszone Greif
    if (finalGreif > 0 && (LIMIT_MAX_Greif - greifPos) < BREMSZONE_Greif) {
      // Bremse Richtung Plus
      finalGreif = finalGreif * (LIMIT_MAX_Greif - greifPos) / BREMSZONE_Greif;

    } else if (finalGreif < 0 && (greifPos - LIMIT_MIN_Greif) < BREMSZONE_Greif) {
      // Bremse Richtung Minus
      finalGreif = finalGreif * (greifPos - LIMIT_MIN_Greif) / BREMSZONE_Greif;
    }

    // Not-Stopp Sicherheitscheck
    if (greifPos >= LIMIT_MAX_Greif && finalGreif > 0) finalGreif = 0;
    if (greifPos <= LIMIT_MIN_Greif && finalGreif < 0) finalGreif = 0;

    // Zurueck in die tatsaechlichen Motor-Geschwindigkeiten wandeln
    Gelenk5X.setSpeed(finalKipp + finalGreif);
    Greifer6Y.setSpeed(-finalKipp + finalGreif);

  } else {
    Gelenk5X.setSpeed(0);
    Greifer6Y.setSpeed(0);
  }
  digitalWrite(ENABLE_PIN, EN);
  Gelenk5X.runSpeed();
  Greifer6Y.runSpeed();
}

// Faehrt beide Achsen mit fester Homing-Geschwindigkeit Richtung 0 (Ursprungslage).
// Gibt true zurueck, waehrend Homing aktiv ist - Stepper() darf dann in diesem
// Zyklus nicht zusaetzlich auf die Motoren schreiben.
bool Homing() {
  if (!(channels[7] > 500 && channels[4] < 500)) return false;

  long currentPosX = Gelenk5X.currentPosition();
  long currentPosY = Greifer6Y.currentPosition();

  // Differenzial-Kinematik umkehren, um die logische Achsposition zu bestimmen.
  long kippPos = (currentPosX - currentPosY) / 2;
  long greifPos = (currentPosX + currentPosY) / 2;

  long homeKipp = 0;
  if (kippPos > HomingToleranz) {
    homeKipp = -HomingSpeedKipp;
    if (kippPos < BREMSZONE_Kipp) homeKipp = homeKipp * kippPos / BREMSZONE_Kipp;
  } else if (kippPos < -HomingToleranz) {
    homeKipp = HomingSpeedKipp;
    if (-kippPos < BREMSZONE_Kipp) homeKipp = homeKipp * -kippPos / BREMSZONE_Kipp;
  }

  long homeGreif = 0;
  if (greifPos > HomingToleranz) {
    homeGreif = -HomingSpeedGreif;
    if (greifPos < BREMSZONE_Greif) homeGreif = homeGreif * greifPos / BREMSZONE_Greif;
  } else if (greifPos < -HomingToleranz) {
    homeGreif = HomingSpeedGreif;
    if (-greifPos < BREMSZONE_Greif) homeGreif = homeGreif * -greifPos / BREMSZONE_Greif;
  }

  digitalWrite(ENABLE_PIN, 0);  // Motoren fuers Homing aktivieren, unabhaengig von EN
  Gelenk5X.setSpeed(homeKipp + homeGreif);
  Greifer6Y.setSpeed(-homeKipp + homeGreif);
  Gelenk5X.runSpeed();
  Greifer6Y.runSpeed();

  return true;
}


void StepperCalib() {

  if (Grab == 1) {
    Gelenk5X.setSpeed(speedX);
    Greifer6Y.setSpeed(speedY);

  } else {
    Gelenk5X.setSpeed(0);
    Greifer6Y.setSpeed(0);
  }
  digitalWrite(ENABLE_PIN, EN);
  Gelenk5X.runSpeed();
  Greifer6Y.runSpeed();
}

void vibrateMotor(AccelStepper &stepper, int intensity, int pulses) {

  long originalPos = stepper.currentPosition();

  for (int i = 0; i < pulses; i++) {
    stepper.moveTo(originalPos + intensity);
    while (stepper.distanceToGo() != 0) stepper.run();
    stepper.moveTo(originalPos - intensity);
    while (stepper.distanceToGo() != 0) stepper.run();
  }
  stepper.moveTo(originalPos);
  while (stepper.distanceToGo() != 0) stepper.run();
}
