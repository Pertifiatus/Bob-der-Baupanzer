#include <AccelStepper.h>

#define ENABLE_PIN 8
int EN = 1;         // 0=Enabled, 1=Disabled
int Grab = 0;       // Grab Status
long speedX = 0;
long speedY = 0;

const long MaxSpeedX = 1000;  //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning
const long MaxSpeedY = 500;

const long LIMIT_MIN_Gelenk5X = -5000;
const long LIMIT_MAX_Gelenk5X = 5000;
const long BREMSZONE_Gelenk5X = 500;

const long LIMIT_MIN_Greifer6Y = -5000;
const long LIMIT_MAX_Greifer6Y = 5000;
const long BREMSZONE_Greifer6Y = 500;

AccelStepper Gelenk5X(AccelStepper::DRIVER, 2, 5);
AccelStepper Greifer6Y(AccelStepper::DRIVER, 3, 6);

int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

const byte ST = 0xA3;
const int NUM_CHANNELS = 8;
int channels[NUM_CHANNELS];
String input = "";

unsigned long lastPacketTime = 0;
const unsigned long TimeoutFailsafe = 500;
bool failsafeActive = false;
bool MeineNachricht = false;
bool Calibrated = false;


void setup() {
  pinMode(4, OUTPUT); //Relais Pin
  digitalWrite(4,0);
  pinMode(ENABLE_PIN, OUTPUT);
  Gelenk5X.setMaxSpeed(MaxSpeedX);
  Gelenk5X.setAcceleration(2000);
  Greifer6Y.setMaxSpeed(MaxSpeedX);
  Greifer6Y.setAcceleration(2000);

  Serial.begin(115200);
  input.reserve(50);

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
      vibrateMotor(Gelenk5X, 10, 2);
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
    lastPacketTime = millis();  //  Timeout-Timer erneuern
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
  speedX = map(channels[0], 0, 1000, -MaxSpeedX, MaxSpeedX);
  speedX = applyDeadzone(speedX, MaxSpeedX * 0.15);

  speedY = map(channels[1], 0, 1000, -MaxSpeedY, MaxSpeedY);
  speedY = applyDeadzone(speedY, MaxSpeedY * 0.1);

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
    //Bremszone X
      long currentPosX = Gelenk5X.currentPosition();
      long finalSpeedX = speedX;

      if (speedX > 0 && (LIMIT_MAX_Gelenk5X - currentPosX) < BREMSZONE_Gelenk5X) {
        // Bremse Richtung Plus
        finalSpeedX = speedX * (LIMIT_MAX_Gelenk5X - currentPosX) / BREMSZONE_Gelenk5X;

      } else if (speedX < 0 && (currentPosX - LIMIT_MIN_Gelenk5X) < BREMSZONE_Gelenk5X) {
        // Bremse Richtung Minus
        finalSpeedX = speedX * (currentPosX - LIMIT_MIN_Gelenk5X) / BREMSZONE_Gelenk5X;
      }

      // Not-Stopp Sicherheitscheck
      if (currentPosX >= LIMIT_MAX_Gelenk5X && speedX > 0) finalSpeedX = 0;
      if (currentPosX <= LIMIT_MIN_Gelenk5X && speedX < 0) finalSpeedX = 0;
  
    //Bremszone Y
      long currentPosY = Greifer6Y.currentPosition();
      long finalSpeedY = speedY;

      if (speedY > 0 && (LIMIT_MAX_Greifer6Y - currentPosY) < BREMSZONE_Greifer6Y) {
        // Bremse Richtung Plus
        finalSpeedY = speedY * (LIMIT_MAX_Greifer6Y - currentPosY) / BREMSZONE_Greifer6Y;

      } else if (speedY < 0 && (currentPosY - LIMIT_MIN_Greifer6Y) < BREMSZONE_Greifer6Y) {
        // Bremse Richtung Minus
        finalSpeedY = speedY * (currentPosY - LIMIT_MIN_Greifer6Y) / BREMSZONE_Greifer6Y;
      }

      // Not-Stopp Sicherheitscheck
      if (currentPosY >= LIMIT_MAX_Greifer6Y && speedY > 0) finalSpeedY = 0;
      if (currentPosY <= LIMIT_MIN_Greifer6Y && speedY < 0) finalSpeedY = 0;
  
    Gelenk5X.setSpeed(finalSpeedX);
    Greifer6Y.setSpeed(finalSpeedY);

  } else {
    Gelenk5X.setSpeed(0);
    Greifer6Y.setSpeed(0);
  }
  digitalWrite(ENABLE_PIN, EN);
  Gelenk5X.runSpeed();
  Greifer6Y.runSpeed();
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
