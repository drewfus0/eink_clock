#include "display_config.h"

// Instantiate the global display object using the pins defined in config.h
GxEPD2_DISPLAY_CLASS display(GxEPD2_DRIVER_CLASS(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

void displayInitHardware(bool initial) {
    // Initialize SPI and display with serial diagnostic output
    // initial = true on first boot or periodic full anti-ghosting refresh
    // initial = false on differential partial refresh cycles (preserves controller SRAM)
    display.init(115200, initial, 2, false);
    display.setRotation(0); // 0 = standard landscape (800x480)
    display.setTextColor(GxEPD_BLACK);
}

void displayPowerOff() {
    display.powerOff();
}

void displayHibernate() {
    display.hibernate();
}
