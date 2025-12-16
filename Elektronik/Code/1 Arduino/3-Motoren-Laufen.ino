#include <AccelStepper.h>

// CNC Shield Pins
// X-Achse
const int X_STEP_PIN = 2;
const int X_DIR_PIN  = 5;
//Trst
// Y-Achse
const int Y_STEP_PIN = 3;
const int Y_DIR_PIN  = 6;

// Z-Achse
const int Z_STEP_PIN = 4;
const int Z_DIR_PIN  = 7;

// Gemeinsamer Enable-Pin
const int EN_PIN = 8;

// Stepper-Objekte (DRIVER-Modus)
AccelStepper stepperX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper stepperY(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);
AccelStepper stepperZ(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);

void setup() {
  // Enable aktivieren
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); // Treiber EIN

  // Motorparameter
  stepperX.setMaxSpeed(1800);
  stepperX.setAcceleration(600);

  stepperY.setMaxSpeed(1800);
  stepperY.setAcceleration(600);

  stepperZ.setMaxSpeed(1800);
  stepperZ.setAcceleration(600);
}

void loop() {

  // Beispielbewegung:
  // Alle Achsen fahren gleichzeitig
  stepperX.moveTo(2500);
  stepperY.moveTo(2500);
  stepperZ.moveTo(2500);

  // Solange eine Achse noch nicht am Ziel ist → alle laufen lassen
  while ( stepperX.distanceToGo() != 0 ||
          stepperY.distanceToGo() != 0 ||
          stepperZ.distanceToGo() != 0 ) {

    stepperX.run();
    stepperY.run();
    stepperZ.run();
  }

  delay(700);

  // Zurückfahren
  stepperX.moveTo(0);
  stepperY.moveTo(0);
  stepperZ.moveTo(0);

  while ( stepperX.distanceToGo() != 0 ||
          stepperY.distanceToGo() != 0 ||
          stepperZ.distanceToGo() != 0 ) {

    stepperX.run();
    stepperY.run();
    stepperZ.run();
  }

  delay(700);
}
