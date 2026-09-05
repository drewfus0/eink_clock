#include "ui.h"
#include "display_config.h"
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
        display.setCursor(220, 26);
        if (sysState.ntpJustSynced) {
            display.print("WiFi: NTP Synced");
        } else if (timeInfo.isValid) {
            display.print("WiFi: Off (RTC Active)");
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

        // Giant Time Display (e.g. "14:28") shifted to the right
        display.setFont(&FreeSansBold24pt7b);
        display.setTextSize(3);
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
            // AQI Badge / Status
            display.setFont(&FreeSansBold18pt7b);
            display.setCursor(card3X + 20, cardY + 75);
            display.print(sensorData.getAqiDescription());

            char aqiNum[16];
            snprintf(aqiNum, sizeof(aqiNum), "(AQI %d)", sensorData.aqiUba);
            display.setFont(&FreeSans9pt7b);
            display.setCursor(card3X + 20, cardY + 102);
            display.print(aqiNum);

            // Detailed gas telemetry: eCO2 and TVOC
            char metricsBuf[36];
            snprintf(metricsBuf, sizeof(metricsBuf), "eCO2: %u ppm | %u ppb", 
                     sensorData.eco2Ppm, sensorData.tvocPpb);
            display.setFont(&FreeSans9pt7b);
            display.setCursor(card3X + 20, cardY + 130);
            display.print(metricsBuf);
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

void renderOtaMessage(const char* title, const char* message) {
    int boxX = 120;
    int boxY = 140;
    int boxW = 560;
    int boxH = 180;

    display.setPartialWindow(boxX, boxY, boxW, boxH);
    display.firstPage();
    do {
        display.fillRect(boxX, boxY, boxW, boxH, GxEPD_WHITE);
        display.drawRoundRect(boxX, boxY, boxW, boxH, 8, GxEPD_BLACK);
        display.drawRoundRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, 6, GxEPD_BLACK);
        display.drawFastHLine(boxX, boxY + 50, boxW, GxEPD_BLACK);

        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(boxX + 20, boxY + 35);
        display.print(title);

        display.setFont(&FreeSans9pt7b);
        display.setCursor(boxX + 20, boxY + 85);
        display.print(message);

        display.setFont(&FreeSans9pt7b);
        display.setCursor(boxX + 20, boxY + 120);
        display.print("Please do not power off the device.");

        display.drawRoundRect(boxX + 20, boxY + 140, boxW - 40, 16, 4, GxEPD_BLACK);
        display.fillRoundRect(boxX + 22, boxY + 142, (boxW - 44) / 2, 12, 3, GxEPD_BLACK);
    } while (display.nextPage());
}
