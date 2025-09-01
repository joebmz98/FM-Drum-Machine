#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine   hatsBodyFMSineModulator1;          //xy=192.5,357.5000047683716
AudioSynthWaveformSine   hatsBodyFMSineModulator2;          //xy=192.5,396.25000762939453
AudioMixer4              hatsBodyFMModulatorMxr;         //xy=466.25000762939453,376.2500190734863
AudioSynthWaveformModulated hatsBodySquareFMCarrier;   //xy=718.2142639160156,382.50000190734863
AudioSynthWaveform       hatsBodySquare5th;      //xy=742.5000076293945,418.7500057220459
AudioSynthNoiseWhite     hatsNoise;         //xy=773.7500114440918,457.50000762939453
AudioMixer4              hatsBodyMxr; //xy=948.7500152587891,402.5000057220459
AudioMixer4              hatsMxr;  //xy=1151.250015258789,422.50000762939453
AudioFilterStateVariable hatsBandPass;        //xy=1335.0000228881836,428.7500057220459
AudioFilterStateVariable hatsHiPass;        //xy=1531.4284210205078,434.99999237060547
AudioEffectEnvelope      hatsEnv;      //xy=1701.4285430908203,451.4285945892334
AudioOutputI2S           dacOut;           //xy=1855.7142944335938,457.14281463623047
AudioConnection          patchCord1(hatsBodyFMSineModulator1, 0, hatsBodyFMModulatorMxr, 0);
AudioConnection          patchCord2(hatsBodyFMSineModulator2, 0, hatsBodyFMModulatorMxr, 1);
AudioConnection          patchCord3(hatsBodyFMModulatorMxr, 0, hatsBodySquareFMCarrier, 0);
AudioConnection          patchCord4(hatsBodySquareFMCarrier, 0, hatsBodyMxr, 0);
AudioConnection          patchCord5(hatsBodySquare5th, 0, hatsBodyMxr, 1);
AudioConnection          patchCord6(hatsNoise, 0, hatsMxr, 1);
AudioConnection          patchCord7(hatsBodyMxr, 0, hatsMxr, 0);
AudioConnection          patchCord8(hatsMxr, 0, hatsBandPass, 0);
AudioConnection          patchCord9(hatsBandPass, 1, hatsHiPass, 0);
AudioConnection          patchCord10(hatsHiPass, 2, hatsEnv, 0);
AudioConnection          patchCord11(hatsEnv, 0, dacOut, 0);
AudioControlSGTL5000     audioControl;     //xy=357.25000762939453,95.00000190734863
// GUItool: end automatically generated code


// MIDI setup
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

// Potentiometer pins
const int decayPotPin = A0;    // Controls envelope decay for the hat
const int tonePotPin = A1;     // Controls cut-off frequency of the band pass filter and modulator frequencies
const int timbrePotPin = A2;   // Controls the noise level
const int resonancePotPin = A3; // Controls resonance of the band-pass filter

// Hi-hat parameters
const float baseFreq = 144.0;  // Base frequency for hi-hat body (Atonal)

// Variables for hi-hat simulation
float closedDecayTime = 50.0;  // Default closed hi-hat decay
float openDecayTime = 200.0;   // Default open hi-hat decay
bool isOpenHat = false;

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (channel == 1) {
    if (note == 59) { // Closed hi-hat (note 59)
      triggerHiHat(false); // Closed hi-hat
    } 
    else if (note == 63) { // Open hi-hat (note 63)
      triggerHiHat(true); // Open hi-hat
    }
  }
}

