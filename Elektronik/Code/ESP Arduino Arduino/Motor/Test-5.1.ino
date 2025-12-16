#include <SoftwareSerial.h>
#include <AccelStepper.h>

#define ENABLE_PIN 8
#define RX_PIN 10   // ESP32 TX → Arduino RX
#define TX_PIN 11   // optional
#define SPEED 2000

AccelStepper stepperX(AccelStepper::DRIVER, 2, 5);
AccelStepper stepperY(AccelStepper::DRIVER, 3, 6);
AccelStepper stepperZ(AccelStepper::DRIVER, 4, 7);

int c[4] = {0,0,0, 0};
int EN = 0;

SoftwareSerial mySerial(RX_PIN, TX_PIN); // RX, TX
char inputBuffer[32];
byte bufferIndex = 0;
unsigned long lastRead = 0;

int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(19200);

  pinMode(ENABLE_PIN, OUTPUT);

  stepperX.setMaxSpeed(SPEED); stepperY.setMaxSpeed(SPEED); stepperZ.setMaxSpeed(SPEED);
  stepperX.setAcceleration(150); stepperY.setAcceleration(150); stepperZ.setAcceleration(150);

  Serial.println("Arduino gestartet (SoftwareSerial 10/11)");
}

void readSerial() {
  while (mySerial.available()) {
    char incoming = mySerial.read();

    if (incoming == '\n') {
      inputBuffer[bufferIndex] = 0;
      bufferIndex = 0;

      int t1, t2, t3;
      int parsed = sscanf(inputBuffer, "%d,%d,%d", &t1,&t2,&t3);

      if (parsed == 3) {
        c[1] = applyDeadzone(t1,80); c[1] = map(c[1],-1000,1000,-SPEED,SPEED);
        c[2] = applyDeadzone(t2,50); c[2] = map(c[2],-1000,1000,-SPEED,SPEED);
        c[3]=t3;
        if(c[3]>0){
          c[3]=0;
        }
        else{
          c[3]=1;
          EN=0;
        }

        if(c[3] == 0){  if(c[1]!=0 || c[2]!= 0){
          EN=0;
        }
        else{
          EN=1;
        }
        }
        Serial.print("Ch1="); Serial.print(c[1]);
        Serial.print(" Ch2="); Serial.print(c[2]);
        Serial.print(" Ch3="); Serial.print(c[3]);
        Serial.print(" EN="); Serial.println(EN);
      }
    } else {
      if(bufferIndex < sizeof(inputBuffer)-1) inputBuffer[bufferIndex++] = incoming;
    }
  }
}

void updateSteppers() {
  digitalWrite(ENABLE_PIN, EN);
  stepperX.setSpeed(c[1]); stepperX.runSpeed();
  stepperY.setSpeed(c[2]); stepperY.runSpeed();
}

void loop() {
  if(millis()-lastRead>=50){
    lastRead=millis();
    readSerial();
  }
  updateSteppers();
}
