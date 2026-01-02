const byte ST = 0x01;
const int NUM_CHANNELS = 14;
int channels[NUM_CHANNELS];
String input = "";

#define RX 14
#define TX 27

bool MeineNachricht = false;

void setup() {
  Serial.begin(115200);
  input.reserve(50);
  pinMode(26, OUTPUT);
  Serial2.begin(115200, SERIAL_8N1, RX, TX);
}

void loop() {

  ReadSerial();

  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 100) {  // Nur alle 100ms Text ausgeben (schont die CPU)
    lastDebugTime = millis();
    printDebugInfo();
  }
}

void parseBuffer() {
  int n = sscanf(input.c_str(), "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                 &channels[0], &channels[1], &channels[2],
                 &channels[3], &channels[4], &channels[5], &channels[6],
                 &channels[7], &channels[8], &channels[9], &channels[10],
                 &channels[11], &channels[12], &channels[13]);
}

void printDebugInfo() {

  Serial.print(" [OK] - Werte: ");

  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.print(channels[i]);
    if (i < NUM_CHANNELS - 1) Serial.print(",");
  }
  Serial.println();
}

void ReadSerial() {
  while (Serial2.available() > 0) {  //Wenn Serielle Daten verfügbar sind
    byte incomingByte = Serial2.read();

    if (incomingByte == ST) {  //Falls Startbyte erkannt wird: input zurücksetzen
      input = "";
      MeineNachricht = true;  //Setze die MeineNachricht Variable auf True
    } else if (incomingByte > 0x80) {
      MeineNachricht = false;
    }

    else if (MeineNachricht) {
      if (incomingByte == '\n') {
        parseBuffer();
        MeineNachricht = false;
        break;
      } else {
        input += (char)incomingByte;
      }
    }
  }
}