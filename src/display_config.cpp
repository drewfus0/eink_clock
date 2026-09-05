#include "display_config.h"

// Instantiate the global display object using the pins defined in config.h
GxEPD2_DISPLAY_CLASS display(GxEPD2_DRIVER_CLASS(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

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

    // Explicitly configure hardware SPI pins for FireBeetle 2 ESP32-E
    SPI.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, EPD_CS_PIN);

    // Initialize SPI and display with serial diagnostic output
    // initial = true on first boot or periodic full anti-ghosting refresh
    // initial = false on differential partial refresh cycles (preserves controller SRAM)
    // reset_duration = 10ms (standard reset pulse)
    display.init(115200, initial, 10, false);
    display.setRotation(0); // 0 = standard landscape (800x480)
    display.setTextColor(GxEPD_BLACK);

    #if defined(EPD_PWR_PIN) && (EPD_PWR_PIN >= 0)
    // Re-assert PWR pin to ensure SPI.begin() didn't alter its mode
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    #endif
}

#include <driver/gpio.h>

void displayPowerOff() {
    // Power off high-voltage driving circuits on the panel
    display.powerOff();

    #if defined(EPD_PWR_PIN) && (EPD_PWR_PIN >= 0)
    // CRITICAL FOR PARTIAL REFRESH:
    // Keep 3.3V power to the HAT controller logic across deep sleep so internal
    // SRAM preserves the previous frame buffer. Differential refresh relies on this!
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    gpio_hold_en((gpio_num_t)EPD_PWR_PIN);
    gpio_deep_sleep_hold_en();
    #endif
}

void displayHibernate() {
    display.hibernate();
    #if defined(EPD_PWR_PIN) && (EPD_PWR_PIN >= 0)
    digitalWrite(EPD_PWR_PIN, LOW);
    pinMode(EPD_PWR_PIN, INPUT);
    #endif
}
