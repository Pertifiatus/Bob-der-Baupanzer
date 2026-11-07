#include <Wire.h>
#include <AS5600.h>
#include <PPMReader.h>

//#define Maxrpm 180  //SPEED... I am SPEED... Faster than fast quicker than quick... I am lightning
#define PinDriverF 5  //Pin für die Motortreiber PWM Vorwerts
#define PinDriverR 6  //Pin für die Motortreiber PWM Rückwerts

#define intervall 100  //Intervall für die Debug ausgabe
#define Debug false     //Debug Ausgabe an/aus

//--------------------------------------------------------
bool Drive = false;  // Drive/Enabled Status
bool DriveMK2 = false;  // Drive/Enabled Status
int TopSpeed = 85;
int ErwuenschteSpeed = 0;
int ErwuenschteSpeedMK2 = 0;
int regelDifferenz = 0;
float rpm = 0;
float raw = 0;
bool Driving = false;

AS5600 as5600;           //Sensor "erstellen"
#define Trustfaktor 0.4  //Wie sehr wir den Sensorwerten vertrauen. 0,5 viel. 0,1 langsam
//--------------------------------------------------------
// Faktoren hier zum Tunen ändern
float PFaktor = 5;
float IFaktor = 7;
float FFaktor = 1.4;
//--------------------------------------------------------

int Gas = 0;
int Pr = 0;
int Ir = 0;
//--------------------------------------------------------

unsigned long t_alt = 0;
unsigned long t_neu = 0;
float summe = 0;
//--------------------------------------------------------

const byte ST = 0xA1;        //Startbyte des Drive Arduinos
const int NUM_CHANNELS = 6;  //Channels welche Empfangen werden sollen
int channels[NUM_CHANNELS];
unsigned long lastDebugTime = 0;
unsigned long lastPacketTime = 0;
const unsigned long TimeoutFailsafe = 500;  //Timeout nach welchem der Arduino in den Failsave Modus wechselt
bool failsafeActive = true;
const int FRAME_PAYLOAD_LEN = NUM_CHANNELS * 2;
byte frameBuffer[FRAME_PAYLOAD_LEN];
int frameIndex = 0;
bool receivingFrame = false;

//--------------------------------------------------------
// Variablen und const's für Empfangen der PPM Daten

PPMReader ppm(3, 7);  // Pin 3, 7 Kanäle
const int PPM_MIN = 1012; 
const int PPM_MAX = 2024; 

const byte STICK_CHANNELS = 4;        // nur Kanäle mit Rauschen prüfen
const unsigned long FREEZE_MS = 500;  // so lange identisch = Failsafe
unsigned lastVal[STICK_CHANNELS + 1];
unsigned long lastChange = 0;

const int NUM_CHANNELS_SECOND = 7;  //Channels welche Empfangen werden sollen
int channels_second[NUM_CHANNELS_SECOND];

//--------------------------------------------------------

int applyDeadzone(int value, int dz) {
  return (abs(value) < dz) ? 0 : value;
}

