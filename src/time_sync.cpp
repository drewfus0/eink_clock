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

bool syncNtpTime(bool keepWiFiOn) {
    if (!ensureWiFiConnected()) {
        return false;
    }
    initTimezone();

    // Wait for NTP synchronization
    log_i("Waiting for NTP response...");
    time_t now = 0;
    struct tm timeinfo;
    int retry = 0;
    const int maxRetries = 20;

    while (retry < maxRetries) {
        time(&now);
        localtime_r(&now, &timeinfo);
        if (timeinfo.tm_year > (2020 - 1900)) {
            log_i("NTP synchronized successfully: %s", asctime(&timeinfo));
            break;
        }
        delay(500);
        retry++;
    }

    if (!keepWiFiOn) {
        disconnectWiFi();
    }

    return (timeinfo.tm_year > (2020 - 1900));
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
