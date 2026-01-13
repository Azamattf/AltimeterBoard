#include <Wire.h>

#define MS5611_ADDR 0x76  // 0x77 if CSB=3.3V

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 10000);
  Serial.println("=== MS5611 RAW I2C TEST ===");
  
  // Teensy 4.0 I2C pins
  pinMode(18, INPUT_PULLUP);
  pinMode(19, INPUT_PULLUP);
  Wire.setSDA(18);
  Wire.setSCL(19);
  Wire.begin();
  delay(1000);
  
  // Test 1: Check if device responds
  Serial.println("\n1. Testing device presence...");
  Wire.beginTransmission(MS5611_ADDR);
  byte error = Wire.endTransmission();
  Serial.print("I2C error code: ");
  Serial.println(error);
  Serial.print("Device present: ");
  Serial.println(error == 0 ? "YES" : "NO");
  
  if (error != 0) {
    Serial.println("ERROR: Device not responding at address 0x");
    Serial.println(MS5611_ADDR, HEX);
    Serial.println("Check: 1) Wiring 2) PS=3.3V 3) CSB=GND/3.3V");
    return;
  }
  
  // Test 2: Send reset command
  Serial.println("\n2. Sending reset command...");
  Wire.beginTransmission(MS5611_ADDR);
  Wire.write(0x1E);  // Reset command
  error = Wire.endTransmission();
  delay(10);  // Wait for reset
  
  // Test 3: Read PROM coefficients
  Serial.println("\n3. Reading PROM coefficients...");
  uint16_t prom[8];
  
  for (int i = 0; i < 8; i++) {
    Wire.beginTransmission(MS5611_ADDR);
    Wire.write(0xA0 + (i * 2));  // PROM read addresses
    error = Wire.endTransmission();
    
    if (error == 0) {
      Wire.requestFrom(MS5611_ADDR, 2);
      if (Wire.available() >= 2) {
        prom[i] = Wire.read() << 8;
        prom[i] |= Wire.read();
        Serial.print("PROM[");
        Serial.print(i);
        Serial.print("] = 0x");
        if (prom[i] < 0x1000) Serial.print("0");
        Serial.print(prom[i], HEX);
        Serial.print(" (");
        Serial.print(prom[i]);
        Serial.println(")");
        
        // Check if value is valid
        if (prom[i] == 0x0000 || prom[i] == 0xFFFF) {
          Serial.println("WARNING: Invalid PROM value!");
        }
      }
    }
    delay(10);
  }
  
  // Test 4: Read pressure
  Serial.println("\n4. Testing pressure conversion...");
  
  // Start D1 conversion (pressure)
  Wire.beginTransmission(MS5611_ADDR);
  Wire.write(0x48);  // D1 conversion, OSR=4096
  error = Wire.endTransmission();
  
  if (error == 0) {
    delay(10);  // Wait for conversion
    Serial.println("Conversion started...");
    
    // Read ADC result
    Wire.beginTransmission(MS5611_ADDR);
    Wire.write(0x00);  // ADC read command
    error = Wire.endTransmission();
    
    if (error == 0) {
      Wire.requestFrom(MS5611_ADDR, 3);
      if (Wire.available() >= 3) {
        uint32_t D1 = Wire.read() << 16;
        D1 |= Wire.read() << 8;
        D1 |= Wire.read();
        
        Serial.print("Raw pressure ADC (D1): ");
        Serial.println(D1);
        
        if (D1 == 0) {
          Serial.println("ERROR: ADC returned 0 - conversion failed!");
        }
      }
    }
  }
}

void loop() {}