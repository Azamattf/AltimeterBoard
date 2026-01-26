#include <main.h>

MS5611 ms5611(0x76); // 0x77 since PS is at 3V3
TM1637TinyDisplay display(CLK_PIN, DIO_PIN);

void onButtonPress() {
  unsigned long currentTime = millis();
  // Simple software debounce
  if ((currentTime - lastDebounceTime) > debounceDelay) {
    requestCalibration = true; // Raise the flag
    lastDebounceTime = currentTime;
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 2000); // Don't wait forever 

  pinMode(CAL_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ALARM_PIN, OUTPUT);

  digitalWrite(ALARM_PIN, LOW);

  display.begin();
  display.setBrightness(BRIGHT_HIGH);
  display.showString("INIT");

  Wire.setSDA(18);  
  Wire.setSCL(19);
  Wire.begin();

 

  if (!ms5611.begin()) {

    Serial.println("MS5611 Error");
    display.showString("Err ");
    while(1); // Stop here if broken
  }

  ms5611.setOversampling(OSR_ULTRA_HIGH);

  attachInterrupt(digitalPinToInterrupt(CAL_BUTTON_PIN), onButtonPress, FALLING);
  performCalibration();
}

void loop() {
  unsigned long currentMillis = millis();

  if (requestCalibration) {
    requestCalibration = false;
    performCalibration();
  }

  // Read Sensor (Frequency of 50Hz)
  if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentMillis;

      float tempReads[3] = {0.0};

      // for(size_t i = 0; i < 3; i++) {
      //   ms5611.read();
      //   float rawPressure = ms5611.getPressure();

      //   // Calculate raw altitude
      //   float rawAltitude = 44330.0 * (1.0 - pow((rawPressure / referencePressure), 0.1903));
      //   tempReads[i] = rawAltitude;

      //   delay(25);
      // }

      // if (tempReads[0] >= tempReads[1] && tempReads[1] >= tempReads[2] ||
      //   tempReads[0] <= tempReads[1] && tempReads[1] <= tempReads[2]) {

          // --- LOW PASS FILTER ---
          // This is the magic. It blends 95% of the old value with 5% of the new value.
          // It smooths out the "spikes" but still reacts to changes.
          // Adjust 0.05 up (0.1) for faster reaction, down (0.01) for smoother numbers.
          // float avgRead = (tempReads[0] + tempReads[1] + tempReads[2]) / 3.0;

          ms5611.read();
          float rawPressure = ms5611.getPressure();

          float rawAltitude = 44330.0 * (1.0 - pow((rawPressure / referencePressure), 0.1903));
          smoothedAltitude = ((smoothedAltitude * 0.99) + (rawAltitude * 0.01));

          delay(25);

        // }
    // Check alarm instantly with every new reading
    updateAlarm(smoothedAltitude);
  }

  // 3. Update Display (Slower, e.g., 4Hz - readable)
  if (currentMillis - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = currentMillis;

    altCm = (int)(smoothedAltitude * 100.0);

    Serial.print("Alt: ");
    Serial.print(smoothedAltitude);
    Serial.print("m | Cm: ");
    Serial.println(altCm);

    displayNumber(altCm);
  }
} 

// ====================================
// ========= Helper Functions =========
// ====================================

void performCalibration() {
  display.showString("CAL ");
  Serial.println("Calibrating... Keep stationary.");

  double pressureSum = 0;
  int samples = 75;

  for (int i = 0; i < samples; i++) {
    if (ms5611.read() != MS5611_READ_OK) {
      Serial.println("ERROR: Failed to read from barometer");
      return;
    }

    pressureSum += ms5611.getPressure();
    delay(30); // Wait 30ms to ensure new data conversion (Safe for OSR 4096)
  }

  referencePressure = pressureSum / samples;

  // Reset our smoothed altitude to 0 immediately
  smoothedAltitude = 0.0;
  Serial.print("New Reference: ");
  Serial.println(referencePressure);
  display.showString("Done");
  delay(500);
}

void displayNumber(int number) {
  // TM1637 can display -999 to 9999
  if (number < -999) number = -999;
  if (number > 9999) number = 9999;

  display.showNumber(number);
}

float readAltitude() {
  ms5611.read();
  float pressure = ms5611.getPressure();

  float altitude = 44330.0 * (1.0 - pow((pressure / referencePressure), 0.1903));
  return altitude;
}

void updateAlarm(float altitude) {
  unsigned long t = millis();
  bool danger = (altitude >= Danger_HIGH) || (altitude <= Danger_LOW);
  bool warn = (!danger) && ((altitude >= warn_HIGH) || (altitude <= warn_LOW));

  if (danger) {
    digitalWrite(ALARM_PIN, HIGH);  // ON
  } else if (warn) {
    bool ledOn = (t % 1200) < 600;  // True for first 600ms of each 1200ms cycle
    digitalWrite(ALARM_PIN, ledOn); // slow blink
  } else {
    digitalWrite(ALARM_PIN, LOW); // off
  }
  static unsigned long lastCall = 0;
  static int callCount = 0;
  
  callCount++;
  if (millis() - lastCall > 1000) {
    Serial.print("updateAlarm called ");
    Serial.print(callCount);
    Serial.print(" times in last second, altitude=");
    Serial.println(altitude);
    Serial.print(", danger=");
    Serial.print(danger);
    Serial.print(", warn=");
    Serial.print(warn);
    Serial.print(", Alarm_Pin=");
    Serial.println(digitalRead(ALARM_PIN));
    callCount = 0;
    lastCall = millis();
  }
}