#include "ui/page_shumacher.h"
#include "ui/icons_xbm.h"
#include "i18n.h"
#include <math.h>

void PageShumacher::updateSpeedTracking(float speed, float lon_g, float lat_g, uint32_t now) {
  _current_speed = speed;

  // Detect braking or heavy cornering (going for brakes / entering corner)
  bool is_braking = (lon_g < -0.25f);
  bool is_cornering = (fabsf(lat_g) > 0.60f);

  if (is_braking || is_cornering) {
    if (!_braking_or_cornering) {
      // Driver just went for the brakes:
      // 1. Lock the maximum speed achieved on the previous straight
      if (_current_straight_max > 20.0f) {
        _held_vmax = _current_straight_max;
      }
      // 2. Reset corner minimum tracking for the new corner
      _current_corner_min = speed;
      _braking_or_cornering = true;
      _flat_throttle_start_ms = 0;
      _straight_tracking_active = false;
    }

    if (speed < _current_corner_min) {
      _current_corner_min = speed;
    }
  } else if (lon_g > 0.10f || (speed > _current_corner_min + 3.0f && fabsf(lat_g) < 0.35f)) {
    // Driver is accelerating out of the corner / on the straight:
    if (_braking_or_cornering) {
      // Corner completed: lock the apex minimum speed (held until next braking)
      if (_current_corner_min > 5.0f && _current_corner_min < 900.0f) {
        _held_vmin = _current_corner_min;
      }
      _braking_or_cornering = false;
      _flat_throttle_start_ms = now;
      _straight_tracking_active = false;
    }

    // On straight: keep holding previous straight max until flat on throttle for ~1.8s
    if (!_straight_tracking_active) {
      if (_flat_throttle_start_ms > 0 && (now - _flat_throttle_start_ms >= 1800)) {
        _straight_tracking_active = true;
        _current_straight_max = speed;
      }
    } else {
      if (speed > _current_straight_max) {
        _current_straight_max = speed;
      }
    }
  } else {
    // Neutral coasting or steady state
    if (_braking_or_cornering && speed < _current_corner_min) {
      _current_corner_min = speed;
    }
    if (_straight_tracking_active && speed > _current_straight_max) {
      _current_straight_max = speed;
    }
  }
}

