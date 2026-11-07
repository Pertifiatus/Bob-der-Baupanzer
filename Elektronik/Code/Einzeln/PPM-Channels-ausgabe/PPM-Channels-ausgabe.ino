#include <PPMReader.h>
PPMReader ppm(3, 7);  // Pin 3, 7 Kanäle

const byte STICK_CHANNELS = 4;        // nur Kanäle mit Rauschen prüfen
const unsigned long FREEZE_MS = 500;  // so lange identisch = Failsafe

unsigned lastVal[STICK_CHANNELS + 1];
unsigned long lastChange = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  ppm.minChannelValue = 900;
  ppm.maxChannelValue = 2100;
}

void loop() {
  if (!checkforFailsave()) {
    for (byte ch = 1; ch <= 7; ch++) {
      Serial.print(ppm.latestValidChannelValue(ch, 1512));
      Serial.print('\t');
    }
    Serial.println();
  }
}

bool checkforFailsave() {
  for (byte ch = 1; ch <= STICK_CHANNELS; ch++) {
    unsigned v = ppm.latestValidChannelValue(ch, 0);
    if (v != lastVal[ch]) {
      lastVal[ch] = v;
      lastChange = millis();
    }
  }
  bool failsafe = (millis() - lastChange > FREEZE_MS) || lastVal[1] == 0;  // 0 = gar kein PPM-Signal
  if (failsafe) {
    Serial.println("FAILSAFE");
    return true;
  } else {
    return false;
  }
}