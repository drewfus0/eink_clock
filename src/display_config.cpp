#include "display_config.h"

// Instantiate the global display object using the pins defined in config.h
GxEPD2_DISPLAY_CLASS display(GxEPD2_DRIVER_CLASS(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

void displayInitHardware() {
    // Initialize SPI and display with serial diagnostic output
    display.init(115200, true, 2, false);
    display.setRotation(0); // 0 = standard landscape (800x480)
    display.setTextColor(GxEPD_BLACK);
}

void displayPowerOff() {
    display.powerOff();
}

void displayHibernate() {
    display.hibernate();
}
