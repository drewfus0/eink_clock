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

        // Text label: space between percent number and % symbol, and dynamic right alignment
        char buf[32];
        snprintf(buf, sizeof(buf), "%d %% (%.2fV)", percentage, voltage);
        display.setFont(&FreeSansBold9pt7b);
        int16_t bx, by;
        uint16_t bw, bh;
        display.getTextBounds(buf, 0, y + 14, &bx, &by, &bw, &bh);
        display.setCursor(x - 8 - bw - bx, y + 14);
        display.print(buf);
    } else {
        // USB / No Battery indicator
        display.setFont(&FreeSans9pt7b);
        const char* pwrStr = "USB PWR";
        int16_t bx, by;
        uint16_t bw, bh;
        display.getTextBounds(pwrStr, 0, y + 14, &bx, &by, &bw, &bh);
        display.setCursor(x - 8 - bw - bx, y + 14);
        display.print(pwrStr);
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

static void drawThickLine(float x1, float y1, float x2, float y2, float thickness, uint16_t color) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.1f) return;
    float px = -dy / len * (thickness * 0.5f);
    float py = dx / len * (thickness * 0.5f);
    display.fillTriangle((int)roundf(x1 + px), (int)roundf(y1 + py),
                         (int)roundf(x2 + px), (int)roundf(y2 + py),
                         (int)roundf(x2 - px), (int)roundf(y2 - py), color);
    display.fillTriangle((int)roundf(x1 + px), (int)roundf(y1 + py),
                         (int)roundf(x2 - px), (int)roundf(y2 - py),
                         (int)roundf(x1 - px), (int)roundf(y1 - py), color);
}

static void drawTaperedHand(int cx, int cy, float length, float baseW, float tipW, float angleRad, float tailLen, uint16_t color) {
    float cosA = cosf(angleRad - 1.5707963f);
    float sinA = sinf(angleRad - 1.5707963f);
    float perpX = -sinA;
    float perpY = cosA;

    float tipX = cx + length * cosA;
    float tipY = cy + length * sinA;

    float brX = cx + perpX * (baseW * 0.5f);
    float brY = cy + perpY * (baseW * 0.5f);
    float blX = cx - perpX * (baseW * 0.5f);
    float blY = cy - perpY * (baseW * 0.5f);

    float trX = tipX + perpX * (tipW * 0.5f);
    float trY = tipY + perpY * (tipW * 0.5f);
    float tlX = tipX - perpX * (tipW * 0.5f);
    float tlY = tipY - perpY * (tipW * 0.5f);

    display.fillTriangle((int)roundf(blX), (int)roundf(blY),
                         (int)roundf(tlX), (int)roundf(tlY),
                         (int)roundf(trX), (int)roundf(trY), color);
    display.fillTriangle((int)roundf(blX), (int)roundf(blY),
                         (int)roundf(trX), (int)roundf(trY),
                         (int)roundf(brX), (int)roundf(brY), color);

    if (tailLen > 0.0f) {
        float tailX = cx - tailLen * cosA;
        float tailY = cy - tailLen * sinA;
        float tailW = baseW * 0.7f;
        float tarX = tailX + perpX * (tailW * 0.5f);
        float tarY = tailY + perpY * (tailW * 0.5f);
        float talX = tailX - perpX * (tailW * 0.5f);
        float talY = tailY - perpY * (tailW * 0.5f);

        display.fillTriangle((int)roundf(blX), (int)roundf(blY),
                             (int)roundf(talX), (int)roundf(talY),
                             (int)roundf(tarX), (int)roundf(tarY), color);
        display.fillTriangle((int)roundf(blX), (int)roundf(blY),
                             (int)roundf(tarX), (int)roundf(tarY),
                             (int)roundf(brX), (int)roundf(brY), color);
    }
}

