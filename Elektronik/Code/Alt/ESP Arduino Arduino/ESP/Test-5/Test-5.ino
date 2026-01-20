#include <CRSF.h>

HardwareSerial ArduinoMotorSerial(2); // UART2 für Arduino
HardwareSerial ArduinoGreifSerial(0); // UART2 für Arduino

// RX = GPIO16, TX = GPIO17
CRSF crossfire(16, 17);

unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  ArduinoMotorSerial.begin(19200, SERIAL_8N1, 27, 26); // TX=26, RX=27
  ArduinoGreifSerial.begin(19200, SERIAL_8N1, 33, 32); // TX=32, RX=33
  crossfire.begin(); // RX=16, TX=17

  Serial.println("ESP32 CRSF Bridge gestartet...");
}

void loop() {
  crossfire.update();

  // Debug-Ausgabe der Kanäle 1–12 auf USB-Serial
  for (int i = 1; i <= 15; i++) {
    Serial.print(" CH");
    Serial.print(i);
    Serial.print(":");
    Serial.print(crossfire.read(i));
  }
  Serial.println("");

  // Alle 20 ms Channels 1–3 an Arduino senden
  if (millis() - lastSend >= 20) {
    lastSend = millis();
    sendChannelsToArduinoMotor();
    sendChannelsToArduinoGreif();
  }
}

void sendChannelsToArduinoMotor() {
  for (int i = 1; i <= 3; i++) {
    int C = map(crossfire.read(i), 167, 1815, -1000, 1000);
    ArduinoMotorSerial.print(C);
    if (i < 3) ArduinoMotorSerial.print(",");
  }
  ArduinoMotorSerial.print("\n");
}
void sendChannelsToArduinoGreif() {
  for (int i = 1; i <= 3; i++) {
    int C = map(crossfire.read(i), 167, 1815, -1000, 1000);
    ArduinoGreifSerial.print(C);
    if (i < 3) ArduinoGreifSerial.print(",");
  }
  ArduinoGreifSerial.print("\n");
}