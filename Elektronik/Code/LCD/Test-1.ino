// YWROBOT
// Compatible with the Arduino IDE 1.0
// Library version:1.1
#include <Wire.h> // WICHTIG: Wird für die Pin-Zuweisung benötigt
#include <LiquidCrystal_I2C.h>

// I2C Pins für den ESP32 definieren
#define I2C_SDA_PIN 18 // GPIO 18 für SDA
#define I2C_SCL_PIN 19 // GPIO 19 für SCL

LiquidCrystal_I2C lcd(0x27,20,4); // Adresse und Display-Größe festlegen

void setup()
{
  
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); 
  
  
  lcd.init(); // initialize the lcd 
  lcd.backlight();
}

void loop()
{
  lcd.setCursor(0,0);
  lcd.print("Zeil 1");
  delay(500);
  lcd.setCursor(4,1);
  lcd.print("Zeile 2");
  delay(500);
  lcd.setCursor(8,2);
  lcd.print("Zeile 3");
  delay(500);
  lcd.setCursor(12,3);
  lcd.print("Zeile 4");
  delay(500);

}