#include <Arduino.h>
#include <esp_sleep.h>
#include "config.h"
#include "display_config.h"
#include "sensors.h"
#include "battery.h"
#include "time_sync.h"
#include "ui.h"

// Variables preserved in ESP32 RTC Slow Memory across deep sleep
RTC_DATA_ATTR static uint32_t bootCount = 0;
RTC_DATA_ATTR static time_t lastNtpSyncEpoch = 0;

void setup() {
    Serial.begin(115200);
    delay(100);
    bootCount++;

    log_i("=================================================");
    log_i("ESP32-E E-Paper Clock Starting. Boot Count: %u", bootCount);
    log_i("=================================================");

    initTimezone();
    initBattery();

    // Check if NTP time sync is required:
    // 1. First boot / invalid RTC time
    // 2. Interval exceeded (NTP_SYNC_INTERVAL_HOURS)
    time_t currentEpoch = time(nullptr);
    bool needNtpSync = false;

    if (lastNtpSyncEpoch == 0 || currentEpoch < 1600000000) {
        needNtpSync = true;
    } else if ((currentEpoch - lastNtpSyncEpoch) >= (NTP_SYNC_INTERVAL_HOURS * 3600)) {
        needNtpSync = true;
    }

    bool ntpSuccess = false;
    if (needNtpSync) {
        log_i("Initiating scheduled NTP synchronization...");
        ntpSuccess = syncNtpTime();
        if (ntpSuccess) {
            lastNtpSyncEpoch = time(nullptr);
        }
    } else {
        log_i("Skipping WiFi/NTP sync (next sync in %ld sec)", 
              (long)((lastNtpSyncEpoch + (NTP_SYNC_INTERVAL_HOURS * 3600)) - currentEpoch));
    }

    // Determine refresh mode: Full anti-ghosting refresh on boot or every FULL_REFRESH_CYCLE_COUNT
    bool fullRefresh = (bootCount <= 1) || ((bootCount % FULL_REFRESH_CYCLE_COUNT) == 0);

    // Initialize I2C sensors and display hardware
    initSensors();
    displayInitHardware(true);

    // Read telemetry
    SensorData sensorData = readSensors();
    BatteryInfo batteryInfo = readBattery();
    TimeInfo timeInfo = getCurrentTimeInfo();

    SystemState sysState;
    sysState.bootCount = bootCount;
    sysState.lastNtpSyncEpoch = lastNtpSyncEpoch;
    sysState.ntpJustSynced = ntpSuccess;
    sysState.updateIntervalSec = DISPLAY_UPDATE_INTERVAL_SEC;

    log_i("Pin states: BUSY(IO%d)=%d, RST(IO%d)=%d, CS(IO%d)=%d", 
          EPD_BUSY_PIN, (EPD_BUSY_PIN >= 0) ? digitalRead(EPD_BUSY_PIN) : -1,
          EPD_RST_PIN, digitalRead(EPD_RST_PIN),
          EPD_CS_PIN, digitalRead(EPD_CS_PIN));

    log_i("Updating e-paper display (Full Refresh: %s, Boot: %u)...", fullRefresh ? "YES" : "NO", bootCount);
    uint32_t tStart = millis();
    renderDashboard(timeInfo, sensorData, batteryInfo, sysState, fullRefresh);
    uint32_t tElapsed = millis() - tStart;
    log_i("Display refresh call completed in %u ms", tElapsed);
    
    // Safety delay only if the driver returned prematurely (< 500ms)
    if (tElapsed < 500) {
        log_w("Display refresh returned prematurely (%u ms). Waiting 5s for physical waveform...", tElapsed);
        delay(5000);
    }

    // Power off panel driving voltages (charges turned off, but controller SRAM retained for fast differential refresh)
    log_i("Powering off display driver voltages...");
    displayPowerOff();

    // Calculate sleep duration
    // If time is valid, synchronize sleep to the exact start of the next minute
    uint64_t sleepSeconds = DISPLAY_UPDATE_INTERVAL_SEC;
    if (timeInfo.isValid) {
        time_t now = time(nullptr);
        struct tm tm_now;
        localtime_r(&now, &tm_now);
        int secRemaining = 60 - tm_now.tm_sec;
        if (secRemaining <= 0) secRemaining = 60;
        sleepSeconds = secRemaining;
    }

    log_i("Entering ESP32 deep sleep for %llu seconds...", sleepSeconds);
    Serial.flush();

    esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {
    // Execution will never reach loop() due to deep sleep
}
