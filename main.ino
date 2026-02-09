#include <Audio.h>

AudioOutputI2S out;
AudioControlSGTL5000 audioShield;

void setup() {
  pinMode(0, INPUT); // configuring digital pin 0 as an input
  AudioMemory(2);
  audioShield.enable();
  audioShield.volume(0.5);
  Serial.begin(9600);

  // state variables :

  float looplenght = 0.0; // duree de la boucle
  float gain = 0.0;
  float volume = 0.0; 

  int recording = 0; // 1 if recording, 0 if not
  int existingFile = 0; // same logic
  }

}

void StartStopButton() {
    
    if (existingFile==0) {
        if (recording==0) {
            recording = 1;
            // start recording, start chrono for looplength
        }
        if (recording==1) {
            recording = 0; 
            // stops recording, stops chrono, save audio file, starts playing saved audio file 
        }
    }

    if (existingFile==1) {
        // stops loop, discards existing file
        existingFile = 0;
    }
}

void loop() {
    // start/stop metronome 

    // start/stop button 1

    // set gain

    // set volume
}