#include <CRSF.h>
#include <Wire.h> // WICHTIG: Wird für die Pin-Zuweisung benötigt
#include <LiquidCrystal_I2C.h>
// RX = GPIO16, TX = GPIO17
CRSF crossfire(16, 17);
#define I2C_SDA_PIN 18 // GPIO 18 für SDA
#define I2C_SCL_PIN 19 // GPIO 19 für SCL

LiquidCrystal_I2C lcd(0x27,20,4); // Adresse und Display-Größe festlegen
void setup() {
  Serial.begin(115200);
  crossfire.begin();
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

      lcd.init(); // initialize the lcd 
  lcd.backlight();
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
  lcd.setCursor(0,0);
  if(crossfire.read(1)==0){
    lcd.print("ELRS disconnected");
  }
  else{
    lcd.print("ELRS connected           ");
  }
}