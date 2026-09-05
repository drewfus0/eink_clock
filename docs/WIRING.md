# Hardware Wiring & Pinout Guide

This guide details the exact electrical connections between the **DFRobot FireBeetle 2 ESP32-E (N16R2)**, the **GooDisplay DESPI-C02** adapter board, the **Waveshare 7.5-inch e-Paper display**, and the **ENS160+AHT21** environmental sensor module.

---

## 1. GooDisplay DESPI-C02 to FireBeetle 2 ESP32-E (SPI)

The DESPI-C02 is an interface/breakout board designed to bridge bare FPC ribbon cables from e-Paper panels to microcontrollers via standard SPI.

| DESPI-C02 Pin | Wire Color (Suggested) | FireBeetle 2 ESP32-E Pin | Function | Notes |
|:---|:---|:---|:---|:---|
| **VCC** (3.3V) | 🔴 Red | **3V3** | Logic & Panel Power | **Must be 3.3V** (Do NOT use 5V) |
| **GND** | ⚫ Black | **GND** | Common Ground | Power & signal reference |
| **SDI** (DIN / MOSI) | 🟡 Yellow | **IO23** (MOSI) | Hardware SPI MOSI | Master Out Slave In |
| **SCK** (CLK) | 🟠 Orange | **IO18** (SCK) | Hardware SPI Clock | SPI Bus Clock |
| **CS** | 🔵 Blue | **IO14** (D10) | SPI Chip Select | Active LOW |
| **D/C** | 🟢 Green | **IO26** (D3) | Data / Command | HIGH = Data, LOW = Command |
| **RES** (RST) | ⚪ White | **IO25** (D2) | Display Reset | Active LOW |
| **BUSY** | 🟣 Purple | **IO4** (D0) | Panel Busy Status | Active HIGH when refreshing |

> [!IMPORTANT]
> **DESPI-C02 RESE Switch Configuration:**
> The DESPI-C02 board features a small slide switch or solder selector labeled **RESE** (0.47Ω vs 3Ω).
> For large **7.5-inch displays**, the current limit resistor selector must typically be set to position **0.47Ω** (position "1" / 0.47R). Please confirm with your panel sticker (panels starting with GDEW075 use 0.47Ω).

---

## 2. ENS160 + AHT21 Sensor Module to FireBeetle 2 ESP32-E (I2C)

The combined ENS160 (Air Quality: AQI, eCO2, TVOC) and AHT21 (Temperature & Relative Humidity) module communicates over a single shared I2C bus.

| Sensor Pin | Wire Color (Suggested) | FireBeetle 2 ESP32-E Pin | Function | Notes |
|:---|:---|:---|:---|:---|
| **VCC** | 🔴 Red | **3V3** | Power Supply | 3.3V operation |
| **GND** | ⚫ Black | **GND** | Common Ground | Ground |
| **SDA** | 🔵 Blue / Green | **IO21** (SDA) | I2C Data Line | 0x38 (AHT21), 0x53 or 0x52 (ENS160) |
| **SCL** | 🟡 Yellow | **IO22** (SCL) | I2C Clock Line | 100 kHz standard clock |

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
