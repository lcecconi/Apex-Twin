# Apex-Dash: Dashboard Connector & Pinout Specification

> **Board Model:** Waveshare ESP32-S3-RLCD-4.2  
> **MCU:** Espressif ESP32-S3-WROOM-1-N16R8 (Xtensa Dual-Core LX7 @ 240 MHz)  
> **Display:** 4.2" Sunlight-Readable Reflective LCD (Sitronix ST7305, 400×300)

This document maps all pinouts, expansion headers, onboard buses, and steering-wheel wiring interfaces for the **Apex-Dash** module.

---

## 1. External Steering-Wheel Expansion Connector Pinout

The expansion connector / header on the Waveshare board exposes GPIOs for steering wheel harnesses (RGB shift lights, backlight driver, external thumb buttons, and power):

| Pin # | Signal Name | ESP32-S3 GPIO | Type / Mode | Voltage Level | Default Function in Apex-Dash | Description & Wiring Guidelines |
| :---: | :--- | :---: | :---: | :---: | :--- | :--- |
| **1** | `+5V_VBUS` | — | Power Out / In | 5.0 V | **Main 5V Power** | USB 5V or external 5V regulated input from steering wheel battery pack. |
| **2** | `+3V3` | — | Power Out | 3.3 V | **3.3V Rail** | Regulated onboard 3.3V rail (Max 500 mA external draw). |
| **3** | `GND` | — | Ground | 0 V | **System Ground** | Common ground reference for LEDs, buttons, and sensors. |
| **4** | `RGB_LED_DATA` | **GPIO 1** | Digital Out (RMT) | 3.3 V (5V tolerant DIN) | **WS2812 Shift & Alarm LEDs** | Data line for 7-LED external RGB strip (5 Shift LEDs + 2 Alarm LEDs). |
| **5** | `BACKLIGHT_PWM`| **GPIO 2** | Digital Out (LEDC) | 3.3 V | **Backlight PWM Signal** | 5 kHz PWM signal (0–100% duty cycle) feeding external LED backlight driver. |
| **6** | `AUX_BTN_LEFT` | **GPIO 3** | Digital In (Pull-Up) | 3.3 V | **Aux Left Button** | Optional steering wheel thumb button (Active LOW to GND). |
| **7** | `AUX_BTN_RIGHT`| **GPIO 17** | Digital In (Pull-Up) | 3.3 V | **Aux Right Button** | Optional steering wheel thumb button (Active LOW to GND). |
| **8** | `I2C_SDA` | **GPIO 13** | I2C Data (Open-Drain)| 3.3 V | **External I2C Bus** | Shared I2C data bus (pull-ups on board). |
| **9** | `I2C_SCL` | **GPIO 14** | I2C Clock | 3.3 V | **External I2C Bus** | Shared I2C clock (400 kHz). |
| **10**| `UART_TX` | **GPIO 43** | UART Out | 3.3 V | **Serial TX / Telemetry** | Debug serial telemetry output (115200 baud). |
| **11**| `UART_RX` | **GPIO 44** | UART In | 3.3 V | **Serial RX** | Serial command input. |
| **12**| `GND` | — | Ground | 0 V | **System Ground** | Ground return line for aux buttons and harness shielding. |

---

## 2. Onboard Subsystems & Internal Peripheral Pinout

### A. 4.2" Reflective LCD Display Interface (Sitronix ST7305 — SPI)
| Signal Name | ESP32-S3 GPIO | SPI Function | Description |
| :--- | :---: | :--- | :--- |
| `LCD_SCK` | **GPIO 11** | SPI Clock | Hardware SPI Clock (24 MHz) |
| `LCD_MOSI` | **GPIO 12** | SPI Master Out | Pixel buffer / command data |
| `LCD_DC` | **GPIO 5** | Data / Command | Low = Command register, High = Data buffer |
| `LCD_CS` | **GPIO 40** | Chip Select | Active LOW chip select |
| `LCD_RST` | **GPIO 41** | Hardware Reset | Active LOW display reset line |

---

### B. MicroSD (TF Card) Storage Interface (SDMMC 1-Bit Mode)
| Signal Name | ESP32-S3 GPIO | SDMMC Function | Description |
| :--- | :---: | :--- | :--- |
| `SD_CLK` | **GPIO 39** | Clock | SDMMC Clock line |
| `SD_CMD` | **GPIO 38** | Command / MOSI | SDMMC Command & Data In |
| `SD_DAT0` | **GPIO 47** | Data 0 / MISO | SDMMC Data line 0 (1-Bit fast mode) |

---

### C. Shared Onboard I2C Bus (400 kHz)
* **SDA:** GPIO 13
* **SCL:** GPIO 14

