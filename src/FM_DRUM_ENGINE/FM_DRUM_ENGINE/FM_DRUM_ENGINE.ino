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

// ================= TOM DRUM AUDIO OBJECTS =================
// Low Tom
AudioSynthWaveformSine   loTomSineMod;
AudioEffectEnvelope      loTomModEnv;
AudioMixer4              loTomSineModMxr;
AudioSynthNoiseWhite     loTomNoise;
AudioSynthWaveformSineModulated loTomSineFM;
AudioEffectEnvelope      loTomNoiseEnvelope;
AudioMixer4              loTomMxr;
AudioEffectEnvelope      loTomEnv;
AudioFilterStateVariable loTomLowPassFilter;

// High Tom
AudioSynthWaveformSine   hiTomSineMod;
AudioEffectEnvelope      hiTomModEnv;
AudioMixer4              hiTomSineModMxr;
AudioSynthNoiseWhite     hiTomNoise;
AudioSynthWaveformSineModulated hiTomSineFM;
AudioEffectEnvelope      hiTomNoiseEnvelope;
AudioMixer4              hiTomMxr;
AudioEffectEnvelope      hiTomEnv;
AudioFilterStateVariable hiTomLowPassFilter;

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

// Low Tom connections
AudioConnection          patchCord40(loTomSineMod, loTomModEnv);
AudioConnection          patchCord41(loTomSineMod, 0, loTomSineModMxr, 0);
AudioConnection          patchCord42(loTomModEnv, 0, loTomSineModMxr, 1);
AudioConnection          patchCord43(loTomSineModMxr, loTomSineFM);
AudioConnection          patchCord44(loTomNoise, loTomNoiseEnvelope);
AudioConnection          patchCord45(loTomSineFM, loTomEnv);
AudioConnection          patchCord46(loTomNoiseEnvelope, 0, loTomMxr, 1);
AudioConnection          patchCord47(loTomEnv, 0, loTomMxr, 0);
AudioConnection          patchCord48(loTomMxr, 0, loTomLowPassFilter, 0);

// High Tom connections
AudioConnection          patchCord50(hiTomSineMod, hiTomModEnv);
AudioConnection          patchCord51(hiTomSineMod, 0, hiTomSineModMxr, 0);
AudioConnection          patchCord52(hiTomModEnv, 0, hiTomSineModMxr, 1);
AudioConnection          patchCord53(hiTomSineModMxr, hiTomSineFM);
AudioConnection          patchCord54(hiTomNoise, hiTomNoiseEnvelope);
AudioConnection          patchCord55(hiTomSineFM, hiTomEnv);
AudioConnection          patchCord56(hiTomNoiseEnvelope, 0, hiTomMxr, 1);
AudioConnection          patchCord57(hiTomEnv, 0, hiTomMxr, 0);
AudioConnection          patchCord58(hiTomMxr, 0, hiTomLowPassFilter, 0);

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
AudioConnection          patchCord60(loTomLowPassFilter, 0, rightOutMxr, 1); // Low Tom to right channel
AudioConnection          patchCord61(hiTomLowPassFilter, 0, rightOutMxr, 2); // High Tom to right channel

// Connect mixers to DAC outputs
AudioConnection          patchCord33(leftOutMxr, 0, dacOut, 0);              // Left mixer to left output
AudioConnection          patchCord34(rightOutMxr, 0, dacOut, 1);             // Right mixer to right output

// ================= MIDI CONFIGURATION =================
const byte MIDI_CHANNEL = 1;
const byte KICK_NOTE = 53;
const byte SNARE_NOTE = 55;
const byte CLOSED_HIHAT_NOTE = 59;
const byte OPEN_HIHAT_NOTE = 63;
const byte LO_TOM_NOTE = 65;
const byte HI_TOM_NOTE = 69;

// ================= POTENTIOMETER PINS =================
// Kick drum pots
const int kickDecayPotPin = A0;
const int kickTunePotPin = A1;
const int kickPunchPotPin = A2;
const int kickNoiseLevelPotPin = A3; // Changed from pitch env length to noise level

// Tom drum pots (share with kick for now)
const int tomDecayPotPin = A0;       // Tom decay control

// Snare drum pots
const int snareDecayPotPin = A0;
const int snareTumePotPin = A1;
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
const unsigned long kickPitchDropDuration = 20; // Fixed at 20ms

