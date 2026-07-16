#include <AccelStepper.h>

#define ENABLE_PIN 8
int EN = 1;    // 0=Enabled, 1=Disabled
int Grab = 0;  // Grab Status
long speedX = 0;
long speedY = 0;
long speedZ = 0;
long speedA = 0;

const long MaxSpeedX = 200;  //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning
const long MaxSpeedY = 200;
const long MaxSpeedZ = 50;
const long MaxSpeedA = 1500;

const long LIMIT_MIN_Drehachse1X = -5000;
const long LIMIT_MAX_Drehachse1X = 5000;
const long BREMSZONE_Drehachse1X = 500;

const long LIMIT_MIN_Gelenk2Y = -5000;
const long LIMIT_MAX_Gelenk2Y = 5000;
const long BREMSZONE_Gelenk2Y = 500;

const long LIMIT_MIN_Gelenk3Z = -5000;
const long LIMIT_MAX_Gelenk3Z = 5000;
const long BREMSZONE_Gelenk3Z = 500;

const long LIMIT_MIN_Drehachse4A = -5000;
const long LIMIT_MAX_Drehachse4A = 5000;
const long BREMSZONE_Drehachse4A = 500;

AccelStepper Drehachse1X(AccelStepper::DRIVER, 2, 5);
AccelStepper Gelenk2Y(AccelStepper::DRIVER, 3, 6);
AccelStepper Gelenk3Z(AccelStepper::DRIVER, 4, 7);
AccelStepper Drehachse4A(AccelStepper::DRIVER, 12, 13);
int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

const byte ST = 0xA2;
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
  pinMode(ENABLE_PIN, OUTPUT);
  Drehachse1X.setMaxSpeed(MaxSpeedX);
  Drehachse1X.setAcceleration(2000);

  Gelenk2Y.setMaxSpeed(MaxSpeedY);
  Gelenk2Y.setAcceleration(2000);

  Gelenk3Z.setMaxSpeed(MaxSpeedZ);
  Gelenk3Z.setAcceleration(2000);

  Drehachse4A.setMaxSpeed(MaxSpeedA);
  Drehachse4A.setAcceleration(2000);

  Serial.begin(250000);

  activateFailsafe();
  lastPacketTime = millis();

  Calibration();
}

