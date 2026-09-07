#include "time_sync.h"
#include "config.h"
#include <WiFi.h>
#include <esp_sntp.h>

void initTimezone() {
    configTzTime(TIMEZONE_POSIX, NTP_SERVER_1, NTP_SERVER_2);
}

bool ensureWiFiConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }
    log_i("Connecting to Wi-Fi SSID '%s'...", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < WIFI_TIMEOUT_MS) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        log_w("Wi-Fi connection failed or timed out.");
        disconnectWiFi();
        return false;
    }

    log_i("Wi-Fi connected. IP: %s, RSSI: %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
}

void disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    log_i("Wi-Fi radio shut down to conserve battery.");
}

static struct timeval s_tvBefore;
static int64_t s_tStartUs = 0;
static bool s_hasValidBefore = false;
static volatile bool s_ntpSyncedThisSession = false;
static float s_lastNtpDiffSec = 0.0f;
static bool s_hasNtpDiff = false;

static void timeSyncNotificationCallback(struct timeval *tv) {
    s_ntpSyncedThisSession = true;
    if (s_hasValidBefore && tv != nullptr) {
        int64_t elapsedUs = esp_timer_get_time() - s_tStartUs;
        int64_t expectedSec = s_tvBefore.tv_sec;
        int64_t expectedUsec = s_tvBefore.tv_usec + elapsedUs;
        expectedSec += expectedUsec / 1000000LL;
        expectedUsec = expectedUsec % 1000000LL;

        int64_t actualSec = tv->tv_sec;
        int64_t actualUsec = tv->tv_usec;

        double diff = (double)(actualSec - expectedSec) + (double)(actualUsec - expectedUsec) / 1000000.0;
        s_lastNtpDiffSec = (float)diff;
        s_hasNtpDiff = true;
        log_i("[NTP Sync] Time adjusted by SNTP! Diff (NTP - RTC): %+.3f s", s_lastNtpDiffSec);
    } else {
        log_i("[NTP Sync] Initial network time synchronization completed.");
    }
}

float getLastNtpDiffSec() {
    return s_lastNtpDiffSec;
}

bool hasLastNtpDiff() {
    return s_hasNtpDiff;
}

bool syncNtpTime(bool keepWiFiOn) {
    if (!ensureWiFiConnected()) {
        return false;
    }

    // Capture RTC time and hardware microsecond counter before initiating sync
    time_t preSyncTime = time(nullptr);
    if (preSyncTime >= 1600000000) {
        gettimeofday(&s_tvBefore, nullptr);
        s_tStartUs = esp_timer_get_time();
        s_hasValidBefore = true;
    } else {
        s_hasValidBefore = false;
    }

    s_ntpSyncedThisSession = false;

    // Register sync notification callback
    sntp_set_time_sync_notification_cb(timeSyncNotificationCallback);

    // Stop and restart SNTP to force an immediate network query
    if (sntp_enabled()) {
        sntp_stop();
    }
    configTzTime(TIMEZONE_POSIX, NTP_SERVER_1, NTP_SERVER_2);

    log_i("Waiting for NTP response from %s / %s...", NTP_SERVER_1, NTP_SERVER_2);

    unsigned long startWait = millis();
    const unsigned long maxWaitMs = 8000; // 8 second timeout

    while (!s_ntpSyncedThisSession && (millis() - startWait) < maxWaitMs) {
        delay(100);
    }

    if (!s_ntpSyncedThisSession) {
        // Fallback check in case sntp callback didn't fire but time updated
        time_t now = time(nullptr);
        struct tm ti;
        localtime_r(&now, &ti);
        if (ti.tm_year > (2020 - 1900) && (preSyncTime < 1600000000 || now != preSyncTime)) {
            s_ntpSyncedThisSession = true;
        }
    }

    if (s_ntpSyncedThisSession) {
        time_t now = time(nullptr);
        struct tm ti;
        localtime_r(&now, &ti);
        log_i("NTP synchronized successfully: %s", asctime(&ti));
    } else {
        log_w("NTP synchronization timed out after %lu ms", millis() - startWait);
    }

    if (!keepWiFiOn) {
        disconnectWiFi();
    }

    return s_ntpSyncedThisSession;
}

TimeInfo getCurrentTimeInfo() {
    TimeInfo info;
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    info.rawEpoch = now;

    if (timeinfo.tm_year > (2020 - 1900)) {
        info.isValid = true;
        // 24-hour format: "14:30"
        strftime(info.timeStr, sizeof(info.timeStr), "%H:%M", &timeinfo);
        strftime(info.ampmStr, sizeof(info.ampmStr), "%p", &timeinfo);
        strftime(info.dayOfWeekStr, sizeof(info.dayOfWeekStr), "%A", &timeinfo);
        strftime(info.dateStr, sizeof(info.dateStr), "%d %B %Y", &timeinfo);
        strftime(info.fullDateStr, sizeof(info.fullDateStr), "%A, %d %b %Y", &timeinfo);
        info.seconds = timeinfo.tm_sec;
    } else {
        info.isValid = false;
        info.seconds = 0;
        snprintf(info.timeStr, sizeof(info.timeStr), "--:--");
        snprintf(info.ampmStr, sizeof(info.ampmStr), "--");
        snprintf(info.dayOfWeekStr, sizeof(info.dayOfWeekStr), "UNKNOWN");
        snprintf(info.dateStr, sizeof(info.dateStr), "NTP Not Synced");
        snprintf(info.fullDateStr, sizeof(info.fullDateStr), "NTP Not Synced");
    }

    return info;
}
