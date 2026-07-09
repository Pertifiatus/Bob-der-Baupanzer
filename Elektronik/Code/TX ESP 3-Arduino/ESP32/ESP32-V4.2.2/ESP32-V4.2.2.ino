#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

const byte ST = 0xA0;
const int NUM_CHANNELS = 14;
int channels[NUM_CHANNELS];
String input = "";

#define RX 14
#define TX 27


#define LED_PIN 33
#define LED_COUNT 10
Servo Pitch;
Servo Sweep;
Servo video_switcher;
ESP32PWM pwm;

int LED_HT = 5;
int CAM;

bool MeineNachricht = false;

LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 20 chars and 4 line display

void setup() {
  Serial.begin(250000);
  ESP32PWM::allocateTimer(0);

  input.reserve(50);
  Serial2.begin(250000, SERIAL_8N1, RX, TX);

  lcd.init();  // initialize the lcd
  lcd.backlight();

  Pitch.attach(25, 500, 2500);
  Sweep.attach(26, 500, 2500);
  video_switcher.attach(32);
}

void loop() {
  ReadSerial();

  Video();

  static unsigned long lastDisplayTime = 0;
  if (millis() - lastDisplayTime > 200) {
    lastDisplayTime = millis();
    updateDisplay();
  }

  /*
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 200) {
    lastDebugTime = millis();
    printDebugInfo();
  }
  */
}





void Video() {
  //Head Tracker
  Pitch.write(map(channels[9], 0, 1000, 0, 180));
  Sweep.write(map(channels[10], 0, 1000, 0, 180));

  //Video Switcher
  CAM = map(channels[8], 0, 1000, 1, 3);
  if (CAM == 1) {
    video_switcher.writeMicroseconds(1000);
  }
  if (CAM == 2) {
    video_switcher.writeMicroseconds(1500);
  }
  if (CAM == 3) {
    video_switcher.writeMicroseconds(2000);
  }
}

void updateDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("Channels Monitor:");

  // Erste Zeile mit Channels
  for (int I = 0; I <= 6; I++) {
    lcd.setCursor(I * 3, 1);
    lcd.print(map(channels[I], 0, 1000, 0, 10));
  }

  // Zweite Zeile mit Channels
  for (int I = 7; I <= 12; I++) {
    lcd.setCursor((I-7) * 3, 2);
    lcd.print(map(channels[I], 0, 1000, 0, 10));
  }

  // Dritte Zeile mit Channels
  for (int I = 13; I <= 15; I++) {
    lcd.setCursor((I-13) * 3, 3);
    lcd.print(map(channels[I], 0, 1000, 0, 10));
  }
}

void parseBuffer() {
  int n = sscanf(input.c_str(), "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                 &channels[0], &channels[1], &channels[2],
                 &channels[3], &channels[4], &channels[5], &channels[6],
                 &channels[7], &channels[8], &channels[9], &channels[10],
                 &channels[11], &channels[12], &channels[13]);
  if (n != NUM_CHANNELS) {
    //Frame verwerfen, da nicht alle Kanäle angekommen oder kaputt sind
    return;
  }
}

void printDebugInfo() {

  Serial.print(" [OK] - Werte: ");

  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.print(channels[i]);
    if (i < NUM_CHANNELS - 1) Serial.print(",");
  }
  Serial.println();
}

void ReadSerial() {
  while (Serial2.available() > 0) {  //Wenn Serielle Daten verfügbar sind
    byte incomingByte = Serial2.read();

    if (incomingByte == ST) {  //Falls Startbyte erkannt wird: input zurücksetzen
      input = "";
      MeineNachricht = true;  //Setze die MeineNachricht Variable auf True
    } else if (incomingByte > 0x80) {
      MeineNachricht = false;
    }

    else if (MeineNachricht) {
      if (incomingByte == '\n') {
        parseBuffer();
        MeineNachricht = false;
        break;
      } else {
        input += (char)incomingByte;
      }
    }
  }
}