#include <AccelStepper.h>

#define ENABLE_PIN 8
#define SPEED 1300  //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning
int EN = 1;         // 0=Enabled, 1=Disabled
int Grab = 0;       // Grab Status
long speedX = 0;
long speedY = 0;
long speedZ = 0;
long speedA = 0;

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
String input = "";

unsigned long lastPacketTime = 0;
const unsigned long TimeoutFailsafe = 500;
bool failsafeActive = false;
bool MeineNachricht = false;
bool Calibrated = false;

void setup() {
  pinMode(ENABLE_PIN, OUTPUT);
  Drehachse1X.setMaxSpeed(SPEED);
  Drehachse1X.setAcceleration(2000);

  Gelenk2Y.setMaxSpeed(SPEED);
  Gelenk2Y.setAcceleration(2000);

  Gelenk3Z.setMaxSpeed(SPEED);
  Gelenk3Z.setAcceleration(2000);

  Drehachse4A.setMaxSpeed(SPEED);
  Drehachse4A.setAcceleration(2000);

  Serial.begin(115200);
  input.reserve(50);

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
      vibrateMotor(Drehachse1X, 10, 2);
      digitalWrite(ENABLE_PIN, 1);
      Serial.print("Calib-SAVE");
    } else {
      Channellogic();
      Stepper();
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

    if (incomingByte == ST) {  //Falls Startbyte erkannt wird: input zurücksetzen
      input = "";
      MeineNachricht = true;  //Setze die MeineNachricht Variable auf True
    } else if (incomingByte > 0x80) {
      MeineNachricht = false;
    }

    else if (MeineNachricht) {
      if (incomingByte == '\n') {
        parseBuffer();
        MeineNachricht = false;
        break;
      } else {
        input += (char)incomingByte;
      }
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

void parseBuffer() {
  int n = sscanf(input.c_str(), "%d,%d,%d,%d,%d,%d,%d,%d",
                 &channels[0], &channels[1], &channels[2],
                 &channels[3], &channels[4], &channels[5],
                 &channels[6], &channels[7]);

  if (n == NUM_CHANNELS) {
    // Wenn Daten korrekt ankommen -> Failsafe beenden
    if (failsafeActive) {
      failsafeActive = false;
    }
    lastPacketTime = millis();  // Zeitstempel für Timeout-Timer erneuern
  }
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
  speedX = map(channels[0], 0, 1000, -SPEED, SPEED);
  speedX = applyDeadzone(speedX, 200);


  speedY = map(channels[1], 0, 1000, -SPEED, SPEED);
  speedY = applyDeadzone(speedY, 100);

  speedZ = map(channels[2], 0, 1000, -SPEED, SPEED);
  speedZ = applyDeadzone(speedZ, 200);


  speedA = map(channels[3], 0, 1000, -SPEED, SPEED);
  speedA = applyDeadzone(speedA, 100);

  // Logik für Enable (EN)
  if (channels[6] < 500) {
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
  if (channels[6] > 500 && channels[4] > 500) {  // Automatischer Override
    EN = 1;
  }
}

void Stepper() {
  if (Grab == 1) {
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

    Drehachse1X.setSpeed(finalSpeedX);
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