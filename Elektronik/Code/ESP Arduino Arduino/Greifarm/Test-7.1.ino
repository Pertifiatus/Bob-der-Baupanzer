#include <AccelStepper.h>

#define ENABLE_PIN 8
#define SPEED 1400      // Speed... I am lightning 
#define MAX_CHANNELS 7  // 7 Kanäle werden empfangen 
#define BUFFER_SIZE 48  // Buffer für 6 Kanäle
#define START_TOKEN '!' // Starttoken

int c[MAX_CHANNELS + 1] = {0, 0, 0, 0, 0, 0, 0, 0}; 
int EN = 1; // 0=Enabled, 1=Disabled
int DR =0; // Drive Status 

char inputBuffer[BUFFER_SIZE];
byte bufferIndex = 0;
static bool readingActive = false; //Zustand für das Lesen des Start-Tokens

AccelStepper stepperX(AccelStepper::DRIVER, 2, 5);
AccelStepper stepperY(AccelStepper::DRIVER, 3, 6);
AccelStepper stepperZ(AccelStepper::Driver, 4, 7);

int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

void setup() {

  Serial.begin(9600); 

  pinMode(ENABLE_PIN, OUTPUT);

  stepperX.setMaxSpeed(SPEED); stepperY.setMaxSpeed(SPEED); stepperZ.setMaxSpeed(SPEED);
}

void readSerial() {
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
        
        int t1, t2, t3, t4, t5, t6, t7;

        // Parsen von 6 Kanälen
        int parsed = sscanf(inputBuffer, "%d,%d,%d,%d,%d,%d,%d", 
                            &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        if (parsed == MAX_CHANNELS) {
          // --- X-Achse (c[1]) ---
          c[1] = applyDeadzone(t1, 80); 
          c[1] = map(c[1], -1000, 1000, -SPEED, SPEED);

          // --- Y-Achse (c[2]) ---
          c[2] = applyDeadzone(t2, 80); 
          c[2] = map(c[2], -1000, 1000, -SPEED, SPEED);
          c[5] = t7;
          c[6] = t6; 
          
          // Logik für Enable (EN) Pin basierend auf c[5] und c[1]/c[2] (wie Original-Code)
          if (c[5] > 0) {
            c[5] = 0;
          } else {
            c[5] = 1;
            EN = 0; // Wenn c[5] = 1, wird EN auf 0 gesetzt (Motor AN)
          }

          if (c[6] > 0) {
            DR = 1;
          } else {
            DR = 0;
          }
          if(DR==1){
            if (c[5] == 0) {
              if (c[1] != 0 || c[2] != 0) {
                EN = 0; // Wenn c[1] oder c[2] aktiv sind, Motor AN
              } else {
                EN = 1; // Wenn c[5]=0 und c[1]/c[2] = 0, Motor AUS
              }
            }
          }
          if(c[6] < 0 & c [5] == 0){
            EN=1;
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

void loop() {
  readSerial(); 
  updateSteppers();
}

void ChannelOUTPUT(){



}


void updateSteppers() {
  
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
}
