#include <Arduino.h>

// Pin Definitions
#define PIN_A       3      // L293D Output pin for track signal (1Y)
#define PIN_B       4      // L293D Output pin for track signal (2Y)
#define ENABLE_PIN  13     // L293D Enable pin for the coil (EN1,2)
#define BUTTON_PIN  2      // Button input for starting playback
#define CLOCK_US    400    // Delay for bit transmission, in microseconds
#define LEADING_ZEROS 40   // Number of leading zeros for synchronization
#define TRAILING_ZEROS 40  // Number of trailing zeros for synchronization

// Track Data
const char* tracks[] = {
  // "%B4503111122223333^MIAOSKI LIN              ^2010200999909999999900990000000?", // Track 1
  // ";4503111122223333=200120099999999?"                                            // Track 2
  
  ";6009550002147835?",  // Anoop 1
  ";6009550001931114?",  // Alex 2 
  ";6009550002492900?",  // Korgan 3 
  ";6009550002372300?"  // Kaiyang 4 




};

// const int sublen[] = {32, 48};  // ASCII offsets for Track 1 and Track 2
// const int bitlen[] = {7, 5};    // Bits per character for Track 1 and Track 2
const int sublen[] = {48, 48, 48, 48};  // ASCII offsets for Track 1 and Track 2
const int bitlen[] = {5, 5, 5, 5};    // Bits per character for Track 1 and Track 2

volatile bool playTrackFlag = false; // Flag to indicate track playback
int polarity = 0;                    // Coil direction toggle
unsigned int curTrack = 0;           // Current track index (alternates between Track 1 and Track 2)

// Set polarity for the coil
void setPolarity(int polarity) {
  if (polarity) {
    digitalWrite(PIN_B, LOW);
    digitalWrite(PIN_A, HIGH);
  } else {
    digitalWrite(PIN_A, LOW);
    digitalWrite(PIN_B, HIGH);
  }
}

// Play a single bit
void playBit(int bit) {
  polarity ^= 1;
  setPolarity(polarity);
  delayMicroseconds(CLOCK_US);

  if (bit == 1) {
    polarity ^= 1;
    setPolarity(polarity);
  }

  delayMicroseconds(CLOCK_US);
}

// Play a full track, including leading/trailing zeros and error checking
void playTrack(int track) {
  int tmp, crc, lrc = 0;
  track--; // Convert track to zero-based index

  Serial.print("Playing Track ");
  Serial.println(track + 1);

  // Enable the H-bridge
  digitalWrite(ENABLE_PIN, HIGH);

  // Send leading zeros
  for (int i = 0; i < LEADING_ZEROS; i++) {
    playBit(0);
  }

  // Send the track data
  for (int i = 0; tracks[track][i] != '\0'; i++) {
    crc = 1; // Start with odd parity
    tmp = tracks[track][i] - sublen[track]; // Adjust ASCII offset

    for (int j = 0; j < bitlen[track] - 1; j++) {
      crc ^= tmp & 1;        // Calculate parity
      lrc ^= (tmp & 1) << j; // Update LRC
      playBit(tmp & 1);
      tmp >>= 1;
    }
    playBit(crc); // Send parity bit
  }

  // Send LRC (Longitudinal Redundancy Check)
  tmp = lrc;
  crc = 1;
  for (int j = 0; j < bitlen[track] - 1; j++) {
    crc ^= tmp & 1;
    playBit(tmp & 1);
    tmp >>= 1;
  }
  playBit(crc); // Send final parity bit for LRC

  // Send trailing zeros
  for (int i = 0; i < TRAILING_ZEROS; i++) {
    playBit(0);
  }

  // Disable the H-bridge
  digitalWrite(ENABLE_PIN, LOW);

  Serial.println("Track playback complete.");
}

void setup() {
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("Setup complete. Waiting for button press.");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) { // Button pressed
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) { // Confirm button press
      Serial.println("Button pressed. Playing track.");
      playTrack(curTrack + 1); // Play the current track (1-based index)
      Serial.println(sizeof(tracks));
      curTrack = (curTrack + 1) % 4; // Alternate between Track 1 and Track 2
    }
    while (digitalRead(BUTTON_PIN) == LOW); // Wait for button release
  }
}
