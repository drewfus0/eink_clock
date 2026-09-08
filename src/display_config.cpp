#include "display_config.h"

// Instantiate the global display object using the pins defined in config.h
GxEPD2_DISPLAY_CLASS display(GxEPD2_DRIVER_CLASS(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

#include <driver/gpio.h>

// GxEPD2_EPD explicitly lists 'friend class GDEW075T7_OTP;' in GxEPD2_EPD_friends.h.
// We use this friend declaration to safely suppress the hardware RST pulse on partial-refresh
// wakeups so the controller SRAM (previous frame buffer) is NOT wiped when waking from deep sleep.
class GDEW075T7_OTP {
public:
    static void setRstPin(GxEPD2_EPD& epd, int16_t pin) {
        epd._rst = pin;
    }
};

void displayInitHardware(bool initial) {
    #if defined(EPD_PWR_PIN) && (EPD_PWR_PIN >= 0)
    // Release hold latch if waking from deep sleep, and ensure power is HIGH
    gpio_hold_dis((gpio_num_t)EPD_PWR_PIN);
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    if (initial) {
        delay(15); // Allow onboard power rail and charge pump to stabilize on initial cold boot
    }
    #endif

    #if defined(EPD_RST_PIN) && (EPD_RST_PIN >= 0)
    gpio_hold_dis((gpio_num_t)EPD_RST_PIN);
    pinMode(EPD_RST_PIN, OUTPUT);
    digitalWrite(EPD_RST_PIN, HIGH);
    #endif

    #if defined(EPD_CS_PIN) && (EPD_CS_PIN >= 0)
    gpio_hold_dis((gpio_num_t)EPD_CS_PIN);
    pinMode(EPD_CS_PIN, OUTPUT);
    digitalWrite(EPD_CS_PIN, HIGH);
    #endif

    // Explicitly configure hardware SPI pins for FireBeetle 2 ESP32-E
    SPI.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, EPD_CS_PIN);

    // Initialize SPI and display with serial diagnostic output
    // initial = true on first boot or periodic full anti-ghosting refresh
    // initial = false on differential partial refresh cycles (preserves controller SRAM)
    if (!initial && (EPD_RST_PIN >= 0)) {
        // Suppress hardware reset pulse during init so controller SRAM is retained
        GDEW075T7_OTP::setRstPin(display.epd2, -1);
        display.init(115200, false, 10, false);
        GDEW075T7_OTP::setRstPin(display.epd2, EPD_RST_PIN);
    } else {
        display.init(115200, initial, 10, false);
    }

    display.setRotation(0); // 0 = standard landscape (800x480)
    display.setTextColor(GxEPD_BLACK);

    #if defined(EPD_PWR_PIN) && (EPD_PWR_PIN >= 0)
    // Re-assert PWR pin to ensure SPI.begin() didn't alter its mode
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    #endif
}

void displayPowerOff() {
    // Power off high-voltage driving circuits on the panel
    display.powerOff();

    #if defined(EPD_PWR_PIN) && (EPD_PWR_PIN >= 0)
    // Keep 3.3V power to the HAT controller logic across deep sleep so internal
    // SRAM preserves the previous frame buffer. Differential refresh relies on this!
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    gpio_hold_en((gpio_num_t)EPD_PWR_PIN);
    #endif

    #if defined(EPD_RST_PIN) && (EPD_RST_PIN >= 0)
    // Hold RST HIGH across deep sleep so the controller is never glitched or reset
    pinMode(EPD_RST_PIN, OUTPUT);
    digitalWrite(EPD_RST_PIN, HIGH);
    gpio_hold_en((gpio_num_t)EPD_RST_PIN);
    #endif

    #if defined(EPD_CS_PIN) && (EPD_CS_PIN >= 0)
    // Hold CS HIGH (inactive) across deep sleep
    pinMode(EPD_CS_PIN, OUTPUT);
    digitalWrite(EPD_CS_PIN, HIGH);
    gpio_hold_en((gpio_num_t)EPD_CS_PIN);
    #endif

    gpio_deep_sleep_hold_en();
}

void displayHibernate() {
    display.hibernate();
    #if defined(EPD_PWR_PIN) && (EPD_PWR_PIN >= 0)
    digitalWrite(EPD_PWR_PIN, LOW);
    pinMode(EPD_PWR_PIN, INPUT);
    #endif
}
