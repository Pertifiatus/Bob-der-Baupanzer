#include <SoftwareSerial.h>
#include <AccelStepper.h>

#define ENABLE_PIN 8
#define RX_PIN 0  // RX vom ESP32 TX
#define TX_PIN 1  // TX zum ESP32 RX (optional)

AccelStepper stepperX(AccelStepper::DRIVER, 2, 5);
AccelStepper stepperY(AccelStepper::DRIVER, 3, 6);
AccelStepper stepperZ(AccelStepper::DRIVER, 4, 7);
int SPEED = 1300;

long Channel1 = 0;
long Channel2 = 0;
long Channel3 = 0;
long Channel4 = 0;

SoftwareSerial mySerial(RX_PIN, TX_PIN); // RX, TX


int applyDeadzone(int value, int dz) {
    if (abs(value) < dz) return 0;
    return value;
}

void setup() {
  stepperX.setMaxSpeed(SPEED);
  stepperY.setMaxSpeed(SPEED);
  stepperZ.setMaxSpeed(SPEED);
  stepperX.setAcceleration(150);
  stepperY.setAcceleration(150);
  stepperZ.setAcceleration(150);
  pinMode(ENABLE_PIN, OUTPUT);

  Serial.begin(115200);
  mySerial.begin(9600);

  Serial.println("Arduino Debug gestartet...");
}

void loop() {

  // Wir lesen eine komplette Zeile (z. B. "512,600,1000,0")
  if (mySerial.available()) {
    String line = mySerial.readStringUntil('\n');

    // Mini-Schutz: nur validieren, wenn 4 Werte vorhanden sind
    if (line.length() > 3 && line.indexOf(',') > 0) {

      int c1, c2, c3, c4;
      int result = sscanf(line.c_str(), "%d,%d,%d,%d", &c1, &c2, &c3, &c4);


      c1 = applyDeadzone(c1, 150);
      c2 = applyDeadzone(c2, 100);

        Serial.print("Ch1="); Serial.print(c1);
        Serial.print(" Ch2="); Serial.print(c2);
        Serial.print(" Ch3="); Serial.print(c3);
        Serial.print(" Ch4="); Serial.println(c4);
      }
    }

  Stepper();
}

void Stepper() {
  digitalWrite(ENABLE_PIN, Channel4);

  stepperX.setSpeed(Channel1);
  stepperX.runSpeed();

  stepperY.setSpeed(Channel2);
  stepperY.runSpeed();
}
