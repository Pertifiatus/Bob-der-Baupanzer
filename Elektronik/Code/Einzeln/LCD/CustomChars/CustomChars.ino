#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 in PCF8574 by NXP and Set to 0x3F in PCF8574A by Ti
LiquidCrystal_I2C lcd(0x27, 20, 4);   // bzw. 16, 2 je nach Display

byte Anhaenger[] = {
  0x06,
  0x0F,
  0x0F,
  0x06,
  0x1E,
  0x1E,
  0x1C,
  0x00
};
byte Heart[] = {
  0x00,
  0x0E,
  0x1F,
  0x1F,
  0x0E,
  0x04,
  0x00,
  0x00
};

byte Arm[] = {
  0x00,
  0x0E,
  0x1E,
  0x17,
  0x03,
  0x01,
  0x07,
  0x0F
};


void setup() {
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, Anhaenger);
  lcd.createChar(1, Heart);
  lcd.createChar(2, Arm);
  lcd.home();
  lcd.setCursor(0,0);
  lcd.print("Status:");
  lcd.write(byte(0));
  lcd.print("  ");
  lcd.write(byte(2));
  lcd.print(":Greifarm");
  lcd.setCursor(0,1);
  lcd.print("Super toll: ");
  lcd.write(byte(1));
}

void loop() { }