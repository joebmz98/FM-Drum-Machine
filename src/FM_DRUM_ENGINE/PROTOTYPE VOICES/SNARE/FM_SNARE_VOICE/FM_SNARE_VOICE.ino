#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveform       snareBodySquare5th; //xy=239.75,351
AudioSynthWaveform       snareBodySine5th; //xy=250.75,315
AudioSynthWaveformSine   snareBodyFMSineModulator;          //xy=403.75,254.75000381469727
AudioMixer4              snareBodyFMMxr; //xy=474.75,333
AudioSynthWaveformSineModulated snareBodyFMSine; //xy=659.75,255
AudioSynthNoiseWhite     snareSnapNoise; //xy=789.75,456
AudioMixer4              snareBodyMxr;   //xy=859.75,316
AudioEffectEnvelope      snareSnapEnv;   //xy=1035.75,444
AudioEffectEnvelope      snareBodyEnv;   //xy=1066.75,321
AudioMixer4              snareMxr;       //xy=1378.75,334
AudioFilterStateVariable snareHPFilter;  //xy=1545.75,331
AudioOutputI2S           dacOut;         //xy=1724.75,332
AudioConnection          patchCord1(snareBodySquare5th, 0, snareBodyFMMxr, 1);
AudioConnection          patchCord2(snareBodySine5th, 0, snareBodyFMMxr, 0);
AudioConnection          patchCord3(snareBodyFMSineModulator, snareBodyFMSine);
AudioConnection          patchCord4(snareBodyFMMxr, 0, snareBodyMxr, 1);
AudioConnection          patchCord5(snareBodyFMSine, 0, snareBodyMxr, 0);
AudioConnection          patchCord6(snareSnapNoise, snareSnapEnv);
AudioConnection          patchCord7(snareBodyMxr, snareBodyEnv);
AudioConnection          patchCord8(snareSnapEnv, 0, snareMxr, 1);
AudioConnection          patchCord9(snareBodyEnv, 0, snareMxr, 0);
AudioConnection          patchCord10(snareMxr, 0, snareHPFilter, 0);
AudioConnection          patchCord11(snareHPFilter, 2, dacOut, 1);
AudioControlSGTL5000     audioControl;     //xy=169.75,853.75
// GUItool: end automatically generated code

// MIDI setup
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

// Potentiometer pins
const int decayPotPin = A0;    // Controls envelope decay for both body and snap
const int pitchPotPin = A1;    // Controls base frequency
const int attackPotPin = A2;   // Controls pitch drop amount
const int snapPotPin = A3;     // Controls level of snareMxr channel 2 (snap)
const int timbrePotPin = A4;   // Controls mix between sine5th and square5th

// Snare parameters
float baseFreq = 180.0;         // Base frequency for snare body
float initialPitchBoost = 3.0;  // Initial pitch boost for snare body
unsigned long pitchDropStartTime = 0;
const unsigned long pitchDropDuration = 20; // Duration of pitch drop in ms

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (channel == 1 && note == 55) { // MIDI channel 1, note 55
    triggerSnare();
  }
}

void setup() {
  Serial.begin(9600);
  
  // Initialize MIDI
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.begin(1); // Listen on channel 1
  
  // Initialize potentiometer pins
  pinMode(decayPotPin, INPUT);
  pinMode(pitchPotPin, INPUT);
  pinMode(attackPotPin, INPUT);
  pinMode(snapPotPin, INPUT);
  pinMode(timbrePotPin, INPUT);
  
  AudioMemory(128);
  
  // Initialize audio control FIRST before enabling
  audioControl.enable();
  audioControl.volume(0.8); 
  
  // Configure oscillators
  snareBodySine5th.begin(WAVEFORM_SINE);
  snareBodySquare5th.begin(WAVEFORM_SQUARE);
  
  // Set initial frequencies 
  updateOscillatorFrequencies(baseFreq);
  
  // Set initial amplitudes 
  snareBodyFMSine.amplitude(0.7);
  snareBodySine5th.amplitude(0.5);
  snareBodySquare5th.amplitude(0.5);
  snareBodyFMSineModulator.amplitude(0.5); // Set modulator amplitude
  snareSnapNoise.amplitude(0.5);

  // Configure Filter
  snareHPFilter.frequency(80);
  snareHPFilter.resonance(0);
  
  // Configure mixers (reduced gains to prevent clipping)
  snareBodyFMMxr.gain(0, 0.3);  // Sine5th modulator
  snareBodyFMMxr.gain(1, 0.3);  // Square5th modulator
  snareBodyFMMxr.gain(2, 0.0);
  snareBodyFMMxr.gain(3, 0.0);
  
  snareBodyMxr.gain(0, 0.5);  // FM Sine (carrier)
  snareBodyMxr.gain(1, 0.3);  // Direct modulator mix
  snareBodyMxr.gain(2, 0.0);
  snareBodyMxr.gain(3, 0.0);
  
  snareMxr.gain(0, 0.5);  // Body
  snareMxr.gain(1, 0.5);  // Snap
  snareMxr.gain(2, 0.0);
  snareMxr.gain(3, 0.0);
  
  // Configure envelopes
  snareBodyEnv.attack(1);
  snareBodyEnv.decay(200);
  snareBodyEnv.sustain(0);
  snareBodyEnv.release(40);
  
  snareSnapEnv.attack(1);
  snareSnapEnv.decay(100);
  snareSnapEnv.sustain(0);
  snareSnapEnv.release(40);
  snareSnapEnv.delay(5);
  
  Serial.println("FM Snare initialized with octave + minor 3rd modulation");
}

