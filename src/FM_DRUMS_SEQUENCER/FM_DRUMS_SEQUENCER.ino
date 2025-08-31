#include <LedControl.h>
#include <MIDI.h>

// Create MIDI instance
MIDI_CREATE_DEFAULT_INSTANCE();

// LED Matrix Control
#define DIN_PIN 11
#define CLK_PIN 13
#define CS_PIN 10
LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, 1);

// MIDI Configuration - UPDATED: All voices on channel 1 with new note assignments
const byte MIDI_CHANNEL = 1; // All voices on channel 1
const byte MIDI_NOTES[6] = {53, 55, 59, 63, 65, 69}; // Kick=53, Snare=55, cHat=59, oHat=63, loTom=65, hiTom=69

// Clock Configuration
const unsigned long CLOCK_TIMEOUT = 500000; // 500ms timeout for external clock (µs)
const byte TEMPO_AVERAGE_WINDOW = 12; // Average over 12 pulses (half quarter note)
const byte PPQN = 24; // Pulses per quarter note (standard MIDI clock resolution)

// Button Matrix Configuration
const byte ROWS = 5;
const byte COLS = 5;
byte rowPins[ROWS] = {A0, A1, A2, A3, A4};  // R1-R5
byte colPins[COLS] = {2, 3, 4, 5, 6};       // C1-C5

// Button State Tracking
byte buttonStates[ROWS][COLS] = {0};
byte lastButtonStates[ROWS][COLS] = {0};

// Sequencer Configuration
#define NUM_STEPS 16
byte patterns[6][NUM_STEPS] = {0};
byte currentStep = 0;
byte selectedVoice = 0;
bool isPlaying = false;
bool recordEnabled = false;
unsigned long lastStepTime = 0;
unsigned int currentBPM = 120;
unsigned int stepInterval = 60000 / (currentBPM * 4); // Will be updated by MIDI clock
unsigned long sequenceStartTime = 0;
unsigned long voiceFlashTime[6] = {0};
const int FLASH_DURATION = 100;

// MIDI Clock Tracking
unsigned long lastClockTime = 0;
unsigned long lastClockReceivedTime = 0;
unsigned long clockIntervals[TEMPO_AVERAGE_WINDOW];
byte clockIndex = 0;
byte clockCount = 0;
bool isExternalClock = false;

// LED Mapping
#define STEP_LEDS_ROW1 0   // Steps 1-8
#define STEP_LEDS_ROW2 8   // Steps 9-16  
#define VOICE_LEDS_ROW 24  // Voice indicators
#define STATUS_LEDS_ROW 32 // Status LEDs

// Button Mapping
const byte STEP_BUTTONS[16][2] = {
  {0,0}, {1,0}, {2,0}, {3,0}, // Steps 1-4 (R1-R4 C1)
  {0,1}, {1,1}, {2,1}, {3,1}, // Steps 5-8 (R1-R4 C2)
  {0,2}, {1,2}, {2,2}, {3,2}, // Steps 9-12 (R1-R4 C3)
  {0,3}, {1,3}, {2,3}, {3,3}  // Steps 13-16 (R1-R4 C4)
};

#define BTN_PLAY_ROW 4
#define BTN_PLAY_COL 0
#define BTN_REC_ROW 4
#define BTN_REC_COL 1
#define BTN_SELECT_ROW 4
#define BTN_SELECT_COL 2

const byte VOICE_BUTTONS[6][2] = {
  {4,3}, // Kick (R5 C4)
  {0,4}, // Snare (R1 C5)
  {1,4}, // cHat (R2 C5)
  {2,4}, // oHat (R3 C5)
  {3,4}, // loTom (R4 C5)
  {4,4}  // hiTom (R5 C5)
};

void setup() {
  // Initialize MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleClock(handleClock);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleContinue(handleContinue);
  MIDI.setHandleStop(handleStop);
  MIDI.setHandleActiveSensing(handleActiveSensing);
  
  // Initialize LED Matrix
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  // Initialize Button Matrix
  for (byte r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], INPUT_PULLUP);
  }
  
  for (byte c = 0; c < COLS; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], HIGH);
  }

  // Initialize clock intervals array
  for (byte i = 0; i < TEMPO_AVERAGE_WINDOW; i++) {
    clockIntervals[i] = stepInterval * 4 / PPQN; // Initialize with internal clock interval
  }

  delay(10);
}

