#include <ESP32Servo.h>

const byte ST = 0x01;
const int NUM_CHANNELS = 14;
int channels[NUM_CHANNELS];
String input = "";

#define RX 14
#define TX 27

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

#define LED_PIN 33
#define LED_COUNT 10
Adafruit_NeoPixel HTLED(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Servo Pitch;
Servo Sweep;
Servo video_switcher;
ESP32PWM pwm;

int LED_HT = 5;
int CAM;

bool MeineNachricht = false;


void setup() {
  Serial.begin(115200);
  ESP32PWM::allocateTimer(0);
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  input.reserve(50);
  pinMode(26, OUTPUT);
  Serial2.begin(115200, SERIAL_8N1, RX, TX);

  Pitch.attach(25, 500, 2500);
  Sweep.attach(26, 500, 2500);
  video_switcher.attach(32);

  HTLED.begin();            
	HTLED.show();
  HTLED.setBrightness(50);  // Set BRIGHTNESS to about 1/5 (max = 255)
}

void loop() {

  ReadSerial();

  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 200) {
    lastDebugTime = millis();
    printDebugInfo();
  }
  Video();
}

void parseBuffer() {
  int n = sscanf(input.c_str(), "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                 &channels[0], &channels[1], &channels[2],
                 &channels[3], &channels[4], &channels[5], &channels[6],
                 &channels[7], &channels[8], &channels[9], &channels[10],
                 &channels[11], &channels[12], &channels[13]);
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

void Video() {
  //Head Tracker
  Pitch.write(map(channels[9], 0, 1000, 0, 180));
  Sweep.write(map(channels[10], 0, 1000, 0, 180));
  LED_HT = map(channels[10], 0, 1000, 10, 0);
  for (int i = 0; i <= 10; i++) {
    HTLED.setPixelColor(i, 255, 255, 255);
  }
  HTLED.setPixelColor((LED_HT - 2), 0, 0, 0); 
  HTLED.setPixelColor((LED_HT - 1), 0, 0, 0); 
  HTLED.setPixelColor((LED_HT), 0, 0, 0); 
  HTLED.show();

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