void loop() {
  // Handle MIDI messages
  MIDI.read();
  
  readPotsAndUpdate();
  updatePitchDrop();
  
  // Add a small delay to prevent overwhelming the processor
  //delay(5);
}

void triggerSnare() {
  pitchDropStartTime = millis();
  
  // Apply initial pitch boost to body oscillators
  float initialFreq = baseFreq * initialPitchBoost;
  updateOscillatorFrequencies(initialFreq);
  
  // Trigger envelopes
  snareBodyEnv.noteOn();
  snareSnapEnv.noteOn();
}

void readPotsAndUpdate() {
  // Read all pots
  baseFreq = map(analogRead(pitchPotPin), 0, 1023, 150, 600);
  
  // Decay pot controls both body and snap decay times
  float bodyDecayTime = map(analogRead(decayPotPin), 0, 1023, 50, 400);
  float snapDecayTime = map(analogRead(decayPotPin), 0, 1023, 40, 380);
  snareBodyEnv.decay(bodyDecayTime);
  snareSnapEnv.decay(snapDecayTime);
  
  // Attack pot controls pitch drop amount
  initialPitchBoost = map(analogRead(attackPotPin), 0, 1023, 20, 80) / 10.0;
  
  // Snap pot controls level of snareMxr channel 2 (snap) 
  float snapLevel = map(analogRead(snapPotPin), 0, 1023, 0, 120) / 100.0;
  snareMxr.gain(1, snapLevel);
  
  // Timbre pot controls mix between sine5th and square5th
  float timbreMix = map(analogRead(timbrePotPin), 0, 1023, 0, 100) / 100.0;
  snareBodyFMMxr.gain(0, 0.3 * (1.0 - timbreMix));  // Sine5th modulator
  snareBodyFMMxr.gain(1, 0.3 * timbreMix);           // Square5th modulator
  
  // Update oscillator frequencies if not in pitch drop
  if (pitchDropStartTime == 0) {
    updateOscillatorFrequencies(baseFreq);
  }
}

void updateOscillatorFrequencies(float freq) {
  // Set both modulator oscillators to a perfect fifth above the base frequency
  float fifthFreq = freq * 1.5;
  snareBodySine5th.frequency(fifthFreq);
  snareBodySquare5th.frequency(fifthFreq);
  
  // Set the FM carrier frequency to the base frequency
  snareBodyFMSine.frequency(freq);
  
  // Set the FM modulator frequency to an octave and a minor third above the base frequency
  // Octave = 2x, minor third = 6/5 ≈ 1.2, so 2 * 1.2 = 2.4x
  float octaveMinorThirdFreq = freq * 2.4;
  snareBodyFMSineModulator.frequency(octaveMinorThirdFreq);
}

void updatePitchDrop() {
  if (pitchDropStartTime > 0) {
    unsigned long elapsed = millis() - pitchDropStartTime;
    if (elapsed < pitchDropDuration) {
      float progress = (float)elapsed / pitchDropDuration;
      float freqMultiplier = initialPitchBoost - (progress * (initialPitchBoost - 1.0));
      updateOscillatorFrequencies(baseFreq * freqMultiplier);
    } else {
      updateOscillatorFrequencies(baseFreq);
      pitchDropStartTime = 0;
    }
  }
}