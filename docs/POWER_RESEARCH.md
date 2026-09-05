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

## 2. Power Consumption Breakdown

| Subsystem / State | Current Draw | Typical Duration | Frequency |
|:---|:---|:---|:---|
| **ESP32 Deep Sleep** (low-power jumper cut) | **~15 µA** | 56 – 58 seconds | Continuous |
| **Active ESP32 + I2C Sensors** | ~35 mA | 0.2 seconds | Once per minute |
| **Display Active Refresh** | ~30 mA | 3.5 seconds (full) | Once per minute (or partial) |
| **Display Hibernation Mode** | **< 1 µA** | 56 seconds | Panel power turned off |
| **WiFi + NTP Time Sync** | ~110 mA | ~2.5 seconds | **Once every 6 hours** |

### Why WiFi is Not Activated Every Minute
Connecting to a 2.4GHz Wi-Fi network, negotiating DHCP, resolving DNS, and performing NTP round-trips consumes ~110mA for 2 to 4 seconds.
- Doing this **every minute** would consume:  
  `110mA × 3s / 60s = 5.5 mA average current` → Battery lasts only **~22 days**.
- By using the **ESP32 internal RTC** during sleep and only syncing NTP **every 6 hours**:  
  WiFi energy contribution drops from 5.5 mA to **~0.012 mA** (a 99.8% energy reduction!).
  The internal RTC drifts less than 1–2 seconds over 6 hours, which is completely imperceptible on a minute clock.

---

## 3. Battery Longevity Projections (3000mAh LiPo)

Assuming an 85% usable battery capacity (2550 mAh) to protect the LiPo cell from deep discharge:

### Scenario A: 1-Minute Update Interval (Default Clock Mode)
- **Active Cycle:** 3.5s @ 32mA (display refresh + sensors) = 112 mA·s
- **Sleep Cycle:** 56.5s @ 0.015mA = 0.85 mA·s
- **Average Current:** `(112 + 0.85) / 60 ≈ 1.88 mA`
- **Expected Battery Life:**  
  `2550 mAh / 1.88 mA ≈ 1,356 hours` ≈ **~56 days (nearly 2 months)** on a single charge!

### Scenario B: 5-Minute Update Interval (Environmental Station Mode)
- **Active Cycle:** 3.5s @ 32mA = 112 mA·s
- **Sleep Cycle:** 296.5s @ 0.015mA = 4.45 mA·s
- **Average Current:** `(112 + 4.45) / 300 ≈ 0.39 mA`
- **Expected Battery Life:**  
  `2550 mAh / 0.39 mA ≈ 6,538 hours` ≈ **~272 days (9 months)** on a single charge!

---

## 4. Hardware Power Saving Recommendations
1. **Low Power Jumper:** Cut the "Low Power" solder jumper pad on the FireBeetle 2 ESP32-E board to disable the always-on power LED and unused voltage regulators.
2. **Display Hibernation:** Ensure `display.hibernate()` is called after every frame (already implemented in `src/main.cpp`).
3. **Sensor Sleep:** ENS160 is placed in low-power idle mode between measurements.
