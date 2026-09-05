#include "sensors.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <ScioSense_ENS16x.h>

static Adafruit_AHTX0 aht;
static ENS160 ens160;
static bool ahtInitialized = false;
static bool ensInitialized = false;
static uint8_t ens160Address = 0x53; // Default ENS160 address (can also be 0x52)

bool initSensors() {
    log_i("Initializing I2C bus on SDA=%d, SCL=%d...", I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
    delay(50);

    // Initialize AHT21 (typically 0x38)
    if (aht.begin(&Wire)) {
        ahtInitialized = true;
        log_i("AHT21 sensor detected and initialized at 0x38");
    } else {
        log_e("Failed to initialize AHT21 sensor at 0x38!");
    }

    // Attempt to detect ENS160 at 0x53 first, then 0x52
    ens160.begin(&Wire, 0x53);
    if (ens160.init()) {
        ensInitialized = true;
        ens160Address = 0x53;
        log_i("ENS160 sensor initialized at address 0x53");
    } else {
        ens160.begin(&Wire, 0x52);
        if (ens160.init()) {
            ensInitialized = true;
            ens160Address = 0x52;
            log_i("ENS160 sensor initialized at address 0x52");
        } else {
            log_e("Failed to initialize ENS160 sensor at both 0x53 and 0x52!");
        }
    }

    if (ensInitialized) {
        ens160.startStandardMeasure();
    }

    return (ahtInitialized || ensInitialized);
}

SensorData readSensors() {
    SensorData data;

    // Read AHT21
    if (ahtInitialized) {
        sensors_event_t humidityEvent, tempEvent;
        if (aht.getEvent(&humidityEvent, &tempEvent)) {
            data.temperatureC = tempEvent.temperature;
            data.temperatureF = (data.temperatureC * 1.8f) + 32.0f;
            data.humidityPercent = humidityEvent.relative_humidity;
            data.ahtSuccess = true;
            log_i("AHT21: Temp=%.1f C, Humidity=%.1f %%", data.temperatureC, data.humidityPercent);

            // Feed environmental compensation to ENS160 if available
            if (ensInitialized) {
                uint16_t tRaw = Ens16x_CalcTempInFromCelsius(data.temperatureC);
                uint16_t rhRaw = Ens16x_CalcRhIn(data.humidityPercent);
                ens160.writeCompensation(tRaw, rhRaw);
            }
        } else {
            log_w("Failed to read data event from AHT21");
        }
    }

    // Read ENS160
    if (ensInitialized) {
        // Wait briefly for measurement conversion if needed
        delay(50);
        if (ens160.update() == RESULT_OK) {
            data.aqiUba = (uint8_t)ens160.getAirQualityIndex_UBA();
            if (data.aqiUba < 1) data.aqiUba = 1;
            if (data.aqiUba > 5) data.aqiUba = 5;

            data.tvocPpb = ens160.getTvoc();
            data.eco2Ppm = ens160.getEco2();
            data.ensSuccess = true;
            log_i("ENS160: AQI=%d (%s), TVOC=%u ppb, eCO2=%u ppm", 
                  data.aqiUba, data.getAqiDescription(), data.tvocPpb, data.eco2Ppm);
        } else {
            log_w("ENS160 update returned non-OK status");
        }
    }

    return data;
}
