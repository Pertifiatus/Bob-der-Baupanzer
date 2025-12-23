#include <SoftwareSerial.h>
#include <AccelStepper.h>

#define ENABLE_PIN 8

// ÄNDERE HIER WIE VIELE CHANNELS DU HABEN WILLST:
const int NUM_CHANNELS = 6;

#define RX_PIN 0
#define TX_PIN 1
SoftwareSerial mySerial(RX_PIN, TX_PIN);

AccelStepper stepperX(AccelStepper::DRIVER, 2, 5);
AccelStepper stepperY(AccelStepper::DRIVER, 3, 6);
AccelStepper stepperZ(AccelStepper::DRIVER, 4, 7);

int SPEED = 1300;

// Array automatisch so groß wie NUM_CHANNELS
int channels[NUM_CHANNELS];

String input = "";

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  stepperX.setMaxSpeed(SPEED);
  stepperY.setMaxSpeed(SPEED);
  stepperZ.setMaxSpeed(SPEED);

  stepperX.setAcceleration(150);
  stepperY.setAcceleration(150);
  stepperZ.setAcceleration(150);

  pinMode(ENABLE_PIN, OUTPUT);

  Serial.println("Stepper Arduino gestartet...");
}

void loop() {
  readChannels();
  Stepper();
}

void readChannels() {
  while (mySerial.available()) {
    char c = mySerial.read();

    if (c == '\n') {
      parseChannels(input);
      input = "";
      return;
    } else {
      input += c;
    }
  }
}

void parseChannels(String line) {
  int idx = 0;

  for (int i = 0; i < NUM_CHANNELS; i++) {
    int comma = line.indexOf(',', idx);
    if (comma == -1) comma = line.length();

    channels[i] = line.substring(idx, comma).toInt();
    idx = comma + 1;
  }

  // Debuganzeige
  Serial.print("Channels: ");
  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.print(channels[i]);
    Serial.print(" ");
  }
  Serial.println();
}

void Stepper() {
  // Beispiel für erstes 4-Kanal-System
  if (NUM_CHANNELS >= 1)
    stepperX.setSpeed(map(channels[0], 0, 1000, 0, SPEED));

  if (NUM_CHANNELS >= 2) {
    int ch2 = map(channels[1], 0, 1000, -SPEED, SPEED);
    if (abs(ch2) < 20) ch2 = 0;
    stepperY.setSpeed(ch2);
  }

  if (NUM_CHANNELS >= 3)
    stepperZ.setSpeed(map(channels[2], 0, 1000, 0, SPEED));

  if (NUM_CHANNELS >= 4)
    digitalWrite(ENABLE_PIN, channels[3] > 500);

  stepperX.runSpeed();
  stepperY.runSpeed();
  stepperZ.runSpeed();
}
