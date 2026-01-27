#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <MS5611.h>
#include <TM1637TinyDisplay.h>

const int errorInCm = 50;

// alpha for Low Pass Filter
const float alpha = 0.03;

// median filter
#define MEDIAN_BUFFER_SIZE 5
float medianBuffer[MEDIAN_BUFFER_SIZE];
int medianIndex = 0;

// TM1637 Pins
const int CLK_PIN = 14; // Clock pin
const int DIO_PIN = 15;  // Data pin

const int CAL_BUTTON_PIN = 21; // Calibration button
const int ALARM_PIN = 22;

int altCm = 0;
int prevAltCm = 0;

float referencePressure = 1013.25; // set the default sea level atmospheric pressure
float smoothedAltitude = 0.0; // We will filter this value
// Alarm limits
const float warn_HIGH = 30.0; // meters
const float warn_LOW = -5.0;  // meters
const float Danger_HIGH = 40.0; // meters
const float Danger_LOW = -10.0;  // meters

// Timers (Non-blocking)
unsigned long lastDisplayUpdate = 0;
unsigned long lastSensorRead = 0;
const int DISPLAY_INTERVAL = 250; // Update screen 4 times a second
const int SENSOR_INTERVAL = 20;   // Read sensor 50 times a second

volatile bool requestCalibration = false;

volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 400; // 400ms lockout to prevent double presses

void performCalibration();

// Display number in centimeters
void displayNumber(int number);

// Get altitude in meters
float readAltitude();

// Alarm function
void updateAlarm(float altitude);