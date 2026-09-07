#pragma once

#include <Arduino.h>
#include <time.h>

struct TimeInfo {
    char timeStr[16];        // e.g. "10:45"
    char ampmStr[8];         // e.g. "AM" / "PM"
    char dayOfWeekStr[16];   // e.g. "Sunday"
    char dateStr[32];        // e.g. "6 September 2026"
    char fullDateStr[64];    // e.g. "Sunday, 06 Sep 2026"
    int seconds = 0;         // 0 - 59
    bool isValid = false;
    time_t rawEpoch = 0;
};

// Initializes timezone from config
void initTimezone();

// Connects to WiFi, synchronizes via NTP. If keepWiFiOn is false, turns WiFi off to conserve battery.
bool syncNtpTime(bool keepWiFiOn = false);

// Connects to Wi-Fi if not already connected
bool ensureWiFiConnected();

// Disconnects Wi-Fi and shuts down radio to conserve battery
void disconnectWiFi();

// Formats the current RTC time
TimeInfo getCurrentTimeInfo();

// Returns the measured time difference (NTP - RTC) in seconds from the most recent sync
float getLastNtpDiffSec();
bool hasLastNtpDiff();
