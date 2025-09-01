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

// ================= HI-HAT AUDIO OBJECTS =================
AudioSynthWaveformSine   hatsBodyFMSineModulator1;
AudioSynthWaveformSine   hatsBodyFMSineModulator2;
AudioMixer4              hatsBodyFMModulatorMxr;
AudioSynthWaveformModulated hatsBodySquareFMCarrier;
AudioSynthWaveform       hatsBodySquare5th;
AudioSynthNoiseWhite     hatsNoise;
AudioMixer4              hatsBodyMxr;
AudioMixer4              hatsMxr;
AudioFilterStateVariable hatsBandPass;
AudioFilterStateVariable hatsHiPass;
AudioEffectEnvelope      hatsEnv;

// ================= OUTPUT MIXERS =================
AudioMixer4              leftOutMxr;  // Mixer for DAC output channel 0 (left)
AudioMixer4              rightOutMxr; // Mixer for DAC output channel 1 (right)
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

// Hi-Hat connections
AudioConnection          patchCord20(hatsBodyFMSineModulator1, 0, hatsBodyFMModulatorMxr, 0);
AudioConnection          patchCord21(hatsBodyFMSineModulator2, 0, hatsBodyFMModulatorMxr, 1);
AudioConnection          patchCord22(hatsBodyFMModulatorMxr, 0, hatsBodySquareFMCarrier, 0);
AudioConnection          patchCord23(hatsBodySquareFMCarrier, 0, hatsBodyMxr, 0);
AudioConnection          patchCord24(hatsBodySquare5th, 0, hatsBodyMxr, 1);
AudioConnection          patchCord25(hatsNoise, 0, hatsMxr, 1);
AudioConnection          patchCord26(hatsBodyMxr, 0, hatsMxr, 0);
AudioConnection          patchCord27(hatsMxr, 0, hatsBandPass, 0);
AudioConnection          patchCord28(hatsBandPass, 1, hatsHiPass, 0);
AudioConnection          patchCord29(hatsHiPass, 2, hatsEnv, 0);

// Output connections - Separate mixers for each channel
AudioConnection          patchCord30(kickLowPassFilter, 0, leftOutMxr, 0);   // Kick to left channel
AudioConnection          patchCord31(hatsEnv, 0, leftOutMxr, 1);             // Hi-hat to left channel
AudioConnection          patchCord32(snareHPFilter, 2, rightOutMxr, 0);      // Snare to right channel

// Connect mixers to DAC outputs
AudioConnection          patchCord33(leftOutMxr, 0, dacOut, 0);              // Left mixer to left output
AudioConnection          patchCord34(rightOutMxr, 0, dacOut, 1);             // Right mixer to right output

// ================= MIDI CONFIGURATION =================
const byte MIDI_CHANNEL = 1;
const byte KICK_NOTE = 53;
const byte SNARE_NOTE = 55;
const byte CLOSED_HIHAT_NOTE = 59;
const byte OPEN_HIHAT_NOTE = 63;

// ================= POTENTIOMETER PINS =================
// Kick drum pots
const int kickDecayPotPin = A0;
const int kickTumePotPin = A1;
const int kickPunchPotPin = A2;
const int kickPitchEnvLengthPotPin = A3;

// Snare drum pots
const int snareDecayPotPin = A0;
const int snareTunePotPin = A1;
const int snarePunchPotPin = A2;
const int snareSnapPotPin = A3;
const int snareTimbrePotPin = A4;

// Hi-hat pots
const int hatsDecayPotPin = A0;
const int hatsTonePotPin = A1;
const int hatsTimbrePotPin = A2;
const int hatsResonancePotPin = A3;

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