void setup() {
  Serial.begin(9600);
  
  // Initialize MIDI
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.begin(1); // Listen on channel 1
  
  // Initialize potentiometer pins
  pinMode(decayPotPin, INPUT);
  pinMode(tonePotPin, INPUT);
  pinMode(timbrePotPin, INPUT);
  pinMode(resonancePotPin, INPUT);
  
  AudioMemory(128);
  
  // Initialize audio control FIRST before enabling
  audioControl.enable();
  audioControl.volume(0.8); 
  
  // Configure oscillators
  hatsBodySquareFMCarrier.begin(WAVEFORM_SQUARE);
  hatsBodySquare5th.begin(WAVEFORM_SQUARE);
  
  // Set initial frequencies 
  updateOscillatorFrequencies(baseFreq);
  
  // Set initial amplitudes 
  hatsBodySquareFMCarrier.amplitude(0.7);
  hatsBodySquare5th.amplitude(0.5);
  hatsBodyFMSineModulator1.amplitude(0.5);
  hatsBodyFMSineModulator2.amplitude(0.5);
  hatsNoise.amplitude(0.3);

  // Configure Filters
  hatsBandPass.frequency(2000);
  hatsBandPass.resonance(0.7);
  hatsHiPass.frequency(2000);  // 2kHz cut-off
  hatsHiPass.resonance(0.5);   // 0.5 resonance
  
  // Configure mixers (reduced gains to prevent clipping)
  hatsBodyFMModulatorMxr.gain(0, 0.5);  // Sine modulator 1
  hatsBodyFMModulatorMxr.gain(1, 0.5);  // Sine modulator 2
  hatsBodyFMModulatorMxr.gain(2, 0.0);
  hatsBodyFMModulatorMxr.gain(3, 0.0);
  
  hatsBodyMxr.gain(0, 0.5);  // FM Square (carrier)
  hatsBodyMxr.gain(1, 0.3);  // Square 5th
  hatsBodyMxr.gain(2, 0.0);
  hatsBodyMxr.gain(3, 0.0);
  
  hatsMxr.gain(0, 0.8);  // Body
  hatsMxr.gain(1, 0.5);  // Noise
  hatsMxr.gain(2, 0.0);
  hatsMxr.gain(3, 0.0);
  
  // Configure envelope with initial settings
  hatsEnv.attack(1);
  hatsEnv.decay(closedDecayTime); // Start with closed hi-hat decay
  hatsEnv.sustain(0);
  hatsEnv.release(40);
  
  Serial.println("FM Hi-Hat initialized");
  Serial.println("Listening for MIDI notes 59 (closed) and 63 (open) on channel 1");
  Serial.println("Closed hi-hat decay: 15ms - 90ms");
  Serial.println("Open hi-hat decay: 120ms - 300ms");
}

void loop() {
  // Handle MIDI messages
  MIDI.read();
  
  readPotsAndUpdate();
  
  // Add a small delay to prevent overwhelming the processor
  delay(5);
}

void triggerHiHat(bool openHat) {
  isOpenHat = openHat;
  
  // Update decay time based on whether it's an open hat or not
  float currentDecayTime = openHat ? openDecayTime : closedDecayTime;
  
  hatsEnv.decay(currentDecayTime);
  
  // Trigger envelope
  hatsEnv.noteOn();
  
  if (openHat) {
    Serial.print("Open hi-hat triggered with decay: ");
    Serial.print(currentDecayTime);
    Serial.println("ms");
  } else {
    Serial.print("Closed hi-hat triggered with decay: ");
    Serial.print(currentDecayTime);
    Serial.println("ms");
  }
}

void readPotsAndUpdate() {
  // Read decay pot - controls both closed and open decay times
  // The same potentiometer position maps to different ranges for open vs closed
  int potValue = analogRead(decayPotPin);
  
  // Map to different ranges based on hi-hat type
  closedDecayTime = map(potValue, 0, 1023, 25, 70);     // Closed: 15ms - 90ms
  openDecayTime = map(potValue, 0, 1023, 150, 300);     // Open: 120ms - 300ms
  
  // Update current decay time based on open/closed state
  float currentDecayTime = isOpenHat ? openDecayTime : closedDecayTime;
  hatsEnv.decay(currentDecayTime);
  
  // Read tone pot - controls bandpass filter frequency and modulator frequencies
  float filterFreq = map(analogRead(tonePotPin), 0, 1023, 2000, 8000);
  hatsBandPass.frequency(filterFreq);
  
  // Update modulator frequencies with out-of-tune ratios
  float modulator1Freq = baseFreq * 1.3;  // Slightly detuned
  float modulator2Freq = baseFreq * 1.7;  // More detuned
  hatsBodyFMSineModulator1.frequency(modulator1Freq);
  hatsBodyFMSineModulator2.frequency(modulator2Freq);
  
  // Read timbre pot - controls noise level
  float noiseLevel = map(analogRead(timbrePotPin), 0, 1023, 0, 100) / 100.0;
  hatsMxr.gain(1, noiseLevel);
  
  // Read resonance pot - controls bandpass filter resonance
  float resonance = map(analogRead(resonancePotPin), 0, 1023, 100, 400) / 100.0; // Scale to 0-4
  hatsBandPass.resonance(resonance);
}

void updateOscillatorFrequencies(float freq) {
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