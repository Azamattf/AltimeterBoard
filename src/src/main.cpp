#include <Arduino.h>
#include <Wire.h>
#include <MS5611.h>
#include <TM1637TinyDisplay.h>

// TM1637 Pins
const int CLK_PIN = 13; // Clock pin
const int DIO_PIN = 11;  // Data pin

const int CAL_BUTTON_PIN = 2; // Calibration button 
const int Alarm_Pin = 3; 

MS5611 ms5611(0x76); // Use address 0x76 since PS is at GND
TM1637TinyDisplay display(CLK_PIN, DIO_PIN);

float referencePressure = 0;

// Alarm limits
const float warn_HIGH = 30.0; // meters
const float warn_LOW = -10.0;  // meters
const float Danger_HIGH = 40.0; // meters
const float Danger_LOW = -15.0;  // meters

void setup() {
  pinMode(CAL_BUTTON_PIN, INPUT_PULLUP);
  pinMode(Alarm_Pin, OUTPUT);
  digitalWrite(Alarm_Pin, LOW); // Ensure alarm starts OFF
  
  display.begin();
  display.setBrightness(BRIGHT_HIGH); // Maximum brightness
  
  Wire.begin();
  Serial.begin(9600);
  delay(1000);

  if (!ms5611.begin()) { 
    Serial.println("MS5611 not detected!");
    display.showString("Err ");
    while (1); // Halt if sensor not found
  }

  // Show "CAL" on display during calibration
  display.showString("CAL ");

  // Wait for sensor to stabilize and take multiple readings
  delay(500);
  float pressureSum = 0;
  for (int i = 0; i < 10; i++) {
    ms5611.read();
    pressureSum += ms5611.getPressure();
    delay(50);
  }
  referencePressure = pressureSum / 10.0; // Average of 10 readings
  Serial.println("Altimeter calibrated (0 m)");
  
  delay(1000); // Show "CAL" for 1 second
}

// Display number in centimeters
void displayNumber(int number) {
  // TM1637 can display -999 to 9999
  if (number < -999) number = -999;
  if (number > 9999) number = 9999;
  
  display.showNumber(number);
}

// Function to read altitude
float readAltitude() {
  ms5611.read();
  float pressure = ms5611.getPressure(); 
  float altitude = 44330.0 * (1.0 - pow(pressure / referencePressure, 0.1903));
  return altitude;
}

// Alarm function
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
    
    // Show "CAL" briefly
    display.showString("CAL ");
    delay(500);
  }
  lastButtonState = buttonState;

  // Read and display altitude
  float altitude = readAltitude();
  if (altitude != -1000) {
    int altCm = (int)((altitude * 100.0) + 0.5); // Convert to centimeters and round
    displayNumber(altCm); // Display in centimeters (can show -999 to 9999 cm)
    updateAlarm(altitude);
    
    Serial.print("Altitude: ");
    Serial.print(altitude, 2);
    Serial.print(" m (");
    Serial.print(altCm);
    Serial.println(" cm)");
  } else {
    Serial.println("Error reading altitude");
    display.showString("Err ");
  }
  
  delay(500); // Update every 500ms
}