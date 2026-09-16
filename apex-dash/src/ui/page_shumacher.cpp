#include "ui/page_shumacher.h"
#include <math.h>

void PageShumacher::updateSpeedTracking(float speed, float lon_g, float lat_g) {
  _current_speed = speed;
  if (_tracking_min > 900.0f) _tracking_min = speed;
  if (_tracking_max < 1.0f) _tracking_max = speed;

  // Corner entry detection: lateral G > 0.6G or braking lon_g < -0.3G
  if (fabsf(lat_g) > 0.6f || lon_g < -0.3f) {
    if (!_in_corner) {
      // Transition from straight to corner: straight peak is locked
      if (_tracking_max > 20.0f) {
        _held_vmax = _tracking_max;
      }
      _tracking_max = speed;
      _tracking_min = speed;
      _in_corner = true;
      _in_straight = false;
    }
    if (speed < _tracking_min) {
      _tracking_min = speed;
    }
  } else if (lon_g > 0.15f || (speed > _tracking_min + 4.0f && fabsf(lat_g) < 0.4f)) {
    // Transition from corner to straight acceleration: corner min is locked
    if (!_in_straight) {
      if (_tracking_min < 900.0f && _tracking_min > 5.0f) {
        _held_vmin = _tracking_min;
      }
      _tracking_min = 999.0f;
      _tracking_max = speed;
      _in_straight = true;
      _in_corner = false;
    }
    if (speed > _tracking_max) {
      _tracking_max = speed;
    }
  } else {
    if (speed < _tracking_min) _tracking_min = speed;
    if (speed > _tracking_max) _tracking_max = speed;
  }
}

