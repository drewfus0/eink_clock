# Power Management & Display Update Cycle Research

This research document analyzes power consumption, e-paper display refresh dynamics, and battery runtime expectations for the 3000mAh LiPo powered E-Ink Clock.

---

## 1. E-Paper Display Refresh Dynamics (Waveshare 7.5")

### Physical Characteristics of E-Ink
Electrophoretic ink displays work by physically migrating micro-capsules of charged black and white pigments through a viscous fluid using electric fields. Once moved, they remain in place indefinitely without consuming any electricity.

### Refresh Modes:
1. **Full Refresh:**
   - **Duration:** Typically **3.0 to 4.5 seconds** for 7.5-inch panels (800x480).
   - **Visual Effect:** Several high-contrast flashes (inverting black/white) to clear particle memory.
   - **Power Consumption:** ~25mA to 35mA during the active 3.5s cycle.
   - **Panel Health:** Essential to prevent image retention ("ghosting") and irreversible particle burn-in.

2. **Partial Refresh (Fast Mode):**
   - **Duration:** ~0.5 to 0.8 seconds.
   - **Visual Effect:** Only the updated pixels change without flashing the whole screen.
   - **Caveat:** Over multiple consecutive partial updates, minor ghosting accumulates. A periodic full refresh (e.g., every 30 to 60 cycles) is recommended by GoodDisplay & Waveshare.

### Recommended Update Intervals:
- **60 Seconds (1 Minute) - *Current Default*:**
  - Standard clock operation where the minute digit increments seamlessly.
  - Aligned to `:00` seconds of each minute using the ESP32 RTC.
  - Periodic full refresh once every 30 minutes to wipe any ghosting clean.
- **300 Seconds (5 Minutes) - *Extended Battery Mode*:**
  - Ideal for wall-mounted weather and indoor environment stations.
  - Multiplies battery life by ~4.5x.

---

## 2. Power Consumption Breakdown (Fast Partial Refresh)

| Subsystem / State | Current Draw | Typical Duration | Frequency |
|:---|:---|:---|:---|
| **ESP32 Deep Sleep** (low-power jumper cut) | **~15 µA** | ~58 seconds | Continuous |
| **Display Controller Standby** (`powerOff()`) | **~20 µA** | ~58 seconds | Retains internal SRAM for fast differential refresh |
| **Active ESP32 + I2C Sensors** | ~35 mA | 0.2 seconds | Once per minute |
| **Fast Partial Refresh** (7.5" V2 / T7) | ~30 mA | **1.6 seconds** | 29 out of 30 minutes (no screen flashing) |
| **Periodic Full Refresh** (Anti-ghosting) | ~30 mA | **4.2 seconds** | Once every 30 cycles (Cycle #30, #60, ...) |
| **WiFi + NTP Time Sync** | ~110 mA | ~2.5 seconds | **Once every 6 hours** |

### Retaining Controller SRAM Across Deep Sleep
- **`display.hibernate()` (< 1 µA):** Shuts off the display controller completely. Because controller SRAM is lost, every subsequent wake-up must do a slow full refresh (4.2s with heavy black/white inversion flickering).
- **`display.powerOff()` (~20 µA):** Turns off the high-voltage panel driving charge pumps (safe for e-ink particles, zero DC bias), but keeps the Waveshare HAT / GD7965 controller logic powered. This preserves the previous frame buffer, enabling instant, differential **fast partial refresh (1.6s)** without screen flash.
- **Standby Impact:** 20 µA draws only `0.020 mA × 24h = 0.48 mAh/day`. Over 30 days, that is only **14.4 mAh** out of a 3000 mAh battery!

---

## 3. Battery Longevity Projections (3000mAh LiPo)

Assuming an 85% usable battery capacity (**2550 mAh**) to safeguard the LiPo cell from deep discharge:

### Scenario A: Fast Partial Refresh (1-Minute Updates, User Target: 1–2 Weeks)
- **Fast Partial Cycles (29 per 30 mins):**  
  - Active: 1.8s (0.2s sensors + 1.6s display) @ ~32mA = 57.6 mA·s  
  - Sleep: 58.2s @ (15 µA ESP32 + 20 µA display) = 2.04 mA·s  
  - Energy per cycle: **59.64 mA·s**
- **Full Refresh Cycle (1 per 30 mins):**  
  - Active: 4.4s (0.2s sensors + 4.2s display) @ ~32mA = 140.8 mA·s  
  - Sleep: 55.6s @ 35 µA = 1.95 mA·s  
  - Energy per cycle: **142.75 mA·s**
- **WiFi NTP Sync (every 6 hours = once per 360 cycles):**  
  - Extra Active: 2.5s @ 110mA = 275 mA·s (~0.76 mA·s per cycle)
- **Average 30-minute block consumption:**  
  `[ (29 × 59.64) + 142.75 + (0.76 × 30) ] = 1729.56 + 142.75 + 22.8 = 1895.1 mA·s`  
  Average current: `1895.1 mA·s / 1800 s ≈ 1.05 mA`
- **Expected Battery Life:**  
  `2550 mAh / 1.05 mA ≈ 2,428 hours` ≈ **~101 days (~3.3 months)** on a single charge!  
  *(Drastically exceeds the user's 1–2 week requirement by 7x, with instant flicker-free updates).*

### Scenario B: 5-Minute Update Interval (Maximum Longevity Mode)
- **Active Cycle:** 1.8s @ 32mA = 57.6 mA·s
- **Sleep Cycle:** 298.2s @ 0.035mA = 10.4 mA·s
- **Average Current:** `(57.6 + 10.4) / 300 ≈ 0.23 mA`
- **Expected Battery Life:**  
  `2550 mAh / 0.23 mA ≈ 11,086 hours` ≈ **~460 days (~1.25 years)** on a single charge!

---

## 4. Hardware Power Saving Recommendations
1. **Low Power Jumper:** Cut the "Low Power" solder jumper pad on the FireBeetle 2 ESP32-E board to disable the always-on power LED and unused USB UART circuitry.
2. **Display Power Mode:** Use `displayPowerOff()` (configured in `src/main.cpp`) to turn off high-voltage rail drivers while preserving controller SRAM.
3. **Anti-Ghosting Interval:** Configured via `FULL_REFRESH_CYCLE_COUNT` in `include/config.h` (default: 30 cycles / 30 minutes).
4. **Sensor Sleep:** The ENS160 enters idle mode between measurements.
