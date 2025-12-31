const byte MY_START_TOKEN = 0xA1;
int channels[7];
String inputBuffer = "";

void setup() {
  // Baudrate muss 115200 sein, damit sowohl ESP als auch PC passen
  Serial.begin(115200);
  inputBuffer.reserve(50);
}

void loop() {
  while (Serial.available() > 0) {
    byte incomingByte = Serial.read();

    if (incomingByte == MY_START_TOKEN) {
      inputBuffer = "";
    } else if (incomingByte == '\n') {
      parseAndDebug(); // Daten verarbeiten und anzeigen
    } else {
      inputBuffer += (char)incomingByte;
    }
  }
}

void parseAndDebug() {
  int n = sscanf(inputBuffer.c_str(), "%d,%d,%d,%d,%d,%d,%d", 
                 &channels[0], &channels[1], &channels[2], 
                 &channels[3], &channels[4], &channels[5], &channels[6]);

  if (n == 7) {
    // Schreibt die Daten direkt zurück in denselben Serial-Port
    // Der PC (Serieller Monitor) zeigt das an.
    Serial.print("Empfangen -> ");
    for (int i = 0; i < 7; i++) {
      Serial.print("CH");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(channels[i]);
      if (i < 6) Serial.print(" | ");
    }
    Serial.println();
  }
}