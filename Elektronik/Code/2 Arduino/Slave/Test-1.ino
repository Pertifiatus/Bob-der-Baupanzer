#include <AccelStepper.h>

// CNC Shield X-Achse Pins
const int X_STEP_PIN = 2;
const int X_DIR_PIN  = 5;
const int EN_PIN     = 8;

// Stepper-Objekt im DRIVER-Modus
AccelStepper stepperX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);

void setup() {
  Serial.begin(9600);          // Kommunikation mit Master
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);   // Treiber EIN
  stepperX.setMaxSpeed(2000);  // maximale Geschwindigkeit (Schritte/s)
  stepperX.setAcceleration(600);
}

void loop() {
  // Prüfen, ob Daten vom Master angekommen sind
  if (Serial.available()) {
    int potValue = Serial.parseInt(); // Wert 0-1023 lesen

    // Geschwindigkeit berechnen (0-1000 Schritte/s)
    float speed = map(potValue, 0, 1023, 0, 2000);

    // Geschwindigkeit setzen
    stepperX.setSpeed(speed);
  }

  // Stepper laufen lassen
  stepperX.runSpeed();
}
