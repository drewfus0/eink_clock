#pragma once

#include <Arduino.h>

// =============================================================================
// Wi-Fi & NTP Configuration
// =============================================================================
// Replace with your local Wi-Fi credentials
#define WIFI_SSID           "ImWifiRick"
#define WIFI_PASSWORD       "1234567890"
#define WIFI_TIMEOUT_MS     15000 // 15 seconds max connection attempt

// NTP Time server configuration
#define NTP_SERVER_1        "pool.ntp.org"
#define NTP_SERVER_2        "time.nist.gov"

// Timezone definition (POSIX format allows automatic Daylight Saving Time)
// Examples:
// Sydney / Melbourne:  "AEST-10AEDT,M10.1.0,M4.1.0/3"
// New York (Eastern):  "EST5EDT,M3.2.0,M11.1.0"
// Los Angeles (Pacific):"PST8PDT,M3.2.0,M11.1.0"
// London (GMT/BST):    "GMT0BST,M3.5.0/1,M10.5.0"
// UTC:                 "UTC0"
#define TIMEZONE_POSIX      "AEST-10AEDT,M10.1.0,M4.1.0/3"

// =============================================================================
// Power & Refresh Intervals
// =============================================================================
// Interval between display updates (in seconds).
// For a standard minute-clock: 60 seconds.
// For maximum battery life (indoor climate station): 300 seconds (5 min).
#define DISPLAY_UPDATE_INTERVAL_SEC   60

// Sync time from NTP periodically instead of every wake-up to conserve battery.
// 6 hours = 21600 seconds.
#define NTP_SYNC_INTERVAL_HOURS       6

// Periodic full refresh to eliminate e-paper ghosting (e.g., every 30 cycles)
#define FULL_REFRESH_CYCLE_COUNT      30

// =============================================================================
// Hardware Pin Definitions (FireBeetle 2 ESP32-E)
// =============================================================================
// GooDisplay DESPI-C02 to ESP32-E (Hardware VSPI)
#define EPD_BUSY_PIN        -1  // Software timed (-1): guarantees full 4.2s refresh without premature busy-pin cutoff
#define EPD_RST_PIN         25  // Silkscreen: "25/D2"  - Display Hardware Reset
#define EPD_DC_PIN          26  // Silkscreen: "26/D3"  - Data / Command Selection
#define EPD_CS_PIN          14  // Silkscreen: "14/D6"  - SPI Chip Select
#define EPD_SCK_PIN         18  // Silkscreen: "18/SCK" - Hardware SPI Clock
#define EPD_MOSI_PIN        23  // Silkscreen: "23/MOSI"- Hardware SPI MOSI (DIN / SDI)
#define EPD_PWR_PIN         13  // Silkscreen: "13/D7"  - Waveshare HAT Rev 2.3 PWR pin (controls power MOSFET)

// I2C Pins for ENS160 + AHT21 sensor module
#define I2C_SDA_PIN         21  // Silkscreen: "SDA"
#define I2C_SCL_PIN         22  // Silkscreen: "SCL"
#define I2C_FREQ_HZ         100000 // 100 kHz standard mode

// Battery Voltage Sensing
// FireBeetle 2 ESP32-E has an onboard 1M/1M divider connected to GPIO 34 (A4)
#define BATTERY_ADC_PIN     34
#define BATTERY_DIVIDER_RATIO 2.0f // 1M + 1M divider
#define BATTERY_MIN_V       3.20f  // Fully drained LiPo safe cutoff
#define BATTERY_MAX_V       4.20f  // Fully charged LiPo