// ================= TOM PARAMETERS =================
// Low Tom
float loTomBaseFreq = 80.0;
float loTomInitialPitchBoost = 1.8;
float loTomNoiseLevel = 0.0;
float loTomDecayTime = 200.0;        // Added decay time for low tom
unsigned long loTomPitchDropStartTime = 0;
const unsigned long loTomPitchDropDuration = 20; // Fixed at 20ms

// High Tom
float hiTomBaseFreq = 120.0;
float hiTomInitialPitchBoost = 1.6;
float hiTomNoiseLevel = 0.0;
float hiTomDecayTime = 150.0;        // Added decay time for high tom
unsigned long hiTomPitchDropStartTime = 0;
const unsigned long hiTomPitchDropDuration = 20; // Fixed at 20ms

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

// ================= CONFIGURATION FUNCTIONS =================
void configureDrumVoice(AudioSynthWaveformSineModulated &sineFM, AudioSynthWaveformSine &sineMod, 
                       AudioSynthNoiseWhite &noise, AudioFilterStateVariable &filter,
                       AudioMixer4 &sineModMxr, AudioMixer4 &mainMxr, AudioEffectEnvelope &env,
                       AudioEffectEnvelope &modEnv, AudioEffectEnvelope &noiseEnv,
                       float baseFreq, float pitchBoost, float decayTime) {
  sineFM.amplitude(0.7);
  sineFM.frequency(baseFreq);
  updateModulatorFrequency(sineMod, baseFreq);
  sineMod.amplitude(0.7);
  noise.amplitude(0.7);
  filter.frequency(2000);
  filter.resonance(0);
  sineModMxr.gain(0, 0.7);
  sineModMxr.gain(1, 1.0);
  sineModMxr.gain(2, 0.0);
  sineModMxr.gain(3, 0.0);
  mainMxr.gain(0, 1.0);
  mainMxr.gain(1, 0.0);
  mainMxr.gain(2, 0.0);
  mainMxr.gain(3, 0.0);
  env.attack(1);
  env.decay(decayTime); // Use parameter for decay time
  env.sustain(0);
  env.release(50);
  modEnv.attack(2);
  modEnv.decay(50);
  modEnv.sustain(0);
  modEnv.release(20);
  noiseEnv.attack(2);
  noiseEnv.decay(10);
  noiseEnv.sustain(0);
  noiseEnv.release(0);
}

void configureSnareVoice() {
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
}

