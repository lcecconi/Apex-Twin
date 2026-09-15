# Waveshare ESP32-S3-RLCD-4.2 — Technical Documentation & Hardware Reference

This directory contains complete technical documentation, datasheets, schematics, board pinouts, 3D CAD files, driver examples, and official guides for the **Waveshare ESP32-S3-RLCD-4.2** development board (AliExpress item ID `1005010730699439` / Waveshare SKU `33298` & `33507`).

---

## 1. Product Summary

The **Waveshare ESP32-S3-RLCD-4.2** is a low-power AIoT (AI + IoT) development board built around the **Espressif ESP32-S3-WROOM-1-N16R8** module and a **4.2-inch Reflective LCD (RLCD)** display.

* **E-Paper-like Daylight Readability:** The 4.2" RLCD reflects ambient light with zero backlight power draw, providing sunlight readability similar to E-Ink but with video-capable, instantaneous refresh rates.
* **Acoustic Front-End for AI Voice Interaction:** Features a dual-microphone array paired with the Everest Semiconductor **ES7210** 4-channel audio ADC for hardware echo cancellation (AEC) and noise suppression, alongside the **ES8311** low-power audio DAC for speaker playback.
* **Sensors & Timekeeping:** Integrated Sensirion **SHTC3** digital temperature/humidity sensor and NXP **PCF85063A** real-time clock with dedicated backup battery support.
* **Storage & Power:** Onboard 18650 lithium battery slot with charging/protection circuit, reverse polarity protection, and a MicroSD (TF) card slot.

---

## 2. Hardware Architecture & Key Components

| Component | IC / Part Number | Description & Function |
| :--- | :--- | :--- |
| **Main MCU / SoC** | **ESP32-S3-WROOM-1-N16R8** | Dual-core Xtensa 32-bit LX7 @ up to 240 MHz, 512 KB SRAM, 384 KB ROM, **16 MB Quad SPI Flash**, **8 MB Octal PSRAM**, 2.4 GHz Wi-Fi 4 (802.11 b/g/n) + Bluetooth 5.0 (LE). |
| **Reflective Display (RLCD)** | **4.2" Memory / Reflective LCD** (Driver: **Sitronix ST7305 / ST7306**) | Resolution: 300 × 400 pixels (monochrome / multi-level grayscale). Reflective display without backlight, ultra-low power static consumption, high refresh rate. |
| **Audio Input (ADC)** | **Everest Semi ES7210** | 4-channel, 24-bit audio ADC (SNR 102 dB, I2C control + I2S/TDM audio interface) connected to the dual microphone array for voice recognition and acoustic echo cancellation. |
| **Audio Output (DAC)** | **Everest Semi ES8311** | Ultra-low power 24-bit mono audio DAC (SNR 96 dB, I2C control + I2S audio interface) feeding the onboard speaker amplifier and MX1.25 2-pin connector. |
| **Digital Temp / Humidity** | **Sensirion SHTC3** | High-precision environmental sensor measuring relative humidity (0–100% RH, ±2%) and temperature (-40°C to 125°C, ±0.2°C). |
| **Real-Time Clock (RTC)** | **NXP PCF85063A** | Low-power CMOS real-time clock and calendar with programmable alarm and timer functions. Supports independent rechargeable RTC backup battery via PH1.0 header. |
| **External Storage** | **MicroSD (TF Card Slot)** | Connected via SDMMC / SPI interface for asset storage, voice prompts, and logging (FAT32 filesystem). |
| **Power Management** | **18650 Battery Circuit & Type-C** | Onboard 18650 lithium battery holder, USB-C 5V input, charge status LED (`CHG`), battery reverse polarity warning LED (`WRN`), power latch button (`PWR`), and battery voltage monitoring via ADC voltage divider. |
| **User Controls** | **BOOT & KEY Buttons** | BOOT button (GPIO 0, also firmware flashing) + KEY button (GPIO 18, user programmable). |

---

## 3. Complete Pinout & GPIO Mapping Table

