/*
  Smart Helmet Reciever code
  Features:
  - Wireless communication via RF module (2000 bps, RX pin 11, TX pin 12)
  - LCD display (I2C Address: 0x27)
  - Ignition control based on helmet status and alcohol detection
  - Buzzer alerts for sleep detection or alcohol presence
  - Automatic page switching on LCD every 3 seconds
  - Timeout alert when no data is received for 5 seconds
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RH_ASK.h>
#include <SPI.h>

// Initialize LCD display and RF module
LiquidCrystal_I2C lcd(0x27, 16, 2);
RH_ASK rf_driver(2000, 7, 12, 0);

// Define pin connections
const int ignitionRelay = 4;
const int buzzerPin = 5;

// Variables to store received data
String str_helmet = "0", str_alcohol = "0", str_sleep = "0";
int helmetStatus = 0, alcoholLevel = 0, sleepStatus = 0;
bool engineState = false;

// Timeout and page switching variables
unsigned long lastReceivedTime = 0;
const unsigned long timeout = 5000;
int page = 0, lastPage = -1;
unsigned long lastPageSwitch = 0;
const unsigned long pageInterval = 3000;

void setup() {
  Serial.begin(115200); // Initialize serial communication
  rf_driver.init(); // Initialize RF module

  pinMode(ignitionRelay, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(ignitionRelay, LOW); // Default: Vehicle OFF
  digitalWrite(buzzerPin, LOW); // Default: No alert

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Helmet");
  delay(1000);
  lcd.clear();

}

void loop() {
  uint8_t buf[20];
  uint8_t buflen = sizeof(buf);

  // Check if RF data is received
  if (rf_driver.recv(buf, &buflen)) {
    String str_out = String((char*)buf);
    lastReceivedTime = millis(); // Update last received time

    // Extract values from received string
    int comma1 = str_out.indexOf(',');
    int comma2 = str_out.lastIndexOf(',');

    if (comma1 > 0 && comma2 > comma1) {
      str_helmet = str_out.substring(0, comma1);
      str_alcohol = str_out.substring(comma1 + 1, comma2);
      str_sleep = str_out.substring(comma2 + 1);

      helmetStatus = str_helmet.toInt();
      alcoholLevel = str_alcohol.toInt();
      sleepStatus = str_sleep.toInt();

      // Control vehicle ignition based on conditions
      engineState = (helmetStatus && !alcoholLevel);
      digitalWrite(ignitionRelay, engineState);

      // Activate buzzer if sleep or alcohol detected
      digitalWrite(buzzerPin, (sleepStatus || alcoholLevel || page == 99) ? HIGH : LOW);

      Serial.print("Received: ");
      Serial.println(str_out);
    }
  }

  // Handle timeout if no data received for a set time
  if (millis() - lastReceivedTime > timeout) {
    if (page != 99) {
      digitalWrite(buzzerPin, HIGH);
      lcd.clear();
      lcd.print("No Data Received");
      lcd.setCursor(0, 1);
      lcd.print("System Offline");
      page = 99;
    }
    return;
  }

  // Handle automatic page switching
  if (millis() - lastPageSwitch > pageInterval) {
    lastPageSwitch = millis();
    page = (page + 1) % 2;
    delay(200); // Small delay to prevent rapid switching
  }

  // Clear LCD only when switching pages
  if (page != lastPage) {
    lcd.clear();
    lastPage = page;
  }

  // Display different pages based on current page index
  switch (page) {
    case 0:  // Page 1: Helmet, Alcohol, and Sleep Status
      lcd.setCursor(0, 0);
      lcd.print("Helmet: ");
      lcd.print(helmetStatus == 1 ? "Yes" : "No ");
      lcd.setCursor(0, 1);
      lcd.print("Alcohol: ");
      lcd.print(alcoholLevel == 1 ? "Yes" : "No ");
      break;

    case 1:  // Page 2: Sleep Status and Engine State
      lcd.setCursor(0, 0);
      lcd.print("Sleep: ");
      lcd.print(sleepStatus == 1 ? "Yes" : "No ");
      lcd.setCursor(0, 1);
      lcd.print("Engine: ");
      lcd.print(engineState ? "ON " : "OFF");
      break;
  }
}