void loop() {
  // Read incoming MIDI messages
  MIDI.read();
  
  // Check for external clock timeout
  if (isExternalClock && micros() - lastClockReceivedTime > CLOCK_TIMEOUT) {
    isExternalClock = false;
    clockCount = 0;
    stepInterval = 60000 / (currentBPM * 4); // Reset to internal interval
  }
  
  unsigned long currentTime = millis();
  
  // Read Button Matrix
  readButtons();
  
  // Sequencer Logic
  if (isPlaying) {
    // If using internal clock and no external clock is detected
    if (!isExternalClock && currentTime - lastStepTime >= stepInterval) {
      advanceStep();
      lastStepTime = currentTime;
    }
  }
  
  // Update Display
  updateDisplay();
}

// MIDI Input Handlers
void handleNoteOn(byte channel, byte note, byte velocity) {
  // Check if note matches any of our drum voices
  for (byte i = 0; i < 6; i++) {
    if (note == MIDI_NOTES[i] && channel == MIDI_CHANNEL) {
      triggerVoice(i);
      
      // Record if enabled
      if (recordEnabled && isPlaying) {
        unsigned long elapsedTime = millis() - sequenceStartTime;
        byte nearestStep = ((elapsedTime % (stepInterval * NUM_STEPS)) / stepInterval) % NUM_STEPS;
        patterns[i][nearestStep] = 1;
      }
      return;
    }
  }
}

void handleClock() {
  unsigned long currentTime = micros();
  lastClockReceivedTime = currentTime;
  
  // Store this interval for averaging
  if (lastClockTime > 0) {
    clockIntervals[clockIndex] = currentTime - lastClockTime;
    clockIndex = (clockIndex + 1) % TEMPO_AVERAGE_WINDOW;
    
    // Calculate average interval
    unsigned long avgInterval = 0;
    for (byte i = 0; i < TEMPO_AVERAGE_WINDOW; i++) {
      avgInterval += clockIntervals[i];
    }
    avgInterval /= TEMPO_AVERAGE_WINDOW;
    
    currentBPM = 60000000 / (avgInterval * PPQN);
    stepInterval = (avgInterval * PPQN) / 4; // 16th notes (PPQN/4)
    
    if (clockCount++ > TEMPO_AVERAGE_WINDOW) {
      isExternalClock = true;
    }
  }
  lastClockTime = currentTime;
  
  // Advance step on every 6th clock pulse (16th notes)
  if (isPlaying && isExternalClock && (clockCount % (PPQN/4) == 0)) {
    advanceStep();
  }
}

void handleStart() {
  isPlaying = true;
  currentStep = 0;
  sequenceStartTime = millis();
  clockCount = 0;
  isExternalClock = true;
  lastStepTime = millis();
}

void handleContinue() {
  isPlaying = true;
  isExternalClock = true;
}

void handleStop() {
  isPlaying = false;
  isExternalClock = false;
}

void handleActiveSensing() {
  lastClockReceivedTime = micros();
}

void readButtons() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 20;

  for (byte c = 0; c < COLS; c++) {
    // Activate column
    digitalWrite(colPins[c], LOW);
    delayMicroseconds(50);
    
    // Read rows
    for (byte r = 0; r < ROWS; r++) {
      bool currentState = (digitalRead(rowPins[r]) == LOW);
      
      // Debounce
      if (currentState != lastButtonStates[r][c]) {
        lastDebounceTime = millis();
      }
      
      if ((millis() - lastDebounceTime) > debounceDelay) {
        if (currentState && !buttonStates[r][c]) {
          handleButtonPress(r, c);
        }
        buttonStates[r][c] = currentState;
      }
      
      lastButtonStates[r][c] = currentState;
    }
    
    // Deactivate column
    digitalWrite(colPins[c], HIGH);
    delayMicroseconds(50);
  }
}

void handleButtonPress(byte row, byte col) {
  // Step buttons
  for (byte i = 0; i < 16; i++) {
    if (row == STEP_BUTTONS[i][0] && col == STEP_BUTTONS[i][1]) {
      if (!isPlaying || (isPlaying && recordEnabled)) {
        patterns[selectedVoice][i] ^= 1;
      }
      return;
    }
  }
  
  // Function buttons
  if (row == BTN_PLAY_ROW && col == BTN_PLAY_COL) {
    isPlaying = !isPlaying;
    if (isPlaying) {
      currentStep = 0;
      lastStepTime = millis();
      sequenceStartTime = millis();
      // Send MIDI Start if we're the master
      if (!isExternalClock) {
        MIDI.sendRealTime(midi::Start);
      }
    } else {
      // Send MIDI Stop if we're the master
      if (!isExternalClock) {
        MIDI.sendRealTime(midi::Stop);
      }
    }
    return;
  }
  
  if (row == BTN_REC_ROW && col == BTN_REC_COL) {
    recordEnabled = !recordEnabled;
    return;
  }
  
  // Voice triggers
  for (byte i = 0; i < 6; i++) {
    if (row == VOICE_BUTTONS[i][0] && col == VOICE_BUTTONS[i][1]) {
      if (buttonStates[BTN_SELECT_ROW][BTN_SELECT_COL]) {
        selectedVoice = i;
      } else {
        triggerVoice(i);
        if (recordEnabled && isPlaying) {
          unsigned long elapsedTime = millis() - sequenceStartTime;
          byte nearestStep = ((elapsedTime % (stepInterval * NUM_STEPS)) / stepInterval) % NUM_STEPS;
          patterns[i][nearestStep] = 1;
        }
      }
      return;
    }
  }
}

