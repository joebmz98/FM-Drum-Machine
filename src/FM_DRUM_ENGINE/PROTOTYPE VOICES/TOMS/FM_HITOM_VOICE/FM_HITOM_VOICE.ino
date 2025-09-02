#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>  // Add MIDI library

// Create MIDI instance
MIDI_CREATE_DEFAULT_INSTANCE();

// GUItool: begin automatically generated code
AudioSynthWaveformSine   hitomSineMod;    //xy=176.75,347
AudioEffectEnvelope      hitomModEnv;     //xy=384.75000762939453,431.0000057220459
AudioMixer4              hitomSineModMxr; //xy=547.75,276
AudioSynthNoiseWhite      hitomNoise;          //xy=558.7500381469727,346.7500057220459
AudioSynthWaveformSineModulated hitomSineFM;     //xy=729.7500114440918,283.00000381469727
AudioEffectEnvelope      hitomNoiseEnvelope; //xy=756.7500152587891,346.7500057220459
AudioMixer4              hitomMxr;        //xy=970.75,309
AudioEffectEnvelope      hitomEnv;        //xy=1134.75,311
AudioFilterStateVariable hitomLowPassFilter;        //xy=1328.7500228881836,304.75000381469727
AudioOutputI2S           dacOut;         //xy=1548.7500190734863,300.00000381469727
AudioConnection          patchCord1(hitomSineMod, hitomModEnv);
AudioConnection          patchCord2(hitomSineMod, 0, hitomSineModMxr, 0);
AudioConnection          patchCord3(hitomModEnv, 0, hitomSineModMxr, 1);
AudioConnection          patchCord4(hitomSineModMxr, hitomSineFM);
AudioConnection          patchCord5(hitomNoise, hitomNoiseEnvelope);
AudioConnection          patchCord6(hitomSineFM, hitomEnv);
AudioConnection          patchCord7(hitomNoiseEnvelope, 0, hitomMxr, 1); // Noise to channel 1
AudioConnection          patchCord8(hitomEnv, 0, hitomMxr, 0); // Sine FM to channel 0
AudioConnection          patchCord9(hitomMxr, 0, hitomLowPassFilter, 0); // Mixer to filter
AudioConnection          patchCord10(hitomLowPassFilter, 0, dacOut, 0);
AudioConnection          patchCord11(hitomLowPassFilter, 0, dacOut, 1);
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

// Hitom parameters
float baseFreq = 150.0;
float initialPitchBoost = 2.0;
float noiseLevel = 0.0; // Added for noise level control
unsigned long pitchDropStartTime = 0;
unsigned long pitchDropDuration = 30; // Now variable, controlled by potentiometer

// MIDI Configuration
const byte MIDI_CHANNEL = 1;  // Listen on MIDI channel 1
const byte HITOM_NOTE = 69;    // MIDI note for hitom drum

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
  hitomSineFM.amplitude(0.7);
  hitomSineFM.frequency(baseFreq);
  updateModulatorFrequency(); // Set modulator to perfect fifth above base frequency
  hitomSineMod.amplitude(0.7);

  // Configure noise
  hitomNoise.amplitude(0.7);

  // Configure Filter
  hitomLowPassFilter.frequency(2500);
  hitomLowPassFilter.resonance(0);
  
  // Configure mod mixer
  hitomSineModMxr.gain(0, 0.7);  // Direct sineMod
  hitomSineModMxr.gain(1, 1.0);  // Enveloped sineMod
  hitomSineModMxr.gain(2, 0.0);  // Unused
  hitomSineModMxr.gain(3, 0.0);  // Unused

  // Configure hitom mixer - UPDATED
  hitomMxr.gain(0, 1.0);  // Sine FM (channel 0)
  hitomMxr.gain(1, 0.0);  // Noise (channel 1) - will be controlled by potentiometer
  hitomMxr.gain(2, 0.0);  // Unused
  hitomMxr.gain(3, 0.0);  // Unused
  
  // Main amplitude envelope
  hitomEnv.attack(1);
  hitomEnv.decay(150);
  hitomEnv.sustain(0);
  hitomEnv.release(50);
  
  // Modulation envelope
  hitomModEnv.attack(2);
  hitomModEnv.decay(50);
  hitomModEnv.sustain(0);
  hitomModEnv.release(20);

  // Noise Envelope
  hitomNoiseEnvelope.attack(0);
  hitomNoiseEnvelope.decay(10);
  hitomNoiseEnvelope.sustain(0);
  hitomNoiseEnvelope.release(0);

  Serial.begin(115200);
  Serial.println("FM Hitom Drum with MIDI Input Ready");
}

