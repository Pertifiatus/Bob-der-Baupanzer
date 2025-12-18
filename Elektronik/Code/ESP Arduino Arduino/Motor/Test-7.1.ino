#include <AccelStepper.h>

#define ENABLE_PIN 8
#define SPEED 1300    //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning 
#define MAX_CHANNELS 6
#define BUFFER_SIZE 48  // Neu: Sicherer Puffer für 6 Kanäle
#define START_TOKEN '!' // Starttoken

int c[MAX_CHANNELS + 1] = {0, 0, 0, 0, 0, 0, 0}; 
int EN = 1; // 0=Enabled, 1=Disabled
int Drive =0; // Drive Status 

char inputBuffer[BUFFER_SIZE];
byte bufferIndex = 0;
static bool readingActive = false; // Zustand für das Lesen des Start-Tokens

AccelStepper stepperX(AccelStepper::DRIVER, 2, 5);
AccelStepper stepperY(AccelStepper::DRIVER, 3, 6);
AccelStepper stepperZ(AccelStepper::DRIVER, 4, 7);

int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;}

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
void loop() {

  readSerial(); 
  
  updateSteppers();

  ChannelOUTPUT();

}

void ChannelOUTPUT(){
  
    // Logik für Enable (EN)
          if (c[3] > 0) {
            c[3] = 0;
          } else {
            c[3] = 1;
            EN = 0; // Wenn c[3] = 1, wird EN auf 0 gesetzt (Motor AN)
          }

          if (c[4] < 0) {
            Drive = 1;
          } else {
            Drive = 0;
          }
          if(Drive==1){  if (c[3] == 0) {
            if (c[1] != 0 || c[2] != 0) {
              EN = 0; // Wenn c[1] oder c[2] aktiv sind, Motor AN
            } else {
              EN = 1; // Wenn c[3]=0 und c[1]/c[2] = 0, Motor AUS
        }
      }
    }
  
}

void updateSteppers() {
  // Stepper-Treiber aktivieren/deaktivieren 
  if(Drive==1){
  // Setzt die Geschwindigkeit
  stepperX.setSpeed(c[1]); 
  stepperY.setSpeed(c[2]); 
  digitalWrite(ENABLE_PIN, EN);
  }
  if(Drive==0){
  stepperX.setSpeed(0); 
  stepperY.setSpeed(0); 
  digitalWrite(ENABLE_PIN, EN);
  }
  stepperX.runSpeed();
  stepperY.runSpeed();
  stepperZ.runSpeed();
}
