#include <ESP32Servo.h>

const byte ST = 0x01;
const int NUM_CHANNELS = 14;
int channels[NUM_CHANNELS];

// Fester Buffer für den Empfang
const int BUFFER_SIZE = 100; // Etwas größer zur Sicherheit
char inputBuffer[BUFFER_SIZE];
int bufferIndex = 0;

#define RX 14
#define TX 27

#define LED_PIN 33
#define LED_COUNT 10
Servo Pitch;
Servo Sweep;
Servo video_switcher;
ESP32PWM pwm;

int LED_HT = 5;
int CAM;

bool MeineNachricht = false;

void setup() {
  Serial.begin(250000);
  ESP32PWM::allocateTimer(0);

  Serial2.begin(250000, SERIAL_8N1, RX, TX);

  Pitch.attach(25, 500, 2500);
  Sweep.attach(26, 500, 2500);
  video_switcher.attach(32);
}

void loop() {
  ReadSerial();
  Video();
}

void parseBuffer() {
  int tempChannels[NUM_CHANNELS];
  
  // sscanf liest die Werte in ein temporäres Array
  int n = sscanf(inputBuffer, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                 &tempChannels[0], &tempChannels[1], &tempChannels[2],
                 &tempChannels[3], &tempChannels[4], &tempChannels[5], &tempChannels[6],
                 &tempChannels[7], &tempChannels[8], &tempChannels[9], &tempChannels[10],
                 &tempChannels[11], &tempChannels[12], &tempChannels[13]);
                 
  // Nur wenn ALLE 14 Kanäle fehlerfrei übertragen wurden, übernehmen wir sie.
  // Das verhindert, dass die Servos bei korrupten Daten zucken.
  if (n == NUM_CHANNELS) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
      channels[i] = tempChannels[i];
    }
  }
}

void ReadSerial() {
  // Wir lesen ALLES aus, was im seriellen Puffer bereitsteht,
  // damit der Hardware-Puffer des ESP32 niemals überläuft.
  while (Serial2.available() > 0) {
    byte incomingByte = Serial2.read();

    if (incomingByte == ST) {  
      bufferIndex = 0;
      inputBuffer[bufferIndex] = '\0'; 
      MeineNachricht = true;
    } 
    else if (incomingByte > 0x80) {
      MeineNachricht = false;
    }
    else if (MeineNachricht) {
      if (incomingByte == '\n') {
        inputBuffer[bufferIndex] = '\0'; // String sauber beenden
        parseBuffer();
        MeineNachricht = false;
      } 
      else {
        if (bufferIndex < BUFFER_SIZE - 1) {
          inputBuffer[bufferIndex++] = (char)incomingByte;
        }
      }
    }
  }
}

void Video() {
  // Head Tracker
  Pitch.write(map(channels[9], 0, 1000, 0, 180));
  Sweep.write(map(channels[10], 0, 1000, 0, 180));

  // Video Switcher
  CAM = map(channels[8], 0, 1000, 1, 3);
  if (CAM == 1) {
    video_switcher.writeMicroseconds(1000);
  }
  if (CAM == 2) {
    video_switcher.writeMicroseconds(1500);
  }
  if (CAM == 3) {
    video_switcher.writeMicroseconds(2000);
  }
}