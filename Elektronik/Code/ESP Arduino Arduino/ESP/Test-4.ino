#include <CRSF.h>

HardwareSerial ArduinoSerial(2); // UART2 für Arduino

// RX = GPIO16, TX = GPIO17
CRSF crossfire(16, 17);

unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  ArduinoSerial.begin(9600, SERIAL_8N1, 27, 26); // TX=26, RX=27
  crossfire.begin(); // RX=16, TX=17

  Serial.println("ESP32 CRSF Bridge gestartet...");
}

void loop() {
  crossfire.update();
for (int i=1;i<13;i++){
    Serial.print(" CH");
    Serial.print(i);
    Serial.print(":");
    int C = map(crossfire.read(i), 174, 1811, -1000, 1000);
    Serial.print(C);
  }
  Serial.println("");

//Alle 20ms Channels an Arduino senden :)
  if (millis() - lastSend >= 20) {
    lastSend = millis();
    sendChannelsToArduino();
  }

}

void sendChannelsToArduino() {
  for (int i = 1; i <= 3; i++) {
      int C = map(crossfire.read(i), 174, 1811, -1000, 1000);
      ArduinoSerial.print(C);
      if (i < 3) ArduinoSerial.print(",");
  }
  ArduinoSerial.print("\n");
}