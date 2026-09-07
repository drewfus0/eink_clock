#pragma once

#include <Arduino.h>
#include "sensors.h"
#include "battery.h"
#include "time_sync.h"

struct SystemState {
    uint32_t bootCount = 0;
    time_t lastNtpSyncEpoch = 0;
    bool ntpJustSynced = false;
    uint32_t updateIntervalSec = 60;
    char otaUrl[64] = "";
    float lastNtpDiffSec = 0.0f;
    bool hasNtpDiff = false;
    int32_t secToNextNtp = 0;
};

// Render full clock and telemetry dashboard to e-paper buffer
void renderDashboard(const TimeInfo& timeInfo, 
                     const SensorData& sensorData, 
                     const BatteryInfo& batteryInfo, 
                     const SystemState& sysState,
                     bool fullRefresh = false);

// Windowed partial refresh for the 10-second circular indicator only
void renderSecondsTickOnly(int seconds);

// Display full OTA update screen with clean background on start of update
void showOtaScreen(const char* title, const char* versionInfo);

// Partial refresh of the OTA progress bar and percentage (called every ~2s)
void updateOtaProgress(int percent, uint32_t currentBytes, uint32_t totalBytes);
