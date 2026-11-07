#define PinMosfet 5  // Runde Ecken 5, Eckige Ecken 3
#define PinTemp A0
#define DesiredTemp 700

float PFaktor = 3;
float IFaktor = 0.5;
unsigned long letzteAusgabe = 0;
const unsigned long intervall = 50;

long t_alt = 0;
long t_neu = 0;

long sum_alt = 0;
long sum_neu = 0;

int stellwert = 0;
int Pr = 0;
int Ir = 0;

int temperaturWert = 0;
int regelDifferenz = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(PinMosfet, OUTPUT);
  pinMode(PinTemp, INPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  temperaturWert = analogRead(PinTemp);
  regelDifferenz = DesiredTemp - temperaturWert;
  t_neu = millis();

  Pregler();
  Iregler();

  stellwert = constrain(stellwert = Pr + Ir, 0, 255);
  analogWrite(PinMosfet, stellwert);
  
  Debug();
  sum_alt = sum_neu;
  t_alt = t_neu;
}

void Pregler() {
  Pr = constrain(regelDifferenz * PFaktor, 0, 255);
}
void Iregler() {
  sum_neu = IFaktor * (regelDifferenz * (t_neu - t_alt) + sum_alt);
  sum_neu = constrain(sum_neu, 0, 255);
  Ir = sum_neu;
}

void Debug() {

  if (millis() - letzteAusgabe >= intervall) {
    letzteAusgabe = millis();
    Serial.print("Temperaturwert:");
    Serial.print(temperaturWert);
    Serial.print("\tStellwert:");
    Serial.print(stellwert);
    Serial.print("\tP-Wert:");
    Serial.print(Pr);
    Serial.print("\tI-Wert:");
    Serial.print(Ir);
    Serial.print("\tErwünschte Temp:");
    Serial.println(DesiredTemp);
  }
}