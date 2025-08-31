#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>

// Create MIDI instance
MIDI_CREATE_DEFAULT_INSTANCE();

// ================= KICK DRUM AUDIO OBJECTS =================
AudioSynthWaveformSine   kickSineMod;
AudioEffectEnvelope      kickModEnv;
AudioMixer4              kickSineModMxr;
AudioSynthNoiseWhite     kickNoise;
AudioSynthWaveformSineModulated kickSineFM;
AudioEffectEnvelope      kickNoiseEnvelope;
AudioMixer4              kickMxr;
AudioEffectEnvelope      kickEnv;
AudioFilterStateVariable kickLowPassFilter;

// ================= SNARE DRUM AUDIO OBJECTS =================
AudioSynthWaveform       snareBodySquare5th;
AudioSynthWaveform       snareBodySine5th;
AudioSynthWaveformSine   snareBodyFMSineModulator;
AudioMixer4              snareBodyFMMxr;
AudioSynthWaveformSineModulated snareBodyFMSine;
AudioSynthNoiseWhite     snareSnapNoise;
AudioMixer4              snareBodyMxr;
AudioEffectEnvelope      snareSnapEnv;
AudioEffectEnvelope      snareBodyEnv;
AudioMixer4              snareMxr;
AudioFilterStateVariable snareHPFilter;

// ================= OUTPUT =================
AudioOutputI2S           dacOut;
AudioControlSGTL5000     audioControl;

// ================= AUDIO CONNECTIONS =================
// Kick connections
AudioConnection          patchCord1(kickSineMod, kickModEnv);
AudioConnection          patchCord2(kickSineMod, 0, kickSineModMxr, 0);
AudioConnection          patchCord3(kickModEnv, 0, kickSineModMxr, 1);
AudioConnection          patchCord4(kickSineModMxr, kickSineFM);
AudioConnection          patchCord5(kickNoise, kickNoiseEnvelope);
AudioConnection          patchCord6(kickSineFM, kickEnv);
AudioConnection          patchCord7(kickNoiseEnvelope, 0, kickMxr, 1);
AudioConnection          patchCord8(kickEnv, 0, kickMxr, 0);
AudioConnection          patchCord9(kickMxr, 0, kickLowPassFilter, 0);

// Snare connections
AudioConnection          patchCord10(snareBodySquare5th, 0, snareBodyFMMxr, 1);
AudioConnection          patchCord11(snareBodySine5th, 0, snareBodyFMMxr, 0);
AudioConnection          patchCord12(snareBodyFMSineModulator, snareBodyFMSine);
AudioConnection          patchCord13(snareBodyFMMxr, 0, snareBodyMxr, 1);
AudioConnection          patchCord14(snareBodyFMSine, 0, snareBodyMxr, 0);
AudioConnection          patchCord15(snareSnapNoise, snareSnapEnv);
AudioConnection          patchCord16(snareBodyMxr, snareBodyEnv);
AudioConnection          patchCord17(snareSnapEnv, 0, snareMxr, 1);
AudioConnection          patchCord18(snareBodyEnv, 0, snareMxr, 0);
AudioConnection          patchCord19(snareMxr, 0, snareHPFilter, 0);

// Output connections
AudioConnection          patchCord20(kickLowPassFilter, 0, dacOut, 0);
AudioConnection          patchCord21(snareHPFilter, 2, dacOut, 1);

// ================= MIDI CONFIGURATION =================
const byte MIDI_CHANNEL = 1;
const byte KICK_NOTE = 53;
const byte SNARE_NOTE = 55;

// ================= POTENTIOMETER PINS =================
const int decayPotPin = A0;
const int pitchPotPin = A1;
const int punchPotPin = A2;
const int pitchDropTimePotPin = A3;
const int snapPotPin = A4;
const int timbrePotPin = A5;

// ================= KICK PARAMETERS =================
float kickBaseFreq = 55.0;
float kickInitialPitchBoost = 2.0;
float kickNoiseLevel = 0.0;
unsigned long kickPitchDropStartTime = 0;
unsigned long kickPitchDropDuration = 30;

// ================= SNARE PARAMETERS =================
float snareBaseFreq = 180.0;
float snareInitialPitchBoost = 3.0;
unsigned long snarePitchDropStartTime = 0;
const unsigned long snarePitchDropDuration = 20;

