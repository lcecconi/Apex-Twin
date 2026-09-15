#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

struct DeviceSensorsData {
  bool shtc3_found = false;
  float ambient_temp_c = 0.0f;
  float ambient_humidity_pct = 0.0f;

  bool rtc_found = false;
  uint16_t year = 2026;
  uint8_t month = 9;
  uint8_t day = 15;
  uint8_t hour = 12;
  uint8_t minute = 0;
  uint8_t second = 0;

  float battery_voltage = 0.0f;
  uint8_t battery_percent = 0;

  uint32_t free_heap_kb = 0;
  uint32_t free_psram_kb = 0;
  uint32_t total_psram_kb = 0;
};

class OnboardSensors {
public:
  void begin();
  void update();
  const DeviceSensorsData &getData() const { return _data; }

  void setRtcTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

private:
  DeviceSensorsData _data;

  bool initSHTC3();
  bool readSHTC3(float &temp, float &humidity);

  bool initRTC();
  bool readRTC(uint16_t &year, uint8_t &month, uint8_t &day, uint8_t &hour, uint8_t &minute, uint8_t &second);

  void readBattery(float &voltage, uint8_t &percent);
};
