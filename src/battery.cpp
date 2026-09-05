#include "battery.h"
#include "config.h"

void initBattery() {
    pinMode(BATTERY_ADC_PIN, INPUT);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
}

// Convert battery voltage to approximate percentage based on typical 3.7V LiPo discharge curve
static uint8_t calculateLipoPercentage(float voltage) {
    if (voltage >= 4.18f) return 100;
    if (voltage <= 3.25f) return 0;

    // Linear piecewise interpolation along LiPo curve
    if (voltage >= 4.05f) return 90 + (uint8_t)((voltage - 4.05f) / (4.18f - 4.05f) * 10);
    if (voltage >= 3.95f) return 80 + (uint8_t)((voltage - 3.95f) / (4.05f - 3.95f) * 10);
    if (voltage >= 3.86f) return 70 + (uint8_t)((voltage - 3.86f) / (3.95f - 3.86f) * 10);
    if (voltage >= 3.80f) return 60 + (uint8_t)((voltage - 3.80f) / (3.86f - 3.80f) * 10);
    if (voltage >= 3.75f) return 50 + (uint8_t)((voltage - 3.75f) / (3.80f - 3.75f) * 10);
    if (voltage >= 3.70f) return 40 + (uint8_t)((voltage - 3.70f) / (3.75f - 3.70f) * 10);
    if (voltage >= 3.65f) return 30 + (uint8_t)((voltage - 3.65f) / (3.70f - 3.65f) * 10);
    if (voltage >= 3.60f) return 20 + (uint8_t)((voltage - 3.60f) / (3.65f - 3.60f) * 10);
    if (voltage >= 3.45f) return 10 + (uint8_t)((voltage - 3.45f) / (3.60f - 3.45f) * 10);
    return (uint8_t)((voltage - 3.25f) / (3.45f - 3.25f) * 10);
}

BatteryInfo readBattery() {
    BatteryInfo info;

    // Take 16 ADC samples to smooth high-impedance 1M divider fluctuations
    uint32_t adcSumMv = 0;
    const int SAMPLES = 16;
    for (int i = 0; i < SAMPLES; i++) {
        adcSumMv += analogReadMilliVolts(BATTERY_ADC_PIN);
        delayMicroseconds(500);
    }

    float measuredPinMv = (float)adcSumMv / (float)SAMPLES;
    // Calculate full battery voltage using voltage divider ratio
    info.voltage = (measuredPinMv / 1000.0f) * BATTERY_DIVIDER_RATIO;

    // A connected LiPo will be between ~3.0V and 4.3V
    if (info.voltage > 2.8f) {
        info.isConnected = true;
        info.percentage = calculateLipoPercentage(info.voltage);
    } else {
        info.isConnected = false;
        info.percentage = 0;
    }

    log_i("Battery: %.2f V (%d%%, connected: %s)", 
          info.voltage, info.percentage, info.isConnected ? "yes" : "no (USB only)");
    return info;
}
