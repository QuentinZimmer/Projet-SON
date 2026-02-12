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
bool playing     = false;

unsigned long loopLengthBytes = 0;   // reference length

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

  // Equal levels in mixer
  for (int i=0;i<MAXTRACKS;i++)
    mixer1.gain(i, 1.0/MAXTRACKS);
}

// =================================================

void loop() {

  buttonLoop.update();
  buttonUndo.update();
  buttonPause.update();

  if (buttonLoop.fallingEdge())
    mainButton();

  if (buttonUndo.fallingEdge())
    undoTrack();

  if (buttonPause.fallingEdge())
    togglePlay();

  if (recording)
    continueRecording();

  if (playing)
    checkLoopSync();
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

      trackNumber = 1;

      startAll();
    }
  }

  // ---- OVERDUBS ----
  else {

    if (!recording) {
      startRecording(trackNumber);
    }
    else {
      stopRecording();

      trackNumber++;
      if (trackNumber > MAXTRACKS)
          trackNumber = MAXTRACKS;

      startAll();
    }
  }
}

// =================================================

void undoTrack() {

  if (trackNumber > 0) {
    trackNumber--;

    stopAll();
    startAll();
  }
}

// =================================================

void togglePlay() {
  if (playing) stopAll();
  else         startAll();
}

// =================================================
// ========== RECORDING ============================
// =================================================

void startRecording(int index) {

  const char* name = trackNames[index];

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
    if (trackNumber > 0 &&
        frec.size() >= loopLengthBytes) {

      stopRecording();
      trackNumber++;
      startAll();
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
// ============ PLAYBACK ===========================
// =================================================

void startAll() {

  for (int i=0; i<trackNumber; i++)
    players[i].play(trackNames[i]);

  playing = true;
}

// -------------------------------------------------

void stopAll() {

  for (int i=0; i<MAXTRACKS; i++)
    players[i].stop();

  playing = false;
}

// -------------------------------------------------

// Keep all tracks in sync
void checkLoopSync() {

  // If ANY track finished → restart all
  for (int i=0; i<trackNumber; i++) {

    if (!players[i].isPlaying()) {

      startAll();     // global restart
      break;
    }
  }
}
