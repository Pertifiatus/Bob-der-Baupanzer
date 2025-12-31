#include <AccelStepper.h>

#define ENABLE_PIN 8
#define SPEED 1300  //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning

const byte ST = 0xA1;
const int NUM_CHANNELS = 6;
int channels[NUM_CHANNELS];
String input = "";

unsigned long lastPacketTime = 0;
const unsigned long TimeoutFailsafe = 500;
bool failsafeActive = false;
bool MeineNachricht = false;

void setup() {
  Serial.begin(115200);
  input.reserve(50);

  activateFailsafe();
  lastPacketTime = millis();
}

void loop() {

  while (Serial.available() > 0) {  //Wenn Serielle Daten verfügbar sind
    byte incomingByte = Serial.read();

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

  //  Failsafe Check
  if (millis() - lastPacketTime > TimeoutFailsafe) {
    if (!failsafeActive) {
      activateFailsafe();
      failsafeActive = true;
    }
  }

  // 3. Debug-Ausgabe im Seriellen Monitor
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 100) {  // Nur alle 100ms Text ausgeben (schont die CPU)
    lastDebugTime = millis();
    printDebugInfo();
  }
}

void parseBuffer() {
  int n = sscanf(input.c_str(), "%d,%d,%d,%d,%d,%d,%d",
                 &channels[0], &channels[1], &channels[2],
                 &channels[3], &channels[4], &channels[5], &channels[6]);

  if (n == NUM_CHANNELS) {
    // Wenn Daten korrekt ankommen -> Failsafe beenden
    if (failsafeActive) {
      failsafeActive = false;
    }
    lastPacketTime = millis();  // Zeitstempel für Timeout-Timer erneuern
  }
}

void activateFailsafe() {
  // Sicherheitswerte definieren (z.B. alles auf 0)
  for (int i = 0; i < NUM_CHANNELS; i++) {
    channels[i] = 0;
  }
}

void printDebugInfo() {
  if (failsafeActive) {
    Serial.print(" [!] - Fallback: ");
  } else {
    Serial.print(" [OK] - Werte: ");
  }

  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.print(channels[i]);
    if (i < NUM_CHANNELS - 1) Serial.print(",");
  }
  Serial.println();
}