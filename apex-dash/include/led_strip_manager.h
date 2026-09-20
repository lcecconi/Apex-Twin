#pragma once

#include <cstdint>
#include "config.h"
#include "telemetry_data.h"
#include "led_strip.h"

class LEDStripManager {
public:
  LEDStripManager();
  ~LEDStripManager();
  void begin();
  void update(const TelemetrySnapshot &telemetry, const SystemSettings &settings);
  void runTestPattern(uint32_t duration_ms = 2000);
  void setBrightness(uint8_t brightness_pct); // 0-100%
  void clear();

private:
  led_strip_handle_t _led_strip = nullptr;
  uint8_t _brightness = 80;
  uint32_t _last_strobe_ms = 0;
  bool _strobe_state = false;
  uint32_t _test_pattern_start_ms = 0;
  bool _in_test_mode = false;

  void _set_pixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b);
};
