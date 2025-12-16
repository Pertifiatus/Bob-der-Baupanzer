//Setup
  #include <Adafruit_NeoPixel.h>
  #ifdef __AVR__
  #include <avr/power.h>
  #endif

  #define PIN        5

  #define NUMPIXELS 16
  Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

  #define DELAYVAL 500 // Time (in milliseconds) to pause between pixels


void setup() {

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(255);
}

void loop() {
delay(5000);
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    pixels.show();   
    delay(DELAYVAL); 
  }
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(255, 255, 0));
    pixels.show();   
    delay(DELAYVAL); 
  }
    for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
    pixels.show();   
    delay(DELAYVAL); 
  }  
    for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(0,255, 255));
    pixels.show();   
    delay(DELAYVAL); 
  }
    for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(0, 0, 255));
    pixels.show();   
    delay(DELAYVAL); 
  }
    for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(255, 0, 255));
    pixels.show();   
    delay(DELAYVAL); 
  }
}
