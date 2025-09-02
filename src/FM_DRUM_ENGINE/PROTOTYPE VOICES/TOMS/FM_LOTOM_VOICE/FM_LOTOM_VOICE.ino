#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>  // Add MIDI library

// Create MIDI instance
MIDI_CREATE_DEFAULT_INSTANCE();

// GUItool: begin automatically generated code
AudioSynthWaveformSine   lotomSineMod;    //xy=176.75,347
AudioEffectEnvelope      lotomModEnv;     //xy=384.75000762939453,431.0000057220459
AudioMixer4              lotomSineModMxr; //xy=547.75,276
AudioSynthNoiseWhite      lotomNoise;          //xy=558.7500381469727,346.7500057220459
AudioSynthWaveformSineModulated lotomSineFM;     //xy=729.7500114440918,283.00000381469727
AudioEffectEnvelope      lotomNoiseEnvelope; //xy=756.7500152587891,346.7500057220459
AudioMixer4              lotomMxr;        //xy=970.75,309
AudioEffectEnvelope      lotomEnv;        //xy=1134.75,311
AudioFilterStateVariable lotomLowPassFilter;        //xy=1328.7500228881836,304.75000381469727
AudioOutputI2S           dacOut;         //xy=1548.7500190734863,300.00000381469727
AudioConnection          patchCord1(lotomSineMod, lotomModEnv);
AudioConnection          patchCord2(lotomSineMod, 0, lotomSineModMxr, 0);
AudioConnection          patchCord3(lotomModEnv, 0, lotomSineModMxr, 1);
AudioConnection          patchCord4(lotomSineModMxr, lotomSineFM);
AudioConnection          patchCord5(lotomNoise, lotomNoiseEnvelope);
AudioConnection          patchCord6(lotomSineFM, lotomEnv);
AudioConnection          patchCord7(lotomNoiseEnvelope, 0, lotomMxr, 1); // Noise to channel 1
AudioConnection          patchCord8(lotomEnv, 0, lotomMxr, 0); // Sine FM to channel 0
AudioConnection          patchCord9(lotomMxr, 0, lotomLowPassFilter, 0); // Mixer to filter
AudioConnection          patchCord10(lotomLowPassFilter, 0, dacOut, 0);
AudioConnection          patchCord11(lotomLowPassFilter, 0, dacOut, 1);
AudioControlSGTL5000     audioControl;     //xy=160.75,80.75
// GUItool: end automatically generated code

// Button setup
const int triggerPin = 2;
int buttonState = 0;
int lastButtonState = 0;

// Potentiometer pins
const int decayPotPin = A0;    // Controls envelope1 decay
const int pitchPotPin = A1;    // Controls base frequency
const int punchPotPin = A2;    // Controls noise level (UPDATED)
const int pitchDropTimePotPin = A3; // Controls pitch drop duration

// lotom parameters
float baseFreq = 70.0;
float initialPitchBoost = 2.0;
float noiseLevel = 0.0; // Added for noise level control
unsigned long pitchDropStartTime = 0;
unsigned long pitchDropDuration = 30; // Now variable, controlled by potentiometer

// MIDI Configuration
const byte MIDI_CHANNEL = 1;  // Listen on MIDI channel 1
const byte LOTOM_NOTE = 65;    // MIDI note for lotom drum

