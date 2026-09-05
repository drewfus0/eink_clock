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
Connectivity:
The ESP32-E has WiFi for connecting to the internet for time sync and data updates.


