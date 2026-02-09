#include <Audio.h>
#include "MyDsp.h"

MyDsp myDsp;
AudioInputI2S in;
AudioOutputI2S out;
AudioControlSGTL5000 audioShield;
AudioConnection patchCord0(myDsp,0,out,0);
AudioConnection patchCord1(myDsp,0,out,1);


unsigned long lastPulse = 0;
int note = 1;
void setup() {
  AudioMemory(2);
  audioShield.enable();
  
  audioShield.volume(0.5);
  
  Serial.begin(9600);
}


float setDelay_() {
  float bpm = analogRead(A0); // entre 0 et 1023
  
  bpm = bpm/4 + 20; // 20 et ~260
  
  float delay_ = 60/bpm;
  
  return delay_;
}

void loop() {
  
  
  unsigned long now = millis();
  
  
  if ((now - lastPulse) >= setDelay_()*1000) {
    lastPulse = now;
    Serial.println("Tempo");
    if (note == 1){
      myDsp.setFreq(440);
      note = 0;
    }else{
      myDsp.setFreq(300);
      note = 1;
    }

    
  }
}