void setup() {
  pinMode(triggerPin, INPUT_PULLDOWN);
  pinMode(pitchPotPin, INPUT);
  pinMode(decayPotPin, INPUT);
  pinMode(pitchDropTimePotPin, INPUT);
  pinMode(punchPotPin, INPUT);
  
  // Initialize MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  
  AudioMemory(128);
  audioControl.enable();
  audioControl.volume(0.8);
  
  // Configure oscillators
  lotomSineFM.amplitude(0.7);
  lotomSineFM.frequency(baseFreq);
  updateModulatorFrequency(); // Set modulator to perfect fifth above base frequency
  lotomSineMod.amplitude(0.7);

  // Configure noise
  lotomNoise.amplitude(0.7);

  // Configure Filter
  lotomLowPassFilter.frequency(2000);
  lotomLowPassFilter.resonance(0);
  
  // Configure mod mixer
  lotomSineModMxr.gain(0, 0.7);  // Direct sineMod
  lotomSineModMxr.gain(1, 1.0);  // Enveloped sineMod
  lotomSineModMxr.gain(2, 0.0);  // Unused
  lotomSineModMxr.gain(3, 0.0);  // Unused

  // Configure lotom mixer - UPDATED
  lotomMxr.gain(0, 1.0);  // Sine FM (channel 0)
  lotomMxr.gain(1, 0.0);  // Noise (channel 1) - will be controlled by potentiometer
  lotomMxr.gain(2, 0.0);  // Unused
  lotomMxr.gain(3, 0.0);  // Unused
  
  // Main amplitude envelope
  lotomEnv.attack(1);
  lotomEnv.decay(150);
  lotomEnv.sustain(0);
  lotomEnv.release(50);
  
  // Modulation envelope
  lotomModEnv.attack(2);
  lotomModEnv.decay(50);
  lotomModEnv.sustain(0);
  lotomModEnv.release(20);

  // Noise Envelope
  lotomNoiseEnvelope.attack(2);
  lotomNoiseEnvelope.decay(10);
  lotomNoiseEnvelope.sustain(0);
  lotomNoiseEnvelope.release(0);

  Serial.begin(115200);
  Serial.println("FM lotom Drum with MIDI Input Ready");
}

void loop() {
  // Read incoming MIDI messages
  MIDI.read();
  
  // Check physical button
  buttonState = digitalRead(triggerPin);
  if (buttonState == HIGH && lastButtonState == LOW) {
    triggerlotom();
  }
  lastButtonState = buttonState;
  
  readPotsAndUpdate();
  updatePitchDrop();
  delay(1);
}

// MIDI Note On handler
void handleNoteOn(byte channel, byte note, byte velocity) {
  // Check if it's the lotom note on the correct channel
  if (channel == MIDI_CHANNEL && note == LOTOM_NOTE) {
    triggerlotom();
    Serial.print("MIDI Trigger: Note ");
    Serial.print(note);
    Serial.print(" on Channel ");
    Serial.println(channel);
  }
}

void triggerlotom() {
  pitchDropStartTime = millis();
  float initialFreq = baseFreq * initialPitchBoost;
  lotomSineFM.frequency(initialFreq);
  updateModulatorFrequency(); // Update modulator frequency to maintain perfect fifth relationship
  lotomEnv.noteOn();
  lotomModEnv.noteOn();
  lotomNoiseEnvelope.noteOn(); // Added noise envelope trigger
  Serial.println("lotom Triggered");
}

void readPotsAndUpdate() {
  // Read all pots
  baseFreq = map(analogRead(pitchPotPin), 0, 1023, 60, 120);
  float decayTime = map(analogRead(decayPotPin), 0, 1023, 50, 700);
  pitchDropDuration = map(analogRead(pitchDropTimePotPin), 0, 1023, 35, 1000);
  initialPitchBoost = map(analogRead(punchPotPin), 0, 1023, 10, 60) / 10.0;
  
  // UPDATED: Pot A2 now controls noise level (0.0 to 1.0)
  noiseLevel = map(analogRead(punchPotPin), 0, 1023, 0, 100) / 100.0;
  
  // Update parameters
  lotomEnv.decay(decayTime);
  lotomMxr.gain(1, noiseLevel); // Set noise level in mixer (channel 1)
  
  // Update modulator frequency to maintain perfect fifth relationship
  updateModulatorFrequency();
}

// Helper function to set modulator frequency to a perfect fifth above carrier
void updateModulatorFrequency() {
  // A perfect fifth is a ratio of 3:2 (1.5 times the frequency)
  float modulatorFreq = baseFreq * 1.5;
  lotomSineMod.frequency(modulatorFreq);
}

void updatePitchDrop() {
  if (pitchDropStartTime > 0) {
    unsigned long elapsed = millis() - pitchDropStartTime;
    if (elapsed < pitchDropDuration) {
      float progress = (float)elapsed / pitchDropDuration;
      float freqMultiplier = initialPitchBoost - (progress * (initialPitchBoost - 1.0));
      lotomSineFM.frequency(baseFreq * freqMultiplier);
      updateModulatorFrequency(); // Keep modulator in perfect fifth relationship during pitch drop
    } else {
      lotomSineFM.frequency(baseFreq);
      updateModulatorFrequency(); // Keep modulator in perfect fifth relationship
      pitchDropStartTime = 0;
    }
  }
}