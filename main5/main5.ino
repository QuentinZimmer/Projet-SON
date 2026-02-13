#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// ================= USER SETTINGS =================
#define MAXTRACKS 4
#define BLOCKS    160

const char* trackNames[] =
{"a.raw","b.raw","c.raw","d.raw"};

// =================================================

// ----- AUDIO OBJECTS -----
AudioInputI2S       i2s2;
AudioRecordQueue    queue1;

AudioPlaySdRaw      players[MAXTRACKS];
AudioMixer4         mixer1;

AudioOutputI2S      i2s1;
AudioControlSGTL5000 sgtl5000_1;

// Audio routing
AudioConnection* patchPlayer[MAXTRACKS];
AudioConnection  patchOutL(mixer1, 0, i2s1, 0);
AudioConnection  patchOutR(mixer1, 0, i2s1, 1);
AudioConnection  patchRec(i2s2, 0, queue1, 0);

// ----- UI -----
Bounce buttonLoop  = Bounce(0, 8);
Bounce buttonUndo  = Bounce(1, 8);

// ----- GLOBALS -----
File frec;

int  trackNumber = 0;
bool recording   = false;
bool playing[MAXTRACKS];

unsigned long loopLengthBytes = 0;   // reference loop size
float analogDiv = 1.0/1023.0;

// =================================================

void setup() {

  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);

  AudioMemory(BLOCKS);

  for (int i=0; i<MAXTRACKS; i++) {
    patchPlayer[i] =
      new AudioConnection(players[i], 0, mixer1, i);
  }

  for (int i=0;i<MAXTRACKS;i++)
    mixer1.gain(i, 1.0/MAXTRACKS);

  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000_1.volume(0.5);
  sgtl5000_1.micGain(20);

  SPI.setMOSI(7);
  SPI.setSCK(14);

  if (!SD.begin(10)) {
    while (1);
  }

  for (int i=0; i<MAXTRACKS; i++)
    playing[i] = false;
}

// =================================================

void loop() {

  buttonLoop.update();
  buttonUndo.update();

  if (buttonLoop.fallingEdge())
    mainButton();

  if (buttonUndo.fallingEdge())
    undoTrack();

  if (recording)
    continueRecording();

  // independent looping
  for (int i=0; i<trackNumber; i++) {
    if (playing[i] && !players[i].isPlaying()) {
      players[i].play(trackNames[i]);
    }
  }

  setVolume();
  setGain();
}

// =================================================
// ================= MAIN LOGIC ====================
// =================================================

void mainButton() {

  // ---- FIRST TRACK ----
  if (trackNumber == 0) {

    if (!recording) {
      startRecording(0);
    }
    else {
      stopRecording();

      // First track defines loop size
      File f = SD.open(trackNames[0]);
      loopLengthBytes = f.size();
      f.close();

      startPlaying(0);
      trackNumber = 1;
    }
  }

  // ---- OVERDUB ----
  else {

    if (!recording && trackNumber < MAXTRACKS) {
      startRecording(trackNumber);
    }
    else if (recording) {
      stopRecording();
      startPlaying(trackNumber);
      trackNumber++;
    }
  }
}

// =================================================

void undoTrack() {

  if (trackNumber > 0) {
    trackNumber--;
    players[trackNumber].stop();
    playing[trackNumber] = false;
  }
}

// =================================================
// ================= RECORDING =====================
// =================================================

void startRecording(int index) {

  if (SD.exists(trackNames[index]))
    SD.remove(trackNames[index]);

  frec = SD.open(trackNames[index], FILE_WRITE);

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

    // Stop overdub when reaching loop length
    if (trackNumber > 0 &&
        frec.size() >= loopLengthBytes) {

      stopRecording();
      startPlaying(trackNumber);
      trackNumber++;
    }
  }
}

// -------------------------------------------------

void stopRecording() {

  queue1.end();

  while (queue1.available()) {
    frec.write((byte*)queue1.readBuffer(),256);
    queue1.freeBuffer();
  }

  frec.close();
  recording = false;
}

// =================================================
// ================= PLAYBACK ======================
// =================================================

void startPlaying(int i) {
  players[i].play(trackNames[i]);
  playing[i] = true;
}

// =================================================

void setGain() {
  float readGain = analogRead(A2)*analogDiv;
  sgtl5000_1.micGain(readGain * 63); // correct range
}

void setVolume() {
  float readVolume = analogRead(A0)*analogDiv;
  sgtl5000_1.volume(readVolume);
}