### A. Display Interface (ST7305 / ST7306 - SPI)
| Signal Name | ESP32-S3 GPIO | Description |
| :--- | :--- | :--- |
| **LCD_MOSI / DIN** | **GPIO 12** | SPI Data Output |
| **LCD_SCLK** | **GPIO 11** | SPI Clock |
| **LCD_DC** | **GPIO 5** | Data / Command Selection |
| **LCD_CS** | **GPIO 40** (or GPIO 48) | SPI Chip Select (Active Low) |
| **LCD_RST** | **GPIO 41** (or GPIO 9) | Display Reset (Active Low) |

### B. Audio Subsystem (ES8311 DAC + ES7210 ADC)
| Signal Name | ESP32-S3 GPIO | Description |
| :--- | :--- | :--- |
| **I2S_MCLK** | **GPIO 16** | Master Clock for Audio Codecs |
| **I2S_BCLK** | **GPIO 9** | Bit Clock |
| **I2S_WS / LRCK** | **GPIO 45** | Word Select (Left/Right Clock) |
| **I2S_DIN (Mic In)** | **GPIO 10** | Audio In from ES7210 ADC (Dual Mic Array) |
| **I2S_DOUT (Spk Out)**| **GPIO 8** | Audio Out to ES8311 DAC |
| **PA_EN / SPK_EN** | **GPIO 46** | Speaker Power Amplifier Enable |

### C. Shared I2C Bus (0x18, 0x40, 0x51, 0x70)
| Signal Name | ESP32-S3 GPIO | Connected Peripherals & I2C Addresses |
| :--- | :--- | :--- |
| **I2C_SDA** | **GPIO 13** | • **0x18**: ES8311 Audio DAC<br>• **0x40**: ES7210 Audio ADC<br>• **0x51**: PCF85063A RTC<br>• **0x70**: SHTC3 Temp/Humidity Sensor |
| **I2C_SCL** | **GPIO 14** | Clock for all onboard I2C slave devices |
| **RTC_INT** | **GPIO 15** | PCF85063A Alarm / Timer Interrupt (Active Low) |

### D. MicroSD (TF Card) Interface
| Signal Name | ESP32-S3 GPIO | Description |
| :--- | :--- | :--- |
| **SD_CLK** | **GPIO 39** | Clock |
| **SD_CMD / MOSI** | **GPIO 38** | Command / Data In |
| **SD_DAT0 / MISO**| **GPIO 47** | Data Out |

### E. Buttons & Battery Monitoring
| Signal Name | ESP32-S3 GPIO / ADC | Description |
| :--- | :--- | :--- |
| **BOOT_BTN** | **GPIO 0** | Boot Mode / User Button 0 (Active Low, Pull-Up) |
| **KEY_BTN** | **GPIO 18** | User Programmable Button 1 (Active Low, Pull-Up) |
| **VBAT_SENSE** | **GPIO 4 (ADC1_CH3)** | Battery Voltage Divider (200kΩ top / 100kΩ bottom, 1/3 ratio) |

---

## 4. Directory Contents in `apex-dash/refs/`

