#include "sensors.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <ScioSense_ENS16x.h>
#include <esp_sleep.h>

static Adafruit_AHTX0 aht;
static ENS160 ens160;
static bool ahtInitialized = false;
static bool ensInitialized = false;
static uint8_t ens160Address = 0x53; // Default ENS160 address (can also be 0x52)

// Variables preserved in ESP32 RTC Slow Memory across deep sleep
RTC_DATA_ATTR static bool rtcHasEverWarmedUp = false;
RTC_DATA_ATTR static uint16_t rtcLastValidEco2 = 400;
RTC_DATA_ATTR static uint16_t rtcLastValidTvoc = 0;
RTC_DATA_ATTR static uint8_t rtcLastValidAqi = 1;
RTC_DATA_ATTR static uint8_t rtcStaleMinutes = 0;

bool initSensors() {
    log_i("Initializing I2C bus on SDA=%d, SCL=%d...", I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
    delay(50);

    // Initialize AHT21 (typically 0x38)
    if (!ahtInitialized) {
        if (aht.begin(&Wire)) {
            ahtInitialized = true;
            log_i("AHT21 sensor detected and initialized at 0x38");
        } else {
            log_e("Failed to initialize AHT21 sensor at 0x38!");
        }
    }

    // Detect ENS160 without resetting it if already running!
    // We check I2C addresses 0x53 (default) and 0x52.
    uint8_t detectedAddr = 0;
    const uint8_t testAddrs[] = {0x53, 0x52};

    for (uint8_t addr : testAddrs) {
        ens160.begin(&Wire, addr);
        uint8_t partIdBuf[2] = {0};
        if (ens160.read(ENS16X_REGISTER_ADDRESS_PART_ID, partIdBuf, 2) == RESULT_OK) {
            uint16_t id = (partIdBuf[1] << 8) | partIdBuf[0];
            if (id == 0x0160) {
                detectedAddr = addr;
                ens160.partId = id;
                break;
            }
        }
    }

    if (detectedAddr != 0) {
        ensInitialized = true;
        ens160Address = detectedAddr;

        // Inspect current operating mode & device status from registers
        uint8_t opMode = 0xFF;
        bool readOpOk = (ens160.read(ENS16X_REGISTER_ADDRESS_OPMODE, &opMode, 1) == RESULT_OK);
        uint8_t devStatus = 0;
        ens160.read(ENS16X_REGISTER_ADDRESS_DEVICE_STATUS, &devStatus, 1);
        uint8_t validity = (devStatus >> 2) & 0x03;

        if (readOpOk && (opMode == ENS16X_OPERATING_MODE_STANDARD)) {
            log_i("ENS160 at 0x%02X is ALREADY ACTIVE (OPMODE=0x%02X, Status=0x%02X, Validity=%u). Preserving continuous measurement state!",
                  detectedAddr, opMode, devStatus, validity);
        } else if (readOpOk) {
            log_i("ENS160 at 0x%02X not in standard mode (OPMODE=0x%02X). Starting standard measurement mode (warm-up begins)...",
                  detectedAddr, opMode);
            ens160.startStandardMeasure();
        } else {
            log_w("ENS160 at 0x%02X could not read OPMODE (I2C glitch). Skipping startStandardMeasure to avoid resetting warm-up.", detectedAddr);
        }
    } else {
        log_e("Failed to detect ENS160 sensor at both 0x53 and 0x52!");
    }

    return (ahtInitialized || ensInitialized);
}

SensorData readSensors() {
    SensorData data;

    // 1. Read AHT21 First
    if (ahtInitialized) {
        sensors_event_t humidityEvent, tempEvent;
        if (aht.getEvent(&humidityEvent, &tempEvent)) {
            data.temperatureC = tempEvent.temperature;
            data.temperatureF = (data.temperatureC * 1.8f) + 32.0f;
            data.humidityPercent = humidityEvent.relative_humidity;
            data.ahtSuccess = true;
            log_i("AHT21: Temp=%.1f C, Humidity=%.1f %%", data.temperatureC, data.humidityPercent);
        } else {
            log_w("Failed to read data event from AHT21");
        }
    }

    // 2. Read ENS160 Gas Telemetry
    if (ensInitialized) {
        // Poll for new conversion data (up to 1200ms timeout)
        bool hasData = false;
        unsigned long tStart = millis();
        while ((millis() - tStart) < 1200) {
            if (ens160.update() == RESULT_OK && ens160.hasNewData()) {
                hasData = true;
                break;
            }
            delay(50);
        }

        // Guaranteed register read: Always ensure dataBuffer is populated from DATA_AQI (0x21)
        if (!hasData) {
            if (ens160.read(ENS16X_REGISTER_ADDRESS_DATA_AQI, ens160.getDataRaw(), ENS16X_BUFFER_INFO_DATA_SIZE) == RESULT_OK) {
                uint8_t devStatus = 0;
                ens160.read(ENS16X_REGISTER_ADDRESS_DEVICE_STATUS, &devStatus, 1);
                ens160.deviceStatus = devStatus;
                hasData = true;
            }
        }

        if (hasData) {
            uint8_t devStatus = ens160.getDeviceStatus();
            uint8_t validity = (devStatus >> 2) & 0x03;
            data.ensValidity = validity;

            uint8_t aqi = (uint8_t)ens160.getAirQualityIndex_UBA();
            if (aqi < 1) aqi = 1;
            if (aqi > 5) aqi = 5;
            data.aqiUba = aqi;

            data.tvocPpb = ens160.getTvoc();
            data.eco2Ppm = ens160.getEco2();
            data.ensSuccess = true;

            // Validity evaluation:
            // 0: Normal operation
            // 1: Warm-Up phase (first 3 minutes)
            // 2: Initial Start-Up phase (first 1 hour, output signals active)
            // 3: Invalid
            if (data.eco2Ppm >= 400 && validity != 1) {
                // Sensor is warmed up and outputting fresh, valid gas telemetry
                data.ensWarmingUp = false;
                data.ensStale = false;
                data.ensStaleMinutes = 0;
                rtcStaleMinutes = 0;
                rtcHasEverWarmedUp = true;
                rtcLastValidEco2 = data.eco2Ppm;
                rtcLastValidTvoc = data.tvocPpb;
                rtcLastValidAqi = data.aqiUba;
            } else if (validity == 1 || (!rtcHasEverWarmedUp && data.eco2Ppm == 0)) {
                // In initial 3-minute warm-up
                data.ensWarmingUp = true;
                data.ensStale = false;
                data.ensStaleMinutes = 0;
            } else if (rtcHasEverWarmedUp && data.eco2Ppm == 0) {
                // Warmed up previously; use last known valid reading for single-cycle transient
                rtcStaleMinutes++;
                data.eco2Ppm = rtcLastValidEco2;
                data.tvocPpb = rtcLastValidTvoc;
                data.aqiUba = rtcLastValidAqi;
                data.ensWarmingUp = false;
                data.ensStale = true;
                data.ensStaleMinutes = rtcStaleMinutes;
                log_w("ENS160 transient zero reading after warm-up; retaining last valid eCO2: %u ppm (%um old)", 
                      data.eco2Ppm, data.ensStaleMinutes);
            }

            log_i("ENS160: Status=0x%02X (Validity=%u, WarmUp=%s, Stale=%s), AQI=%d (%s), TVOC=%u ppb, eCO2=%u ppm", 
                  devStatus, validity, data.ensWarmingUp ? "YES" : "NO", data.ensStale ? "YES" : "NO",
                  data.aqiUba, data.getAqiDescription(), data.tvocPpb, data.eco2Ppm);
        } else {
            log_w("ENS160 read/update failed!");
            if (rtcHasEverWarmedUp) {
                rtcStaleMinutes++;
                data.eco2Ppm = rtcLastValidEco2;
                data.tvocPpb = rtcLastValidTvoc;
                data.aqiUba = rtcLastValidAqi;
                data.ensSuccess = true;
                data.ensWarmingUp = false;
                data.ensStale = true;
                data.ensStaleMinutes = rtcStaleMinutes;
                log_i("Retaining previous valid gas readings across cycle (%um old)", data.ensStaleMinutes);
            }
        }

        // 3. Write Environmental Compensation for next measurement cycle
        if (data.ahtSuccess) {
            uint16_t tRaw = Ens16x_CalcTempInFromCelsius(data.temperatureC);
            uint16_t rhRaw = Ens16x_CalcRhIn(data.humidityPercent);
            ens160.writeCompensation(tRaw, rhRaw);
        }
    }

    return data;
}
