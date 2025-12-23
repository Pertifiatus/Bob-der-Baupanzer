#include <CRSF.h>

HardwareSerial ArduinoMotorSerial(2); 
HardwareSerial ArduinoGreifSerial(0); 

CRSF crossfire(16, 17); 

unsigned long lastSend = 0;
const char START_TOKEN = '!'; // Neues Start-Token

void setup() {
  // Serial.begin(115200); 
  
  ArduinoMotorSerial.begin(9600, SERIAL_8N1, 27, 26); // TX = 26 , RX = 27
  
  ArduinoGreifSerial.begin(9600, SERIAL_8N1, 33, 32); // TX = 32 , RX = 33
  
  crossfire.begin();                                   // RX = 16 , TX = 17
 
}

void loop() {
  crossfire.update();

  /*
  for (int i = 1; i <= 15; i++) {
    Serial.print(" CH");
    Serial.print(i);
    Serial.print(":");
    Serial.print(crossfire.read(i));
  }
  Serial.println("");
  */

  if (millis() - lastSend >= 80) {
    lastSend = millis();
    sendChannelsToArduinoMotor();
    sendChannelsToArduinoGreif();
  }
}

void sendChannelsToArduinoMotor() {
  
  ArduinoMotorSerial.print(START_TOKEN);

  for (int i = 1; i <= 6; i++) {
    int C = map(crossfire.read(i), 167, 1815, -1000, 1000);
    ArduinoMotorSerial.print(C);
    if (i < 6) ArduinoMotorSerial.print(",");
  }
  ArduinoMotorSerial.print("\n");
}

void sendChannelsToArduinoGreif() {

  ArduinoGreifSerial.print(START_TOKEN);

  for (int i = 1; i <=7 ; i++) {
    int C = map(crossfire.read(i), 167, 1815, -1000, 1000);
    ArduinoGreifSerial.print(C);
    if (i < 7) ArduinoGreifSerial.print(",");
  }
  ArduinoGreifSerial.print("\n");
}