```text
apex-dash/refs/
├── Makefile                                   # Makefile to download/populate/clean datasheets
├── README.md                                  # This master documentation file
├── datasheets/                                # Component datasheets & CAD files (populated via `make`)
│   ├── .gitignore                             # Ignores large binary PDFs/RARs from git
│   ├── ESP32-S3-RLCD-4.2-schematic.pdf        # Complete schematic diagram (PDF)
│   ├── ESP32-S3-RLCD-4.2-3dFile.rar           # 3D CAD step/stl model files
│   ├── ESP32-S3_Datasheet.pdf                 # Espressif ESP32-S3 SoC datasheet
│   ├── ESP32-S3_Technical_Reference_Manual.pdf # Espressif 1500+ page reference manual
│   ├── ST7305_LCD_Controller_Datasheet.pdf   # Sitronix ST7305 display driver datasheet
│   ├── ES8311_Audio_DAC_Datasheet.pdf         # Everest Semi ES8311 DAC datasheet
│   ├── ES7210_Audio_ADC_ProductBrief.pdf      # Everest Semi ES7210 ADC product brief
│   ├── PCF85063A_RTC_Datasheet.pdf            # NXP PCF85063A RTC datasheet
│   └── SHTC3_Humidity_Temperature_Sensor_Datasheet.pdf # Sensirion SHTC3 sensor datasheet
├── waveshare-wiki/                            # Full offline Waveshare Wiki documentation
│   ├── 01_overview.md                         # Product overview & features
│   ├── 02_arduino_guide.md                    # Arduino IDE configuration & examples
│   ├── 03_esp_idf_guide.md                    # ESP-IDF setup, building & flashing
│   ├── 05_faq.md                              # FAQ & troubleshooting
│   ├── 06_resources.md                        # Official resource index & links
│   └── 07_xiaozhi_ai.md                       # XiaoZhi AI voice assistant guide
├── images/                                    # 47 offline diagrams, pinouts, and hardware photos
├── zephyr-board-doc/                          # Zephyr RTOS support
│   ├── esp32s3_rlcd_4_2_esp32s3_procpu.dts    # Zephyr DeviceTree source with full hardware bindings
│   ├── board.yml & Kconfig                    # Zephyr board configuration files
│   └── index.rst                              # Zephyr board documentation source
└── official-code-examples/                    # Cloned official Waveshare repository
    ├── 01_Arduino_Libraries/                  # Pre-configured Arduino libraries (LVGL, ST7305, etc.)
    ├── 02_Example/
    │   ├── Arduino/                           # 10 Arduino sketch demos (WiFi, ADC, RTC, SHTC3, Audio, LVGL v8/v9, U8g2)
    │   ├── ESP-IDF/                           # ESP-IDF CMake projects (Audio, Sensors, Factory test)
    │   ├── ESPHome/                           # ESPHome YAML configs
    │   └── XiaoZhi/                           # Full XiaoZhi AI voice assistant firmware source
    └── 03_Firmware/                           # Precompiled factory test and demo binaries
```

### Populating Datasheets via Makefile

To download all datasheets on a fresh clone without storing large PDFs in the Git repository:

```bash
cd apex-dash/refs
make             # Downloads all missing datasheets into datasheets/
make check       # Verifies that all datasheets exist and are valid
make clean       # Removes downloaded datasheets
```

---

## 5. Quick Development Guide

### A. Arduino IDE Setup
1. **Board Package:** Install **ESP32 by Espressif Systems** (version `v3.3.0` or higher recommended).
2. **Board Selection:** Select `ESP32S3 Dev Module`.
3. **IDE Settings:**
   * **Flash Size:** `16MB (128Mb)`
   * **Partition Scheme:** `16M Flash (3MB APP/9.9MB FATFS)` or `Custom`
   * **PSRAM:** `OPI PSRAM`
   * **USB CDC On Boot:** `Enabled`
   * **USB Mode:** `Hardware CDC and JTAG`
   * **Upload Mode:** `UART0 / Hardware CDC`

### B. ESP-IDF Setup
1. Use **ESP-IDF v5.1** or newer (v5.3+ recommended for latest ST7305/audio driver support).
2. Set target: `idf.py set-target esp32s3`
3. Flash configuration: 16MB Octal/Quad Flash, 8MB Octal PSRAM (`CONFIG_SPIRAM_MODE_OCT=y`).
4. Build and flash: `idf.py build flash monitor`

---

## 6. Community & Third-Party References

* **Official Waveshare Wiki:** [docs.waveshare.com/ESP32-S3-RLCD-4.2](https://docs.waveshare.com/ESP32-S3-RLCD-4.2)
* **Waveshare GitHub Repository:** [github.com/waveshareteam/ESP32-S3-RLCD-4.2](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2)
* **Zephyr Project Board Support:** [docs.zephyrproject.org/latest/boards/waveshare/esp32s3_rlcd_4_2](https://docs.zephyrproject.org/latest/boards/waveshare/esp32s3_rlcd_4_2/doc/index.html)
* **Volos Projects (eBike & UI Demos):** [github.com/VolosR/waveshareLRCL](https://github.com/VolosR/waveshareLRCL)
* **SolarOS / CyberDeck:** [github.com/nilseuropa/solar_os](https://github.com/nilseuropa/solar_os)
* **TRMNL Client for RLCD 4.2:** [github.com/la-lo-go/trmnl-waveshare-esp32-s3-rlcd-4.2](https://github.com/la-lo-go/trmnl-waveshare-esp32-s3-rlcd-4.2)