void Calibration() {
  Serial.print("Calibration-Initiated");
  while (!Calibrated) {
    ReadSerial();
    static unsigned long lastDebugTime = 0;

    if (millis() - lastDebugTime > 100) {  // Nur alle 100ms Text ausgeben (schont die CPU)
      lastDebugTime = millis();
      printDebugInfo();
    }

    if (channels[4] < 500 && channels[7] > 500) {
      Drehachse4A.setCurrentPosition(0);
      Gelenk3Z.setCurrentPosition(0);
      Gelenk2Y.setCurrentPosition(0);
      Drehachse1X.setCurrentPosition(0);
      Calibrated = true;
      digitalWrite(ENABLE_PIN, 0);
      vibrateMotor(Gelenk2Y, 10, 2);
      vibrateMotor(Gelenk3Z, 10, 2);
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
  Stepper();

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
  lastPacketTime = millis();  // Zeitstempel für Timeout-Timer erneuern
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
  speedX = map(channels[0], 0, 1000, -MaxSpeedX, MaxSpeedX);
  speedX = applyDeadzone(speedX, MaxSpeedX * 0.15);


  speedY = map(channels[1], 0, 1000, -MaxSpeedY, MaxSpeedY);
  speedY = applyDeadzone(speedY, MaxSpeedY * 0.1);

  speedZ = map(channels[2], 0, 1000, -MaxSpeedZ, MaxSpeedZ);
  speedZ = applyDeadzone(speedZ, MaxSpeedZ * 0.1);


  speedA = map(channels[3], 0, 1000, -MaxSpeedA, MaxSpeedA);
  speedA = applyDeadzone(speedA, MaxSpeedA * 0.1);

  // Logik für Enable (EN)
  if (channels[5] < 500) {
    EN = 0;  // Motor AN
  } else {
    EN = 1;  // Motor AUS
  }

  if (channels[7] < 500 && channels[4] > 500) {
    Grab = 1;
  } else {
    Grab = 0;
  }

  if (Grab == 1) {  // Arming in Grab Action
    if (speedX != 0 || speedY != 0 || speedZ != 0 || speedA != 0) {
      EN = 0;  // Wenn die Sticks nicht center sind, Motor AN
    }
  }
  if (channels[6] > 500 && channels[4] < 500) {  // Automatischer Override
    EN = 1;
  }
}

void Stepper() {
  if (Grab == 1) {
    //Bremszone X
    long currentPosX = Drehachse1X.currentPosition();
    long finalSpeedX = speedX;

    if (speedX > 0 && (LIMIT_MAX_Drehachse1X - currentPosX) < BREMSZONE_Drehachse1X) {
      // Bremse Richtung Plus
      finalSpeedX = speedX * (LIMIT_MAX_Drehachse1X - currentPosX) / BREMSZONE_Drehachse1X;

    } else if (speedX < 0 && (currentPosX - LIMIT_MIN_Drehachse1X) < BREMSZONE_Drehachse1X) {
      // Bremse Richtung Minus
      finalSpeedX = speedX * (currentPosX - LIMIT_MIN_Drehachse1X) / BREMSZONE_Drehachse1X;
    }

    // Not-Stopp Sicherheitscheck
    if (currentPosX >= LIMIT_MAX_Drehachse1X && speedX > 0) finalSpeedX = 0;
    if (currentPosX <= LIMIT_MIN_Drehachse1X && speedX < 0) finalSpeedX = 0;

    //Bremszone Y
    long currentPosY = Gelenk2Y.currentPosition();
    long finalSpeedY = speedY;

    if (speedY > 0 && (LIMIT_MAX_Gelenk2Y - currentPosY) < BREMSZONE_Gelenk2Y) {
      // Bremse Richtung Plus
      finalSpeedY = speedY * (LIMIT_MAX_Gelenk2Y - currentPosY) / BREMSZONE_Gelenk2Y;

    } else if (speedY < 0 && (currentPosY - LIMIT_MIN_Gelenk2Y) < BREMSZONE_Gelenk2Y) {
      // Bremse Richtung Minus
      finalSpeedY = speedY * (currentPosY - LIMIT_MIN_Gelenk2Y) / BREMSZONE_Gelenk2Y;
    }

    // Not-Stopp Sicherheitscheck
    if (currentPosY >= LIMIT_MAX_Gelenk2Y && speedY > 0) finalSpeedY = 0;
    if (currentPosY <= LIMIT_MIN_Gelenk2Y && speedY < 0) finalSpeedY = 0;

    //Bremszone Z
    long currentPosZ = Gelenk3Z.currentPosition();
    long finalSpeedZ = speedZ;

    if (speedZ > 0 && (LIMIT_MAX_Gelenk3Z - currentPosZ) < BREMSZONE_Gelenk3Z) {
      // Bremse Richtung Plus
      finalSpeedZ = speedZ * (LIMIT_MAX_Gelenk3Z - currentPosZ) / BREMSZONE_Gelenk3Z;

    } else if (speedZ < 0 && (currentPosZ - LIMIT_MIN_Gelenk3Z) < BREMSZONE_Gelenk3Z) {
      // Bremse Richtung Minus
      finalSpeedZ = speedZ * (currentPosZ - LIMIT_MIN_Gelenk3Z) / BREMSZONE_Gelenk3Z;
    }

    // Not-Stopp Sicherheitscheck
    if (currentPosZ >= LIMIT_MAX_Gelenk3Z && speedZ > 0) finalSpeedZ = 0;
    if (currentPosZ <= LIMIT_MIN_Gelenk3Z && speedZ < 0) finalSpeedZ = 0;

    //Bremszone A
    long currentPosA = Drehachse4A.currentPosition();
    long finalSpeedA = speedA;

    if (speedA > 0 && (LIMIT_MAX_Drehachse4A - currentPosA) < BREMSZONE_Drehachse4A) {
      // Bremse Richtung Plus
      finalSpeedA = speedA * (LIMIT_MAX_Drehachse4A - currentPosA) / BREMSZONE_Drehachse4A;

    } else if (speedA < 0 && (currentPosA - LIMIT_MIN_Drehachse4A) < BREMSZONE_Drehachse4A) {
      // Bremse Richtung Minus
      finalSpeedA = speedA * (currentPosA - LIMIT_MIN_Drehachse4A) / BREMSZONE_Drehachse4A;
    }

    // Not-Stopp Sicherheitscheck
    if (currentPosA >= LIMIT_MAX_Drehachse4A && speedA > 0) finalSpeedA = 0;
    if (currentPosA <= LIMIT_MIN_Drehachse4A && speedA < 0) finalSpeedA = 0;

    Drehachse1X.setSpeed(finalSpeedX);
    Gelenk2Y.setSpeed(finalSpeedY);
    Gelenk3Z.setSpeed(finalSpeedZ);
    Drehachse4A.setSpeed(finalSpeedA);

  } else {
    Drehachse1X.setSpeed(0);
    Gelenk2Y.setSpeed(0);
    Gelenk3Z.setSpeed(0);
    Drehachse4A.setSpeed(0);
  }

  digitalWrite(ENABLE_PIN, EN);
  Drehachse1X.runSpeed();
  Gelenk2Y.runSpeed();
  Gelenk3Z.runSpeed();
  Drehachse4A.runSpeed();
}

void StepperCalib() {

  if (Grab == 1) {
    Drehachse1X.setSpeed(speedX);
    Gelenk2Y.setSpeed(speedY);
    Gelenk3Z.setSpeed(speedZ);
    Drehachse4A.setSpeed(speedA);
  } else {
    Drehachse1X.setSpeed(0);
    Gelenk2Y.setSpeed(0);
    Gelenk3Z.setSpeed(0);
    Drehachse4A.setSpeed(0);
  }
  digitalWrite(ENABLE_PIN, EN);
  Drehachse1X.runSpeed();
  Gelenk2Y.runSpeed();
  Gelenk3Z.runSpeed();
  Drehachse4A.runSpeed();
}

void vibrateMotor(AccelStepper &stepper, int intensity, int pulses) {
  // intensity = Schritte pro Richtung (5-20)
  // pulses = Anzahl Vibrationen (2-5)

  long originalPos = stepper.currentPosition();

  for (int i = 0; i < pulses; i++) {
    stepper.moveTo(originalPos + intensity);
    while (stepper.distanceToGo() != 0) stepper.run();

    stepper.moveTo(originalPos - intensity);
    while (stepper.distanceToGo() != 0) stepper.run();
  }

  // Zurück zur Ausgangsposition
  stepper.moveTo(originalPos);
  while (stepper.distanceToGo() != 0) stepper.run();
}
