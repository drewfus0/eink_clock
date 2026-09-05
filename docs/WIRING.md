# Hardware Wiring & Pinout Guide

This guide details the exact electrical connections between the **DFRobot FireBeetle 2 ESP32-E (N16R2)**, the **GooDisplay DESPI-C02** adapter board, the **Waveshare 7.5-inch e-Paper display**, and the **ENS160+AHT21** environmental sensor module.

---

## 1. Adapter Board Wiring Options

### Option A: Waveshare e-Paper Driver HAT (Rev 2.3) [RECOMMENDED]

The Waveshare e-Paper Driver HAT Rev 2.3 is the official driver board specifically designed for Waveshare 7.5-inch e-Paper panels.

#### Front Slide Switches:
- **Display Config Switch:** Set to **`B`** (configures driver for 7.5" panels).
- **Interface Config Switch:** Set to **`0`** (selects standard 4-line SPI).

#### Pin Connections to FireBeetle 2 ESP32-E:
| Waveshare HAT Pin | FireBeetle 2 Silkscreen | ESP32 GPIO | Function | Critical Notes |
|:---|:---|:---|:---|:---|
| **VCC** | **3V3** | 3.3V | Power Supply | 3.3V logic & panel supply |
| **GND** | **GND** | GND | Ground | Common reference |
| **DIN** | **23/MOSI** | GPIO 23 | SPI MOSI | Master Out Slave In |
| **CLK** | **18/SCK** (or MSIO) | GPIO 18 | SPI Clock | SPI Bus Clock |
| **CS** | **14/D6** | GPIO 14 | SPI CS | Chip Select (Active LOW) |
| **DC** | **26/D3** | GPIO 26 | Data/Command | HIGH = Data, LOW = Command |
| **RST** | **25/D2** | GPIO 25 | Reset | Active LOW reset pulse |
| **BUSY** | **4/D12** | GPIO 4 | Busy Status | Hardware status pin |
| **PWR** | **13/D7** (or **3V3**) | GPIO 13 | **Power Switch** | Controls the HAT's onboard MOSFET. Plug into `13/D7` (or directly into `3V3`). |

> [!TIP]
> **Connecting `PWR` to `13/D7` (or `3V3`):**
> - **Option A (Recommended):** Connect `PWR` to **`13/D7` (GPIO 13)**. The firmware automatically drives `13/D7` HIGH (3.3V) on boot and LOW on sleep.
> - **Option B:** Connect `PWR` directly to **`3V3`** (together with VCC). This keeps the HAT permanently enabled.
> *(Note: Do NOT use `19/MISO` for PWR because the ESP32 SPI hardware peripheral claims GPIO 19 as an SPI input, turning off power).*

---

### Option B: GooDisplay DESPI-C02 to FireBeetle 2 ESP32-E (SPI)

The DESPI-C02 is a generic breakout board designed for GoodDisplay panels.

| DESPI-C02 Pin | Wire Color (Suggested) | FireBeetle 2 PCB Silkscreen | ESP32 GPIO | Function | Notes |
|:---|:---|:---|:---|:---|:---|
| **VCC** (3.3V) | Orange | **3V3** | 3.3V Rail | Logic & Panel Power | **Must be 3.3V** (Do NOT use 5V) |
| **GND** | Red | **GND** | Ground | Common Ground | Power & signal reference |
| **SDI** (DIN / MOSI) | Brown (SDI) | **23/MOSI** | GPIO 23 | Hardware SPI MOSI | Master Out Slave In |
| **SCK** (CLK) | Black | **18/SCK** (or MSIO)| GPIO 18 | Hardware SPI Clock | SPI Bus Clock |
| **CS** | White | **14/D6** | GPIO 14 | SPI Chip Select | Active LOW |
| **D/C** | Grey | **26/D3** | GPIO 26 | Data / Command | HIGH = Data, LOW = Command |
| **RES** (RST) | Purple | **25/D2** | GPIO 25 | Display Reset | Active LOW |
| **BUSY** | Yellow | **4/D12** | GPIO 4 | Panel Busy Status | Active HIGH when refreshing |

> [!IMPORTANT]
> **DESPI-C02 RESE Switch Configuration:**
> The DESPI-C02 board features a small slide switch or solder selector labeled **RESE** (0.47Ω vs 3Ω).
> For large **7.5-inch displays**, the current limit resistor selector must typically be set to position **0.47Ω** (position "1" / 0.47R).

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
