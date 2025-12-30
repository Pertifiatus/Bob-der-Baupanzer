#include <CRSF.h>
#include "SimplePacket.h" // Unsere neue Library einbinden

// Serial Definitionen
HardwareSerial ArduinoMotorSerial(2); 
HardwareSerial ArduinoGreifSerial(0); 

CRSF crossfire(16, 17); // Dein Crossfire Setup

// Instanzen unserer neuen Library erstellen
SimplePacket commMotor(ArduinoMotorSerial);
SimplePacket commGreif(ArduinoGreifSerial);

unsigned long lastSend = 0;

void setup() {
  // Serials starten
  ArduinoMotorSerial.begin(9600, SERIAL_8N1, 27, 26); // RX=27, TX=26
  ArduinoGreifSerial.begin(9600, SERIAL_8N1, 33, 32); // RX=33, TX=32
  
  crossfire.begin();
  
  commMotor.begin();
  commGreif.begin();
}

void loop() {
  crossfire.update();

  // Sende-Intervall (80ms)
  if (millis() - lastSend >= 80) {
    lastSend = millis();
    
    sendToMotor();
    sendToGreif();
  }
}

void sendToMotor() {
  // Kanäle 1 bis 6 vorbereiten
  for (int i = 1; i <= 6; i++) {
    // Hier mappen wir direkt beim Setzen
    int val = map(crossfire.read(i), 172, 1811, -1000, 1000);
    commMotor.write(i, val); 
  }
  // Alles Abschicken (wir senden 6 Kanäle)
  commMotor.send(6);
}

void sendToGreif() {
  // Kanäle 1 bis 7 vorbereiten
  for (int i = 1; i <= 7; i++) {
    int val = map(crossfire.read(i), 172, 1811, -1000, 1000);
    commGreif.write(i, val);
  }
  // Alles Abschicken (wir senden 7 Kanäle)
  commGreif.send(7);
}