void loop() {
  // Read incoming MIDI messages
  MIDI.read();
  
  // Check physical button
  buttonState = digitalRead(triggerPin);
  if (buttonState == HIGH && lastButtonState == LOW) {
    triggerHitom();
  }
  lastButtonState = buttonState;
  
  readPotsAndUpdate();
  updatePitchDrop();
  delay(1);
}

// MIDI Note On handler
void handleNoteOn(byte channel, byte note, byte velocity) {
  // Check if it's the hitom note on the correct channel
  if (channel == MIDI_CHANNEL && note == HITOM_NOTE) {
    triggerHitom();
    Serial.print("MIDI Trigger: Note ");
    Serial.print(note);
    Serial.print(" on Channel ");
    Serial.println(channel);
  }
}

void triggerHitom() {
  pitchDropStartTime = millis();
  float initialFreq = baseFreq * initialPitchBoost;
  hitomSineFM.frequency(initialFreq);
  updateModulatorFrequency(); // Update modulator frequency to maintain perfect fifth relationship
  hitomEnv.noteOn();
  hitomModEnv.noteOn();
  hitomNoiseEnvelope.noteOn(); // Added noise envelope trigger
  Serial.println("Hitom Triggered");
}

void readPotsAndUpdate() {
  // Read all pots
  baseFreq = map(analogRead(pitchPotPin), 0, 1023, 150, 220);
  float decayTime = map(analogRead(decayPotPin), 0, 1023, 70, 700);
  pitchDropDuration = map(analogRead(pitchDropTimePotPin), 0, 1023, 1, 200);
  initialPitchBoost = map(analogRead(punchPotPin), 0, 1023, 10, 60) / 10.0;
  
  // UPDATED: Pot A2 now controls noise level (0.0 to 1.0)
  noiseLevel = map(analogRead(punchPotPin), 0, 1023, 0, 100) / 100.0;
  
  // Update parameters
  hitomEnv.decay(decayTime);
  hitomMxr.gain(1, noiseLevel); // Set noise level in mixer (channel 1)
  
  // Update modulator frequency to maintain perfect fifth relationship
  updateModulatorFrequency();
}

// Helper function to set modulator frequency to a perfect fifth above carrier
void updateModulatorFrequency() {
  // A perfect fifth is a ratio of 3:2 (1.5 times the frequency)
  float modulatorFreq = baseFreq * 1.5;
  hitomSineMod.frequency(modulatorFreq);
}

void updatePitchDrop() {
  if (pitchDropStartTime > 0) {
    unsigned long elapsed = millis() - pitchDropStartTime;
    if (elapsed < pitchDropDuration) {
      float progress = (float)elapsed / pitchDropDuration;
      float freqMultiplier = initialPitchBoost - (progress * (initialPitchBoost - 1.0));
      hitomSineFM.frequency(baseFreq * freqMultiplier);
      updateModulatorFrequency(); // Keep modulator in perfect fifth relationship during pitch drop
    } else {
      hitomSineFM.frequency(baseFreq);
      updateModulatorFrequency(); // Keep modulator in perfect fifth relationship
      pitchDropStartTime = 0;
    }
  }
}