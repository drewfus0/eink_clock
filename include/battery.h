#pragma once

#include <Arduino.h>

struct BatteryInfo {
    float voltage = 0.0f;
    uint8_t percentage = 0;
    bool isConnected = false;
};

void initBattery();
BatteryInfo readBattery();