void setup() {
  // Initialize potentiometer pins
  pinMode(decayPotPin, INPUT);
  pinMode(pitchPotPin, INPUT);
  pinMode(punchPotPin, INPUT);
  pinMode(pitchDropTimePotPin, INPUT);
  pinMode(snapPotPin, INPUT);
  pinMode(timbrePotPin, INPUT);
  
  // Initialize MIDI
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  AudioMemory(256);
  audioControl.enable();
  audioControl.volume(0.8);
  
  // ================= CONFIGURE KICK =================
  kickSineFM.amplitude(0.7);
  kickSineFM.frequency(kickBaseFreq);
  updateKickModulatorFrequency();
  kickSineMod.amplitude(0.7);
  kickNoise.amplitude(0.7);
  kickLowPassFilter.frequency(2000);
  kickLowPassFilter.resonance(0);
  kickSineModMxr.gain(0, 0.7);
  kickSineModMxr.gain(1, 1.0);
  kickSineModMxr.gain(2, 0.0);
  kickSineModMxr.gain(3, 0.0);
  kickMxr.gain(0, 1.0);
  kickMxr.gain(1, 0.0);
  kickMxr.gain(2, 0.0);
  kickMxr.gain(3, 0.0);
  kickEnv.attack(1);
  kickEnv.decay(150);
  kickEnv.sustain(0);
  kickEnv.release(50);
  kickModEnv.attack(2);
  kickModEnv.decay(50);
  kickModEnv.sustain(0);
  kickModEnv.release(20);
  kickNoiseEnvelope.attack(2);
  kickNoiseEnvelope.decay(10);
  kickNoiseEnvelope.sustain(0);
  kickNoiseEnvelope.release(0);

  // ================= CONFIGURE SNARE =================
  snareBodySine5th.begin(WAVEFORM_SINE);
  snareBodySquare5th.begin(WAVEFORM_SQUARE);
  updateSnareOscillatorFrequencies(snareBaseFreq);
  snareBodyFMSine.amplitude(0.7);
  snareBodySine5th.amplitude(0.5);
  snareBodySquare5th.amplitude(0.5);
  snareBodyFMSineModulator.amplitude(0.5);
  snareSnapNoise.amplitude(0.5);
  snareHPFilter.frequency(80);
  snareHPFilter.resonance(0);
  snareBodyFMMxr.gain(0, 0.3);
  snareBodyFMMxr.gain(1, 0.3);
  snareBodyFMMxr.gain(2, 0.0);
  snareBodyFMMxr.gain(3, 0.0);
  snareBodyMxr.gain(0, 0.5);
  snareBodyMxr.gain(1, 0.3);
  snareBodyMxr.gain(2, 0.0);
  snareBodyMxr.gain(3, 0.0);
  snareMxr.gain(0, 0.5);
  snareMxr.gain(1, 0.5);
  snareMxr.gain(2, 0.0);
  snareMxr.gain(3, 0.0);
  snareBodyEnv.attack(1);
  snareBodyEnv.decay(200);
  snareBodyEnv.sustain(0);
  snareBodyEnv.release(40);
  snareSnapEnv.attack(1);
  snareSnapEnv.decay(100);
  snareSnapEnv.sustain(0);
  snareSnapEnv.release(40);
  snareSnapEnv.delay(5);

  Serial.begin(115200);
  Serial.println("FM Drum Machine with Kick and Snare Ready");
}

void loop() {
  MIDI.read();
  readPotsAndUpdate();
  updateKickPitchDrop();
  updateSnarePitchDrop();
  delay(1);
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (channel == MIDI_CHANNEL) {
    if (note == KICK_NOTE) {
      triggerKick();
      Serial.print("Kick Triggered via MIDI: Note ");
      Serial.println(note);
    } else if (note == SNARE_NOTE) {
      triggerSnare();
      Serial.print("Snare Triggered via MIDI: Note ");
      Serial.println(note);
    }
  }
}

void triggerKick() {
  kickPitchDropStartTime = millis();
  float initialFreq = kickBaseFreq * kickInitialPitchBoost;
  kickSineFM.frequency(initialFreq);
  updateKickModulatorFrequency();
  kickEnv.noteOn();
  kickModEnv.noteOn();
  kickNoiseEnvelope.noteOn();
}

