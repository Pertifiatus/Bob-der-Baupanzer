#include <AccelStepper.h>

#define ENABLE_PIN 8 // Enable Pin für Stepper-Treiber
#define SPEED 200   // Maximale Geschwindigkeit der Motoren
#define MAX_CHANNELS 6  // Neu: Erwartet 6 Kanäle vom ESP32
#define BUFFER_SIZE 48  // Neu: Sicherer Puffer für 6 Kanäle
#define START_TOKEN '!' // Token für robuste Synchronisation

// c[1]=X-Speed, c[2]=Y-Speed, c[3]=Control (Der dritte Wert des Pakets)
// Die anderen 3 gelesenen Kanäle werden in den Variablen c[4]-c[6] gespeichert, aber nicht verwendet.
int c[MAX_CHANNELS + 1] = {0, 0, 0, 0, 0, 0, 0}; 
int EN = 1; // Enable/Disable Status (0=Enabled, 1=Disabled)
int DR =0; // Drive Status 

char inputBuffer[BUFFER_SIZE];
byte bufferIndex = 0;
static bool readingActive = false; // Zustand für das Lesen des Start-Tokens

AccelStepper stepperX(AccelStepper::DRIVER, 2, 5);
AccelStepper stepperY(AccelStepper::DRIVER, 3, 6);
// AccelStepper stepperZ(AccelStepper::DRIVER, 4, 7); // Z-Achse entfernt

int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

void setup() {
  // Wichtig: Baudrate auf 9600 reduziert
  Serial.begin(9600); 

  pinMode(ENABLE_PIN, OUTPUT);

  stepperX.setMaxSpeed(SPEED); stepperY.setMaxSpeed(SPEED); 
}

// NEUE, ROBUSTE LESEFUNKTION
void readSerialRobust() {
  while (Serial.available()) {
    char incoming = Serial.read();

    if (incoming == START_TOKEN) {
      bufferIndex = 0;
      readingActive = true;
    } 
    else if (readingActive) {
      
      if (incoming == '\n') {
        // Stringende markieren
        inputBuffer[bufferIndex] = 0;
        
        int t1, t2, t3, t4, t5, t6;
        
        // Parsen von 6 Kanälen
        int parsed = sscanf(inputBuffer, "%d,%d,%d,%d,%d,%d", 
                            &t1, &t2, &t3, &t4, &t5, &t6);

        if (parsed == MAX_CHANNELS) {
          // --- X-Achse (c[1]) ---
          c[1] = applyDeadzone(t1, 80); 
          c[1] = map(c[1], -1000, 1000, -SPEED, SPEED);

          // --- Y-Achse (c[2]) ---
          c[2] = applyDeadzone(t2, 80); 
          c[2] = map(c[2], -1000, 1000, -SPEED, SPEED);

          // --- Control (c[3]): Dritter geparster Wert ---
          c[3] = t5;
          c[4] = t6;
          
          // Logik für Enable (EN) Pin basierend auf c[3] und c[1]/c[2] (wie Original-Code)
          if (c[3] > 0) {
            c[3] = 0;
          } else {
            c[3] = 1;
            EN = 0; // Wenn c[3] = 1, wird EN auf 0 gesetzt (Motor AN)
          }

          if (c[4] < 0) {
            DR = 1;
          } else {
            DR = 0;
          }
          if(DR==1){
          if (c[3] == 0) {
            if (c[1] != 0 || c[2] != 0) {
              EN = 0; // Wenn c[1] oder c[2] aktiv sind, Motor AN
            } else {
              EN = 1; // Wenn c[3]=0 und c[1]/c[2] = 0, Motor AUS
            }
          }
          }
        } 
        
        readingActive = false; 
      } else {
        // Zeichen in den Puffer legen (mit Overflow-Schutz)
        if (bufferIndex < BUFFER_SIZE - 1) {
          inputBuffer[bufferIndex++] = incoming;
        } else {
          readingActive = false; 
        }
      }
    }
  }
}

/**
 * Aktualisiert die Stepper-Geschwindigkeit und den Enable-Pin.
 */
void updateSteppers() {
  // Stepper-Treiber aktivieren/deaktivieren 
  if(DR==1){
  // Setzt die Geschwindigkeit
  stepperX.setSpeed(c[1]); 
  stepperY.setSpeed(c[2]); 
  digitalWrite(ENABLE_PIN, EN);
  }
  if(DR==0){
  stepperX.setSpeed(0); 
  stepperY.setSpeed(0); 
  digitalWrite(ENABLE_PIN, EN);
  }
  // Führt den nächsten Schritt aus (zeitkritische Funktion!)
  stepperX.runSpeed();
  stepperY.runSpeed();
  // stepperZ.runSpeed(); // Z-Achse entfernt
}

void loop() {
  // readSerialRobust() wird so oft wie möglich aufgerufen
  readSerialRobust(); 
  
  // updateSteppers() muss SOFORT danach aufgerufen werden
  updateSteppers();
}
