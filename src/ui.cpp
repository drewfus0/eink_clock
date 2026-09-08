#include "ui.h"
#include "display_config.h"
#include "FreeSansBold70pt7b.h"
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// Draw a stylized battery icon with fill level
static void drawBattery(int x, int y, uint8_t percentage, float voltage, bool connected) {
    const int w = 40;
    const int h = 18;

    // Outer battery body
    display.drawRoundRect(x, y, w, h, 3, GxEPD_BLACK);
    // Positive terminal cap
    display.fillRect(x + w, y + 4, 3, h - 8, GxEPD_BLACK);

    if (connected) {
        // Inner fill proportional to percentage
        int innerW = (w - 6) * percentage / 100;
        if (innerW > 0) {
            display.fillRect(x + 3, y + 3, innerW, h - 6, GxEPD_BLACK);
        }

        // Text label
        char buf[32];
        snprintf(buf, sizeof(buf), "%d%%  (%.2fV)", percentage, voltage);
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(x - 110, y + 14);
        display.print(buf);
    } else {
        // USB / No Battery indicator
        display.setFont(&FreeSans9pt7b);
        display.setCursor(x - 85, y + 14);
        display.print("USB PWR");
    }
}

// Draw a modern rounded card with a title banner
static void drawCard(int x, int y, int w, int h, const char* title) {
    display.drawRoundRect(x, y, w, h, 6, GxEPD_BLACK);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(x + 14, y + 22);
    display.print(title);
    display.drawFastHLine(x + 10, y + 28, w - 20, GxEPD_BLACK);
}

// Draw animated circular seconds progress ring with 10-second tick marks
static void drawSecondsRing(int cx, int cy, int radius, int seconds) {
    // Outer concentric circle outlines
    display.drawCircle(cx, cy, radius, GxEPD_BLACK);
    display.drawCircle(cx, cy, radius - 1, GxEPD_BLACK);

    // 6 Segment Pips (every 10 seconds: :00, :10, :20, :30, :40, :50)
    // Angles: 0s=-90° (top), 10s=-30°, 20s=+30°, 30s=+90°, 40s=+150°, 50s=+210°
    const int pipDist = radius - 9;
    const int currentSecStep = (seconds / 10) * 10;

    for (int i = 0; i < 6; i++) {
        float angleDeg = (i * 60.0f) - 90.0f;
        float angleRad = angleDeg * (3.14159265f / 180.0f);
        int px = cx + (int)round(pipDist * cos(angleRad));
        int py = cy + (int)round(pipDist * sin(angleRad));

        int pipSec = i * 10;
        if (seconds >= pipSec) {
            // Elapsed ticks are filled solid black
            display.fillCircle(px, py, 4, GxEPD_BLACK);
        } else {
            // Future ticks are hollow rings
            display.drawCircle(px, py, 4, GxEPD_BLACK);
        }
    }

    // Digital seconds indicator in the center
    char secBuf[8];
    snprintf(secBuf, sizeof(secBuf), ":%02d", currentSecStep);
    display.setFont(&FreeSansBold12pt7b);
    display.setTextSize(1);
    display.setCursor(cx - 18, cy + 5);
    display.print(secBuf);

    // Sub-label "SEC"
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(cx - 14, cy + 22);
    display.print("SEC");
}

void renderSecondsTickOnly(int seconds) {
    const int boxX = 48;
    const int boxY = 96;
    const int boxW = 120;
    const int boxH = 120;

    display.setPartialWindow(boxX, boxY, boxW, boxH);
    display.firstPage();
    do {
        display.fillRect(boxX, boxY, boxW, boxH, GxEPD_WHITE);
        drawSecondsRing(105, 155, 48, seconds);
    } while (display.nextPage());
}

