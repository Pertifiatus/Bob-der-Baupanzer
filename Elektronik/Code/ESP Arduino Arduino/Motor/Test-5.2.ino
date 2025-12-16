#include <AccelStepper.h>

#define ENABLE_PIN 8 // Enable Pin für Stepper-Treiber
#define SPEED 1400   // Maximale Geschwindigkeit der Motoren

AccelStepper stepperX(AccelStepper::DRIVER, 2, 5);
AccelStepper stepperY(AccelStepper::DRIVER, 3, 6);
AccelStepper stepperZ(AccelStepper::DRIVER, 4, 7);

int c[4] = {0, 0, 0, 0}; // c[1]=X-Speed, c[2]=Y-Speed, c[3]=Control
int EN = 0;              // Enable/Disable Status (0=Enabled, 1=Disabled)

char inputBuffer[32];
byte bufferIndex = 0;

int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

void setup() {

  Serial.begin(19200); 

  pinMode(ENABLE_PIN, OUTPUT);

  stepperX.setMaxSpeed(SPEED); stepperY.setMaxSpeed(SPEED); stepperZ.setMaxSpeed(SPEED);
  stepperX.setAcceleration(150); stepperY.setAcceleration(150); stepperZ.setAcceleration(150);
}

void readSerial() {
  // Prüfen, ob Daten über HardwareSerial verfügbar sind
  while (Serial.available()) {
    char incoming = Serial.read();

    if (incoming == '\n') {
      // Stringende markieren und Puffer zurücksetzen
      inputBuffer[bufferIndex] = 0;
      bufferIndex = 0;

      int t1, t2, t3;
      int parsed = sscanf(inputBuffer, "%d,%d,%d", &t1, &t2, &t3);

      if (parsed == 3) {
        // --- X-Achse (c[1]) ---
        c[1] = applyDeadzone(t1, 80); 
        c[1] = map(c[1], -1000, 1000, -SPEED, SPEED);

        // --- Y-Achse (c[2]) ---
        c[2] = applyDeadzone(t2, 80); 
        c[2] = map(c[2], -1000, 1000, -SPEED, SPEED);

        // --- Z-Achse/Steuerung (c[3]) ---
        c[3] = t3;
        
        // Logik für Enable (EN) Pin basierend auf c[3] und c[1]/c[2]
        if (c[3] > 0) {
          c[3] = 0;
        } else {
          c[3] = 1;
          EN = 0; // Wenn c[3] = 1, wird EN auf 0 gesetzt (Motor AN)
        }

        if (c[3] == 0) {
          if (c[1] != 0 || c[2] != 0) {
            EN = 0; // Wenn c[1] oder c[2] aktiv sind, Motor AN
          } else {
            EN = 1; // Wenn c[3]=0 und c[1]/c[2] = 0, Motor AUS
          }
        }
      }
    } else {
      // Zeichen in den Puffer legen
      if (bufferIndex < sizeof(inputBuffer) - 1) inputBuffer[bufferIndex++] = incoming;
    }
  }
}

/**
 * Aktualisiert die Stepper-Geschwindigkeit und den Enable-Pin.
 */
void updateSteppers() {
  // Stepper-Treiber aktivieren/deaktivieren
  digitalWrite(ENABLE_PIN, EN); 
  
  // Setzt die Geschwindigkeit (basierend auf der geparsten Controllereingabe)
  stepperX.setSpeed(c[1]); 
  stepperY.setSpeed(c[2]); 
  
  // Führt den nächsten Schritt aus, falls nötig (zeitkritische Funktion!)
  stepperX.runSpeed();
  stepperY.runSpeed();
}

void loop() {
  // readSerial() wird so oft wie möglich aufgerufen, um Pufferüberläufe zu vermeiden.
  readSerial(); 
  
  // updateSteppers() muss SOFORT danach aufgerufen werden, um präzise Timing-Impulse
  // für die Motoren zu gewährleisten.
  updateSteppers();
}