#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "telemetry_data.h"

class PageShumacher {
public:
  void render(U8G2 *u8g2, const TelemetrySnapshot &telemetry, const SystemSettings &settings);

private:
  float _current_speed = 0.0f;
  float _tracking_min = 999.0f;
  float _tracking_max = 0.0f;
  float _held_vmin = 48.0f;
  float _held_vmax = 124.0f;
  bool _in_corner = false;
  bool _in_straight = false;

  uint8_t _prev_sector = 0;
  uint16_t _prev_lap = 0;
  uint32_t _prev_best_lap = 0;
  float _prev_delta_val = 999.0f;
  uint32_t _delta_flash_start_ms = 0;

  void updateSpeedTracking(float speed, float lon_g, float lat_g);
};