void triggerSnare() {
  snarePitchDropStartTime = millis();
  float initialFreq = snareBaseFreq * snareInitialPitchBoost;
  updateSnareOscillatorFrequencies(initialFreq);
  snareBodyEnv.noteOn();
  snareSnapEnv.noteOn();
}

void readPotsAndUpdate() {
  // Read all pots
  int pitchVal = analogRead(pitchPotPin);
  int decayVal = analogRead(decayPotPin);
  int punchVal = analogRead(punchPotPin);
  int pitchDropTimeVal = analogRead(pitchDropTimePotPin);
  int snapVal = analogRead(snapPotPin);
  int timbreVal = analogRead(timbrePotPin);
  
  // Update kick parameters
  kickBaseFreq = map(pitchVal, 0, 1023, 30, 120);
  float kickDecayTime = map(decayVal, 0, 1023, 50, 700);
  kickPitchDropDuration = map(pitchDropTimeVal, 0, 1023, 35, 700);
  kickInitialPitchBoost = map(punchVal, 0, 1023, 10, 60) / 10.0;
  kickNoiseLevel = map(punchVal, 0, 1023, 0, 100) / 100.0;
  
  kickEnv.decay(kickDecayTime);
  kickMxr.gain(1, kickNoiseLevel);
  updateKickModulatorFrequency();
  
  // Update snare parameters
  snareBaseFreq = map(pitchVal, 0, 1023, 150, 600);
  float snareBodyDecayTime = map(decayVal, 0, 1023, 50, 400);
  float snareSnapDecayTime = map(decayVal, 0, 1023, 40, 380);
  snareInitialPitchBoost = map(punchVal, 0, 1023, 20, 80) / 10.0;
  float snapLevel = map(snapVal, 0, 1023, 0, 120) / 100.0;
  float timbreMix = map(timbreVal, 0, 1023, 0, 100) / 100.0;
  
  snareBodyEnv.decay(snareBodyDecayTime);
  snareSnapEnv.decay(snareSnapDecayTime);
  snareMxr.gain(1, snapLevel);
  snareBodyFMMxr.gain(0, 0.3 * (1.0 - timbreMix));
  snareBodyFMMxr.gain(1, 0.3 * timbreMix);
  
  if (snarePitchDropStartTime == 0) {
    updateSnareOscillatorFrequencies(snareBaseFreq);
  }
}

void updateKickModulatorFrequency() {
  float modulatorFreq = kickBaseFreq * 1.5;
  kickSineMod.frequency(modulatorFreq);
}

void updateSnareOscillatorFrequencies(float freq) {
  float fifthFreq = freq * 1.5;
  snareBodySine5th.frequency(fifthFreq);
  snareBodySquare5th.frequency(fifthFreq);
  snareBodyFMSine.frequency(freq);
  float octaveMinorThirdFreq = freq * 2.4;
  snareBodyFMSineModulator.frequency(octaveMinorThirdFreq);
}

void updateKickPitchDrop() {
  if (kickPitchDropStartTime > 0) {
    unsigned long elapsed = millis() - kickPitchDropStartTime;
    if (elapsed < kickPitchDropDuration) {
      float progress = (float)elapsed / kickPitchDropDuration;
      float freqMultiplier = kickInitialPitchBoost - (progress * (kickInitialPitchBoost - 1.0));
      kickSineFM.frequency(kickBaseFreq * freqMultiplier);
      updateKickModulatorFrequency();
    } else {
      kickSineFM.frequency(kickBaseFreq);
      updateKickModulatorFrequency();
      kickPitchDropStartTime = 0;
    }
  }
}

void updateSnarePitchDrop() {
  if (snarePitchDropStartTime > 0) {
    unsigned long elapsed = millis() - snarePitchDropStartTime;
    if (elapsed < snarePitchDropDuration) {
      float progress = (float)elapsed / snarePitchDropDuration;
      float freqMultiplier = snareInitialPitchBoost - (progress * (snareInitialPitchBoost - 1.0));
      updateSnareOscillatorFrequencies(snareBaseFreq * freqMultiplier);
    } else {
      updateSnareOscillatorFrequencies(snareBaseFreq);
      snarePitchDropStartTime = 0;
    }
  }
}