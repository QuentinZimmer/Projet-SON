#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// ================= USER SETTINGS =================

#define MAXTRACKS 4        // <---- CHANGE HERE
#define BLOCKS    120      // more memory for multitrack

const char* trackNames[] =
  {"a.raw","b.raw","c.raw","d.raw","e.raw","f.raw"};

// =================================================

// ----- AUDIO OBJECTS -----

AudioInputI2S       i2s2;
AudioRecordQueue    queue1;

AudioPlaySdRaw      players[MAXTRACKS];
AudioMixer4         mixer1;

AudioOutputI2S      i2s1;
AudioControlSGTL5000 sgtl5000_1;

// Dynamic connections
AudioConnection* patchPlayer[MAXTRACKS];
AudioConnection  patchOutL(mixer1, 0, i2s1, 0);
AudioConnection  patchOutR(mixer1, 0, i2s1, 1);
AudioConnection  patchRec(i2s2, 0, queue1, 0);

// ----- UI -----
Bounce buttonLoop  = Bounce(0, 8);
Bounce buttonUndo  = Bounce(1, 8);
Bounce buttonPause = Bounce(2, 8);

File frec;

int  trackNumber = 0;   // how many tracks recorded
bool recording   = false;
bool playing[MAXTRACKS];

float analogDiv = 1.0/1023.0;

unsigned long loopLengthBytes = 0;   // reference length
unsigned long tracksLength;
unsigned long tracksBeginning[MAXTRACKS];

// =================================================

void setup() {

  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);

  AudioMemory(BLOCKS);

  // Create dynamic patching
  for (int i=0; i<MAXTRACKS; i++) {
    patchPlayer[i] =
      new AudioConnection(players[i], 0, mixer1, i);
  }

  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000_1.volume(0.5);
  sgtl5000_1.micGain(20);

  SPI.setMOSI(7);
  SPI.setSCK(14);

  if (!SD.begin(10)) {
    while (1) {
      Serial.println("SD fail");
      delay(300);
    }
  }
  for (int i=0; i<MAXTRACKS; i++) playing[i]=false;

  // Equal levels in mixer
  for (int i=0;i<MAXTRACKS;i++)
    mixer1.gain(i, 1.0/MAXTRACKS);
}

// =================================================

void loop() {

  setVolume();
  setGain();

  buttonLoop.update();
  buttonUndo.update();
  buttonPause.update();

  if (buttonLoop.fallingEdge())
    mainButton();

  if (buttonUndo.fallingEdge())
    undoTrack();

  //if (buttonPause.fallingEdge())
    //togglePlay();

  if (recording)
    continueRecording();

  for (int i=0; i<trackNumber; i++) {
    checkLoopEnd(i);
  }
}

// =================================================
// ============== MAIN LOGIC =======================
// =================================================

void mainButton() {

  // ---- FIRST TRACK ----
  if (trackNumber == 0) {

    if (!recording) {
      startRecording(0);
    }
    else {
      stopRecording();

      // first track defines loop length
      loopLengthBytes =
        SD.open(trackNames[0]).size();

      startPlaying(0);
      trackNumber = 1;
    }
  }

  // ---- OVERDUBS ----
  else {

    if (!recording) {
      startRecording(trackNumber);
    }
    else {
      stopRecording();
      startPlaying(trackNumber);
      trackNumber++;

      if (trackNumber > MAXTRACKS)
          trackNumber = MAXTRACKS;
    }
  }
}

// =================================================

void undoTrack() {

  if (trackNumber > 0) {
    stopPlaying(trackNumber);
    trackNumber--;
  }
}

// =================================================
/*
void togglePlay() {
  if (playing) stopAll();
  else         startAll();
}
*/
// =================================================
// ========== RECORDING ============================
// =================================================

void startRecording(int index) {

  const char* name = trackNames[index];
  tracksBeginning[index]= micros();

  if (SD.exists(name))
    SD.remove(name);

  frec = SD.open(name, FILE_WRITE);

  queue1.begin();
  recording = true;
}

// -------------------------------------------------

void continueRecording() {

  if (queue1.available() >= 2) {

    byte buffer[512];

    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();

    memcpy(buffer+256, queue1.readBuffer(), 256);
    queue1.freeBuffer();

    frec.write(buffer, 512);

    // --- LIMIT LENGTH TO LOOP SIZE ---
    /*
    if (trackNumber > 0 && frec.size() >= loopLengthBytes) {
      stopRecoring();
    }
    */
    Serial.println(micros() - tracksBeginning[trackNumber]);
    Serial.println(tracksLength);
    if (micros() - tracksBeginning[trackNumber] > tracksLength && trackNumber>0) {
      stopRecording();
      startPlaying(trackNumber);
      trackNumber++;
    }
    
  }
}

// -------------------------------------------------

void stopRecording() {

  if (trackNumber==0) tracksLength = micros() - tracksBeginning[trackNumber];
  
  queue1.end();

  while (queue1.available()) {
    frec.write((byte*)queue1.readBuffer(),256);
    queue1.freeBuffer();
  }

  frec.close();
  recording = false;
  
}

// =================================================
// ============ PLAYBACK ===========================
// =================================================

void startPlaying(int i) {
  players[i].play(trackNames[i]);
  playing[i] = true;
  tracksBeginning[i] = micros();
}

// -------------------------------------------------

void stopAll() {

  for (int i=0; i<MAXTRACKS; i++){
    players[i].stop();
    playing[i] = false;
  }
}

void stopPlaying(int i) {
  players[i].stop();
  playing[i] = false;
}

void checkLoopEnd(int i) {
  if (micros()>tracksBeginning[i]+tracksLength) {
    startPlaying(i);
  }
}

// -------------------------------------------------


void setGain() {
  // TODO: read the peak1 object and adjust sgtl5000_1.micGain()
  // if anyone gets this working, please submit a github pull request :-)
  float readGain = analogRead(A2)*analogDiv;
  sgtl5000_1.micGain(readGain);
}

void setVolume() {
  float readVolume = analogRead(A0)*analogDiv;
  sgtl5000_1.volume(readVolume);
}
