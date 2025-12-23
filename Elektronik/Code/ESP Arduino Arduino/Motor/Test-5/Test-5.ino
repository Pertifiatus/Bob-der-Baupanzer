#include <SoftwareSerial.h>
#include <AccelStepper.h>

// --------------------------------------
// PIN DEFINITIONS (CNC SHIELD V3)
// --------------------------------------
#define ENABLE_PIN 8

// UART Pins
#define RX_PIN 10   // ESP32 TX → Arduino RX
#define TX_PIN 11   // ESP32 RX ← Arduino TX

// Stepper pins (CNC Shield standard)
AccelStepper stepperX(AccelStepper::DRIVER, 2, 5);
AccelStepper stepperY(AccelStepper::DRIVER, 3, 6);
AccelStepper stepperZ(AccelStepper::DRIVER, 4, 7);

int SPEED = 2000;

// --------------------------------------
// VARIABLES
// --------------------------------------
int c[4] = {0, 0, 0, 0};   // Channels 1–3
int EN = 0;

SoftwareSerial mySerial(RX_PIN, TX_PIN); // RX, TX

// Input buffer for non-blocking parsing
char inputBuffer[32];
byte bufferIndex = 0;

// --------------------------------------
// DEADZONE FUNCTION
// --------------------------------------
int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

// --------------------------------------
// SETUP
// --------------------------------------
void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  pinMode(ENABLE_PIN, OUTPUT);

  stepperX.setMaxSpeed(SPEED);
  stepperY.setMaxSpeed(SPEED);
  stepperZ.setMaxSpeed(SPEED);

  stepperX.setAcceleration(150);
  stepperY.setAcceleration(150);
  stepperZ.setAcceleration(150);

  Serial.println("Arduino gestartet (non-blocking UART)...");
}

// --------------------------------------
// NON-BLOCKING SERIAL PARSER
// --------------------------------------
void readSerialNonBlocking() {
  while (mySerial.available()) {
    char incoming = mySerial.read();

    if (incoming == '\n') {
      // Complete line received → parse it
      inputBuffer[bufferIndex] = 0;  // Null-terminate the string
      bufferIndex = 0;

      int t1, t2, t3;
      int parsed = sscanf(inputBuffer, "%d,%d,%d", &t1, &t2, &t3);

      if (parsed == 3) {
        c[1] = t1;
        c[2] = t2;
        c[3] = t3;

        // Deadzone + Mapping
        c[1] = applyDeadzone(c[1], 150);
        c[1] = map(c[1], -1000, 1000, -SPEED, SPEED);

        c[2] = applyDeadzone(c[2], 100);
        c[2] = map(c[2], -1000, 1000, -SPEED, SPEED);

        if(c[3]<0){
          c[3]=0;
        }
        else{
        c[3]=1;
        }
        
        // Enable logic


        // Debug
        Serial.print("Ch1=");
        Serial.print(c[1]);
        Serial.print(" Ch2=");
        Serial.print(c[2]);
        Serial.print(" Ch3=");
        Serial.print(c[3]);
 
        Serial.print(" EN=");
        Serial.println(EN);
      }
    }
    else {
      // Append character to buffer
      if (bufferIndex < sizeof(inputBuffer) - 1) {
        inputBuffer[bufferIndex++] = incoming;
      }
    }
  }
}

// --------------------------------------
// STEPPER UPDATE
// --------------------------------------
void updateSteppers() {
  digitalWrite(ENABLE_PIN, EN);

  stepperX.setSpeed(c[1]);
  stepperX.runSpeed();

  stepperY.setSpeed(c[2]);
  stepperY.runSpeed();

  // Z currently unused — add if needed
  // stepperZ.setSpeed(...);
  // stepperZ.runSpeed();
}

// --------------------------------------
// MAIN LOOP
// --------------------------------------
void loop() {
  readSerialNonBlocking();
  updateSteppers();
}
