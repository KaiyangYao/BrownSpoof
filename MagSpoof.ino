#include <ArduinoHttpClient.h>
#include <WiFiS3.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <WDT.h>

// Pin Definitions
#define PIN_A       3       // L293D Output pin for track signal (1Y)
#define PIN_B       4       // L293D Output pin for track signal (2Y)
#define ENABLE_PIN  13      // L293D Enable pin for the coil (EN1,2)
#define BUTTON_PIN  2       // Button input for starting playback
#define BUTTON_PIN_SELECT 6 // Button input for select track index
#define CLOCK_US    400     // Delay for bit transmission, in microseconds
#define LEADING_ZEROS 25    // Number of leading zeros for synchronization
#define TRAILING_ZEROS 25   // Number of trailing zeros for synchronization

// Track2 Data Options
char* track2s[10] = {
  ";6009550002147835?",  // Anoop 1
  ";6009550001931114?",  // Alex 2 
  ";6009550002492900?",  // Korgan 3 
  ";6009550002372300?"   // Kaiyang 4 
};

const int numTrack2s = sizeof(track2s) / sizeof(track2s[0]);
const int sublen = 48;  // ASCII offsets for Track 2
const int bitlen = 5;   // Bits per character for Track 2
volatile int currentTrack2Index = 0;

int polarity = 0;       // Coil direction toggle

// Wi-Fi and HTTP details
// char ssid[] = "Pixel_2078";    // Replace with your hotspot name
// char pass[] = "embedded";      // Replace with your hotspot password

char ssid[] = "Pixel_2078";    // Replace with your hotspot name
char pass[] = "embedded";      // Replace with your hotspot password

char serverAddress[] = "brown-spoof-server.vercel.app"; // Server address
int port = 443;                 // HTTP port

WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;

ArduinoLEDMatrix matrix;

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
void playTrack2(const char* trackData) {
  int tmp, crc, lrc = 0;

  Serial.println("Playing Track 2 data:");
  Serial.println(trackData);

  // Enable the H-bridge
  digitalWrite(ENABLE_PIN, HIGH);

  // Send leading zeros
  for (int i = 0; i < LEADING_ZEROS; i++) {
    playBit(0);
  }

  // Send the track data
  for (int i = 0; trackData[i] != '\0'; i++) {
    crc = 1; // Start with odd parity
    tmp = trackData[i] - sublen; // Adjust ASCII offsets
    for (int j = 0; j < bitlen - 1; j++) {
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
  for (int j = 0; j < bitlen - 1; j++) {
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

void displayIndexOnMatrix(int index) {
  matrix.beginDraw();
  matrix.textFont(Font_5x7);
  matrix.beginText(4, 1, 0xFFFFFF);
  matrix.println(index);
  matrix.endText(NO_SCROLL);
  matrix.endDraw();
}

// Fetch data from the HTTP endpoint
void fetchTrack2Data() {
  Serial.println("Making GET request...");
  client.get("/track2s");

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  if (statusCode == 200) {
    // Example: Update track2s array based on server response (assume JSON array)
    // This part can be extended to parse the response dynamically
    Serial.println("Fetched track data successfully.");
  } else {
    Serial.println("Failed to fetch track data.");
  }
}

void setup() {
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN_SELECT, INPUT_PULLUP);

  matrix.begin();
  displayIndexOnMatrix(currentTrack2Index); // Display the initial index

  Serial.begin(9600);
  Serial.println("Setup complete. Waiting for button press.");
  if (WDT.begin(4000)) {
    Serial.println("Watchdog initialized with 4s timeout.");
  } else {
    Serial.println("Failed to initialize watchdog.");
  }
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(2000);
  }

  Serial.println("Wi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Fetch initial data
  fetchTrack2Data();
}

void loop() {
  if (digitalRead(BUTTON_PIN_SELECT) == LOW) {
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN_SELECT) == LOW) {
      currentTrack2Index = (currentTrack2Index + 1) % numTrack2s;
      Serial.print("Selected index: ");
      Serial.println(currentTrack2Index);
      displayIndexOnMatrix(currentTrack2Index);
    }
    while (digitalRead(BUTTON_PIN_SELECT) == LOW); // Wait for release
  }

  if (digitalRead(BUTTON_PIN) == LOW) { // Button pressed
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) { // Confirm button press
      Serial.print("Button pressed. Spoofing Track 2 data index: ");
      Serial.println(currentTrack2Index);
      playTrack2(track2s[currentTrack2Index]);
    }
    while (digitalRead(BUTTON_PIN) == LOW); // Wait for release
  }

  // Periodically fetch new data every 60 seconds
  static unsigned long lastFetchTime = 0;
  if (millis() - lastFetchTime > 60000) {
    fetchTrack2Data();
    lastFetchTime = millis();
  }
}
