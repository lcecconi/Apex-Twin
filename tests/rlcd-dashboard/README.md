# Waveshare ESP32-S3-RLCD-4.2 Dashboard Test

Dedicated hardware validation and telemetry dashboard for the **Waveshare ESP32-S3-RLCD-4.2** development board, designed for the **Apex-Dash** module of the Apex-Twin telemetry system.

---

## 1. Overview

This test exercises and validates all key onboard hardware components of the Waveshare ESP32-S3-RLCD-4.2 board:
* **4.2" Reflective LCD (RLCD):** Driven by the Sitronix ST7305 controller over high-speed SPI with a custom U8g2 full-frame buffer engine running in landscape mode (400 × 300 px) at ~20 FPS.
* **Sensirion SHTC3 Temperature & Humidity Sensor:** Polled over I2C at 0x70 with 8-bit polynomial CRC validation.
* **NXP PCF85063A Real-Time Clock (RTC):** Communicating over I2C at 0x51 providing real-time calendar and clock timekeeping.
* **Battery Voltage Monitor (ADC):** Calibrated voltage monitoring on GPIO 4 with the onboard 200k/100k voltage divider (1/3 ratio).
* **Hardware Buttons:** User interaction support via KEY button (GPIO 18) and BOOT button (GPIO 0).
* **Octal PSRAM & Memory Health:** Validates the 8 MB OPI PSRAM and 16 MB Flash operation.

---

## 2. Pin Mapping & Hardware Configuration

| Function | Pin / GPIO | Notes |
| :--- | :--- | :--- |
| **LCD SPI SCLK** | GPIO 11 | SPI Clock (24 MHz) |
| **LCD SPI MOSI** | GPIO 12 | SPI Data |
| **LCD DC / RS** | GPIO 5 | Data / Command select |
| **LCD CS** | GPIO 40 | Chip Select (Active Low) |
| **LCD RST** | GPIO 41 | Hardware Reset |
| **I2C SDA** | GPIO 13 | Shared I2C Bus (SHTC3: 0x70, PCF85063A: 0x51) |
| **I2C SCL** | GPIO 14 | Shared I2C Clock (400 kHz) |
| **VBAT ADC** | GPIO 4 | ADC1 Channel 3 (1/3 Divider: $V_{bat} = V_{adc} \times 3$) |
| **BOOT Button** | GPIO 0 | Active Low (Pull-Up) — Toggles display color inversion (Normal / Inverted) |
| **KEY Button** | GPIO 18 | Active Low (Pull-Up) — Toggles view modes (Sensors / Telemetry) |

---

## 3. UI Views & Controls

Pressing the **KEY Button (GPIO 18)** toggles between two display modes:

### Mode 1: Sensors & System Dashboard (Default)
* **Header:** APEX-TWIN [DASH-TEST], Live RTC Clock (`HH:MM:SS`), Battery Voltage & Percentage.
* **Card 1 (Top-Left) — Environment (SHTC3):** Live Ambient Temperature (°C) and Relative Humidity (% RH) with I2C online status badge.
* **Card 2 (Top-Right) — Power & Battery:** Live Battery Voltage (V), Charge Level (%), and animated visual charge bar.
* **Card 3 (Bottom-Left) — ESP32-S3 System:** CPU Frequency (240 MHz Dual-Core), Free Heap RAM, Free 8 MB Octal PSRAM, Flash size.
* **Card 4 (Bottom-Right) — Hardware Diagnostics:** RTC Date (`YYYY-MM-DD`), live BOOT/KEY button press states, rendering FPS and frame time.
* **Footer:** Control hints and system uptime counter.

### Mode 2: Kart Telemetry Mode Preview
* Full-width dynamic RPM tachometer bar across the top (0–14,000 RPM).
* Large high-contrast Speedometer readout (km/h) in the center.
* Gear indicator display box.
* Lap time readouts: Last Lap, Best Lap, Delta, and Sector indicators.
* Live ambient temperature and battery status overlay.

---

## 4. Building and Flashing

### Build
```bash
uvx platformio run -d tests/rlcd-dashboard
```

### Upload to Device
```bash
uvx platformio run -d tests/rlcd-dashboard --target upload --upload-port /dev/ttyACM0
```

### Serial Monitor (115200 baud)
```bash
uvx platformio device monitor -d tests/rlcd-dashboard --port /dev/ttyACM0 --baud 115200
```
Telemetry logs are output every second via USB CDC:
```text
[DASH] Temp: +31.63 C | Humi: 35.6% | Bat: 4.16V (100%) | RTC: 14:49:26 | Heap: 357KB | PSRAM: 8174KB | FPS: 20.0
```
