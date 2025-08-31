#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>  // Add MIDI library

// Create MIDI instance
MIDI_CREATE_DEFAULT_INSTANCE();

// GUItool: begin automatically generated code
AudioSynthWaveformSine   kickSineMod;    //xy=176.75,347
AudioEffectEnvelope      kickModEnv;     //xy=384.75000762939453,431.0000057220459
AudioMixer4              kickSineModMxr; //xy=547.75,276
AudioSynthNoiseWhite      kickNoise;          //xy=558.7500381469727,346.7500057220459
AudioSynthWaveformSineModulated kickSineFM;     //xy=729.7500114440918,283.00000381469727
AudioEffectEnvelope      kickNoiseEnvelope; //xy=756.7500152587891,346.7500057220459
AudioMixer4              kickMxr;        //xy=970.75,309
AudioEffectEnvelope      kickEnv;        //xy=1134.75,311
AudioFilterStateVariable kickLowPassFilter;        //xy=1328.7500228881836,304.75000381469727
AudioOutputI2S           dacOut;         //xy=1548.7500190734863,300.00000381469727
AudioConnection          patchCord1(kickSineMod, kickModEnv);
AudioConnection          patchCord2(kickSineMod, 0, kickSineModMxr, 0);
AudioConnection          patchCord3(kickModEnv, 0, kickSineModMxr, 1);
AudioConnection          patchCord4(kickSineModMxr, kickSineFM);
AudioConnection          patchCord5(kickNoise, kickNoiseEnvelope);
AudioConnection          patchCord6(kickSineFM, kickEnv);
AudioConnection          patchCord7(kickNoiseEnvelope, 0, kickMxr, 1); // Noise to channel 1
AudioConnection          patchCord8(kickEnv, 0, kickMxr, 0); // Sine FM to channel 0
AudioConnection          patchCord9(kickMxr, 0, kickLowPassFilter, 0); // Mixer to filter
AudioConnection          patchCord10(kickLowPassFilter, 0, dacOut, 0);
AudioConnection          patchCord11(kickLowPassFilter, 0, dacOut, 1);
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

// Kick parameters
float baseFreq = 55.0;
float initialPitchBoost = 2.0;
float noiseLevel = 0.0; // Added for noise level control
unsigned long pitchDropStartTime = 0;
unsigned long pitchDropDuration = 30; // Now variable, controlled by potentiometer

// MIDI Configuration
const byte MIDI_CHANNEL = 1;  // Listen on MIDI channel 1
const byte KICK_NOTE = 53;    // MIDI note for kick drum

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
  kickSineFM.amplitude(0.7);
  kickSineFM.frequency(baseFreq);
  updateModulatorFrequency(); // Set modulator to perfect fifth above base frequency
  kickSineMod.amplitude(0.7);

  // Configure noise
  kickNoise.amplitude(0.7);

  // Configure Filter
  kickLowPassFilter.frequency(2000);
  kickLowPassFilter.resonance(0);
  
  // Configure mod mixer
  kickSineModMxr.gain(0, 0.7);  // Direct sineMod
  kickSineModMxr.gain(1, 1.0);  // Enveloped sineMod
  kickSineModMxr.gain(2, 0.0);  // Unused
  kickSineModMxr.gain(3, 0.0);  // Unused

  // Configure kick mixer - UPDATED
  kickMxr.gain(0, 1.0);  // Sine FM (channel 0)
  kickMxr.gain(1, 0.0);  // Noise (channel 1) - will be controlled by potentiometer
  kickMxr.gain(2, 0.0);  // Unused
  kickMxr.gain(3, 0.0);  // Unused
  
  // Main amplitude envelope
  kickEnv.attack(1);
  kickEnv.decay(150);
  kickEnv.sustain(0);
  kickEnv.release(50);
  
  // Modulation envelope
  kickModEnv.attack(2);
  kickModEnv.decay(50);
  kickModEnv.sustain(0);
  kickModEnv.release(20);

  // Noise Envelope
  kickNoiseEnvelope.attack(2);
  kickNoiseEnvelope.decay(10);
  kickNoiseEnvelope.sustain(0);
  kickNoiseEnvelope.release(0);

  Serial.begin(115200);
  Serial.println("FM Kick Drum with MIDI Input Ready");
}

void loop() {
  // Read incoming MIDI messages
  MIDI.read();
  
  // Check physical button
  buttonState = digitalRead(triggerPin);
  if (buttonState == HIGH && lastButtonState == LOW) {
    triggerKick();
  }
  lastButtonState = buttonState;
  
  readPotsAndUpdate();
  updatePitchDrop();
  delay(1);
}

// MIDI Note On handler
void handleNoteOn(byte channel, byte note, byte velocity) {
  // Check if it's the kick note on the correct channel
  if (channel == MIDI_CHANNEL && note == KICK_NOTE) {
    triggerKick();
    Serial.print("MIDI Trigger: Note ");
    Serial.print(note);
    Serial.print(" on Channel ");
    Serial.println(channel);
  }
}

void triggerKick() {
  pitchDropStartTime = millis();
  float initialFreq = baseFreq * initialPitchBoost;
  kickSineFM.frequency(initialFreq);
  updateModulatorFrequency(); // Update modulator frequency to maintain perfect fifth relationship
  kickEnv.noteOn();
  kickModEnv.noteOn();
  kickNoiseEnvelope.noteOn(); // Added noise envelope trigger
  Serial.println("Kick Triggered");
}

void readPotsAndUpdate() {
  // Read all pots
  baseFreq = map(analogRead(pitchPotPin), 0, 1023, 30, 120);
  float decayTime = map(analogRead(decayPotPin), 0, 1023, 50, 700);
  pitchDropDuration = map(analogRead(pitchDropTimePotPin), 0, 1023, 35, 700);
  initialPitchBoost = map(analogRead(punchPotPin), 0, 1023, 10, 60) / 10.0;
  
  // UPDATED: Pot A2 now controls noise level (0.0 to 1.0)
  noiseLevel = map(analogRead(punchPotPin), 0, 1023, 0, 100) / 100.0;
  
  // Update parameters
  kickEnv.decay(decayTime);
  kickMxr.gain(1, noiseLevel); // Set noise level in mixer (channel 1)
  
  // Update modulator frequency to maintain perfect fifth relationship
  updateModulatorFrequency();
}

// Helper function to set modulator frequency to a perfect fifth above carrier
void updateModulatorFrequency() {
  // A perfect fifth is a ratio of 3:2 (1.5 times the frequency)
  float modulatorFreq = baseFreq * 1.5;
  kickSineMod.frequency(modulatorFreq);
}

void updatePitchDrop() {
  if (pitchDropStartTime > 0) {
    unsigned long elapsed = millis() - pitchDropStartTime;
    if (elapsed < pitchDropDuration) {
      float progress = (float)elapsed / pitchDropDuration;
      float freqMultiplier = initialPitchBoost - (progress * (initialPitchBoost - 1.0));
      kickSineFM.frequency(baseFreq * freqMultiplier);
      updateModulatorFrequency(); // Keep modulator in perfect fifth relationship during pitch drop
    } else {
      kickSineFM.frequency(baseFreq);
      updateModulatorFrequency(); // Keep modulator in perfect fifth relationship
      pitchDropStartTime = 0;
    }
  }
}