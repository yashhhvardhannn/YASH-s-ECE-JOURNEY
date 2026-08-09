/*
  Smart Helmet Transmitter Code
  Features:
  - Wireless transmission of helmet status, alcohol level, and sleep detection
  - Drowsiness detection using a sleep sensor
  - Buzzer alert for drowsiness detection
  - Data sent every 50ms for real-time monitoring
*/

// Include necessary libraries
#include <RH_ASK.h>
#include <SPI.h>

// Debug Mode (Set to true for debugging, false for silent mode)
#define DEBUG_MODE false

// Sensor Pins
const int alcoholSensorPin = A0;
const int wearSensorPin = A1;
const int sleepSensorPin = A2;
const int buzzerPin = 7;  // Buzzer connected to pin 7

// Create RF Object
RH_ASK rf_driver;

// Variables for drowsiness detection
const int drowsinessThreshold = 5;  // Number of sleep signals needed within time window
const unsigned long drowsinessTimeWindow = 10000; // 10 seconds window
const unsigned long resetTime = 5000;  // Reset if no sleep signals within 5 seconds

unsigned long sleepTimestamps[drowsinessThreshold];  // Stores timestamps of detected sleep signals
int sleepIndex = 0;
bool isDrowsy = false;

unsigned long lastSleepDetectionTime = 0;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 50; // Send data every 50ms

void setup() {
#if DEBUG_MODE
  Serial.begin(9600);  // Start Serial Monitor
  Serial.println("System Initialized...");
#endif

  rf_driver.init();  // Initialize RF module
  pinMode(wearSensorPin, INPUT);
  pinMode(alcoholSensorPin, INPUT);
  pinMode(sleepSensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  
  digitalWrite(buzzerPin, LOW);  // Ensure buzzer is OFF initially
}

void loop() {
  unsigned long currentMillis = millis();

  // Read sensor values
  int alcoholLevel = !digitalRead(alcoholSensorPin);
  int sleepSignal = !digitalRead(sleepSensorPin);
  int helmetStatus = !digitalRead(wearSensorPin);

#if DEBUG_MODE
  Serial.print("Helmet: "); Serial.print(helmetStatus);
  Serial.print(" | Alcohol: "); Serial.print(alcoholLevel);
  Serial.print(" | Sleep Signal: "); Serial.println(sleepSignal);
#endif

  // **Drowsiness Detection Logic**
  if (sleepSignal) {
    lastSleepDetectionTime = currentMillis;

    // Store the current timestamp in the circular buffer
    sleepTimestamps[sleepIndex] = currentMillis;
    sleepIndex = (sleepIndex + 1) % drowsinessThreshold;

    // Check if there are 'drowsinessThreshold' sleep detections within 'drowsinessTimeWindow'
    int count = 0;
    for (int i = 0; i < drowsinessThreshold; i++) {
      if (currentMillis - sleepTimestamps[i] <= drowsinessTimeWindow) {
        count++;
      }
    }
    
    isDrowsy = (count >= drowsinessThreshold);
  }

  // **Reset drowsiness detection if no sleep signals for resetTime**
  if (currentMillis - lastSleepDetectionTime > resetTime) {
    isDrowsy = false;
  }

#if DEBUG_MODE
  Serial.print("Drowsiness Detected: ");
  Serial.println(isDrowsy ? "YES" : "NO");
#endif

  // **Activate Buzzer on Drowsiness Detection**
  if (isDrowsy) {
    digitalWrite(buzzerPin, HIGH);  // Turn ON buzzer
  } else {
    digitalWrite(buzzerPin, LOW);   // Turn OFF buzzer
  }

  // **Send data at regular intervals**
  if (currentMillis - lastSendTime >= sendInterval) {
    lastSendTime = currentMillis;

    // Convert sensor values to a formatted string
    String data = String(helmetStatus) + "," + String(alcoholLevel) + "," + String(isDrowsy ? 1 : 0);

    // Convert string to char array for RF transmission
    char msg[data.length() + 1];
    data.toCharArray(msg, data.length() + 1);

    // Send data via RF
    rf_driver.send((uint8_t *)msg, strlen(msg));
    rf_driver.waitPacketSent();

#if DEBUG_MODE
    Serial.print("Data Sent: ");
    Serial.println(data);
#endif
  }

  delay(10); // Small delay to avoid overloading Serial output
}