// ================= HI-HAT PARAMETERS =================
const float hatsBaseFreq = 144.0;  // Base frequency for hi-hat body (Atonal)
float closedDecayTime = 50.0;      // Default closed hi-hat decay
float openDecayTime = 200.0;       // Default open hi-hat decay
bool isOpenHat = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing FM Drum Machine...");
  
  // Initialize potentiometer pins
  pinMode(kickDecayPotPin, INPUT);
  pinMode(kickTumePotPin, INPUT);
  pinMode(kickPunchPotPin, INPUT);
  pinMode(kickPitchEnvLengthPotPin, INPUT);
  pinMode(snareDecayPotPin, INPUT);
  pinMode(snareTunePotPin, INPUT);
  pinMode(snarePunchPotPin, INPUT);
  pinMode(snareSnapPotPin, INPUT);
  pinMode(snareTimbrePotPin, INPUT);
  pinMode(hatsDecayPotPin, INPUT);
  pinMode(hatsTonePotPin, INPUT);
  pinMode(hatsTimbrePotPin, INPUT);
  pinMode(hatsResonancePotPin, INPUT);
  
  // Initialize MIDI
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  AudioMemory(300);
  audioControl.enable();
  audioControl.volume(0.8);
  
  // Configure output mixers
  leftOutMxr.gain(0, 0.8);  // Kick level on left channel
  leftOutMxr.gain(1, 0.8);  // Hi-hat level on left channel
  leftOutMxr.gain(2, 0.0);  // Unused
  leftOutMxr.gain(3, 0.0);  // Unused
  
  rightOutMxr.gain(0, 0.8); // Snare level on right channel
  rightOutMxr.gain(1, 0.0); // Unused
  rightOutMxr.gain(2, 0.0); // Unused
  rightOutMxr.gain(3, 0.0); // Unused
  
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

  // ================= CONFIGURE HI-HAT =================
  hatsBodySquareFMCarrier.begin(WAVEFORM_SQUARE);
  hatsBodySquare5th.begin(WAVEFORM_SQUARE);
  // Set initial frequencies 
  updateHiHatOscillatorFrequencies(hatsBaseFreq);
  
  // Set initial amplitudes 
  hatsBodySquareFMCarrier.amplitude(0.7);
  hatsBodySquare5th.amplitude(0.5);
  hatsBodyFMSineModulator1.amplitude(0.5);
  hatsBodyFMSineModulator2.amplitude(0.5);
  hatsNoise.amplitude(0.5); // Increased noise level

  // Configure Filters
  hatsBandPass.frequency(5000); // Higher frequency for more hi-hat character
  hatsBandPass.resonance(0.7);
  hatsHiPass.frequency(3000);  // Higher cutoff for brighter sound
  hatsHiPass.resonance(0.5);
  
  // Configure mixers
  hatsBodyFMModulatorMxr.gain(0, 0.5);
  hatsBodyFMModulatorMxr.gain(1, 0.5);
  hatsBodyFMModulatorMxr.gain(2, 0.0);
  hatsBodyFMModulatorMxr.gain(3, 0.0);
  
  hatsBodyMxr.gain(0, 0.5);
  hatsBodyMxr.gain(1, 0.3);
  hatsBodyMxr.gain(2, 0.0);
  hatsBodyMxr.gain(3, 0.0);
  
  hatsMxr.gain(0, 0.8);
  hatsMxr.gain(1, 0.8); // Increased noise level
  hatsMxr.gain(2, 0.0);
  hatsMxr.gain(3, 0.0);
  
  // Configure envelope with initial settings
  hatsEnv.attack(1);
  hatsEnv.decay(closedDecayTime);
  hatsEnv.sustain(0);
  hatsEnv.release(5); // Shorter release for hi-hat

  Serial.println("FM Drum Machine with Kick, Snare and Hi-Hat Ready");
  Serial.println("Listening for MIDI notes:");
  Serial.println("53: Kick, 55: Snare, 59: Closed Hi-Hat, 63: Open Hi-Hat");
  Serial.println("Closed hi-hat decay: 15ms - 90ms");
  Serial.println("Open hi-hat decay: 120ms - 300ms");
  Serial.println("Output routing:");
  Serial.println("Left channel: Kick + Hi-hat");
  Serial.println("Right channel: Snare");
}

