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
};

// Render full clock and telemetry dashboard to e-paper buffer
void renderDashboard(const TimeInfo& timeInfo, 
                     const SensorData& sensorData, 
                     const BatteryInfo& batteryInfo, 
                     const SystemState& sysState,
                     bool fullRefresh = false);

// Windowed partial refresh for the 10-second circular indicator only
void renderSecondsTickOnly(int seconds);
