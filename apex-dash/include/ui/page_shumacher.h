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

  void updateSpeedTracking(float speed, float lon_g, float lat_g);
};
