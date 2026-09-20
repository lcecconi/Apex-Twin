#include "led_strip_manager.h"
#include "esp_timer.h"
#include "led_strip_rmt.h"

LEDStripManager::LEDStripManager() {}

LEDStripManager::~LEDStripManager() {
  if (_led_strip) {
    led_strip_del(_led_strip);
    _led_strip = nullptr;
  }
}

void LEDStripManager::begin() {
  led_strip_config_t strip_config = {};
  strip_config.strip_gpio_num = PIN_RGB_LED_STRIP;
  strip_config.max_leds = NUM_TOTAL_LEDS;
  strip_config.led_pixel_format = LED_PIXEL_FORMAT_GRB;
  strip_config.led_model = LED_MODEL_WS2812;

  led_strip_rmt_config_t rmt_config = {};
  rmt_config.resolution_hz = 10 * 1000 * 1000;
  rmt_config.flags.with_dma = false;

  led_strip_new_rmt_device(&strip_config, &rmt_config, &_led_strip);
  clear();
}

void LEDStripManager::_set_pixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (!_led_strip || index >= NUM_TOTAL_LEDS) return;
  uint32_t cr = ((uint32_t)r * _brightness) / 100;
  uint32_t cg = ((uint32_t)g * _brightness) / 100;
  uint32_t cb = ((uint32_t)b * _brightness) / 100;
  led_strip_set_pixel(_led_strip, index, cr, cg, cb);
}

void LEDStripManager::setBrightness(uint8_t brightness_pct) {
  _brightness = (brightness_pct > 100) ? 100 : brightness_pct;
}

void LEDStripManager::clear() {
  if (!_led_strip) return;
  led_strip_clear(_led_strip);
}

void LEDStripManager::runTestPattern(uint32_t duration_ms) {
  _in_test_mode = true;
  _test_pattern_start_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void LEDStripManager::update(const TelemetrySnapshot &telemetry, const SystemSettings &settings) {
  if (!_led_strip) return;
  uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);

  // Test Pattern Mode
  if (_in_test_mode) {
    if (now - _test_pattern_start_ms > 2000) {
      _in_test_mode = false;
      clear();
    } else {
      for (int i = 0; i < NUM_TOTAL_LEDS; i++) {
        _set_pixel(i, 0, 150, 255);
      }
      led_strip_refresh(_led_strip);
      return;
    }
  }

  // Check strobe cadence (50ms toggle for fast flash)
  if (now - _last_strobe_ms >= 60) {
    _strobe_state = !_strobe_state;
    _last_strobe_ms = now;
  }

  // 1. Shift Lights (Progressive RPM ladder mapped across NUM_SHIFT_LEDS)
  if (settings.led_shift_enable && settings.rpm_display_mode != RPM_DISP_DISPLAY_ONLY) {
    uint16_t shift_rpm = settings.shift_rpm;
    uint16_t rpm = telemetry.rpm;

    if (rpm >= shift_rpm && shift_rpm > 0) {
      // Shift Point Strobe: Flash all shift LEDs in brilliant Blue/White
      for (int i = 0; i < NUM_SHIFT_LEDS; i++) {
        if (_strobe_state) {
          _set_pixel(i, 0, 150, 255);
        } else {
          _set_pixel(i, 0, 0, 0);
        }
      }
    } else if (rpm > 100) {
      // Linear mapping: First LED lights up at > 100 RPM, then linearly up to max_rpm
      uint32_t max_rpm = (settings.max_rpm > 100) ? settings.max_rpm : DEFAULT_MAX_RPM;
      uint32_t span_rpm = max_rpm - 100;
      uint32_t curr_rpm = (rpm > max_rpm) ? span_rpm : (rpm - 100);
      int active_leds = 1 + (int)((curr_rpm * (NUM_SHIFT_LEDS - 1)) / span_rpm);
      if (active_leds > NUM_SHIFT_LEDS) active_leds = NUM_SHIFT_LEDS;

      // Color stage boundaries (for 16 LEDs: 6 Green, 5 Yellow/Amber, 5 Red)
      int green_count = (NUM_SHIFT_LEDS * 6) / 16;
      int yellow_count = (NUM_SHIFT_LEDS * 5) / 16;
      if (green_count < 1) green_count = 1;
      if (yellow_count < 1) yellow_count = 1;

      for (int i = 0; i < NUM_SHIFT_LEDS; i++) {
        if (i < active_leds) {
          if (i < green_count) {
            _set_pixel(i, 0, 255, 0);       // Green
          } else if (i < green_count + yellow_count) {
            _set_pixel(i, 255, 200, 0);     // Yellow / Amber
          } else {
            _set_pixel(i, 255, 0, 0);       // Red
          }
        } else {
          _set_pixel(i, 0, 0, 0);
        }
      }
    } else {
      for (int i = 0; i < NUM_SHIFT_LEDS; i++) {
        _set_pixel(i, 0, 0, 0);
      }
    }
  } else {
    for (int i = 0; i < NUM_SHIFT_LEDS; i++) {
      _set_pixel(i, 0, 0, 0);
    }
  }

#if NUM_ALARM_LEDS > 0
  // 2. Alarm Lights (LED 5 = Left Alarm, LED 6 = Right Alarm)
  if (settings.led_alarm_enable) {
    // Left Alarm: Water Overheat (> threshold) or Low Battery (< 3.4V)
    if (telemetry.water_temp_c >= settings.water_temp_alarm_c && settings.water_temp_alarm_c > 0) {
      if (_strobe_state) _set_pixel(5, 255, 0, 0); else _set_pixel(5, 0, 0, 0);
    } else if (telemetry.battery_voltage < settings.low_bat_alarm_v && telemetry.battery_voltage > 1.0f) {
      _set_pixel(5, 255, 100, 0);
    } else {
      _set_pixel(5, 0, 0, 0);
    }

    // Right Alarm: EGT (> threshold) or Over-Rev (> threshold)
    if (telemetry.exhaust_temp_c >= settings.exhaust_temp_alarm_c && settings.exhaust_temp_alarm_c > 0) {
      if (_strobe_state) _set_pixel(6, 255, 0, 200); else _set_pixel(6, 0, 0, 0);
    } else if (telemetry.rpm >= settings.over_rev_rpm && settings.over_rev_rpm > 0) {
      if (_strobe_state) _set_pixel(6, 255, 255, 255); else _set_pixel(6, 255, 0, 0);
    } else {
      _set_pixel(6, 0, 0, 0);
    }
  } else {
    _set_pixel(5, 0, 0, 0);
    _set_pixel(6, 0, 0, 0);
  }
#endif

  led_strip_refresh(_led_strip);
}
