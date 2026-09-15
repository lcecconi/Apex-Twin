#pragma once

#include <Arduino.h>

// ==========================================
// Hardware Pinout: Waveshare ESP32-S3-RLCD-4.2
// ==========================================

// 4.2" Reflective LCD (Sitronix ST7305 - SPI)
#define PIN_LCD_SCK       11
#define PIN_LCD_MOSI      12
#define PIN_LCD_DC        5
#define PIN_LCD_CS        40
#define PIN_LCD_RST       41
#define LCD_WIDTH         400
#define LCD_HEIGHT        300

// I2C Shared Bus (SHTC3: 0x70, PCF85063: 0x51, ES8311: 0x18, ES7210: 0x40)
#define PIN_I2C_SDA       13
#define PIN_I2C_SCL       14
#define I2C_BUS_SPEED     400000

// MicroSD (TF Card Slot) - SDMMC 1-Bit Mode
#define PIN_SD_CLK        39
#define PIN_SD_CMD        38
#define PIN_SD_DAT0       47

// Power Sensing
#define PIN_VBAT_ADC      4   // ADC1_CH3, 200k/100k (1/3) divider

// Onboard User Controls
#define PIN_BTN_BOOT      0   // Active Low, internal pull-up
#define PIN_BTN_KEY       18  // Active Low, internal pull-up

// ==========================================
// Expansion Header Interfaces (Steering Wheel Harness)
// ==========================================
#define PIN_RGB_LED_STRIP       1   // WS2812B NeoPixel Data line (7 LEDs total)
#define NUM_SHIFT_LEDS          5   // LEDs 0..4: Progressive RPM shift bar
#define NUM_ALARM_LEDS          2   // LED 5: Left Alarm, LED 6: Right Alarm
#define NUM_TOTAL_LEDS          7

#define PIN_BACKLIGHT_PWM       2   // PWM output to external backlight LED driver
#define LEDC_BACKLIGHT_CH       0
#define LEDC_BACKLIGHT_FREQ     5000
#define LEDC_BACKLIGHT_RES      8   // 8-bit (0-255)

// Optional Steering Wheel Controls (Aux GPIOs on header)
#define PIN_AUX_BTN_LEFT        3
#define PIN_AUX_BTN_RIGHT       17

// ==========================================
// System & UI Defaults
// ==========================================
#define DEFAULT_MAX_RPM             16000
#define DEFAULT_SHIFT_RPM           14200
#define DEFAULT_OVER_REV_RPM        15500
#define DEFAULT_WATER_TEMP_ALARM_C  65.0f
#define DEFAULT_EGT_ALARM_C         640.0f
#define DEFAULT_SPEED_UNIT_KMH      true
#define DEFAULT_TEMP_UNIT_CELSIUS   true
#define DEFAULT_INVERTED_DISPLAY    true  // High-contrast Black on Silver
#define DEFAULT_LED_BRIGHTNESS      80    // 0-100%
#define DEFAULT_BACKLIGHT_PERCENT   0     // 0-100% (Default 0% for pure reflective mode)
