/**
 * @file led_strip_rmt.h
 * Native ESP-IDF RMT Driver for WS2812B Shift & Alarm LEDs
 */

#pragma once

#include <cstdint>
#include "telemetry_data.h"

namespace ApexLeds {

class LedStripRmt {
public:
    void init();
    void update(const TelemetrySnapshot &telemetry, const SystemSettings &settings);
    void clear();

private:
    void setPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b);
    void refresh();

    void *_strip_handle = nullptr;
    uint32_t _flash_timer_ms = 0;
    bool _flash_state = false;
};

} // namespace ApexLeds
