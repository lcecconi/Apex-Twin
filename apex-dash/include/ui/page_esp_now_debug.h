#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "telemetry_data.h"
#include "esp_now_receiver.h"

class PageEspNowDebug {
public:
  void render(U8G2 *u8g2, const TelemetrySnapshot &telemetry,
              const EspNowDebugStats &stats);
};