void configureHiHatVoice() {
  hatsBodySquareFMCarrier.begin(WAVEFORM_SQUARE);
  hatsBodySquare5th.begin(WAVEFORM_SQUARE);
  
  // Set initial frequencies 
  updateHiHatOscillatorFrequencies(hatsBaseFreq);
  
  // Set initial amplitudes 
  hatsBodySquareFMCarrier.amplitude(0.7);
  hatsBodySquare5th.amplitude(0.5);
  hatsBodyFMSineModulator1.amplitude(0.5);
  hatsBodyFMSineModulator2.amplitude(0.5);
  hatsNoise.amplitude(0.5);

  // Configure Filters
  hatsBandPass.frequency(5000);
  hatsBandPass.resonance(0.7);
  hatsHiPass.frequency(3000);
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
  hatsMxr.gain(1, 0.8);
  hatsMxr.gain(2, 0.0);
  hatsMxr.gain(3, 0.0);
  
  // Configure envelope with initial settings
  hatsEnv.attack(1);
  hatsEnv.decay(closedDecayTime);
  hatsEnv.sustain(0);
  hatsEnv.release(5);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing FM Drum Machine...");
  
  // Initialize potentiometer pins
  pinMode(kickDecayPotPin, INPUT);
  pinMode(kickTunePotPin, INPUT);
  pinMode(kickPunchPotPin, INPUT);
  pinMode(kickNoiseLevelPotPin, INPUT);
  pinMode(snareDecayPotPin, INPUT);
  pinMode(snareTumePotPin, INPUT);
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
  
  AudioMemory(400); // Increased memory for additional voices
  audioControl.enable();
  audioControl.volume(0.8);
  
  // Configure output mixers
  leftOutMxr.gain(0, 0.8);  // Kick level on left channel
  leftOutMxr.gain(1, 0.8);  // Hi-hat level on left channel
  leftOutMxr.gain(2, 0.0);  // Unused
  leftOutMxr.gain(3, 0.0);  // Unused
  
  rightOutMxr.gain(0, 0.8); // Snare level on right channel
  rightOutMxr.gain(1, 0.8); // Low Tom level on right channel
  rightOutMxr.gain(2, 0.8); // High Tom level on right channel
  rightOutMxr.gain(3, 0.0); // Unused
  
  // ================= CONFIGURE KICK =================
  configureDrumVoice(kickSineFM, kickSineMod, kickNoise, kickLowPassFilter, 
                    kickSineModMxr, kickMxr, kickEnv, kickModEnv, kickNoiseEnvelope,
                    kickBaseFreq, kickInitialPitchBoost, 150.0);

  // ================= CONFIGURE LOW TOM =================
  configureDrumVoice(loTomSineFM, loTomSineMod, loTomNoise, loTomLowPassFilter, 
                    loTomSineModMxr, loTomMxr, loTomEnv, loTomModEnv, loTomNoiseEnvelope,
                    loTomBaseFreq, loTomInitialPitchBoost, loTomDecayTime);

  // ================= CONFIGURE HIGH TOM =================
  configureDrumVoice(hiTomSineFM, hiTomSineMod, hiTomNoise, hiTomLowPassFilter, 
                    hiTomSineModMxr, hiTomMxr, hiTomEnv, hiTomModEnv, hiTomNoiseEnvelope,
                    hiTomBaseFreq, hiTomInitialPitchBoost, hiTomDecayTime);

  // ================= CONFIGURE SNARE =================
  configureSnareVoice();

  // ================= CONFIGURE HI-HAT =================
  configureHiHatVoice();

  Serial.println("FM Drum Machine with Kick, Snare, Hi-Hat and Toms Ready");
  Serial.println("Listening for MIDI notes:");
  Serial.println("53: Kick, 55: Snare, 59: Closed Hi-Hat, 63: Open Hi-Hat");
  Serial.println("65: Low Tom, 69: High Tom");
  Serial.println("Output routing:");
  Serial.println("Left channel: Kick + Hi-hat");
  Serial.println("Right channel: Snare + Low Tom + High Tom");
  Serial.println("Tom decay controlled by A0");
}

void loop() {
  MIDI.read();
  readPotsAndUpdate();
  updateKickPitchDrop();
  updateLoTomPitchDrop();
  updateHiTomPitchDrop();
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
    } else if (note == CLOSED_HIHAT_NOTE) {
      triggerHiHat(false);
      Serial.print("Closed Hi-Hat Triggered via MIDI: Note ");
      Serial.println(note);
    } else if (note == OPEN_HIHAT_NOTE) {
      triggerHiHat(true);
      Serial.print("Open Hi-Hat Triggered via MIDI: Note ");
      Serial.println(note);
    } else if (note == LO_TOM_NOTE) {
      triggerLoTom();
      Serial.print("Low Tom Triggered via MIDI: Note ");
      Serial.println(note);
    } else if (note == HI_TOM_NOTE) {
      triggerHiTom();
      Serial.print("High Tom Triggered via MIDI: Note ");
      Serial.println(note);
    }
  }
}

void triggerKick() {
  kickPitchDropStartTime = millis();
  float initialFreq = kickBaseFreq * kickInitialPitchBoost;
  kickSineFM.frequency(initialFreq);
  updateModulatorFrequency(kickSineMod, kickBaseFreq);
  kickEnv.noteOn();
  kickModEnv.noteOn();
  kickNoiseEnvelope.noteOn();
}

void triggerLoTom() {
  loTomPitchDropStartTime = millis();
  float initialFreq = loTomBaseFreq * loTomInitialPitchBoost;
  loTomSineFM.frequency(initialFreq);
  updateModulatorFrequency(loTomSineMod, loTomBaseFreq);
  loTomEnv.noteOn();
  loTomModEnv.noteOn();
  loTomNoiseEnvelope.noteOn();
}

void triggerHiTom() {
  hiTomPitchDropStartTime = millis();
  float initialFreq = hiTomBaseFreq * hiTomInitialPitchBoost;
  hiTomSineFM.frequency(initialFreq);
  updateModulatorFrequency(hiTomSineMod, hiTomBaseFreq);
  hiTomEnv.noteOn();
  hiTomModEnv.noteOn();
  hiTomNoiseEnvelope.noteOn();
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
  float currentDecayTime = openHat ? openDecayTime : closedDecayTime;
  hatsEnv.decay(currentDecayTime);
  hatsEnv.noteOn();
}

