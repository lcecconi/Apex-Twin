#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "telemetry_data.h"
#include "telemetry_provider.h"

class PageDataRecall {
public:
  void render(U8G2 *u8g2, const TelemetryProvider &provider, const SystemSettings &settings);
};
