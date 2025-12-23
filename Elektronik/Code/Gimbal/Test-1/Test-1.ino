#include <CRSF.h>
#include <ESP32Servo.h>

Servo pitch, roll; 
CRSF crossfire(16, 17);

// Filter-Variablen (float für sanftere Übergänge)
float smoothCh10 = 1000;
float smoothCh11 = 1000;

// Glättungsfaktor: 0.01 (extrem glatt) bis 1.0 (keine Glättung)
// Empfehlung für Servos: 0.05 bis 0.2
float filterAlpha = 0.5; 

void setup() {
  Serial.begin(115200);
  crossfire.begin(420000);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1); 
  
  pitch.setPeriodHertz(50);
  roll.setPeriodHertz(50);

  pitch.attach(5, 500, 2500);
  roll.attach(18, 500, 2500);
}

void loop() {
  crossfire.update();

  int rawCh10 = crossfire.read(10);
  int rawCh11 = crossfire.read(11);

  // Nur filtern, wenn gültige Signale vorhanden sind
  if (rawCh10 > 0 && rawCh11 > 0) {
    
    // EMA Filter Anwendung
    smoothCh10 = (rawCh10 * filterAlpha) + (smoothCh10 * (1.0 - filterAlpha));
    smoothCh11 = (rawCh11 * filterAlpha) + (smoothCh11 * (1.0 - filterAlpha));

    // Mapping auf 0-180 Grad basierend auf den geglätteten Werten
    int posPitch = map((int)smoothCh10, 172, 1811, 0, 180);
    int posRoll  = map((int)smoothCh11, 172, 1811, 0, 180);

    // Servos ansteuern
    pitch.write(posPitch);
    roll.write(posRoll);
  }
}