void PageShumacher::render(U8G2 *u8g2, const TelemetrySnapshot &telemetry, const SystemSettings &settings) {
  char buf[48];
  uint32_t now = millis();

  // Update dynamic min/max corner speed tracking
  updateSpeedTracking(telemetry.speed_kmh, telemetry.longitudinal_g, telemetry.lateral_g, now);

  // Determine displayed speed values
  float disp_vmin_val = _braking_or_cornering ? _current_corner_min : _held_vmin;
  float disp_vmax_val = _straight_tracking_active ? _current_straight_max : _held_vmax;

  // Unit conversion
  float unit_mult = settings.use_kmh ? 1.0f : 0.621371f;
  float disp_live = telemetry.speed_kmh * unit_mult;
  float disp_vmin = disp_vmin_val * unit_mult;
  float disp_vmax = disp_vmax_val * unit_mult;

  // ==========================================
  // 1. TOP TACHOMETER (RPM BAR GRAPH)
  // ==========================================
  if (settings.rpm_display_mode != RPM_DISP_LEDS_ONLY) {
    u8g2->drawRFrame(6, 4, 388, 26, 3);

    int shift_x = 6 + (int)((uint32_t)settings.shift_rpm * 384 / settings.max_rpm);
    if (shift_x < 392) {
      u8g2->drawVLine(shift_x, 2, 30);
      u8g2->drawVLine(shift_x + 1, 2, 30);
    }

    int rpm_fill = (int)((uint32_t)telemetry.rpm * 384 / settings.max_rpm);
    if (rpm_fill > 384) rpm_fill = 384;
    if (rpm_fill > 0) {
      u8g2->drawBox(8, 6, rpm_fill, 22);
    }
  }

  // ==========================================
  // 2. THE THREE SPEEDOMETER DIALS (y = 38, h = 118)
  // ==========================================

  // --- Left Dial: Held Minimum Corner Speed ---
  u8g2->drawRFrame(10, 38, 120, 118, 6);
  u8g2->drawBox(10, 38, 120, 18);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_helvB08_tr);
  int tw_vmin = u8g2->getStrWidth("V-MIN (APEX)");
  u8g2->drawStr(10 + (120 - tw_vmin) / 2, 51, "V-MIN (APEX)");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_logisoso42_tn);
  snprintf(buf, sizeof(buf), "%d", (int)roundf(disp_vmin));
  int w_vmin = u8g2->getStrWidth(buf);
  u8g2->drawStr(10 + (120 - w_vmin) / 2, 126, buf);

  // --- Center Dial: Live Real-time Speed ---
  u8g2->drawRFrame(138, 38, 124, 118, 6);
  u8g2->drawBox(138, 38, 124, 18);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_helvB08_tr);
  int tw_live = u8g2->getStrWidth("LIVE SPEED");
  u8g2->drawStr(138 + (124 - tw_live) / 2, 51, "LIVE SPEED");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_logisoso42_tn);
  snprintf(buf, sizeof(buf), "%d", (int)roundf(disp_live));
  int w_live = u8g2->getStrWidth(buf);
  u8g2->drawStr(138 + (124 - w_live) / 2, 126, buf);

  // --- Right Dial: Held Maximum Straight Speed ---
  u8g2->drawRFrame(270, 38, 120, 118, 6);
  u8g2->drawBox(270, 38, 120, 18);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_helvB08_tr);
  int tw_vmax = u8g2->getStrWidth("V-MAX (STR)");
  u8g2->drawStr(270 + (120 - tw_vmax) / 2, 51, "V-MAX (STR)");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_logisoso42_tn);
  snprintf(buf, sizeof(buf), "%d", (int)roundf(disp_vmax));
  int w_vmax = u8g2->getStrWidth(buf);
  u8g2->drawStr(270 + (120 - w_vmax) / 2, 126, buf);

  // ==========================================
  // 3. BOTTOM-LEFT: LAP TIME & BEST LAP DELTA
  // ==========================================

  // --- Sub-panel A: Current Lap Time (y = 162, h = 52) ---
  u8g2->drawRFrame(10, 162, 185, 52, 4);
  u8g2->setFont(u8g2_font_helvB10_tr);
  snprintf(buf, sizeof(buf), "%s %02u  [%s %d]", 
           I18n::get(STR_LABEL_LAP), telemetry.lap_number, 
           I18n::get(STR_LABEL_SECTOR), telemetry.current_sector);
  u8g2->drawStr(18, 178, buf);

  uint32_t active_lap_time = telemetry.current_lap_time_ms;
  uint32_t lap_min = (active_lap_time / 60000);
  uint32_t lap_sec = (active_lap_time % 60000) / 1000;
  uint32_t lap_cen = (active_lap_time % 1000) / 10;
  u8g2->setFont(u8g2_font_helvB14_tr);
  snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", (unsigned long)lap_min, (unsigned long)lap_sec, (unsigned long)lap_cen);
  int tw = u8g2->getStrWidth(buf);
  u8g2->drawStr(10 + (185 - tw) / 2, 202, buf);

  // --- Sub-panel B: Predictive Best Lap Delta (y = 220, h = 52) ---
  float delta_val = telemetry.predictive_delta_s;
  if ((telemetry.current_sector != _prev_sector && _prev_sector != 0) ||
      (telemetry.lap_number != _prev_lap && _prev_lap != 0) ||
      (telemetry.best_lap_time_ms != _prev_best_lap && _prev_best_lap != 0) ||
      (fabsf(delta_val - _prev_delta_val) > 0.001f && _prev_delta_val < 900.0f)) {
    _delta_flash_start_ms = millis();
  }
  _prev_sector = telemetry.current_sector;
  _prev_lap = telemetry.lap_number;
  _prev_best_lap = telemetry.best_lap_time_ms;
  _prev_delta_val = delta_val;

  char delta_buf[32];
  if (telemetry.best_lap_time_ms > 0 || fabsf(delta_val) > 0.001f) {
    if (delta_val >= 0.0f) {
      snprintf(delta_buf, sizeof(delta_buf), "+ %.2f", delta_val);
    } else {
      snprintf(delta_buf, sizeof(delta_buf), "- %.2f", -delta_val);
    }
  } else {
    snprintf(delta_buf, sizeof(delta_buf), "+ 0.00");
  }

  u8g2->setFont(u8g2_font_helvB24_tr);
  int d_w = u8g2->getStrWidth(delta_buf);

  uint32_t flash_elapsed = millis() - _delta_flash_start_ms;
  bool is_flashing = (flash_elapsed < 1500 && _delta_flash_start_ms > 0);
  bool is_inverted = is_flashing && (((flash_elapsed / 250) % 2) == 0);

  if (is_inverted) {
    u8g2->drawRBox(10, 220, 185, 52, 4);
    u8g2->setDrawColor(0);
    u8g2->drawStr(10 + (185 - d_w) / 2, 256, delta_buf);
    u8g2->setDrawColor(1);
  } else {
    u8g2->drawRFrame(10, 220, 185, 52, 4);
    u8g2->drawStr(10 + (185 - d_w) / 2, 256, delta_buf);
  }

  // ==========================================
  // 4. BOTTOM-RIGHT: UNIFIED ALARM PANEL (185 x 110 px)
  // ==========================================
  bool alm_active[5];
  alm_active[ALARM_WATER] = (telemetry.water_temp_c >= settings.water_temp_alarm_c && settings.water_temp_alarm_c > 0);
  alm_active[ALARM_EGT]   = (telemetry.exhaust_temp_c >= settings.exhaust_temp_alarm_c && settings.exhaust_temp_alarm_c > 0);
  alm_active[ALARM_REV]   = (telemetry.rpm >= settings.over_rev_rpm && settings.over_rev_rpm > 0);
  alm_active[ALARM_BAT]   = (telemetry.battery_voltage < settings.low_bat_alarm_v && telemetry.battery_voltage > 1.0f);
  alm_active[ALARM_LINK]  = (!telemetry.track_module_connected);

  bool alm_warn[5];
  alm_warn[ALARM_WATER] = alm_active[ALARM_WATER] && settings.warn_trigger_water;
  alm_warn[ALARM_EGT]   = alm_active[ALARM_EGT]   && settings.warn_trigger_egt;
  alm_warn[ALARM_REV]   = alm_active[ALARM_REV]   && settings.warn_trigger_rev;
  alm_warn[ALARM_BAT]   = alm_active[ALARM_BAT]   && settings.warn_trigger_battery;
  alm_warn[ALARM_LINK]  = alm_active[ALARM_LINK]  && settings.warn_trigger_link;

  int8_t top_alarm_id = -1;
  for (int i = 0; i < 5; i++) {
    uint8_t aid = settings.alarm_priority[i];
    if (aid < 5 && alm_warn[aid]) {
      top_alarm_id = aid;
      break;
    }
  }

  if (top_alarm_id >= 0) {
    bool flash_phase = ((millis() / 350) % 2) == 0;
    u8g2->drawRBox(205, 162, 185, 110, 6);
    u8g2->setDrawColor(0);

    if (flash_phase) {
      // Phase A: Warning triangle icon + "WARN"
      u8g2->drawXBMP(205 + (185 - 24) / 2, 180, 24, 24, icon_warn_24x24);
      u8g2->setFont(u8g2_font_helvB24_tr);
      int w_warn = u8g2->getStrWidth("WARN");
      u8g2->drawStr(205 + (185 - w_warn) / 2, 248, "WARN");
    } else {
      // Phase B: Triggering alarm icon + short text
      static const uint8_t* const alarm_icons[5] = {
        icon_water_16x16,
        icon_egt_16x16,
        icon_rev_16x16,
        icon_bat_16x16,
        icon_link_16x16
      };
      static const char* const alarm_labels[5] = {
        "H2O HIGH",
        "EGT HIGH",
        "OVER-REV",
        "LOW BATT",
        "NO LINK"
      };

      u8g2->drawXBMP(205 + (185 - 16) / 2, 184, 16, 16, alarm_icons[top_alarm_id]);
      u8g2->setFont(u8g2_font_helvB18_tr);
      int w_lbl = u8g2->getStrWidth(alarm_labels[top_alarm_id]);
      u8g2->drawStr(205 + (185 - w_lbl) / 2, 246, alarm_labels[top_alarm_id]);
    }
    u8g2->setDrawColor(1);
  } else {
    // Normal System Status
    u8g2->drawRFrame(205, 162, 185, 110, 6);
    u8g2->setFont(u8g2_font_helvB14_tr);
    int ok_w = u8g2->getStrWidth("SYSTEM OK");
    u8g2->drawStr(205 + (185 - ok_w) / 2, 222, "SYSTEM OK");
  }

  // ==========================================
  // 5. BOTTOM LINE: SENSOR ICONS & RUNTIMES (y = 276..300)
  // ==========================================
  u8g2->drawHLine(0, 276, 400);

  float w_temp = settings.use_celsius ? telemetry.water_temp_c : (telemetry.water_temp_c * 1.8f + 32.0f);
  float e_temp = settings.use_celsius ? telemetry.exhaust_temp_c : (telemetry.exhaust_temp_c * 1.8f + 32.0f);

  // Water Temp
  u8g2->drawXBMP(6, 280, 16, 16, icon_water_16x16);
  u8g2->setFont(u8g2_font_helvB10_tr);
  snprintf(buf, sizeof(buf), "%d", (int)roundf(w_temp));
  u8g2->drawStr(24, 293, buf);

  // EGT Temp
  u8g2->drawXBMP(84, 280, 16, 16, icon_egt_16x16);
  snprintf(buf, sizeof(buf), "%d", (int)roundf(e_temp));
  u8g2->drawStr(102, 293, buf);

  // Total Engine Hours
  uint32_t eng_hrs = telemetry.engine_total_hours_sec / 3600;
  uint32_t eng_min = (telemetry.engine_total_hours_sec % 3600) / 60;
  u8g2->drawXBMP(156, 280, 16, 16, icon_engine_16x16);
  snprintf(buf, sizeof(buf), "%02luh%02lu", (unsigned long)eng_hrs, (unsigned long)eng_min);
  u8g2->drawStr(174, 293, buf);

  // Current Session Time
  uint32_t sess_hrs = telemetry.session_time_sec / 3600;
  uint32_t sess_min = (telemetry.session_time_sec % 3600) / 60;
  u8g2->drawXBMP(232, 280, 16, 16, icon_stopwatch_16x16);
  snprintf(buf, sizeof(buf), "%02luh%02lu", (unsigned long)sess_hrs, (unsigned long)sess_min);
  u8g2->drawStr(250, 293, buf);

  // Battery & Link Status (Blinks if low/disconnected)
  bool blink_1hz = ((millis() / 500) % 2) == 0;
  bool show_bat = (telemetry.battery_percent >= 10) || blink_1hz;
  bool show_link = telemetry.track_module_connected || blink_1hz;

  if (show_bat) {
    u8g2->drawXBMP(308, 280, 16, 16, icon_bat_16x16);
    snprintf(buf, sizeof(buf), "%d%%", telemetry.battery_percent);
    u8g2->drawStr(326, 293, buf);
  }
  u8g2->drawStr(354, 293, "|");
  if (show_link) {
    u8g2->drawStr(360, 293, telemetry.track_module_connected ? "LINK" : "ERR");
  }
}