void triggerVoice(byte voice) {
  // Send MIDI Note On message on channel 1
  MIDI.sendNoteOn(MIDI_NOTES[voice], 127, MIDI_CHANNEL);
  
  // Flash the voice LED
  voiceFlashTime[voice] = millis();
}

void advanceStep() {
  // Only trigger voices that have this step activated
  for (int i = 0; i < 6; i++) {
    if (patterns[i][currentStep]) {
      triggerVoice(i);
    }
  }
  currentStep = (currentStep + 1) % NUM_STEPS;
}

void updateDisplay() {
  lc.clearDisplay(0);
  unsigned long currentTime = millis();

  // Step LEDs (rows 1-3, columns 1-5)
  for (int step = 0; step < NUM_STEPS; step++) {
    // Determine row (D0-D2 for steps 1-15)
    byte row;
    if (step < 5) {         // Steps 1-5 (row 1)
      row = 0;
    } else if (step < 10) { // Steps 6-10 (row 2)
      row = 1;
    } else if (step < 15) { // Steps 11-15 (row 3)
      row = 2;
    } else {                // Step 16 (row 4 column 1)
      row = 3;
    }
    
    // Determine column (1-5)
    byte col;
    if (step < 15) {        // Steps 1-15
      col = (step % 5) + 1; // Columns 1-5
    } else {                // Step 16 (column 1)
      col = 1;
    }
    
    if (patterns[selectedVoice][step]) {
      lc.setLed(0, row, col, true);
    }
  }

  // Current step indicator
  byte currentRow;
  byte currentCol;
  if (currentStep < 5) {         // Steps 1-5 (row 1)
    currentRow = 0;
    currentCol = (currentStep % 5) + 1;
  } else if (currentStep < 10) { // Steps 6-10 (row 2)
    currentRow = 1;
    currentCol = (currentStep % 5) + 1;
  } else if (currentStep < 15) { // Steps 11-15 (row 3)
    currentRow = 2;
    currentCol = (currentStep % 5) + 1;
  } else {                       // Step 16 (row 4 column 1)
    currentRow = 3;
    currentCol = 1;
  }
  lc.setLed(0, currentRow, currentCol, true);

  // Voice triggers (row 4 columns 2-5 and row 5 columns 1-2)
  // Kick (row 4 column 2)
  bool kickFlash = (currentTime - voiceFlashTime[0]) < FLASH_DURATION;
  lc.setLed(0, 3, 2, kickFlash || selectedVoice == 0);
  
  // Snare (row 4 column 3)
  bool snareFlash = (currentTime - voiceFlashTime[1]) < FLASH_DURATION;
  lc.setLed(0, 3, 3, snareFlash || selectedVoice == 1);
  
  // cHat (row 4 column 4)
  bool chatFlash = (currentTime - voiceFlashTime[2]) < FLASH_DURATION;
  lc.setLed(0, 3, 4, chatFlash || selectedVoice == 2);
  
  // oHat (row 4 column 5)
  bool ohatFlash = (currentTime - voiceFlashTime[3]) < FLASH_DURATION;
  lc.setLed(0, 3, 5, ohatFlash || selectedVoice == 3);
  
  // loTom (row 5 column 1)
  bool lotomFlash = (currentTime - voiceFlashTime[4]) < FLASH_DURATION;
  lc.setLed(0, 4, 1, lotomFlash || selectedVoice == 4);
  
  // hiTom (row 5 column 2)
  bool hitomFlash = (currentTime - voiceFlashTime[5]) < FLASH_DURATION;
  lc.setLed(0, 4, 2, hitomFlash || selectedVoice == 5);

  // Status LEDs (row 5 columns 3-4)
  lc.setLed(0, 4, 3, isPlaying);      // Play (row 5 column 3)
  lc.setLed(0, 4, 4, recordEnabled);  // Record (row 5 column 4)
}