void renderDashboard(const TimeInfo& timeInfo, 
                     const SensorData& sensorData, 
                     const BatteryInfo& batteryInfo, 
                     const SystemState& sysState,
                     bool fullRefresh) {
    
    // GxEPD2 paged rendering loop ensures safe memory usage and clean output
    if (fullRefresh) {
        display.setFullWindow();
    } else {
        display.setPartialWindow(0, 0, display.width(), display.height());
    }
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        // =====================================================================
        // 1. Header Bar (Status & Battery)
        // =====================================================================
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(20, 26);
        display.print("E-INK SMART CLOCK");

        display.setFont(&FreeSans9pt7b);
        display.setCursor(215, 26);
        if (timeInfo.isValid) {
            int nextHours = sysState.secToNextNtp / 3600;
            int nextMins = (sysState.secToNextNtp % 3600) / 60;
            char statusBuf[96];

            const char* stateLabel = sysState.ntpJustSynced ? "NTP Synced" : "RTC Active";

            if (sysState.hasNtpDiff) {
                snprintf(statusBuf, sizeof(statusBuf), "%s • Next in %dh %02dm • Diff: %+.2fs",
                         stateLabel, nextHours, nextMins, sysState.lastNtpDiffSec);
            } else {
                snprintf(statusBuf, sizeof(statusBuf), "%s • Next in %dh %02dm",
                         stateLabel, nextHours, nextMins);
            }
            display.print(statusBuf);
        } else {
            display.print("WiFi: Offline");
        }

        // Battery Status (Right aligned)
        drawBattery(735, 12, batteryInfo.percentage, batteryInfo.voltage, batteryInfo.isConnected);

        // Header separator rule
        display.drawFastHLine(20, 42, 760, GxEPD_BLACK);

        // =====================================================================
        // 2. Main Hero Section: CIRCLE SECONDS, TIME & DATE
        // =====================================================================
        // Circular Seconds Ring on the left
        drawSecondsRing(105, 155, 48, timeInfo.seconds);

        // Giant Time Display (e.g. "14:28") rendered natively at 1:1 pixel resolution (no 3x3 blockiness)
        display.setFont(&FreeSansBold70pt7b);
        display.setTextSize(1);
        display.setCursor(215, 175);
        display.print(timeInfo.timeStr);

        // Date Display
        display.setTextSize(1);
        display.setFont(&FreeSansBold18pt7b);
        display.setCursor(220, 235);
        display.print(timeInfo.fullDateStr);

        // Mid-screen separator rule
        display.drawFastHLine(20, 265, 760, GxEPD_BLACK);

        // =====================================================================
        // 3. Environmental Sensors Section (Priorities 3, 4, 5)
        // =====================================================================
        const int cardY = 280;
        const int cardW = 240;
        const int cardH = 150;

        // --- Card 1: TEMPERATURE (Priority 3) ---
        int card1X = 20;
        drawCard(card1X, cardY, cardW, cardH, "TEMPERATURE");
        if (sensorData.ahtSuccess) {
            char tempBuf[16];
            snprintf(tempBuf, sizeof(tempBuf), "%.1f", sensorData.temperatureC);
            
            display.setFont(&FreeSansBold24pt7b);
            display.setTextSize(1);
            display.setCursor(card1X + 20, cardY + 85);
            display.print(tempBuf);

            display.setFont(&FreeSansBold18pt7b);
            display.print(" ");
            display.setFont(&FreeSansBold12pt7b);
            display.print("o"); // Degree circle
            display.setFont(&FreeSansBold18pt7b);
            display.print("C");

            // Secondary Fahrenheit reading
            display.setFont(&FreeSans9pt7b);
            char fBuf[24];
            snprintf(fBuf, sizeof(fBuf), "%.1f  F  (AHT21)", sensorData.temperatureF);
            display.setCursor(card1X + 20, cardY + 125);
            display.print(fBuf);
        } else {
            display.setFont(&FreeSans12pt7b);
            display.setCursor(card1X + 20, cardY + 90);
            display.print("Sensor Offline");
        }

        // --- Card 2: HUMIDITY (Priority 4) ---
        int card2X = 280;
        drawCard(card2X, cardY, cardW, cardH, "HUMIDITY");
        if (sensorData.ahtSuccess) {
            char humBuf[16];
            snprintf(humBuf, sizeof(humBuf), "%.1f", sensorData.humidityPercent);

            display.setFont(&FreeSansBold24pt7b);
            display.setCursor(card2X + 20, cardY + 85);
            display.print(humBuf);

            display.setFont(&FreeSansBold18pt7b);
            display.print(" %");

            // Comfort indicator
            const char* comfortStr = "NORMAL";
            if (sensorData.humidityPercent < 30.0f) comfortStr = "DRY";
            else if (sensorData.humidityPercent > 65.0f) comfortStr = "HUMID";
            else comfortStr = "COMFORTABLE";

            display.setFont(&FreeSans9pt7b);
            char labelBuf[32];
            snprintf(labelBuf, sizeof(labelBuf), "%s", comfortStr);
            display.setCursor(card2X + 20, cardY + 125);
            display.print(labelBuf);
        } else {
            display.setFont(&FreeSans12pt7b);
            display.setCursor(card2X + 20, cardY + 90);
            display.print("Sensor Offline");
        }

        // --- Card 3: AIR QUALITY (Priority 5) ---
        int card3X = 540;
        drawCard(card3X, cardY, cardW, cardH, "AIR QUALITY (ENS160)");
        if (sensorData.ensSuccess) {
            if (sensorData.ensWarmingUp) {
                // Clear indication of the initial 3-minute warm-up phase
                display.setFont(&FreeSansBold12pt7b);
                display.setCursor(card3X + 20, cardY + 75);
                display.print("WARMING UP");

                display.setFont(&FreeSans9pt7b);
                display.setCursor(card3X + 20, cardY + 102);
                display.print("Heating MOX (3m)...");

                char metricsBuf[36];
                if (sensorData.eco2Ppm > 0) {
                    snprintf(metricsBuf, sizeof(metricsBuf), "eCO2: %u ppm", sensorData.eco2Ppm);
                } else {
                    snprintf(metricsBuf, sizeof(metricsBuf), "Standby for reading");
                }
                display.setCursor(card3X + 20, cardY + 130);
                display.print(metricsBuf);
            } else {
                // AQI Badge / Status
                display.setFont(&FreeSansBold18pt7b);
                display.setCursor(card3X + 20, cardY + 75);
                display.print(sensorData.getAqiDescription());

                char aqiNum[32];
                if (sensorData.ensStale) {
                    if (sensorData.ensStaleMinutes > 1) {
                        snprintf(aqiNum, sizeof(aqiNum), "(AQI %d) [%um old]", sensorData.aqiUba, sensorData.ensStaleMinutes);
                    } else {
                        snprintf(aqiNum, sizeof(aqiNum), "(AQI %d) [missed]", sensorData.aqiUba);
                    }
                } else {
                    snprintf(aqiNum, sizeof(aqiNum), "(AQI %d)", sensorData.aqiUba);
                }
                display.setFont(&FreeSans9pt7b);
                display.setCursor(card3X + 20, cardY + 102);
                display.print(aqiNum);

                // Detailed gas telemetry: eCO2 and TVOC
                char metricsBuf[40];
                if (sensorData.ensStale) {
                    snprintf(metricsBuf, sizeof(metricsBuf), "eCO2: ~%u ppm | %u ppb", 
                             sensorData.eco2Ppm, sensorData.tvocPpb);
                } else {
                    snprintf(metricsBuf, sizeof(metricsBuf), "eCO2: %u ppm | %u ppb", 
                             sensorData.eco2Ppm, sensorData.tvocPpb);
                }
                display.setFont(&FreeSans9pt7b);
                display.setCursor(card3X + 20, cardY + 130);
                display.print(metricsBuf);
            }
        } else {
            display.setFont(&FreeSans12pt7b);
            display.setCursor(card3X + 20, cardY + 90);
            display.print("Sensor Offline");
        }

        // =====================================================================
        // 4. Footer System Bar (Priority 6)
        // =====================================================================
        display.setFont(&FreeSans9pt7b);
        display.setCursor(20, 465);
        if (sysState.otaUrl[0] != '\0') {
            char footerOta[80];
            snprintf(footerOta, sizeof(footerOta), "OTA: %s (or eink-clock.local)", sysState.otaUrl);
            display.print(footerOta);
        } else {
            char footerLeft[64];
            snprintf(footerLeft, sizeof(footerLeft), "Cycle #%u [%s] | Interval: %us", 
                     sysState.bootCount, fullRefresh ? "Full" : "Fast", sysState.updateIntervalSec);
            display.print(footerLeft);
        }

        display.setCursor(540, 465);
        char footerRight[64];
        snprintf(footerRight, sizeof(footerRight), "v%s • Waveshare 7.5\"", FIRMWARE_VERSION);
        display.print(footerRight);

    } while (display.nextPage());
}