void PageShumacher::render(U8G2 *u8g2, const TelemetrySnapshot &telemetry, const SystemSettings &settings) {
  char buf[64];

  // Update dynamic min/max corner speed tracking
  updateSpeedTracking(telemetry.speed_kmh, telemetry.longitudinal_g, telemetry.lateral_g);

  // Unit conversions
  float unit_mult = settings.use_kmh ? 1.0f : 0.621371f;
  const char *unit_str = settings.use_kmh ? "km/h" : "mph";

  float disp_live = telemetry.speed_kmh * unit_mult;
  float disp_vmin = _held_vmin * unit_mult;
  float disp_vmax = _held_vmax * unit_mult;

  // ==========================================
  // TOP BAR: TACHOMETER & TITLE (y = 4..32)
  // ==========================================
  u8g2->drawRFrame(10, 2, 380, 14, 2);
  int rpm_fill = (int)((uint32_t)telemetry.rpm * 376 / max(1, (int)settings.max_rpm));
  if (rpm_fill > 376) rpm_fill = 376;
  if (rpm_fill > 0) {
    u8g2->drawBox(12, 4, rpm_fill, 10);
  }
  // Shift RPM marker
  int shift_x = 10 + (int)((uint32_t)settings.shift_rpm * 376 / max(1, (int)settings.max_rpm));
  if (shift_x < 390) {
    u8g2->drawVLine(shift_x, 1, 16);
  }

  u8g2->setFont(u8g2_font_helvB08_tr);
  u8g2->drawStr(10, 28, "SCHUMACHER B194 3-SPEED");
  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "LAP %02d | %5u RPM", telemetry.lap_number, telemetry.rpm);
  u8g2->drawStr(275, 28, buf);
  u8g2->drawHLine(10, 32, 380);

  // ==========================================
  // THE THREE SPEEDOMETERS (y = 36..170)
  // ==========================================

  // --- 1. LEFT: V-MIN (APEX SPEED) ---
  u8g2->drawRFrame(10, 36, 120, 134, 4);
  u8g2->drawBox(10, 36, 120, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(16, 48, "V-MIN (APEX)");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_logisoso32_tn);
  snprintf(buf, sizeof(buf), "%d", (int)roundf(disp_vmin));
  int w_vmin = u8g2->getStrWidth(buf);
  u8g2->drawStr(10 + (120 - w_vmin) / 2, 94, buf);

  u8g2->setFont(u8g2_font_helvB10_tr);
  int w_unit1 = u8g2->getStrWidth(unit_str);
  u8g2->drawStr(10 + (120 - w_unit1) / 2, 114, unit_str);

  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(16, 138, "CORNER APEX");
  u8g2->drawStr(16, 154, "HELD MIN SPEED");

  // --- 2. CENTER: LIVE SPEED (CURRENT) ---
  u8g2->drawRFrame(138, 36, 124, 134, 4);
  u8g2->drawBox(138, 36, 124, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(150, 48, "LIVE SPEED");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_logisoso32_tn);
  snprintf(buf, sizeof(buf), "%d", (int)roundf(disp_live));
  int w_live = u8g2->getStrWidth(buf);
  u8g2->drawStr(138 + (124 - w_live) / 2, 94, buf);

  u8g2->setFont(u8g2_font_helvB10_tr);
  int w_unit2 = u8g2->getStrWidth(unit_str);
  u8g2->drawStr(138 + (124 - w_unit2) / 2, 114, unit_str);

  if (settings.drive_type == DRIVE_SHIFTER_6SPEED) {
    if (telemetry.gear == 0) {
      snprintf(buf, sizeof(buf), "GEAR: N");
    } else {
      snprintf(buf, sizeof(buf), "GEAR: %d", telemetry.gear);
    }
  } else {
    snprintf(buf, sizeof(buf), "DIRECT DRIVE");
  }
  int w_gear = u8g2->getStrWidth(buf);
  u8g2->drawStr(138 + (124 - w_gear) / 2, 138, buf);

  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "%+0.2f G Lat", telemetry.lateral_g);
  int w_lat = u8g2->getStrWidth(buf);
  u8g2->drawStr(138 + (124 - w_lat) / 2, 154, buf);

  // --- 3. RIGHT: V-MAX (STRAIGHT TOP SPEED) ---
  u8g2->drawRFrame(270, 36, 120, 134, 4);
  u8g2->drawBox(270, 36, 120, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(276, 48, "V-MAX (EXIT)");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_logisoso32_tn);
  snprintf(buf, sizeof(buf), "%d", (int)roundf(disp_vmax));
  int w_vmax = u8g2->getStrWidth(buf);
  u8g2->drawStr(270 + (120 - w_vmax) / 2, 94, buf);

  u8g2->setFont(u8g2_font_helvB10_tr);
  int w_unit3 = u8g2->getStrWidth(unit_str);
  u8g2->drawStr(270 + (120 - w_unit3) / 2, 114, unit_str);

  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(276, 138, "STRAIGHT PEAK");
  u8g2->drawStr(276, 154, "HELD TOP SPEED");

  // ==========================================
  // BOTTOM SECTION: ANALYSIS & TIMING (y = 176..270)
  // ==========================================

  // --- Left Box: Corner Delta & Dynamics ---
  u8g2->drawRFrame(10, 176, 185, 94, 4);
  u8g2->drawBox(10, 176, 185, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(16, 188, "CORNER DELTA & DYNAMICS");
  u8g2->setDrawColor(1);

  float speed_gain = disp_vmax - disp_vmin;
  float apex_ratio = (disp_vmax > 0.0f) ? (disp_vmin / disp_vmax * 100.0f) : 0.0f;

  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "Speed Gain (\xce\x94V): +%.1f %s", speed_gain > 0.0f ? speed_gain : 0.0f, unit_str);
  u8g2->drawStr(16, 210, buf);

  snprintf(buf, sizeof(buf), "Apex Lat Grip:  %+.2f G", telemetry.lateral_g);
  u8g2->drawStr(16, 226, buf);

  snprintf(buf, sizeof(buf), "Entry Braking:  %+.2f G", telemetry.longitudinal_g);
  u8g2->drawStr(16, 242, buf);

  snprintf(buf, sizeof(buf), "Apex Ratio:     %.1f %%", apex_ratio);
  u8g2->drawStr(16, 258, buf);

  // --- Right Box: Lap Timing & Engine Vitals ---
  u8g2->drawRFrame(205, 176, 185, 94, 4);
  u8g2->drawBox(205, 176, 185, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(211, 188, "LAP TIMING & ENGINE");
  u8g2->setDrawColor(1);

  if (telemetry.best_lap_time_ms > 0) {
    uint32_t b_sec = (telemetry.best_lap_time_ms % 60000) / 1000;
    uint32_t b_cen = (telemetry.best_lap_time_ms % 1000) / 10;
    snprintf(buf, sizeof(buf), "Best Lap:  %02lu.%02lu s", (unsigned long)b_sec, (unsigned long)b_cen);
  } else {
    snprintf(buf, sizeof(buf), "Best Lap:  --.-- s");
  }
  u8g2->drawStr(211, 210, buf);

  if (telemetry.best_lap_time_ms > 0 || fabsf(telemetry.predictive_delta_s) > 0.001f) {
    snprintf(buf, sizeof(buf), "Lap Delta: %+0.2f s", telemetry.predictive_delta_s);
  } else {
    snprintf(buf, sizeof(buf), "Lap Delta: --.-- s");
  }
  u8g2->drawStr(211, 226, buf);

  float water = settings.use_celsius ? telemetry.water_temp_c : (telemetry.water_temp_c * 1.8f + 32.0f);
  float egt = settings.use_celsius ? telemetry.exhaust_temp_c : (telemetry.exhaust_temp_c * 1.8f + 32.0f);
  char t_unit = settings.use_celsius ? 'C' : 'F';
  snprintf(buf, sizeof(buf), "H2O: %.1f\xb0%c | EGT: %d\xb0%c", water, t_unit, (int)egt, t_unit);
  u8g2->drawStr(211, 242, buf);

  uint32_t eng_hrs = telemetry.engine_total_hours_sec / 3600;
  uint32_t eng_mins = (telemetry.engine_total_hours_sec % 3600) / 60;
  snprintf(buf, sizeof(buf), "Eng: %02luh%02lu | Bat: %d%%", (unsigned long)eng_hrs, (unsigned long)eng_mins, telemetry.battery_percent);
  u8g2->drawStr(211, 258, buf);
}
