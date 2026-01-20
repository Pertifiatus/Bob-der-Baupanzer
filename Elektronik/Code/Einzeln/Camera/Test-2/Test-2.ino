#include <CRSF.h>

#include <ESP32Servo.h>
ESP32PWM pwm;
Servo video_switcher;

// RX = GPIO16, TX = GPIO17
CRSF crossfire(16, 17);

int Ch;

void setup() {
  Serial.begin(115200);
  video_switcher.attach(14);
  crossfire.begin();
Serial.write("Startup");
}

void loop() {
  crossfire.update();
      
      Ch=crossfire.read(8);
    Serial.println(Ch);
  Ch = map(Ch , 172,1810,1,3);
if (Ch==1){
  // Schaltet auf Position 1 (1000 µs Pulsweite)
  video_switcher.writeMicroseconds(1000);
  }
if (Ch==2){
  // Schaltet auf Position 2 (1500 µs Pulsweite)
  video_switcher.writeMicroseconds(1500);
  }
if (Ch==3){
  // Schaltet auf Position 3 (2000 µs Pulsweite)
  video_switcher.writeMicroseconds(2000);
  }
}