#include "led_strip_manager.h"

LEDStripManager::LEDStripManager()
  : _strip(NUM_TOTAL_LEDS, PIN_RGB_LED_STRIP, NEO_GRB + NEO_KHZ800) {}

void LEDStripManager::begin() {
  _strip.begin();
  _strip.setBrightness((_brightness * 255) / 100);
  clear();
}

void LEDStripManager::setBrightness(uint8_t brightness_pct) {
  _brightness = constrain(brightness_pct, (uint8_t)0, (uint8_t)100);
  _strip.setBrightness((_brightness * 255) / 100);
}

void LEDStripManager::clear() {
  _strip.clear();
  _strip.show();
}

void LEDStripManager::runTestPattern(uint32_t duration_ms) {
  _in_test_mode = true;
  _test_pattern_start_ms = millis();
}

void LEDStripManager::update(const TelemetrySnapshot &telemetry, const SystemSettings &settings) {
  uint32_t now = millis();

  // Test Pattern Mode
  if (_in_test_mode) {
    if (now - _test_pattern_start_ms > 2000) {
      _in_test_mode = false;
      clear();
    } else {
      // Running rainbow chase across all 7 LEDs
      uint16_t hue = ((now - _test_pattern_start_ms) * 65536 / 2000) % 65536;
      for (int i = 0; i < NUM_TOTAL_LEDS; i++) {
        uint32_t color = _strip.gamma32(_strip.ColorHSV(hue + (i * 65536 / NUM_TOTAL_LEDS), 255, 255));
        _strip.setPixelColor(i, color);
      }
      _strip.show();
      return;
    }
  }

  // Check strobe cadence (50ms toggle for fast flash)
  if (now - _last_strobe_ms >= 60) {
    _strobe_state = !_strobe_state;
    _last_strobe_ms = now;
  }

  // 1. Shift Lights (LEDs 0..4)
  if (settings.led_shift_enable) {
    uint16_t shift_rpm = settings.shift_rpm;
    uint16_t rpm = telemetry.rpm;

    if (rpm >= shift_rpm) {
      // Shift Point Strobe: Flash all 5 shift LEDs in brilliant Blue/White
      uint32_t strobe_color = _strobe_state ? _strip.Color(0, 150, 255) : _strip.Color(0, 0, 0);
      for (int i = 0; i < NUM_SHIFT_LEDS; i++) {
        _strip.setPixelColor(i, strobe_color);
      }
    } else {
      // Progressive Shift Ladder: 5 LEDs
      // Step size = 1600 / 4 = 400 RPM steps before shift point
      uint16_t rpm_start = (shift_rpm > 1600) ? (shift_rpm - 1600) : 0;
      uint16_t step = (shift_rpm - rpm_start) / 4;
      if (step == 0) step = 1;

      // LED 0: Green
      _strip.setPixelColor(0, (rpm >= rpm_start) ? _strip.Color(0, 255, 0) : 0);
      // LED 1: Green
      _strip.setPixelColor(1, (rpm >= rpm_start + step) ? _strip.Color(0, 255, 0) : 0);
      // LED 2: Yellow / Amber
      _strip.setPixelColor(2, (rpm >= rpm_start + step * 2) ? _strip.Color(255, 200, 0) : 0);
      // LED 3: Yellow / Orange
      _strip.setPixelColor(3, (rpm >= rpm_start + step * 3) ? _strip.Color(255, 120, 0) : 0);
      // LED 4: Red
      _strip.setPixelColor(4, (rpm >= shift_rpm - 50) ? _strip.Color(255, 0, 0) : 0);
    }
  } else {
    for (int i = 0; i < NUM_SHIFT_LEDS; i++) {
      _strip.setPixelColor(i, 0);
    }
  }

  // 2. Alarm Lights (LED 5 = Left Alarm, LED 6 = Right Alarm)
  if (settings.led_alarm_enable) {
    // Left Alarm: Water Overheat (> threshold) or Low Battery (< 3.4V)
    if (telemetry.water_temp_c >= settings.water_temp_alarm_c && settings.water_temp_alarm_c > 0) {
      // Fast Red Flash for engine overheat
      _strip.setPixelColor(5, _strobe_state ? _strip.Color(255, 0, 0) : 0);
    } else if (telemetry.battery_voltage < settings.low_bat_alarm_v && telemetry.battery_voltage > 1.0f) {
      // Slow Amber warning for low battery
      _strip.setPixelColor(5, _strip.Color(255, 100, 0));
    } else {
      _strip.setPixelColor(5, 0);
    }

    // Right Alarm: EGT (> threshold) or Over-Rev (> threshold)
    if (telemetry.exhaust_temp_c >= settings.exhaust_temp_alarm_c && settings.exhaust_temp_alarm_c > 0) {
      // Fast Purple / Magenta flash for high EGT
      _strip.setPixelColor(6, _strobe_state ? _strip.Color(255, 0, 200) : 0);
    } else if (telemetry.rpm >= settings.over_rev_rpm && settings.over_rev_rpm > 0) {
      // Red / White strobe for over-rev
      _strip.setPixelColor(6, _strobe_state ? _strip.Color(255, 255, 255) : _strip.Color(255, 0, 0));
    } else {
      _strip.setPixelColor(6, 0);
    }
  } else {
    _strip.setPixelColor(5, 0);
    _strip.setPixelColor(6, 0);
  }

  _strip.show();
}
