#define PinMosfet 5  // Runde Ecken 5, Eckige Ecken 3
#define PinTemp A0
#define DesiredTemp 600
float IFaktor = 0.4;
unsigned long letzteAusgabe = 0;
const unsigned long intervall = 50;
long t_alt = 0;
long t_neu = 0;

long sum_alt = 0;
long sum_neu = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(PinMosfet, OUTPUT);
  pinMode(PinTemp, INPUT);
  Serial.begin(9600);
}

void loop() {
  int temperaturWert = analogRead(PinTemp);
  int regelDifferenz = DesiredTemp - temperaturWert;
  t_neu = millis();
  sum_neu = IFaktor * (regelDifferenz * (t_neu - t_alt) + sum_alt);
  sum_neu = constrain(sum_neu, 0,255);
  analogWrite(PinMosfet, sum_neu);
  if (millis() - letzteAusgabe >= intervall) {
    letzteAusgabe = millis();
    Serial.print("Temperaturwert:");
    Serial.print(temperaturWert);
    Serial.print("\tRegeldifferenz:");
    Serial.print(regelDifferenz);
    Serial.print("\tErwünschte Temp:");
    Serial.print(DesiredTemp);
    Serial.print("\tSum-neu:");
    Serial.print(sum_neu);



    Serial.println();
  }
  sum_alt = sum_neu;
  t_alt = t_neu;
}