static const int PROG_X = 120;
static const int PROG_Y = 220;
static const int PROG_W = 560;
static const int PROG_H = 110;

void showOtaScreen(const char* title, const char* versionInfo) {
    // Perform full hardware initialization with clear waveform to wipe previous clock display
    displayInitHardware(true);
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Outer & Inner Cards
        display.drawRoundRect(20, 20, 760, 440, 12, GxEPD_BLACK);
        display.drawRoundRect(24, 24, 752, 432, 10, GxEPD_BLACK);

        // Header Title
        display.setFont(&FreeSansBold18pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(60, 85);
        display.print(title ? title : "FIRMWARE UPDATE IN PROGRESS");
        display.drawFastHLine(60, 110, 680, GxEPD_BLACK);

        // Subtitle / Version Info
        display.setFont(&FreeSansBold12pt7b);
        display.setCursor(60, 160);
        display.print(versionInfo ? versionInfo : "Receiving new firmware binary...");

        // Initial Progress Bar Frame (0%)
        display.drawRoundRect(PROG_X + 20, PROG_Y + 20, 520, 28, 6, GxEPD_BLACK);
        display.setFont(&FreeSansBold12pt7b);
        display.setCursor(PROG_X + 20, PROG_Y + 85);
        display.print("Connecting and starting download (0%)...");

        // Bottom Warning Message
        display.setFont(&FreeSans9pt7b);
        display.setCursor(60, 410);
        display.print("Writing to flash partition... Please keep power connected.");

        display.setCursor(540, 410);
        display.print("ESP32-E E-Paper Clock");

    } while (display.nextPage());
}

