Hardware list
- FireBeetle 2 ESP32-E(N16R2)
- Waveshare e-Paper Driver HAT (Rev 2.3)
- Waveshare 7.5inch e-Paper (V2 / 800x480)
- ENS160+AHT21 Air Quality Sensor Module 
- Battery 3.7 lipo 3.7v 11.1Wh 3000mah

Simple Project Breif.
This is a clock using the ESP32-E, it should sync to the internet and display the time and date. It should also display the temperature and humidity from the AHT21 sensor and air quality from the ENS160 sensor. All should be displayed on the e-paper screen.
Display Priority:
1. Time.
2. Date.
3. Temperature.
4. Humidity.
5. Air Quality.
6. Battery/other info.

The code should be in platformIO. 
Power Management:
 The device should sleep most of the time to save power. Interval to be research display update cycle for display and power usage.
Display:
The display is a 7.5inch e-Paper (800x480) and uses a Waveshare e-Paper Driver HAT (Rev 2.3).
Sensors:
The sensors are connected to the ESP32-E via the I2C bus.
Connectivity & Updates:
The ESP32-E has WiFi for connecting to the internet for NTP time synchronization and Over-The-Air (OTA) firmware updates.

OTA Firmware Updates:
1. Dual-slot partition table (partitions_16MB_ota.csv) with 6.25MB app0/app1 slots.
2. Automated GitHub Releases Updates:
   - Repository: https://github.com/drewfus0/eink_clock
   - Every 6 hours during NTP sync (or on boot/reset), the clock checks version.json on GitHub.
   - If a new version is detected, it automatically downloads firmware.bin from the GitHub Release, renders an updating dialog on the e-paper panel, flashes into the alternate slot, and reboots.
   - Use `./create-release.sh` to auto-bump the version and build release binaries.
   - GitHub Actions (.github/workflows/release.yml) automatically compiles and attaches firmware.bin whenever a git tag (e.g. `v1.0.1`) is pushed.
3. Web Browser Upload: http://<IP>/update or http://eink-clock.local/update.
4. PlatformIO CLI Upload: pio run -e firebeetle2_esp32e_ota -t upload.
5. Deep-Sleep Integration:
   - On cold boot or reset button press, a 30-second local update window opens.
   - During normal unattended 6-hour syncs, Wi-Fi stays on for only ~2.3 seconds to sync NTP and check GitHub, preserving battery longevity.
