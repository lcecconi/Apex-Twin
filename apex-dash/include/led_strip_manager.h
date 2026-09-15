#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "telemetry_data.h"

class LEDStripManager {
public:
  LEDStripManager();
  void begin();
  void update(const TelemetrySnapshot &telemetry, const SystemSettings &settings);
  void runTestPattern(uint32_t duration_ms = 2000);
  void setBrightness(uint8_t brightness_pct); // 0-100%
  void clear();

private:
  Adafruit_NeoPixel _strip;
  uint8_t _brightness = 80;
  uint32_t _last_strobe_ms = 0;
  bool _strobe_state = false;
  uint32_t _test_pattern_start_ms = 0;
  bool _in_test_mode = false;
};