void setup() {
  Serial.begin(250000);
  ppm.minChannelValue = 900;
  ppm.maxChannelValue = 2100;
  Wire.begin();  // muss bei neueren Versionen selbst aufgerufen werden
  Wire.setClock(400000);
  as5600.begin();
  as5600.setDirection(AS5600_CLOCK_WISE);
  //--------------------------------------------------------

  pinMode(PinDriverF, OUTPUT);
  pinMode(PinDriverR, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(PinDriverF, 0);
  digitalWrite(PinDriverR, 0);
  //-------------------------------------------------------- Super duper viel Stuff damit falls der Sensor nicht verbunden ist wir nicht crashout gehen :)
  activateFailsafe();
  while (!as5600.isConnected()) {
    digitalWrite(LED_BUILTIN, 1);
  }
  digitalWrite(LED_BUILTIN, 0);
  while (as5600.magnetTooWeak() || as5600.magnetTooStrong()) {
    digitalWrite(LED_BUILTIN, 1);
    delay(500);
    digitalWrite(LED_BUILTIN, 0);
    delay(500);
  }
  //--------------------------------------------------------

  lastPacketTime = millis();
}

void loop() {
  t_neu = millis();
  ReadSerial();
  ReadPPM();
  ChannellogicMK2();
  Channellogic();
  Fahren();
  if (Debug) printDebugInfo();
  t_alt = t_neu;
}

void Fahren() {
  ReadRPM();

  regelDifferenz = ErwuenschteSpeed - rpm;

  CalculateP();
  CalculateI();
  CalculatePI();
  Motor();
}


void printDebugInfo() {
  if (millis() - lastDebugTime > intervall) {

    lastDebugTime = millis();
    if (failsafeActive) {
      Serial.print(" [!] - Fallback: ");
    } else {
      Serial.print(" [OK] - Werte: ");
    }

    for (int i = 0; i < NUM_CHANNELS; i++) {
      Serial.print(channels[i]);
      if (i < NUM_CHANNELS - 1) Serial.print(",");
    }
    if (checkforFailsave()) {
      Serial.print(" [!] PPM Failsave");
    } else {
      Serial.print(" PPM Channels: ");
      for (int i = 0; i < NUM_CHANNELS_SECOND; i++) {
        Serial.print(channels_second[i]);
        if (i < NUM_CHANNELS_SECOND - 1) Serial.print(",");
      }
    }
    Serial.print("  RPM");
    Serial.print(rpm);
    Serial.print("  Gas");
    Serial.print(Gas);
    Serial.print("  PFaktor");
    Serial.print(Pr);
    Serial.print("  IFaktor");
    Serial.print(Ir);

    Serial.println();
  }
}

void Channellogic() {
  if (channels[4] < 500) Drive = true;
  else Drive = false;

  int stick = 500;
  if (Drive == true) stick = channels[1];  // Serial
  else if(DriveMK2 == true)stick = channels_second[1];      // PPM
  ErwuenschteSpeed = map(stick, 0, 1000, -TopSpeed, TopSpeed);
  ErwuenschteSpeed = applyDeadzone(ErwuenschteSpeed, TopSpeed * 0.15);
}

void ChannellogicMK2() {
  if(channels_second[5]<550){
    DriveMK2 = true;
  }
  else DriveMK2 = false;
}

void Motor() {
  if (Drive == true || (DriveMK2 && !checkforFailsave())) {
    Driving = true;
    if (Gas != 0) {
      if (Gas > 0) {
        analogWrite(PinDriverF, Gas);
        analogWrite(PinDriverR, 0);
      }
      if (Gas < 0) {
        analogWrite(PinDriverF, 0);
        analogWrite(PinDriverR, -Gas);
      }

    } else {
      analogWrite(PinDriverF, 0);
      analogWrite(PinDriverR, 0);
    }

  } else {
    analogWrite(PinDriverF, 0);
    analogWrite(PinDriverR, 0);
    Driving = false;
  }
}

//--------------------------------------------------------

void ReadRPM() {
  static unsigned long last = 0;
  if (millis() - last < 10) return;  // feste 10 ms Abtastung
  last = millis();
  raw = as5600.getAngularSpeed(AS5600_MODE_RPM);
  rpm = (1 - Trustfaktor) * rpm + Trustfaktor * raw;  // Tiefpassfilter
}

void CalculateP() {
  Pr = constrain(regelDifferenz * PFaktor, -255, 255);
}

void CalculateI() {
  if (Driving == false || ErwuenschteSpeed == 0) summe = 0;
  float dt = (t_neu - t_alt) / 1000.0;
  summe += regelDifferenz * dt;
  summe = constrain(summe, -255.0 / IFaktor, 255.0 / IFaktor);
  Ir = IFaktor * summe;
}

void CalculatePI() {
  int Vorsteuerung = FFaktor * ErwuenschteSpeed;
  Gas = constrain(Pr + Ir + Vorsteuerung, -255, 255);
}

void activateFailsafe() {
  // Sicherheitswerte definieren (z.B. alles auf 0)
  channels[0] = 500;
  channels[1] = 500;
  channels[2] = 500;
  channels[3] = 500;
  channels[4] = 1000;
  channels[5] = 1000;
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
    return true;
  } else {
    return false;
  }
}

void ReadPPM() {
  for (byte i = 0; i < NUM_CHANNELS_SECOND; i++) {
    unsigned us = ppm.latestValidChannelValue(i + 1, 1500);
    channels_second[i] = constrain(map(us, PPM_MIN, PPM_MAX, 0, 1000), 0, 1000);
  }
}

//--------------------------------------------------------

void ReadSerial() {
  while (Serial.available() > 0) {  //Wenn Serielle Daten verfügbar sind
    byte incomingByte = Serial.read();

    if (incomingByte == ST) {  //Falls unser Startbyte erkannt wird: neuen Frame beginnen
      receivingFrame = true;
      frameIndex = 0;
      continue;
    }

    if (incomingByte & 0x80) {
      // Startbyte eines anderen Boards -> unser Frame ist ungültig/beendet
      receivingFrame = false;
      continue;
    }

    if (!receivingFrame) continue;

    if (frameIndex < FRAME_PAYLOAD_LEN) {
      frameBuffer[frameIndex++] = incomingByte;
    } else {
      // Dieses Byte ist die Prüfsumme -> Frame ist komplett
      decodeFrame(incomingByte);
      receivingFrame = false;
    }
  }

  //  Failsafe Check
  if (millis() - lastPacketTime > TimeoutFailsafe) {
    if (!failsafeActive) {
      activateFailsafe();
      failsafeActive = true;
    }
  }
}
// Prüft die Prüfsumme und übernimmt bei Erfolg die Kanalwerte aus frameBuffer.
void decodeFrame(byte receivedChecksum) {
  byte checksum = 0;
  for (int i = 0; i < FRAME_PAYLOAD_LEN; i++) checksum ^= frameBuffer[i];
  checksum &= 0x7F;

  if (checksum != receivedChecksum) {
    // Frame verwerfen, da Prüfsumme nicht passt (z.B. durch Bitfehler)
    return;
  }

  // Die XOR-Prüfsumme erkennt nicht jeden Bitfehler (z.B. zwei Bitkipper an
  // derselben Position in unterschiedlichen Bytes können sich gegenseitig
  // aufheben). Als zweite Verteidigungslinie werden die dekodierten Werte
  // zusätzlich auf den gültigen Bereich 0-1000 begrenzt, damit ein durch die
  // Prüfsumme rutschender Fehler keinen Kanalwert weit außerhalb des
  // erwarteten Bereichs an die Motorsteuerung weiterreichen kann.
  for (int i = 0; i < NUM_CHANNELS; i++) {
    byte lowBits = frameBuffer[i * 2];
    byte highBits = frameBuffer[i * 2 + 1];
    channels[i] = constrain(lowBits | (highBits << 7), 0, 1000);
  }

  // Wenn Daten korrekt ankommen -> Failsafe beenden
  if (failsafeActive) {
    failsafeActive = false;
  }
  lastPacketTime = millis();  // Zeitstempel für Timeout-Timer erneuern
}