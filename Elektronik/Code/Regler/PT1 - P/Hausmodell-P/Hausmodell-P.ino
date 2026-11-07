#define PinMosfet 5 // Runde Ecken 5, Eckige Ecken 3
#define PinTemp A0  
#define DesiredTemp 600
float PFaktor = 0.8;
unsigned long letzteAusgabe = 0;
const unsigned long intervall = 250;

void setup() {
  // put your setup code here, to run once:
  pinMode(PinMosfet, OUTPUT);
  pinMode(PinTemp, INPUT);
  Serial.begin(9600);
}

void loop() {
  int temperaturWert = analogRead(PinTemp);
  int regelDifferenz = DesiredTemp - temperaturWert;

  int stellwert = constrain(regelDifferenz * PFaktor, 0, 255);
  analogWrite(PinMosfet, stellwert);

  if (millis() - letzteAusgabe >= intervall) {
    letzteAusgabe = millis();
    Serial.print("Temperaturwert:");
    Serial.print(temperaturWert);
    Serial.print("\tStellwert:");
    Serial.print(stellwert);
    Serial.print("\tErwünschte Temp:");
    Serial.println(DesiredTemp);
  }
}