void readPotsAndUpdate() {
  // Read all pots
  int kickDecayVal = analogRead(kickDecayPotPin);
  int kickTuneVal = analogRead(kickTunePotPin);
  int kickPunchVal = analogRead(kickPunchPotPin);
  int kickNoiseLevelVal = analogRead(kickNoiseLevelPotPin);
  
  int tomDecayVal = analogRead(tomDecayPotPin); // Tom decay control
  
  int snareDecayVal = analogRead(snareDecayPotPin);
  int snareTuneVal = analogRead(snareTumePotPin);
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
  kickInitialPitchBoost = map(kickPunchVal, 0, 1023, 10, 60) / 10.0;
  kickNoiseLevel = map(kickNoiseLevelVal, 0, 1023, 0, 100) / 100.0;
  
  kickEnv.decay(kickDecayTime);
  kickMxr.gain(1, kickNoiseLevel);
  updateModulatorFrequency(kickSineMod, kickBaseFreq);
  
  // Update tom parameters
  loTomBaseFreq = map(kickTuneVal, 0, 1023, 60, 100);   // Low tom range
  hiTomBaseFreq = map(kickTuneVal, 0, 1023, 100, 180);  // High tom range
  
  loTomInitialPitchBoost = map(kickPunchVal, 0, 1023, 10, 40) / 10.0;
  hiTomInitialPitchBoost = map(kickPunchVal, 0, 1023, 10, 30) / 10.0;
  
  loTomNoiseLevel = map(kickNoiseLevelVal, 0, 1023, 0, 80) / 100.0;
  hiTomNoiseLevel = map(kickNoiseLevelVal, 0, 1023, 0, 60) / 100.0;
  
  // TOM DECAY CONTROL - Added this section
  loTomDecayTime = map(tomDecayVal, 0, 1023, 100, 500);  // Tom decay range
  hiTomDecayTime = map(tomDecayVal, 0, 1023, 80, 400);   // High tom slightly shorter decay
  
  loTomEnv.decay(loTomDecayTime);
  hiTomEnv.decay(hiTomDecayTime);
  
  loTomMxr.gain(1, loTomNoiseLevel);
  hiTomMxr.gain(1, hiTomNoiseLevel);
  
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

void updateModulatorFrequency(AudioSynthWaveformSine &sineMod, float baseFreq) {
  float modulatorFreq = baseFreq * 1.5;
  sineMod.frequency(modulatorFreq);
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
  hatsBodySquareFMCarrier.frequency(freq);
  float modulator1Freq = freq * 1.3;
  float modulator2Freq = freq * 1.7;
  hatsBodyFMSineModulator1.frequency(modulator1Freq);
  hatsBodyFMSineModulator2.frequency(modulator2Freq);
  float fifthFreq = freq * 1.5;
  hatsBodySquare5th.frequency(fifthFreq);
}

void updatePitchDrop(AudioSynthWaveformSineModulated &sineFM, AudioSynthWaveformSine &sineMod, 
                    float baseFreq, float initialPitchBoost, unsigned long &pitchDropStartTime) {
  if (pitchDropStartTime > 0) {
    unsigned long elapsed = millis() - pitchDropStartTime;
    if (elapsed < kickPitchDropDuration) {
      float progress = (float)elapsed / kickPitchDropDuration;
      float freqMultiplier = initialPitchBoost - (progress * (initialPitchBoost - 1.0));
      sineFM.frequency(baseFreq * freqMultiplier);
      updateModulatorFrequency(sineMod, baseFreq * freqMultiplier);
    } else {
      sineFM.frequency(baseFreq);
      updateModulatorFrequency(sineMod, baseFreq);
      pitchDropStartTime = 0;
    }
  }
}

void updateKickPitchDrop() {
  updatePitchDrop(kickSineFM, kickSineMod, kickBaseFreq, kickInitialPitchBoost, kickPitchDropStartTime);
}

void updateLoTomPitchDrop() {
  updatePitchDrop(loTomSineFM, loTomSineMod, loTomBaseFreq, loTomInitialPitchBoost, loTomPitchDropStartTime);
}

void updateHiTomPitchDrop() {
  updatePitchDrop(hiTomSineFM, hiTomSineMod, hiTomBaseFreq, hiTomInitialPitchBoost, hiTomPitchDropStartTime);
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