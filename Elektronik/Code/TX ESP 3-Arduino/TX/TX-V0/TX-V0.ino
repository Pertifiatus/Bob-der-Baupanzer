#include <CRSF.h>

// RX = GPIO6, TX = GPIO5
CRSF crossfire(6, 5);
HardwareSerial TXSerial(0); 

void setup() {
  
  Serial.begin(115200);
  crossfire.begin();
  TXSerial.begin(9600, SERIAL_8N1, 27, 26); // TX = 26 , RX = 27

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
}
