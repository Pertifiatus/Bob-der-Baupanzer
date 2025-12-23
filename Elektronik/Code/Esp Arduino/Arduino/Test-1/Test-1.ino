void setup() {
  Serial.begin(115200); // USB Monitor
  Serial.println("Arduino ready to receive CRSF channels");
}

void loop() {
  static uint16_t ch[4];
  static int byteCount = 0;
  static uint8_t buf[8]; // 4 Channels x 2 Bytes

  while (Serial.available()) {
    buf[byteCount++] = Serial.read();
    if (byteCount >= 8) {
      // 4 Channels zusammensetzen
      for (int i=0; i<4; i++) {
        ch[i] = (buf[i*2] << 8) | buf[i*2+1];
      }
      // Ausgabe
      Serial.print("Channels 1-4: ");
      for (int i=0; i<4; i++) {
        Serial.print(ch[i]);
        if (i<3) Serial.print(" | ");
      }
      Serial.println();
      byteCount = 0; // reset für nächste 8 Bytes
    }
  }
}
