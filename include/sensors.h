#pragma once

#include <Arduino.h>

struct SensorData {
    float temperatureC = 0.0f;
    float temperatureF = 0.0f;
    float humidityPercent = 0.0f;
    uint8_t aqiUba = 1;        // 1: Excellent, 2: Good, 3: Moderate, 4: Poor, 5: Unhealthy
    uint16_t tvocPpb = 0;      // Total Volatile Organic Compounds (ppb)
    uint16_t eco2Ppm = 400;    // Equivalent CO2 (ppm)
    bool ahtSuccess = false;
    bool ensSuccess = false;

    const char* getAqiDescription() const {
        switch (aqiUba) {
            case 1: return "EXCELLENT";
            case 2: return "GOOD";
            case 3: return "MODERATE";
            case 4: return "POOR";
            case 5: return "VERY POOR";
            default: return "UNKNOWN";
        }
    }
};

// Initialize I2C bus and sensors
bool initSensors();

// Read all sensor values with environmental compensation
SensorData readSensors();
