#define PinMosfet 5  // Runde Ecken 5, Eckige Ecken 3
#define PinTemp A0
#define DesiredTemp 700

float PFaktor = 3;
float IFaktor = 0.5;
float DFaktor = 500;
unsigned long letzteAusgabe = 0;
unsigned long letzteAusgabeD = 0;
const unsigned long intervall = 50;
const unsigned long Dintervall = 200;

long t_dalt = 0;
long t_alt = 0;
long t_neu = 0;

long sum_alt = 0;
long sum_neu = 0;

int stellwert = 0;
int Pr = 0;
int Ir = 0;
float Dr = 0;

int temperaturWert = 0;
int regelDifferenz = 0;
int regelDifferenzalt = 0;

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
  Dregler();

  stellwert = constrain(stellwert = Pr + Ir + Dr, 0, 255);
  analogWrite(PinMosfet, stellwert);

  Debug();
  sum_alt = sum_neu;
  t_alt = t_neu;

  if (millis() - letzteAusgabeD >= Dintervall) {
    t_dalt = t_neu;
    regelDifferenzalt = regelDifferenz;
    letzteAusgabeD = millis();
  }
}

void Pregler() {
  Pr = constrain(regelDifferenz * PFaktor, 0, 255);
}
void Iregler() {
  sum_neu = IFaktor * (regelDifferenz * (t_neu - t_alt) + sum_alt);
  sum_neu = constrain(sum_neu, 0, 255);
  Ir = sum_neu;
}
void Dregler() {
  int deltae = regelDifferenzalt - regelDifferenz;
  int deltat = t_neu - t_dalt;
  Dr = DFaktor * ((float)deltae / deltat);
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
    Serial.print("\tD-Wert:");
    Serial.print(Dr);
    Serial.print("\tErwünschte Temp:");
    Serial.println(DesiredTemp);
  }
}