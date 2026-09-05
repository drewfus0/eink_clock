#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include "config.h"

// =============================================================================
// E-Paper Display Model Selection
// =============================================================================
// Default: Waveshare 7.5inch V2 (800x480 Black/White) or GoodDisplay GDEW075T7
// Uncomment ONE display configuration below matching your specific panel:

#define DISPLAY_MODEL_WS_75_V2
// #define DISPLAY_MODEL_WS_75_V1      // 640x384 Black/White
// #define DISPLAY_MODEL_WS_75_3C      // 800x480 3-Color (Black/White/Red)
// #define DISPLAY_MODEL_WS_75_HD      // 880x528 Black/White High-Res

#if defined(DISPLAY_MODEL_WS_75_V2)
    // 7.5" b/w 800x480 (V2 / 075BN-T7-D2 / GDEW075T7)
    #define GxEPD2_DISPLAY_CLASS GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>
    #define GxEPD2_DRIVER_CLASS GxEPD2_750_T7
    // Alternate 7.5" V2 (UC8179 ultra-fast 1.2s):
    // #define GxEPD2_DISPLAY_CLASS GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>
    // #define GxEPD2_DRIVER_CLASS GxEPD2_750_GDEY075T7
#elif defined(DISPLAY_MODEL_WS_75_V1)
    // 7.5" b/w 640x384 (V1 / T8)
    #define GxEPD2_DISPLAY_CLASS GxEPD2_BW<GxEPD2_750, GxEPD2_750::HEIGHT>
    #define GxEPD2_DRIVER_CLASS GxEPD2_750
#elif defined(DISPLAY_MODEL_WS_75_3C)
    // 7.5" 3-color 800x480 (Z08)
    #define GxEPD2_DISPLAY_CLASS GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT>
    #define GxEPD2_DRIVER_CLASS GxEPD2_750c_Z08
#endif

// Declare the external display instance
extern GxEPD2_DISPLAY_CLASS display;

// Helper management routines
// initial = true on first boot / full anti-ghosting refresh
// initial = false on deep-sleep wake to preserve controller SRAM for fast differential refresh
void displayInitHardware(bool initial = true);
void displayPowerOff();
void displayHibernate();
