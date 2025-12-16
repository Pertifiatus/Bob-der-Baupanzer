// MASTER - liest Potentiometer an A0 und sendet Wert an Slave

const int potPin = A0;

void setup() {
  Serial.begin(9600); // Kommunikation mit Slave
}

void loop() {
  int potValue = analogRead(potPin); // 0-1023

  Serial.println(potValue);          // an Slave senden

  delay(50); // 20 Mal pro Sekunde senden
}
