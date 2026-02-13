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

int  trackCount = 0;   // how many tracks recorded
int currentRec = -1;
bool recording = false;
bool playing[MAXTRACKS];

float analogDiv = 1.0/1023.0;

unsigned long loopLengthBytes = 0;   // reference length
unsigned long loopLength;
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
  for (int i=0;i<MAXTRACKS;i++) mixer1.gain(i, 1.0/MAXTRACKS);
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

  checkLoopEnd(); // relauches loops when finished
}


// =================================================
// ============== MAIN LOGIC =======================
// =================================================

void mainButton() {

  // ---- FIRST TRACK ----
  if (trackCount == 0) {

    if (!recording) {
      startRecording();
    }
    else {
      stopRecording();
      startLast();
    }
  }

  // ---- OVERDUBS ----
  else {

    if (!recording) {
      startRecording();
    }
    else {
      stopRecording();
      startLast();

      if (trackCount > MAXTRACKS)
          trackCount = MAXTRACKS;
    }
  }
}

// =================================================

void undoTrack() {

  if (trackCount > 0) {

    Serial.print("Undo");
    Serial.println(trackCount);

    players[trackCount-1].stop();
    const char* name = trackNames[trackCount-1];
    if (SD.exists(name)) SD.remove(name);

    playing[trackCount-1] = false;
    trackCount--;
  }
}

void startRecording() {

  currentRec = trackCount+1;
  recording = true;

  Serial.print("Starting recording of ");
  Serial.println(currentRec);

  const char* name = trackNames[currentRec-1]; // pcq les indices ils commencent à 0 gneugneugneu
  tracksBeginning[currentRec-1]= micros();

  if (SD.exists(name)) SD.remove(name);

  frec = SD.open(name, FILE_WRITE);

  queue1.begin();
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
    if (currentRec!=1 && currentRec!=-1) { // for secondary loops only
      if (micros() - tracksBeginning[currentRec-1] > loopLength) {
        stopRecording();
        startLast();
      }
    }
  }
}

// -------------------------------------------------

void stopRecording() {

  Serial.print("Stopping record of ");
  Serial.println(currentRec);

  if (trackCount==0) loopLength = micros() - tracksBeginning[currentRec-1]; // saving mainloop duration
  
  queue1.end();

  while (queue1.available()) {
    frec.write((byte*)queue1.readBuffer(),256);
    queue1.freeBuffer();
  }

  frec.close();

  // Update parameters
  recording = false;
  currentRec = -1;
  trackCount +=1;
}

// =================================================
// ============ PLAYBACK ===========================
// =================================================

void startLast() { // starts to play last track (previous tracks already running)
  players[trackCount-1].play(trackNames[trackCount-1]); // indices stills beginning at 0
  playing[trackCount-1] = true;
  tracksBeginning[trackCount-1] = micros();
}

void startPlaying(int i) { // starts to play last track (previous tracks already running)
  players[i].play(trackNames[i]); // indices stills beginning at 0
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

void checkLoopEnd() {
  
  for (int i=0; i<trackCount; i++) {
    if (micros()>tracksBeginning[i]+loopLength) { // loops base itself on timing only
      players[i].stop(); // safety bound
      startPlaying(i);
      Serial.print("Relauch ");
      Serial.println(i+1);
    }
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
