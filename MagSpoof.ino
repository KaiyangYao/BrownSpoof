#include <ArduinoHttpClient.h>
#include <WiFiS3.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <WDT.h>

// Pin Definitions
#define PIN_A       3       // L293D Output pin for track signal (1Y)
#define PIN_B       4       // L293D Output pin for track signal (2Y)
#define ENABLE_PIN  13      // L293D Enable pin for the coil (EN1,2)
#define BUTTON_PIN  5       // Button input for starting playback
#define BUTTON_PIN_SERVER 2        // initializing button for sync with server, use 2 or 3 for interrupt
#define BUTTON_PIN_SELECT 6 // Button input for select track index
#define CLOCK_US    400     // Delay for bit transmission, in microseconds
#define LEADING_ZEROS 25    // Number of leading zeros for synchronization
#define TRAILING_ZEROS 25   // Number of trailing zeros for synchronization


// Track2 Data Options
char track2s[10][32] = {";0?"}; // Initialize all entries to empty strings
int numTrack2s = 0;          // Count of currently loaded tracks
const int sublen = 48;       // ASCII offsets for Track 2
const int bitlen = 5;        // Bits per character for Track 2
volatile int currentTrack2Index = 0;
volatile bool getFlag = false;


int polarity = 0;       // Coil direction toggle

// Wi-Fi and HTTP details
char ssid[] = "Magspoof";  // Replace with your hotspot name
char pass[] = "12345678";    // Replace with your hotspot password

char serverAddress[] = "brown-spoof-server.vercel.app"; // Server address
int port = 443;             // HTTPS port

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
  WDT.refresh();
  Serial.println("Making GET request...");
  client.get("/track2s");

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  if (statusCode == 200) {
    // Clean up the response
    response.replace("[", "");  // Remove opening bracket
    response.replace("]", "");  // Remove closing bracket
    response.replace("\"", ""); // Remove quotes

    numTrack2s = 0;

    // Split the response by commas and populate the track2s array
    int start = 0;
    int end = response.indexOf(",");
    while (end != -1 && numTrack2s < 10) {
      String track = response.substring(start, end);
      track.trim();
      strncpy(track2s[numTrack2s], track.c_str(), 32);      // Copy to array
      track2s[numTrack2s][31] = '\0';                       // Null-terminate
      numTrack2s++;
      start = end + 1;
      end = response.indexOf(",", start);
    }

    // Add the last track
    if (numTrack2s < 10) {
      String track = response.substring(start, end);
      track.trim();
      strncpy(track2s[numTrack2s], track.c_str(), 32);
      track2s[numTrack2s][31] = '\0'; // Null-terminate
      numTrack2s++;
    }

    Serial.println("Fetched and updated Track 2 data:");
    for (int i = 0; i < numTrack2s; i++) {
      Serial.print("Track ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(track2s[i]);
      WDT.refresh();
      
    }
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
  pinMode(BUTTON_PIN_SERVER, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN_SERVER), getISR, FALLING);

  matrix.begin();
  displayIndexOnMatrix(currentTrack2Index); // Display the initial index

  Serial.begin(9600);
  Serial.println("Setup complete. Waiting for button press.");
  if (WDT.begin(5000)) {
    Serial.println("Watchdog initialized with 5s timeout.");
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

  
}
//ISR intialization
void getISR() {
  getFlag = true;  
}

void loop() {
  
  if (digitalRead(BUTTON_PIN_SELECT) == LOW) {
    // noInterrupts();
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN_SELECT) == LOW) {
      currentTrack2Index = (currentTrack2Index + 1) % numTrack2s;
      Serial.print("Selected index: ");
      Serial.println(currentTrack2Index);
      displayIndexOnMatrix(currentTrack2Index);
    }
    while (digitalRead(BUTTON_PIN_SELECT) == LOW); // Wait for release
    WDT.refresh();
    interrupts();
  }

  if (digitalRead(BUTTON_PIN) == LOW) { // Button pressed
    // noInterrupts();
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) { // Confirm button press
      Serial.print("Button pressed. Spoofing Track 2 data index: ");
      Serial.println(currentTrack2Index);
      playTrack2(track2s[currentTrack2Index]);
    }
    while (digitalRead(BUTTON_PIN) == LOW); // Wait for release
    WDT.refresh();
  }


  if (getFlag) {
    //turning interrupt service off
    Serial.println("Fetching track data...");

    
    // Fetch track data
    fetchTrack2Data();

    detachInterrupt(digitalPinToInterrupt(BUTTON_PIN_SERVER));
    delay(3000);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN_SERVER), getISR, FALLING);
    getFlag = false;
    WDT.refresh();
  }
  WDT.refresh();
}


