#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MS5611.h>

const int LATCH_PIN = 10; // 7seg Click latch
const int CAL_BUTTON_PIN = 2; // Calibration button 
const int Alarm_Pin = 3; 

SPISettings spiSettings(2e6, MSBFIRST, SPI_MODE0);

MS5611 ms5611(0x76); // Use address 0x76 since PS is at GND

const byte segmentMap[] = {
  0x7E, // 0
  0x0A, // 1
  0xB6, // 2
  0x9E, // 3
  0xCA, // 4
  0xDC, // 5
  0xFC, // 6
  0x0E, // 7
  0xFE, // 8
  0xDE  // 9
};

float referencePressure = 0;

// Alarm limits
const float warn_HIGH = 30.0; // meters
const float warn_LOW = -10.0;  // meters
const float Danger_HIGH = 40.0; // meters
const float Danger_LOW = -15.0;  // meters

void setup() {
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CAL_BUTTON_PIN, INPUT_PULLUP);
  pinMode(Alarm_Pin, OUTPUT);
  digitalWrite(Alarm_Pin, LOW); // Ensure alarm starts OFF
  
  SPI.begin();
  Wire.begin();
  Serial.begin(9600);
  delay(1000);

  if (!ms5611.begin()) { 
    Serial.println("MS5611 not detected!");
    while (1); // Halt if sensor not found
  }

  // Wait for sensor to stabilize and take multiple readings for accurate calibration
  delay(500);
  float pressureSum = 0;
  for (int i = 0; i < 10; i++) {
    ms5611.read();
    pressureSum += ms5611.getPressure();
    delay(50);
  }
  referencePressure = pressureSum / 10.0; // Average of 10 readings
  Serial.println("Altimeter calibrated (0 m)");
}

// --- Function to display 2-digit number ---
void displayNumber(int number) {
  if (number < 0) number = 0;
  if (number > 99) number = 99;

  int tens = number / 10;
  int units = number % 10;

  digitalWrite(LATCH_PIN, LOW);
  SPI.beginTransaction(spiSettings);
  SPI.transfer(segmentMap[units]);
  SPI.transfer(segmentMap[tens]);
  SPI.endTransaction();
  digitalWrite(LATCH_PIN, HIGH);
}

//  Function to read altitude
float readAltitude() {
  ms5611.read();
  float pressure = ms5611.getPressure(); 
  float altitude = 44330.0 * (1.0 - pow(pressure / referencePressure, 0.1903));
  return altitude;
}

//  Alarm function
void updateAlarm(float altitude) {
  unsigned long t = millis();
  bool danger = (altitude >= Danger_HIGH) || (altitude <= Danger_LOW);
  bool warn = (!danger) && ((altitude >= warn_HIGH) || (altitude <= warn_LOW));
  
  if (danger) {
    digitalWrite(Alarm_Pin, (t / 200) % 2); // fast blink
  } else if (warn) {
    digitalWrite(Alarm_Pin, (t / 600) % 2); // slow blink
  } else {
    digitalWrite(Alarm_Pin, LOW); // off
  }
}

void loop() {
  // Button handling - Recalibration
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(CAL_BUTTON_PIN);

  if (lastButtonState == HIGH && buttonState == LOW) {
    delay(50); // Debounce
    ms5611.read();
    referencePressure = ms5611.getPressure();
    Serial.println("Recalibrated: altitude set to 0 m");
  }
  lastButtonState = buttonState;

  // Read and display altitude
  float altitude = readAltitude();
  if (altitude != -1000) {
    int altCm = (int)((altitude * 100.0) + 0.5); // Convert to centimeters and round
    displayNumber(abs(altCm)); // Display absolute value (0-99 cm)
    updateAlarm(altitude);
    
    Serial.print("Altitude: ");
    Serial.print(altitude, 2); // float with two decimal places
    Serial.print(" m (");
    Serial.print(altCm);
    Serial.println(" cm)");
  } else {
    Serial.println("Error reading altitude");
  }
  
  delay(500); // Update every 500ms
}