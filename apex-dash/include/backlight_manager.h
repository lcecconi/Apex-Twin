#pragma once

#include <cstdint>
#include "config.h"

class BacklightManager {
public:
  void begin();
  void setBrightness(uint8_t percent); // 0 to 100%
  uint8_t getBrightness() const;

private:
  uint8_t _brightness_pct = DEFAULT_BACKLIGHT_PERCENT;
};
