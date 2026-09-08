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
        ens160.read(ENS16X_REGISTER_ADDRESS_OPMODE, &opMode, 1);
        uint8_t devStatus = 0;
        ens160.read(ENS16X_REGISTER_ADDRESS_DEVICE_STATUS, &devStatus, 1);
        uint8_t validity = (devStatus >> 2) & 0x03;

        if (opMode == ENS16X_OPERATING_MODE_STANDARD) {
            log_i("ENS160 at 0x%02X is ALREADY ACTIVE (OPMODE=0x%02X, Status=0x%02X, Validity=%u). Preserving continuous measurement state!",
                  detectedAddr, opMode, devStatus, validity);
        } else {
            log_i("ENS160 at 0x%02X not active (OPMODE=0x%02X). Starting standard measurement mode (warm-up begins)...",
                  detectedAddr, opMode);
            ens160.startStandardMeasure();
        }
    } else {
        log_e("Failed to detect ENS160 sensor at both 0x53 and 0x52!");
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
        // Poll for new data with up to 1200ms timeout.
        // If the sensor was already running in STANDARD mode during ESP32 sleep,
        // NEWDAT is typically already set and update() succeeds on the first try!
        bool updated = false;
        unsigned long tStart = millis();
        while ((millis() - tStart) < 1200) {
            if (ens160.update() == RESULT_OK) {
                updated = true;
                break;
            }
            delay(50);
        }

        // Fallback: Read registers directly if NEWDAT flag was cleared earlier
        if (!updated) {
            if (ens160.read(ENS16X_REGISTER_ADDRESS_DATA_AQI, ens160.getDataRaw(), ENS16X_BUFFER_INFO_DATA_SIZE) == RESULT_OK) {
                uint8_t devStatus = 0;
                ens160.read(ENS16X_REGISTER_ADDRESS_DEVICE_STATUS, &devStatus, 1);
                ens160.deviceStatus = devStatus;
                updated = true;
            }
        }

        if (updated) {
            uint8_t devStatus = ens160.getDeviceStatus();
            uint8_t validity = (devStatus >> 2) & 0x03;
            data.ensValidity = validity;

            data.aqiUba = (uint8_t)ens160.getAirQualityIndex_UBA();
            if (data.aqiUba < 1) data.aqiUba = 1;
            if (data.aqiUba > 5) data.aqiUba = 5;

            data.tvocPpb = ens160.getTvoc();
            data.eco2Ppm = ens160.getEco2();
            data.ensSuccess = true;

            // Validity: 0 = Normal, 1 = Warm-Up (3 min), 2 = Initial Start-Up (1 hr), 3 = Invalid
            // During warm-up or before algorithm stabilization (eCO2 == 0), mark as warming up
            data.ensWarmingUp = (validity == 1 || data.eco2Ppm == 0);

            log_i("ENS160: Status=0x%02X (Validity=%u, WarmUp=%s), AQI=%d (%s), TVOC=%u ppb, eCO2=%u ppm", 
                  devStatus, validity, data.ensWarmingUp ? "YES" : "NO",
                  data.aqiUba, data.getAqiDescription(), data.tvocPpb, data.eco2Ppm);
        } else {
            log_w("ENS160 read/update failed!");
        }
    }

    return data;
}
