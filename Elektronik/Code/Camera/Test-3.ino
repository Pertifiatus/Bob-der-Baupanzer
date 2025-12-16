#include <CRSF.h>

#include <ESP32Servo.h>
ESP32PWM pwm;
Servo video_switcher;

// RX = GPIO16, TX = GPIO17
CRSF crossfire(16, 17);
int CAM;

unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  crossfire.begin();
  video_switcher.attach(14);
Serial.write("Startup");
}

void loop() {
  crossfire.update();
for (int i=1;i<17;i++){
  Serial.print(" CH");
  Serial.print(i);
  Serial.print(":");
  Serial.print(crossfire.read(i));
  }
  Serial.println("");
  CAM = map(crossfire.read(8), 172,1810,1,3);

  if (millis() - lastSend >= 80) {
    lastSend = millis();
  if(CAM==1){
  video_switcher.writeMicroseconds(1000);
  }
  if(CAM==2){
  video_switcher.writeMicroseconds(1500);
  }
  if(CAM==3){
  video_switcher.writeMicroseconds(2000);
  }
delay(1);
}





}