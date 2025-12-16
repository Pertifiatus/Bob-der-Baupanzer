#include <AccelStepper.h>

// CNC Shield Pins
// X-Achse
const int X_STEP_PIN = 2;
const int X_DIR_PIN  = 5;

// Gemeinsamer Enable-Pin
const int EN_PIN = 8;

// Stepper-Objekte (DRIVER-Modus)
AccelStepper stepperX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
void setup() {
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); // Treiber EIN

  // Motorparameter
  stepperX.setMaxSpeed(1800);
  stepperX.setAcceleration(50);
}

void loop() {

  // Beispielbewegung:
  // Alle Achsen fahren gleichzeitig
  stepperX.moveTo(50000);
  // Solange eine Achse noch nicht am Ziel ist → alle laufen lassen
  while ( stepperX.distanceToGo() != 0 ) {
    stepperX.run();
  }

  delay(700);

  // Zurückfahren
  stepperX.moveTo(0);
  
  while ( stepperX.distanceToGo() != 0 ){
    stepperX.run();}

  delay(700);
}
