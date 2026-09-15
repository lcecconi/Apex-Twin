#pragma once

#include <Arduino.h>
#include <Wire.h>

// GPIO definitions for Waveshare ESP32-S3-RLCD-4.2
#define PIN_I2C_SDA     13
#define PIN_I2C_SCL     14
#define PIN_VBAT_ADC    4
#define PIN_BTN_BOOT    0
#define PIN_BTN_KEY     18

struct SensorData {
  // SHTC3 Temp/Humidity Sensor (0x70)
  bool shtc3_found = false;
  float temperature = 0.0f; // Celsius
  float humidity = 0.0f;    // % RH

  // PCF85063A Real-Time Clock (0x51)
  bool rtc_found = false;
  uint16_t year = 2026;
  uint8_t month = 9;
  uint8_t day = 15;
  uint8_t hour = 12;
  uint8_t minute = 0;
  uint8_t second = 0;

  // Battery ADC (GPIO 4)
  float battery_voltage = 0.0f; // Volts
  uint8_t battery_percent = 0;  // 0 - 100%

  // Buttons
  bool boot_btn_pressed = false; // GPIO 0
  bool key_btn_pressed = false;  // GPIO 18

  // System metrics
  uint32_t free_heap_kb = 0;
  uint32_t free_psram_kb = 0;
  uint32_t total_psram_kb = 0;
  uint32_t cpu_freq_mhz = 240;
  uint32_t uptime_sec = 0;
};

class OnboardSensors {
public:
  void begin();
  void update();
  const SensorData &getData() const { return _data; }

  void setRtcTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

private:
  SensorData _data;

  bool initSHTC3();
  bool readSHTC3(float &temp, float &humidity);

  bool initRTC();
  bool readRTC(uint16_t &year, uint8_t &month, uint8_t &day, uint8_t &hour, uint8_t &minute, uint8_t &second);

  void readBattery(float &voltage, uint8_t &percent);
};