| I2C Address | Peripheral Device | Description |
| :---: | :--- | :--- |
| **`0x70`** | **Sensirion SHTC3** | High-precision ambient temperature & relative humidity sensor. |
| **`0x51`** | **NXP PCF85063A** | Low-power real-time clock (RTC) with rechargeable battery backup. |
| **`0x18`** | **Everest Semi ES8311** | Low-power mono audio DAC (speaker amplifier). |
| **`0x40`** | **Everest Semi ES7210** | 4-channel audio ADC (dual microphone array). |

---

### D. User Buttons & Power Monitoring
| Signal Name | ESP32-S3 Pin | Type | Logic / Circuit | Description |
| :--- | :---: | :---: | :--- | :--- |
| `BOOT_BTN` | **GPIO 0** | Digital In | Active LOW (Internal Pull-Up) | Short press: Scroll / Back. Long press: Open / Exit Menu. |
| `KEY_BTN` | **GPIO 18** | Digital In | Active LOW (Internal Pull-Up) | Short press: Next page. Long press: Invert display / Confirm. |
| `VBAT_SENSE` | **GPIO 4** | Analog In | ADC1_CH3 (1/3 divider: 200kΩ / 100kΩ) | Monitors 18650 / LiPo battery voltage (3.0 V – 4.2 V). |
| `RTC_INT` | **GPIO 15** | Digital In | Active LOW | PCF85063A programmable timer & alarm interrupt. |

---

### E. Audio Subsystem Interface (I2S Interface)
| Signal Name | ESP32-S3 GPIO | Audio Bus | Description |
| :--- | :---: | :--- | :--- |
| `I2S_MCLK` | **GPIO 16** | Master Clock | Codec system master clock |
| `I2S_BCLK` | **GPIO 9** | Bit Clock | Audio data bit clock |
| `I2S_WS` | **GPIO 45** | Word Select | Left/Right channel clock |
| `I2S_DIN` | **GPIO 10** | Audio In | Microphone ADC input (ES7210) |
| `I2S_DOUT` | **GPIO 8** | Audio Out | Speaker DAC output (ES8311) |
| `SPK_EN` | **GPIO 46** | Digital Out | Speaker Power Amplifier enable (Active HIGH) |

---

## 3. Steering Wheel Harness Wiring Diagram

```text
       ┌─────────────────────────────────────────────────────────────┐
       │             WAVESHARE ESP32-S3-RLCD CONNECTOR               │
       │                                                             │
       │  [Pin 1: +5V/VBUS] ──────> 5V Power (WS2812 Strip & Driver) │
       │  [Pin 2: +3.3V]    ──────> 3.3V Logic Supply                │
       │  [Pin 3: GND]      ──────> Common Ground                    │
       │                                                             │
       │  [Pin 4: GPIO 1]   ──────> DIN ──[ 7x WS2812B RGB Strip ]   │
       │                                   • LEDs 0..4: Shift Ladder │
       │                                   • LED 5: Left Alarm (H2O) │
       │                                   • LED 6: Right Alarm(EGT) │
       │                                                             │
       │  [Pin 5: GPIO 2]   ──────> PWM In [ Backlight LED Driver ]  │
       │                                                             │
       │  [Pin 6: GPIO 3]   ──────> [ Aux Left Wheel Button ] ── GND │
       │  [Pin 7: GPIO 17]  ──────> [ Aux Right Wheel Button] ── GND │
       │                                                             │
       │  [Pin 8: GPIO 13]  ──────> SDA (External I2C Sensors)       │
       │  [Pin 9: GPIO 14]  ──────> SCL (External I2C Sensors)       │
       │  [Pin 12: GND]     ──────> Shielding / Harness Ground       │
       └─────────────────────────────────────────────────────────────┘
```

---

## 4. Software Pin Mapping Reference in `config.h`

```c
// apex-dash/include/config.h

// WS2812 RGB LED Strip
#define PIN_RGB_LED_STRIP       1   // 7 LEDs (5 Shift + 2 Alarm)
#define NUM_SHIFT_LEDS          5
#define NUM_ALARM_LEDS          2
#define NUM_TOTAL_LEDS          7

// PWM Backlight Output
#define PIN_BACKLIGHT_PWM       2   // 5 kHz LEDC PWM
#define LEDC_BACKLIGHT_CH       0
#define LEDC_BACKLIGHT_FREQ     5000
#define LEDC_BACKLIGHT_RES      8   // 8-bit resolution (0-255)

// MicroSD Slot
#define PIN_SD_CLK              39
#define PIN_SD_CMD              38
#define PIN_SD_DAT0             47

// Onboard Controls & Sensors
#define PIN_BTN_BOOT            0
#define PIN_BTN_KEY             18
#define PIN_VBAT_ADC            4
#define PIN_I2C_SDA             13
#define PIN_I2C_SCL             14
```
