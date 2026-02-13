#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// ================= SETTINGS =================

#define MAXTRACKS 4
#define BLOCKS    120

const char* trackNames[] =
  {"a.raw","b.raw","c.raw","d.raw"};

// ============================================

// ---------- AUDIO OBJECTS ----------

AudioInputI2S        i2s2;
AudioRecordQueue     queue1;
AudioPlaySdRaw       players[MAXTRACKS];
AudioMixer4          mixer1;
AudioOutputI2S       i2s1;
AudioControlSGTL5000 sgtl5000_1;

AudioConnection* patchPlayer[MAXTRACKS];
AudioConnection  patchRec(i2s2, 0, queue1, 0);
AudioConnection  patchOutL(mixer1, 0, i2s1, 0);
AudioConnection  patchOutR(mixer1, 0, i2s1, 1);

// ---------- UI ----------

Bounce buttonLoop  = Bounce(0, 8);
Bounce buttonUndo  = Bounce(1, 8);

File frec;

int  trackCount = 0;          // number of active tracks
int  currentRecording = -1;   // index of track being recorded
bool recording = false;

unsigned long loopLengthBytes = 0;

// ============================================

void setup() {

  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);

  AudioMemory(BLOCKS);

  for (int i=0; i<MAXTRACKS; i++)
    patchPlayer[i] =
      new AudioConnection(players[i], 0, mixer1, i);

  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000_1.volume(0.6);
  sgtl5000_1.micGain(20);

  SPI.setMOSI(7);
  SPI.setSCK(14);

  if (!SD.begin(10)) {
    while (1);
  }

  for (int i=0;i<MAXTRACKS;i++)
    mixer1.gain(i, 1.0/MAXTRACKS);
}

// ============================================

void loop() {

  buttonLoop.update();
  buttonUndo.update();

  if (buttonLoop.fallingEdge())
    mainButton();

  if (buttonUndo.fallingEdge())
    undoTrack();

  if (recording)
    continueRecording();

  // Independent looping
  for (int i=0; i<trackCount; i++)
    checkLoop(i);
}

// ============================================
// ============= MAIN BUTTON ==================
// ============================================

void mainButton() {

  // Prevent overflow
  if (!recording && trackCount >= MAXTRACKS)
    return;

  if (!recording) {
    startRecording(trackCount);
  }
  else {
    stopRecording();

    // First track defines loop length
    if (trackCount == 0) {
      File f = SD.open(trackNames[0]);
      loopLengthBytes = f.size();
      f.close();
    }

    startPlaying(trackCount);
    trackCount++;
  }
}

// ============================================
// ============= RECORDING ====================
// ============================================

void startRecording(int index) {

  currentRecording = index;

  const char* name = trackNames[index];

  if (SD.exists(name))
    SD.remove(name);

  frec = SD.open(name, FILE_WRITE);
  if (!frec) return;

  queue1.begin();
  recording = true;
}

// --------------------------------------------

void continueRecording() {

  if (queue1.available() >= 2) {

    byte buffer[512];

    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();

    memcpy(buffer+256, queue1.readBuffer(), 256);
    queue1.freeBuffer();

    frec.write(buffer, 512);

    // For overdubs: auto-stop at loop length
    if (trackCount > 0 &&
        frec.size() >= loopLengthBytes) {

      stopRecording();
      startPlaying(currentRecording);
      trackCount++;
    }
  }
}

// --------------------------------------------

void stopRecording() {

  queue1.end();

  while (queue1.available()) {
    frec.write((byte*)queue1.readBuffer(),256);
    queue1.freeBuffer();
  }

  frec.close();
  recording = false;
  currentRecording = -1;
}

// ============================================
// ============= PLAYBACK =====================
// ============================================

void startPlaying(int i) {
  players[i].play(trackNames[i]);
}

// --------------------------------------------

void stopPlaying(int i) {
  players[i].stop();
}

// --------------------------------------------

void undoTrack() {

  if (trackCount == 0) return;

  trackCount--;
  stopPlaying(trackCount);

  SD.remove(trackNames[trackCount]);
}

// --------------------------------------------

void checkLoop(int i) {

  if (!players[i].isPlaying()) {
    players[i].stop();
    players[i].play(trackNames[i]);
  }
}