void updateOtaProgress(int percent, uint32_t currentBytes, uint32_t totalBytes) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    // Partial update solely for the progress bar bounding box
    display.setPartialWindow(PROG_X, PROG_Y, PROG_W, PROG_H);
    display.firstPage();
    do {
        display.fillRect(PROG_X, PROG_Y, PROG_W, PROG_H, GxEPD_WHITE);
        display.drawRoundRect(PROG_X + 20, PROG_Y + 20, 520, 28, 6, GxEPD_BLACK);

        if (percent > 0) {
            int fillW = (514 * percent) / 100;
            if (fillW > 514) fillW = 514;
            display.fillRoundRect(PROG_X + 23, PROG_Y + 23, fillW, 22, 4, GxEPD_BLACK);
        }

        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(PROG_X + 20, PROG_Y + 85);

        char pBuf[64];
        if (percent >= 100) {
            snprintf(pBuf, sizeof(pBuf), "Complete 100%%! Finalizing & Rebooting...");
        } else if (totalBytes > 0) {
            snprintf(pBuf, sizeof(pBuf), "Updating: %d%% (%u / %u KB)", 
                     percent, (unsigned int)(currentBytes / 1024), (unsigned int)(totalBytes / 1024));
        } else {
            snprintf(pBuf, sizeof(pBuf), "Updating: %d%% (%u KB written)", 
                     percent, (unsigned int)(currentBytes / 1024));
        }
        display.print(pBuf);

    } while (display.nextPage());
}