// Draw Option 2B Railway Station analog clock face with full 1-12 Arabic numerals
static void drawAnalogClock(int cx, int cy, int R, int hour, int minute) {
    // 1. Double Chapter Ring
    display.drawCircle(cx, cy, R, GxEPD_BLACK);
    display.drawCircle(cx, cy, R - 1, GxEPD_BLACK);
    const int rMid = R - 13;
    display.drawCircle(cx, cy, rMid, GxEPD_BLACK);

    // 2. 60 Minute Ticks
    for (int m = 0; m < 60; m++) {
        float angle = (m * 6.0f - 90.0f) * (3.14159265f / 180.0f);
        float cosA = cosf(angle);
        float sinA = sinf(angle);
        if (m % 5 == 0) {
            // 5-minute ticks bridge the chapter track (3px wide)
            float x1 = cx + R * cosA;
            float y1 = cy + R * sinA;
            float x2 = cx + rMid * cosA;
            float y2 = cy + rMid * sinA;
            drawThickLine(x1, y1, x2, y2, 3.0f, GxEPD_BLACK);
        } else {
            // Minute ticks (1px line, 5px long)
            int x1 = (int)roundf(cx + R * cosA);
            int y1 = (int)roundf(cy + R * sinA);
            int x2 = (int)roundf(cx + (R - 5) * cosA);
            int y2 = (int)roundf(cy + (R - 5) * sinA);
            display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
        }
    }

    // 3. Full Arabic Numbers (1 through 12)
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    const float rNum = rMid - 14.0f;
    for (int h = 1; h <= 12; h++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", h);
        float angle = (h * 30.0f - 90.0f) * (3.14159265f / 180.0f);
        int16_t bx, by;
        uint16_t bw, bh;
        display.getTextBounds(buf, 0, 0, &bx, &by, &bw, &bh);
        int nx = (int)roundf(cx + rNum * cosf(angle)) - (bw / 2) - bx;
        int ny = (int)roundf(cy + rNum * sinf(angle)) - (bh / 2) - by;
        display.setCursor(nx, ny);
        display.print(buf);
    }

    // 4. Hands
    float minAngle = (minute * 6.0f) * (3.14159265f / 180.0f);
    float hourAngle = (((hour % 12) + minute / 60.0f) * 30.0f) * (3.14159265f / 180.0f);

    // Hour hand: Spade hand with circular eye
    float hLen = R * 0.52f;
    drawTaperedHand(cx, cy, hLen * 0.65f, 5.0f, 3.0f, hourAngle, 8.0f, GxEPD_BLACK);

    float eyeDist = hLen * 0.72f;
    float eyeX = cx + eyeDist * cosf(hourAngle - 1.5707963f);
    float eyeY = cy + eyeDist * sinf(hourAngle - 1.5707963f);
    display.fillCircle((int)roundf(eyeX), (int)roundf(eyeY), 6, GxEPD_BLACK);
    display.fillCircle((int)roundf(eyeX), (int)roundf(eyeY), 2, GxEPD_WHITE);

    float tipDist = hLen;
    float tipX = cx + tipDist * cosf(hourAngle - 1.5707963f);
    float tipY = cy + tipDist * sinf(hourAngle - 1.5707963f);
    float sideAng = hourAngle - 1.5707963f;
    float px = -sinf(sideAng) * 5.0f;
    float py = cosf(sideAng) * 5.0f;
    display.fillTriangle((int)roundf(eyeX + px), (int)roundf(eyeY + py),
                         (int)roundf(tipX), (int)roundf(tipY),
                         (int)roundf(eyeX - px), (int)roundf(eyeY - py), GxEPD_BLACK);

    // Minute hand: Tapered leaf hand
    float mLen = R * 0.82f;
    drawTaperedHand(cx, cy, mLen, 6.0f, 2.0f, minAngle, 14.0f, GxEPD_BLACK);

    // Center concentric pivot
    display.fillCircle(cx, cy, 6, GxEPD_BLACK);
    display.fillCircle(cx, cy, 3, GxEPD_WHITE);
    display.fillCircle(cx, cy, 1, GxEPD_BLACK);
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
        // 2. Main Hero Section: DIGITAL (LEFT 2/3) + ANALOG CLOCK (RIGHT 1/3)
        // =====================================================================
        // Giant Time Display (e.g. "14:28") centered horizontally in left 2/3rds (x = 20 .. 520)
        display.setFont(&FreeSansBold70pt7b);
        display.setTextSize(1);
        int16_t tbx, tby;
        uint16_t tbw, tbh;
        display.getTextBounds(timeInfo.timeStr, 0, 175, &tbx, &tby, &tbw, &tbh);
        int timeX = 20 + (500 - tbw) / 2 - tbx;
        display.setCursor(timeX, 175);
        display.print(timeInfo.timeStr);

        // Date Display centered horizontally in left 2/3rds
        display.setFont(&FreeSansBold18pt7b);
        display.setTextSize(1);
        int16_t dbx, dby;
        uint16_t dbw, dbh;
        display.getTextBounds(timeInfo.fullDateStr, 0, 235, &dbx, &dby, &dbw, &dbh);
        int dateX = 20 + (500 - dbw) / 2 - dbx;
        display.setCursor(dateX, 235);
        display.print(timeInfo.fullDateStr);

        // Subtle vertical hairline divider between digital and analog sections
        display.drawFastVLine(525, 55, 195, GxEPD_BLACK);

        // Analog Clock on the right 1/3rd (Option 2B: Railway Station with Full 1-12 Arabic Numbers)
        drawAnalogClock(655, 153, 88, timeInfo.hours, timeInfo.minutes);

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
        snprintf(footerRight, sizeof(footerRight), "version: %s", FIRMWARE_VERSION);
        display.print(footerRight);

    } while (display.nextPage());

    // Explicitly sync current buffer to controller previous buffer (0x10)
    // so differential refresh on subsequent updates knows precisely which pixels were black,
    // ensuring old digits are fully cleared to white without ghosting/overlap.
    display.nextPageToPrevious();
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