void loop() {
  MIDI.read();
  readPotsAndUpdate();
  updateKickPitchDrop();
  updateSnarePitchDrop();
  
  // Debug: Print hi-hat status occasionally
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 2000) {
    lastDebugTime = millis();
    Serial.print("Hi-hat active: ");
    Serial.println(hatsEnv.isActive() ? "YES" : "NO");
  }
  
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
    } else if (note == CLOSED_HIHAT_NOTE) {
      triggerHiHat(false); // Closed hi-hat
      Serial.print("Closed Hi-Hat Triggered via MIDI: Note ");
      Serial.println(note);
    } else if (note == OPEN_HIHAT_NOTE) {
      triggerHiHat(true); // Open hi-hat
      Serial.print("Open Hi-Hat Triggered via MIDI: Note ");
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

void triggerHiHat(bool openHat) {
  isOpenHat = openHat;
  
  // Update decay time based on whether it's an open hat or not
  float currentDecayTime = openHat ? openDecayTime : closedDecayTime;
  
  hatsEnv.decay(currentDecayTime);
  
  // Trigger envelope
  hatsEnv.noteOn();
  
  Serial.print("Hi-hat triggered: ");
  Serial.print(openHat ? "OPEN" : "CLOSED");
  Serial.print(" with decay: ");
  Serial.print(currentDecayTime);
  Serial.println("ms");
}

void readPotsAndUpdate() {
  // Read all pots
  int kickDecayVal = analogRead(kickDecayPotPin);
  int kickTuneVal = analogRead(kickTumePotPin);
  int kickPunchVal = analogRead(kickPunchPotPin);
  int kickPitchEnvLengthVal = analogRead(kickPitchEnvLengthPotPin);
  
  int snareDecayVal = analogRead(snareDecayPotPin);
  int snareTuneVal = analogRead(snareTunePotPin);
  int snarePunchVal = analogRead(snarePunchPotPin);
  int snareSnapVal = analogRead(snareSnapPotPin);
  int snareTimbreVal = analogRead(snareTimbrePotPin);
  
  int hatsDecayVal = analogRead(hatsDecayPotPin);
  int hatsToneVal = analogRead(hatsTonePotPin);
  int hatsTimbreVal = analogRead(hatsTimbrePotPin);
  int hatsResonanceVal = analogRead(hatsResonancePotPin);
  
  // Update kick parameters
  kickBaseFreq = map(kickTuneVal, 0, 1023, 30, 120);
  float kickDecayTime = map(kickDecayVal, 0, 1023, 50, 700);
  kickPitchDropDuration = map(kickPitchEnvLengthVal, 0, 1023, 35, 700);
  kickInitialPitchBoost = map(kickPunchVal, 0, 1023, 10, 60) / 10.0;
  kickNoiseLevel = map(kickPunchVal, 0, 1023, 0, 100) / 100.0;
  
  kickEnv.decay(kickDecayTime);
  kickMxr.gain(1, kickNoiseLevel);
  updateKickModulatorFrequency();
  
  // Update snare parameters
  snareBaseFreq = map(snareTuneVal, 0, 1023, 150, 600);
  float snareBodyDecayTime = map(snareDecayVal, 0, 1023, 50, 400);
  float snareSnapDecayTime = map(snareDecayVal, 0, 1023, 40, 380);
  snareInitialPitchBoost = map(snarePunchVal, 0, 1023, 20, 80) / 10.0;
  float snapLevel = map(snareSnapVal, 0, 1023, 0, 120) / 100.0;
  float timbreMix = map(snareTimbreVal, 0, 1023, 0, 100) / 100.0;
  
  snareBodyEnv.decay(snareBodyDecayTime);
  snareSnapEnv.decay(snareSnapDecayTime);
  snareMxr.gain(1, snapLevel);
  snareBodyFMMxr.gain(0, 0.3 * (1.0 - timbreMix));
  snareBodyFMMxr.gain(1, 0.3 * timbreMix);
  
  if (snarePitchDropStartTime == 0) {
    updateSnareOscillatorFrequencies(snareBaseFreq);
  }
  
  // Update hi-hat parameters
  closedDecayTime = map(hatsDecayVal, 0, 1023, 15, 90);
  openDecayTime = map(hatsDecayVal, 0, 1023, 120, 300);
  
  float currentDecayTime = isOpenHat ? openDecayTime : closedDecayTime;
  hatsEnv.decay(currentDecayTime);
  
  float filterFreq = map(hatsToneVal, 0, 1023, 2000, 10000);
  hatsBandPass.frequency(filterFreq);
  
  float modulator1Freq = hatsBaseFreq * 1.3;
  float modulator2Freq = hatsBaseFreq * 1.7;
  hatsBodyFMSineModulator1.frequency(modulator1Freq);
  hatsBodyFMSineModulator2.frequency(modulator2Freq);
  
  float noiseLevel = map(hatsTimbreVal, 0, 1023, 0, 100) / 100.0;
  hatsMxr.gain(1, noiseLevel);
  
  float resonance = map(hatsResonanceVal, 0, 1023, 100, 400) / 100.0;
  hatsBandPass.resonance(resonance);
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

void updateHiHatOscillatorFrequencies(float freq) {
  // Set the FM carrier frequency to the base frequency
  hatsBodySquareFMCarrier.frequency(freq);
  
  // Set modulator frequencies with out-of-tune ratios
  float modulator1Freq = freq * 1.3;  // Slightly detuned
  float modulator2Freq = freq * 1.7;  // More detuned
  hatsBodyFMSineModulator1.frequency(modulator1Freq);
  hatsBodyFMSineModulator2.frequency(modulator2Freq);
  
  // Set the 5th oscillator to a perfect fifth above the base frequency
  float fifthFreq = freq * 1.5;
  hatsBodySquare5th.frequency(fifthFreq);
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