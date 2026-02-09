#include <Audio.h>

AudioOutputI2S out;
AudioControlSGTL5000 audioShield;

// state variables :

//float looplenght = 0.0; // duree de la boucle
//float gain = 0.0;
//float volume = 0.0; 

int recording = 0; // 1 if recording, 0 if not
int existingFile = 0; // same logic

void setup() {
  pinMode(0, INPUT); // configuring digital pin 0 as an input
  AudioMemory(2);
  audioShield.enable();
  audioShield.volume(0.5);
  Serial.begin(9600);
}

void StartStopButton() {
    
    if (existingFile==0) {
        if (recording==0) {
            recording = 1;
            Serial.println("recording");
            // start recording, start chrono for looplength
        }
        else if (recording==1) {
            recording = 0;
            Serial.println("Saving");
            // stops recording, stops chrono, save audio file, starts playing saved audio file 
            existingFile = 1;
        }
    }

    else if (existingFile==1) {
        // stops loop, discards existing file
        existingFile = 0;
        Serial.println("Stoping");
    }
}

void loop() {
    // start/stop metronome 

    // start/stop button 1
    if (digitalRead(0)) {
        StartStopButton();
        }

    // set gain

    // set volume
}


















