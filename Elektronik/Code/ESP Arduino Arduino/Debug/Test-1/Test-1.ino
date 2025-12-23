#include <AccelStepper.h> 

// Definiert die Pins und Konstanten
#define BUFFER_SIZE 48        
#define MAX_CHANNELS 7        // <-- GEÄNDERT: Die maximale Anzahl der erwarteten Kanäle ist jetzt 7
#define START_TOKEN '!'       

// Nur zur Vollständigkeit, Werte werden hier gespeichert
int c[MAX_CHANNELS + 1] = {0, 0, 0, 0, 0, 0, 0, 0}; // <-- GEÄNDERT: Array-Größe für 7 Kanäle (+1 für 1-basierte Zählung)

char inputBuffer[BUFFER_SIZE];
byte bufferIndex = 0;

// Zustand, ob wir innerhalb einer gültigen Nachricht sind
static bool readingActive = false; 

void setup() {
  // Wichtig: Baudrate MUSS mit dem ESP32 übereinstimmen
  Serial.begin(9600); 

  Serial.println("--- Arduino Debug Start ---");
  Serial.print("Warte auf "); Serial.print(MAX_CHANNELS); Serial.println(" Kanäle (Start-Token '!')");
  Serial.println("---");
}

/**
 * Liest serielle Daten von der HardwareSerial-Schnittstelle und synchronisiert über das Start-Token.
 */
void readSerialAndDebug() {
  
  while (Serial.available()) {
    char incoming = Serial.read();

    if (incoming == START_TOKEN) {
      // START-TOKEN gefunden: Puffer zurücksetzen und Lesezustand aktivieren
      bufferIndex = 0;
      readingActive = true;
    } 
    else if (readingActive) {
      
      if (incoming == '\n') {
        // Ende der Nachricht gefunden: Parsen versuchen
        inputBuffer[bufferIndex] = 0;
        
        int t1, t2, t3, t4, t5, t6, t7; // Alle 7 temporären Variablen sind vorhanden
        
        // Versuch, die erwartete Anzahl an Ganzzahlen zu parsen
        // KORREKT: Das sscanf-Format ist bereits auf 7 Werte eingestellt
        int parsed = sscanf(inputBuffer, "%d,%d,%d,%d,%d,%d,%d", 
                            &t1, &t2, &t3, &t4, &t5, &t6, &t7);

        if (parsed == MAX_CHANNELS) {
          // Erfolgreich geparst
          // KORREKT: Alle 7 Werte werden korrekt in das c-Array gespeichert
          c[1] = t1; c[2] = t2; c[3] = t3; c[4] = t4; c[5] = t5; c[6] = t6; c[7] = t7;
          
          // DEBUG-Ausgabe der 7 Kanäle
          Serial.print("SUCCESS | ");
          for(int i = 1; i <= MAX_CHANNELS; i++) { // <-- GEÄNDERT: Schleife läuft bis MAX_CHANNELS (7)
            Serial.print("CH");
            Serial.print(i);
            Serial.print(":");
            Serial.print(c[i]);
            if(i < MAX_CHANNELS) Serial.print(" | ");
          }
          Serial.println("");
        } else {
          // Parsen fehlgeschlagen 
          Serial.print("ERROR: Konnte ");
          Serial.print(parsed);
          Serial.print(" von "); Serial.print(MAX_CHANNELS); Serial.print(" Werten parsen. Daten: ");
          Serial.println(inputBuffer);
        }
        
        readingActive = false; // Warten auf neues Start-Token
      } 
      else {
        // Normale Daten speichern
        if (bufferIndex < BUFFER_SIZE - 1) {
          inputBuffer[bufferIndex++] = incoming;
        } else {
          // Pufferüberlauf: Nachricht ist zu lang, Puffer verwerfen
          Serial.println("ERROR: Buffer Overflow! Daten verworfen und Synchronisation verloren.");
          readingActive = false; 
        }
      }
    }
  }
}

void loop() {
  readSerialAndDebug();
  // Keine motor-Aktionen
}