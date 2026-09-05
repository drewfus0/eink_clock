#include <Arduino.h>
#include <esp_sleep.h>
#include <WiFi.h>
#include "config.h"
#include "display_config.h"
#include "sensors.h"
#include "battery.h"
#include "time_sync.h"
#include "ota_manager.h"
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

    // OTA update window is enabled on initial boot (or reset) and on periodic NTP sync
    bool shouldOpenOta = (bootCount <= 1) || needNtpSync;

    bool ntpSuccess = false;
    if (needNtpSync) {
        log_i("Initiating scheduled NTP synchronization...");
        // Keep Wi-Fi connected if we need to open the OTA window
        ntpSuccess = syncNtpTime(shouldOpenOta);
        if (ntpSuccess) {
            lastNtpSyncEpoch = time(nullptr);
        }
    } else {
        log_i("Skipping WiFi/NTP sync (next sync in %ld sec)", 
              (long)((lastNtpSyncEpoch + (NTP_SYNC_INTERVAL_HOURS * 3600)) - currentEpoch));
    }

    // Fetch RTC time
    TimeInfo timeInfo = getCurrentTimeInfo();

    // Determine refresh mode: Full anti-ghosting refresh on boot or every FULL_REFRESH_CYCLE_COUNT
    bool fullRefresh = (bootCount <= 1) || ((bootCount % FULL_REFRESH_CYCLE_COUNT) == 0);

    // Full dashboard update on boot or on minute rollover (:00 to :09)
    bool isMinuteRollover = (bootCount <= 1) || (timeInfo.seconds < 10);

    uint32_t tStart = millis();

    if (isMinuteRollover || fullRefresh) {
        // Full dashboard update: Read sensors and redraw the entire screen
        initSensors();
        displayInitHardware(fullRefresh);

        SensorData sensorData = readSensors();
        BatteryInfo batteryInfo = readBattery();

        SystemState sysState;
        sysState.bootCount = bootCount;
        sysState.lastNtpSyncEpoch = lastNtpSyncEpoch;
        sysState.ntpJustSynced = ntpSuccess;
        sysState.updateIntervalSec = DISPLAY_UPDATE_INTERVAL_SEC;

        if (shouldOpenOta && WiFi.status() == WL_CONNECTED) {
            snprintf(sysState.otaUrl, sizeof(sysState.otaUrl), "http://%s/update", WiFi.localIP().toString().c_str());
        }

        log_i("Pin states: BUSY(IO%d)=%d, RST(IO%d)=%d, CS(IO%d)=%d", 
              EPD_BUSY_PIN, (EPD_BUSY_PIN >= 0) ? digitalRead(EPD_BUSY_PIN) : -1,
              EPD_RST_PIN, digitalRead(EPD_RST_PIN),
              EPD_CS_PIN, digitalRead(EPD_CS_PIN));

        log_i("Updating full dashboard (Full: %s, Boot: %u, Sec: :%02d)...", 
              fullRefresh ? "YES" : "NO", bootCount, timeInfo.seconds);
        renderDashboard(timeInfo, sensorData, batteryInfo, sysState, fullRefresh);
    } else {
        // Intermediate 10-second tick (:10, :20, :30, :40, :50):
        // Skip I2C sensor bus and update only the circular seconds window!
        displayInitHardware(false);

        log_i("Updating seconds ring only (Sec: :%02d, Boot: %u)...", timeInfo.seconds, bootCount);
        renderSecondsTickOnly(timeInfo.seconds);
    }

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

    // If an OTA listening window is scheduled, open it now
    if (shouldOpenOta) {
        runOtaWindow(OTA_WINDOW_TIMEOUT_SEC);
    }

    // Calculate sleep duration synchronized to the exact next 10-second boundary
    uint64_t sleepSeconds = DISPLAY_UPDATE_INTERVAL_SEC;
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    if (timeInfo.isValid || tm_now.tm_year > (2020 - 1900)) {
        int secRemaining = 10 - (tm_now.tm_sec % 10);
        if (secRemaining <= 0) secRemaining = 10;
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
