#include <Audio.h>
#include <SD.h>
#include <SPI.h>

// ----- Audio objects -----
AudioInputI2S         i2s1;
AudioRecordQueue      queue1;
AudioConnection       patchCord1(i2s1, 0, queue1, 0);
AudioControlSGTL5000  sgtl5000;

File frec;
bool isRecording = false;

// ----- WAV header -----
void writeWavHeader(File &file, uint32_t sampleRate, uint16_t bits, uint16_t channels) {
  file.seek(0);

  file.write("RIFF", 4);
  uint32_t chunkSize = 0;
  file.write((byte *)&chunkSize, 4);

  file.write("WAVE", 4);
  file.write("fmt ", 4);

  uint32_t subchunk1Size = 16;
  uint16_t audioFormat = 1;
  file.write((byte *)&subchunk1Size, 4);
  file.write((byte *)&audioFormat, 2);
  file.write((byte *)&channels, 2);
  file.write((byte *)&sampleRate, 4);

  uint32_t byteRate = sampleRate * channels * bits / 8;
  uint16_t blockAlign = channels * bits / 8;

  file.write((byte *)&byteRate, 4);
  file.write((byte *)&blockAlign, 2);
  file.write((byte *)&bits, 2);

  file.write("data", 4);
  uint32_t dataSize = 0;
  file.write((byte *)&dataSize, 4);
}

// ----- Control -----
void startRecording(const char *filename) {
  frec = SD.open(filename, FILE_WRITE);
  if (!frec) return;

  writeWavHeader(frec, 44100, 16, 1);

  queue1.begin();
  isRecording = true;
}

void stopRecording() {
  queue1.end();

  uint32_t dataSize = frec.size() - 44;
  uint32_t riffSize = frec.size() - 8;

  frec.seek(4);
  frec.write((byte *)&riffSize, 4);

  frec.seek(40);
  frec.write((byte *)&dataSize, 4);

  frec.close();
  isRecording = false;
}

void continueRecording() {
  if (!isRecording) return;

  while (queue1.available() > 0) {
    int16_t *buffer = queue1.readBuffer();
    frec.write((byte *)buffer, 256 * 2);
    queue1.freeBuffer();
  }
}

// ----- Arduino -----
void setup() {
  Serial.begin(9600);

  AudioMemory(60);
  sgtl5000.enable();
  sgtl5000.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000.micGain(36);

  SD.begin(BUILTIN_SDCARD);

  startRecording("TEST.WAV");
}

void loop() {
  continueRecording();

  // Example: stop after 10 seconds
  if (millis() > 10000 && isRecording) {
    stopRecording();
  }
}
