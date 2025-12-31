#include <AccelStepper.h>

#define ENABLE_PIN 8
#define SPEED 1300  //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning
int EN = 1;         // 0=Enabled, 1=Disabled
int Drive = 0;      // Drive Status
long speedX = 0;
long speedY = 0;

AccelStepper stepperX(AccelStepper::DRIVER, 2, 5);
AccelStepper stepperY(AccelStepper::DRIVER, 3, 6);
AccelStepper stepperZ(AccelStepper::DRIVER, 4, 7);
int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

const byte ST = 0xA1;
const int NUM_CHANNELS = 6;
int channels[NUM_CHANNELS];
String input = "";

unsigned long lastPacketTime = 0;
const unsigned long TimeoutFailsafe = 500;
bool failsafeActive = false;
bool MeineNachricht = false;

void setup() {
  pinMode(ENABLE_PIN, OUTPUT);
  stepperX.setMaxSpeed(SPEED);
  stepperY.setMaxSpeed(SPEED);
  stepperZ.setMaxSpeed(SPEED);


  Serial.begin(115200);
  input.reserve(50);

  activateFailsafe();
  lastPacketTime = millis();
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
  int n = sscanf(input.c_str(), "%d,%d,%d,%d,%d,%d",
                 &channels[0], &channels[1], &channels[2],
                 &channels[3], &channels[4], &channels[5]);

  if (n == NUM_CHANNELS) {
    // Wenn Daten korrekt ankommen -> Failsafe beenden
    if (failsafeActive) {
      failsafeActive = false;
    }
    lastPacketTime = millis();  // Zeitstempel für Timeout-Timer erneuern
  }
}

void activateFailsafe() {
  // Sicherheitswerte definieren (z.B. alles auf 0)
  channels[0] = 0;
  channels[1] = 0;
  channels[2] = 0;
  channels[3] = 0;
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
  speedX = map(channels[0], 0, 1000, -SPEED, SPEED);
  speedX = applyDeadzone(speedX, 200);

  speedY = map(channels[1], 0, 1000, -SPEED, SPEED);
  speedY = applyDeadzone(speedY, 100);

  // Logik für Enable (EN)
  if (channels[5] < 500) {
    EN = 0;  // Motor AN
  } else {
    EN = 1;  // Motor AUS
  }

  if (channels[4] > 500) {
    Drive = 1;
  } else {
    Drive = 0;
  }
  if (Drive == 1) {  // Arming in Drive Action
    if (speedX != 0 || speedY != 0) {
      EN = 0;  // Wenn die Sticks nicht center sind, Motor AN
    } else {
      EN = 1;  // Wenn die Sticks Center sind, Motor AUS
    }
  }
  if (channels[5] > 0 && channels[4] == 0) {
    EN = 1;
  }
}

void Stepper() {

  if (Drive == 1) {
    stepperX.setSpeed(speedX);
    stepperY.setSpeed(speedY);
    digitalWrite(ENABLE_PIN, EN);
  }
  if (Drive == 0) {
    stepperX.setSpeed(0);
    stepperY.setSpeed(0);
    digitalWrite(ENABLE_PIN, EN);
  }
  stepperX.runSpeed();
  stepperY.runSpeed();
  stepperZ.runSpeed();
}