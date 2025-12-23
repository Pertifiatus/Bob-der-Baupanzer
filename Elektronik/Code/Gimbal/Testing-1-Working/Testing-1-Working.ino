#include <CRSF.h>

// RX = GPIO16, TX = GPIO17
CRSF crossfire(16, 17);

void setup() {
  Serial.begin(115200);
  crossfire.begin();
Serial.write("Startup");
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