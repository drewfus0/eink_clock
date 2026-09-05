# Hardware Wiring & Pinout Guide

This guide details the exact electrical connections between the **DFRobot FireBeetle 2 ESP32-E (DFR1139/DFR0654)**, the **Waveshare e-Paper Driver HAT (Rev 2.3)**, the **Waveshare 7.5-inch e-Paper display (V2 / 800x480)**, and the **ENS160+AHT21** environmental sensor module.

---

## 1. Waveshare e-Paper Driver HAT (Rev 2.3) to FireBeetle 2 ESP32-E (SPI)

The Waveshare e-Paper Driver HAT Rev 2.3 is the official driver board designed specifically for Waveshare 7.5-inch e-Paper panels.

### Onboard Slide Switches:
- **Display Config Switch:** Set to **`B`** (7.5-inch series).
- **Interface Config Switch:** Set to **`0`** (4-line SPI).

### Pin Connections to FireBeetle 2 ESP32-E:
| Waveshare HAT Pin | Wire Color (Standard 8-pin) | FireBeetle 2 Silkscreen | ESP32 GPIO | Function | Critical Notes |
|:---|:---|:---|:---|:---|:---|
| **VCC** | Red | **3V3** | 3.3V | Power Supply | 3.3V logic & panel supply |
| **GND** | Black | **GND** | GND | Ground | Common Ground |
| **DIN** | Blue | **23/MOSI** | GPIO 23 | SPI MOSI | Master Out Slave In |
| **CLK** | Yellow | **18/SCK** | GPIO 18 | SPI Clock | SPI Bus Clock |
| **CS** | Orange | **14/D6** | GPIO 14 | SPI CS | Chip Select (Active LOW) |
| **DC** | Green | **26/D3** | GPIO 26 | Data/Command | HIGH = Data, LOW = Command |
| **RST** | White | **25/D2** | GPIO 25 | Reset | Active LOW reset pulse |
| **BUSY** | Purple | **4/D12** | GPIO 4 | Busy Status | Hardware status pin |
| **PWR** | Grey (if 9-pin) | **13/D7** (or **3V3**) | GPIO 13 | **Power Control** | Connect to `13/D7` (or directly to `3V3`). |

> [!TIP]
> **Why `PWR` is kept powered across deep sleep:**
> E-paper partial refresh relies on the controller chip's internal SRAM to compare the previous frame against the new frame. Keeping `PWR` at 3.3V during sleep (or having GPIO 13 hold HIGH) preserves the controller SRAM so partial updates render dark and sharp with zero spottiness. Standby current in this state is only ~2 to 5 µA!

---

## 2. ENS160 + AHT21 Sensor Module to FireBeetle 2 ESP32-E (I2C)

The combined ENS160 (Air Quality: AQI, eCO2, TVOC) and AHT21 (Temperature & Relative Humidity) module communicates over a single shared I2C bus.

| Sensor Pin | Wire Color (Suggested) | FireBeetle 2 PCB Silkscreen | ESP32 GPIO | Function | Notes |
|:---|:---|:---|:---|:---|:---|
| **VCC** | Black | **3V3** | 3.3V Rail | Power Supply | 3.3V operation |
| **GND** | White | **GND** | Ground | Common Ground | Ground |
| **SDA** | Purple | **SDA** | GPIO 21 | I2C Data Line | 0x38 (AHT21), 0x53 or 0x52 (ENS160) |
| **SCL** | Grey | **SCL** | GPIO 22 | I2C Clock Line | 100 kHz standard clock |

*Both sensors reside on the same two I2C wires. The firmware automatically detects the ENS160 on either 0x53 or 0x52, and AHT21 on 0x38.*

---

## 3. Battery & Power Connection

- **Battery:** 3.7V LiPo rechargeable battery (3000mAh, 11.1Wh).
- **Connector:** Connect the battery JST-PH 2.0mm lead directly to the onboard **BAT** connector on the FireBeetle 2 ESP32-E.
- **Charging:** When a USB Type-C cable is connected, the onboard charging IC will charge the LiPo safely at up to 500mA/1000mA with LED charge status.
- **Voltage Sensing:** The FireBeetle 2 ESP32-E includes an integrated 1MΩ / 1MΩ voltage divider already wired directly to **GPIO 34** (ADC1_CH6). No external resistors or extra wiring are needed!

---

## 2. ENS160 + AHT21 Sensor Module to FireBeetle 2 ESP32-E (I2C)

The combined ENS160 (Air Quality: AQI, eCO2, TVOC) and AHT21 (Temperature & Relative Humidity) module communicates over a single shared I2C bus.

| Sensor Pin | Wire Color (Suggested) | FireBeetle 2 PCB Silkscreen | ESP32 GPIO | Function | Notes |
|:---|:---|:---|:---|:---|:---|
| **VCC** | Black | **3V3** | 3.3V Rail | Power Supply | 3.3V operation |
| **GND** | White | **GND** | Ground | Common Ground | Ground |
| **SDA** | Purple | **SDA** | GPIO 21 | I2C Data Line | 0x38 (AHT21), 0x53 or 0x52 (ENS160) |
| **SCL** | Grey | **SCL** | GPIO 22 | I2C Clock Line | 100 kHz standard clock |

*Both sensors reside on the same two I2C wires. The firmware automatically detects the ENS160 on either 0x53 or 0x52, and AHT21 on 0x38.*

---

## 3. Battery & Power Connection

- **Battery:** 3.7V LiPo rechargeable battery (3000mAh, 11.1Wh).
- **Connector:** Connect the battery JST-PH 2.0mm lead directly to the onboard **BAT** connector on the FireBeetle 2 ESP32-E.
- **Charging:** When a USB Type-C cable is connected, the onboard charging IC will charge the LiPo safely at up to 500mA/1000mA with LED charge status.
- **Voltage Sensing:** The FireBeetle 2 ESP32-E includes an integrated 1MΩ / 1MΩ voltage divider already wired directly to **GPIO 34** (ADC1_CH6). No external resistors or extra wiring are needed!

---

## 4. FireBeetle 2 Ultra-Low Power Optimization

The FireBeetle 2 ESP32-E has a dedicated low-power solder pad on the bottom/top of the board:
- By default, the power LED remains lit during deep sleep, consuming ~1.5mA.
- Cutting or desoldering the **Low Power / LED jumper** (marked on the PCB) disconnects the standby LED and peripheral LDO, dropping deep sleep current down to **~10 to 15 µA**!
