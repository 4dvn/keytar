#include "Arduino.h"
typedef struct {
  unsigned long timeStart;
  unsigned long timeEnd;
  int pitch;
  int velocity;
} note;

const int delta = 20;
const int on = 144;
const int off = 128;
const int octave = 12;
const int lowerOctaveButton = 1;
const int raiseOctaveButton = 2;
const int recordButton = 3;
const int playbackButton = 4;
const int buttonDelayTime = 200;
int velocity = 127;
int keySensorLevels[12];
int keyDefault[12];
boolean keyPressed[12];
note notes[60];
int tempRecordingIndices[120];
int nextIndex = 0;
unsigned long recordStartTime;
unsigned long keyPressAtTime[120];
unsigned long playbackStartTime;
unsigned long timeElapsed;
boolean recording = false;

int transpose = 48; //Lowest = C4
boolean recordingPlayback = false;
boolean recordingAvailable = false;
boolean isPlayingBack = false;
boolean justCalibrated = false;

void blinkThrice() {
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    delay(300);
  }
  digitalWrite(13, HIGH);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  for (int i = 0; i < 12; i++)
  {
    keyPressed[i] = false;
    keyDefault[i] = analogRead(i);
  }
  for (int i = 0; i < 120; i++)
  {
    tempRecordingIndices[i] = -1;
    keyPressAtTime[i] = -1;
  }
  for (int i = 0; i < 60; i++)
  {
    notes[i].velocity = -1;
  }
  pinMode(13, OUTPUT);
  pinMode(lowerOctaveButton, INPUT);
  pinMode(raiseOctaveButton, INPUT);
  pinMode(recordButton, INPUT);
  blinkThrice();
}


void noteOn(int pitch) {
  Serial.write(on);
  Serial.write(pitch);
  Serial.write(velocity);
}
void noteOff(int pitch) {
  Serial.write(off);
  Serial.write(pitch);
  Serial.write(velocity);
}

void play() {
  for (int i = 0; i < 12; i++)
  {
    keySensorLevels[i] = analogRead(i);
    int diff = keyDefault[i] - keySensorLevels[i];
    if (diff > delta)
    {
      if (!keyPressed[i])
      {
        keyPressed[i] = true;
        noteOn(i + transpose);
      }
    }
    else
    {
      if (keyPressed[i])
      {
        keyPressed[i] = false;
        noteOff(i + transpose);
      }
    }
  }
}
void detectLowerOctave() {
  if (digitalRead(lowerOctaveButton) == HIGH && transpose - octave >= 12)
  {
    transpose -= octave;
    delay(buttonDelayTime);
  }
}
void detectRaiseOctave() {
  if (digitalRead(raiseOctaveButton) == HIGH && transpose + octave <= 96)
  {
    transpose += octave;
    delay(buttonDelayTime);
  }
}
void record() {
  recordStartTime = millis();
  for (int i = 0; i < 12; i++)
  {
    keySensorLevels[i] = analogRead(i);
    int diff = keyDefault[i] - keySensorLevels[i];
    if (diff > delta)
    {
      if (!keyPressed[i])
      {
        keyPressed[i] = true;
        int pitch = i + transpose;
        notes[nextIndex].timeStart = millis();
        keyPressAtTime[pitch] = notes[nextIndex].timeStart;
        notes[nextIndex].pitch = pitch;
        notes[nextIndex].velocity = velocity;
        tempRecordingIndices[pitch] = nextIndex;
        noteOn(pitch);
        if (nextIndex + 1 < 60)
        {
          nextIndex++;
        }
        else
        {
          i = 12;
        }
      }
    }
    else
    {
      if (keyPressed[i])
      {
        keyPressed[i] = false;
        int pitch = i + transpose;
        notes[tempRecordingIndices[pitch]].timeEnd = millis();
        noteOff(pitch);
      }
    }
  }
  recordingAvailable = true;
}
void detectRecord() {
  if (digitalRead(recordButton) == HIGH)
  {
    delay(buttonDelayTime);
    if (recording)
    {
      recordingAvailable = true;
    }
    recording = !recording;
  }
}
void playback() {
  playbackStartTime = millis();
  int backIndex = 0;
  int frontIndex = 0;
  while (notes[frontIndex].velocity != -1 && frontIndex < 60)
  {
    timeElapsed = millis() - playbackStartTime;
    for (int i = backIndex; i <= frontIndex; i++)
    {
      if (notes[i].timeStart - recordStartTime >= timeElapsed)
      {
        noteOn(notes[i].pitch);
        frontIndex++;
      }
      if (notes[i].timeEnd - recordStartTime >= timeElapsed)
      {
        noteOff(notes[i].pitch);
        if (i == backIndex)
        {
          backIndex++;
        }
      }
    }
  }
  isPlayingBack = false;
}
void detectPlayback() {
  if (recordingAvailable && digitalRead(playbackButton) == HIGH)
  {
    delay(buttonDelayTime);
    playback();
  }
}

void eraseRecording() {
  recordingAvailable = false;
  for (int i = 0; i < 60; i++)
  {
    notes[i].velocity = -1;
  }
}

void detectEraseRecording() {
  unsigned long timer = millis();
  while (digitalRead(playbackButton) == HIGH)
  {
    ;
  }
  if (millis() - timer > 2000)
  {
    eraseRecording();
  }
}

void detectRecalibration() {
  unsigned long timer = millis();
  while (digitalRead(playbackButton) == HIGH && digitalRead(recordButton) == HIGH)
  {
    ;
  }
  if (millis() - timer > 4000)
  {
    for (int i = 0; i < 12; i++)
    {
      keyPressed[i] = false;
      keyDefault[i] = analogRead(i);
    }
    for (int i = 0; i < 120; i++)
    {
      tempRecordingIndices[i] = -1;
      keyPressAtTime[i] = -1;
    }
    for (int i = 0; i < 60; i++)
    {
      notes[i].velocity = -1;
    }

    blinkThrice();
    justCalibrated = true;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  detectLowerOctave();
  detectRaiseOctave();
  detectRecalibration();
  if (!justCalibrated)
  {
    detectRecord();
    detectPlayback();
  }
  if (isPlayingBack)
  {
    playback();
  }
  else
  {
    recording ? record() : play();
  }
  detectEraseRecording();
  justCalibrated = false;
  //delay(10);
}
