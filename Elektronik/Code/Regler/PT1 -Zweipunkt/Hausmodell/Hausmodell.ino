#define PinMosfet 3
#define PinTemp A0
#define DesiredTemp 1023
#define Hysterese 50
unsigned long letzteAusgabe = 0;
const unsigned long intervall = 20;

void setup() {
  // put your setup code here, to run once:
  pinMode(PinMosfet, OUTPUT);
  pinMode(PinTemp, INPUT);
  Serial.begin(9600);
}

void loop() {
  int temperaturWert = analogRead(PinTemp);

  if (millis() - letzteAusgabe >= intervall) {
    letzteAusgabe = millis();
    Serial.print("Temperaturwert:");
    Serial.print(temperaturWert);
    Serial.print("\tErwünschte Temp:");
    Serial.print(DesiredTemp);
    Serial.print("\tOberlimit:");
    Serial.print(DesiredTemp+Hysterese*0.5);
    Serial.print("\tUnterlimit:");
    Serial.println(DesiredTemp - Hysterese * 0.5);
  }

  if (temperaturWert > DesiredTemp + Hysterese * 0.5) {
    digitalWrite(PinMosfet, 0);
  } else if (temperaturWert < DesiredTemp - Hysterese * 0.5) {
    digitalWrite(PinMosfet, 1);
  }
}