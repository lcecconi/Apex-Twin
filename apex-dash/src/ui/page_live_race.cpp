#include "ui/page_live_race.h"
#include "ui/icons_xbm.h"
#include "i18n.h"

void PageLiveRace::render(U8G2 *u8g2, const TelemetrySnapshot &telemetry, const SystemSettings &settings) {
  char buf[48];

  // ==========================================
  // 1. TOP TACHOMETER (RPM BAR GRAPH)
  // ==========================================
  if (settings.rpm_display_mode != RPM_DISP_LEDS_ONLY) {
    // Wide Outline bar: 388 px wide, 26 px tall (Margins 6px)
    u8g2->drawRFrame(6, 4, 388, 26, 3);

    // Shift light marker line at shift_rpm
    int shift_x = 6 + (int)((uint32_t)settings.shift_rpm * 384 / settings.max_rpm);
    if (shift_x < 392) {
      u8g2->drawVLine(shift_x, 2, 30);
      u8g2->drawVLine(shift_x + 1, 2, 30);
    }

    // Fill current RPM
    int rpm_fill = (int)((uint32_t)telemetry.rpm * 384 / settings.max_rpm);
    if (rpm_fill > 384) rpm_fill = 384;
    if (rpm_fill > 0) {
      u8g2->drawBox(8, 6, rpm_fill, 22);
    }
  }


  // ==========================================
  // 2 & 3. CENTER AREA (SPEED/GEAR & LAP TIME)
  // ==========================================
  bool has_left_pane = settings.show_speed || (settings.drive_type == DRIVE_SHIFTER_6SPEED);
  bool is_shift = (telemetry.rpm >= settings.shift_rpm && settings.shift_rpm > 0);
  bool shift_blink = is_shift && (((millis() / 150) % 2) == 0);

  if (has_left_pane) {
    // Left Pane: Speed & Gear (160 px width)
    u8g2->drawRFrame(10, 38, 160, 118, 6);

    if (settings.show_speed) {
      float disp_speed = settings.use_kmh ? telemetry.speed_kmh : (telemetry.speed_kmh * 0.621371f);
      const char *unit_str = settings.use_kmh ? "KM/H" : "MPH";

      if (settings.drive_type == DRIVE_SHIFTER_6SPEED) {
        // 6-Speed Shifter Kart: Dominant Gear on LEFT, Speed on RIGHT
        // Left: Gear Box (60 px width, 96 px height)
        if (shift_blink) {
          u8g2->drawRBox(18, 50, 60, 96, 4);
          u8g2->setDrawColor(0);
        } else {
          u8g2->drawRFrame(18, 50, 60, 96, 4);
        }

        if (telemetry.gear == 0) {
          u8g2->setFont(u8g2_font_logisoso50_tr);
          int nw = u8g2->getStrWidth("N");
          u8g2->drawStr(18 + (60 - nw) / 2, 122, "N");
        } else {
          u8g2->setFont(u8g2_font_logisoso58_tn);
          snprintf(buf, sizeof(buf), "%d", telemetry.gear);
          int gw = u8g2->getStrWidth(buf);
          u8g2->drawStr(18 + (60 - gw) / 2, 126, buf);
        }

        if (shift_blink) {
          u8g2->setDrawColor(1);
        }

        // Right: Speed Display (Unit at top, Numeral centered below)
        u8g2->setFont(u8g2_font_helvB12_tr);
        int uw = u8g2->getStrWidth(unit_str);
        u8g2->drawStr(82 + (82 - uw) / 2, 68, unit_str);

        u8g2->setFont(u8g2_font_logisoso42_tn);
        snprintf(buf, sizeof(buf), "%03d", (int)disp_speed);
        int sw = u8g2->getStrWidth(buf);
        u8g2->drawStr(82 + (82 - sw) / 2, 124, buf);
      } else {
        // Single Speed (Direct Drive / Clutch) — Centered Large Speed Display
        u8g2->setFont(u8g2_font_logisoso58_tn);
        snprintf(buf, sizeof(buf), "%03d", (int)disp_speed);
        u8g2->drawStr(32, 110, buf);

        u8g2->setFont(u8g2_font_helvB12_tr);
        u8g2->drawStr(66, 138, unit_str);
      }
    } else {
      // Speed Hidden Mode (Shifter Kart Only) — Large Centered Gear Indicator
      if (shift_blink) {
        u8g2->drawRBox(10, 38, 160, 118, 6);
        u8g2->setDrawColor(0);
      }

      if (telemetry.gear == 0) {
        u8g2->setFont(u8g2_font_logisoso58_tr);
        int nw = u8g2->getStrWidth("N");
        u8g2->drawStr(10 + (160 - nw) / 2, 126, "N");
      } else {
        u8g2->setFont(u8g2_font_logisoso92_tn);
        snprintf(buf, sizeof(buf), "%d", telemetry.gear);
        int gw = u8g2->getStrWidth(buf);
        u8g2->drawStr(10 + (160 - gw) / 2, 134, buf);
      }

      if (shift_blink) {
        u8g2->setDrawColor(1);
      }
    }

    // Right Pane: Standard Lap Time (214 px width)
    u8g2->drawRFrame(176, 38, 214, 118, 6);

    u8g2->setFont(u8g2_font_helvB10_tr);
    snprintf(buf, sizeof(buf), "%s %02u  [%s %d]", 
             I18n::get(STR_LABEL_LAP), telemetry.lap_number, 
             I18n::get(STR_LABEL_SECTOR), telemetry.current_sector);
    u8g2->drawStr(186, 56, buf);

    // Active lap time
    uint32_t active_lap_time = telemetry.current_lap_time_ms;
    uint32_t lap_min = (active_lap_time / 60000);
    uint32_t lap_sec = (active_lap_time % 60000) / 1000;
    uint32_t lap_cen = (active_lap_time % 1000) / 10;

    u8g2->setFont(u8g2_font_logisoso38_tn);
    snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", (unsigned long)lap_min, (unsigned long)lap_sec, (unsigned long)lap_cen);
    u8g2->drawStr(184, 102, buf);

    // Best Lap reference (Inverted Badge - Double Size)
    char b_buf[32], l_buf[32];
    if (telemetry.best_lap_time_ms > 0) {
      uint32_t b_sec = (telemetry.best_lap_time_ms % 60000) / 1000;
      uint32_t b_cen = (telemetry.best_lap_time_ms % 1000) / 10;
      snprintf(b_buf, sizeof(b_buf), " %s %02lu.%02lu ", I18n::get(STR_LABEL_BEST), (unsigned long)b_sec, (unsigned long)b_cen);
    } else {
      snprintf(b_buf, sizeof(b_buf), " %s --.-- ", I18n::get(STR_LABEL_BEST));
    }

    u8g2->setFont(u8g2_font_helvB10_tr);
    int b_w = u8g2->getStrWidth(b_buf);
    u8g2->drawRBox(182, 122, b_w, 22, 3);
    u8g2->setDrawColor(0);
    u8g2->drawStr(182, 138, b_buf);
    u8g2->setDrawColor(1);

    // Last Lap reference (Normal text - Double Size)
    if (telemetry.last_lap_time_ms > 0) {
      uint32_t l_sec = (telemetry.last_lap_time_ms % 60000) / 1000;
      uint32_t l_cen = (telemetry.last_lap_time_ms % 1000) / 10;
      snprintf(l_buf, sizeof(l_buf), "%s: %02lu.%02lu", I18n::get(STR_LABEL_LAST), (unsigned long)l_sec, (unsigned long)l_cen);
    } else {
      snprintf(l_buf, sizeof(l_buf), "%s: --.--", I18n::get(STR_LABEL_LAST));
    }
    int l_w = u8g2->getStrWidth(l_buf);
    u8g2->drawStr(386 - l_w, 138, l_buf);
  } else {
    // Full-Width Lap Time Pane (380 px width)
    u8g2->drawRFrame(10, 38, 380, 118, 6);

    u8g2->setFont(u8g2_font_helvB12_tr);
    snprintf(buf, sizeof(buf), "%s %02u  [%s %d]", 
             I18n::get(STR_LABEL_LAP), telemetry.lap_number, 
             I18n::get(STR_LABEL_SECTOR), telemetry.current_sector);
    u8g2->drawStr(24, 58, buf);

    // Main Lap Time (Centered Large)
    uint32_t active_lap_time = telemetry.current_lap_time_ms;
    uint32_t lap_min = (active_lap_time / 60000);
    uint32_t lap_sec = (active_lap_time % 60000) / 1000;
    uint32_t lap_cen = (active_lap_time % 1000) / 10;

    u8g2->setFont(u8g2_font_logisoso50_tn);
    snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", (unsigned long)lap_min, (unsigned long)lap_sec, (unsigned long)lap_cen);
    int time_w = u8g2->getStrWidth(buf);
    u8g2->drawStr(200 - (time_w / 2), 108, buf);

    // Best Lap reference on bottom-left (Inverted Badge - Double Size)
    char b_buf[32], l_buf[32];
    if (telemetry.best_lap_time_ms > 0) {
      uint32_t b_sec = (telemetry.best_lap_time_ms % 60000) / 1000;
      uint32_t b_cen = (telemetry.best_lap_time_ms % 1000) / 10;
      snprintf(b_buf, sizeof(b_buf), " %s %02lu.%02lu ", I18n::get(STR_LABEL_BEST), (unsigned long)b_sec, (unsigned long)b_cen);
    } else {
      snprintf(b_buf, sizeof(b_buf), " %s --.-- ", I18n::get(STR_LABEL_BEST));
    }

    u8g2->setFont(u8g2_font_helvB12_tr);
    int b_w = u8g2->getStrWidth(b_buf);
    u8g2->drawRBox(22, 122, b_w, 24, 3);
    u8g2->setDrawColor(0);
    u8g2->drawStr(22, 140, b_buf);
    u8g2->setDrawColor(1);

    // Last Lap reference on bottom-right (Normal text - Double Size)
    if (telemetry.last_lap_time_ms > 0) {
      uint32_t l_sec = (telemetry.last_lap_time_ms % 60000) / 1000;
      uint32_t l_cen = (telemetry.last_lap_time_ms % 1000) / 10;
      snprintf(l_buf, sizeof(l_buf), "%s: %02lu.%02lu", I18n::get(STR_LABEL_LAST), (unsigned long)l_sec, (unsigned long)l_cen);
    } else {
      snprintf(l_buf, sizeof(l_buf), "%s: --.--", I18n::get(STR_LABEL_LAST));
    }
    int last_w = u8g2->getStrWidth(l_buf);
    u8g2->drawStr(376 - last_w, 140, l_buf);
  }

  // ==========================================
  // 4. PREDICTIVE DELTA (LEFT) & ALARMS (RIGHT)
  // ==========================================
  // Left Pane: Predictive Lap Time Delta (185 px width)
  static uint8_t _prev_sector = 0;
  static uint16_t _prev_lap = 0;
  static uint32_t _prev_best_lap = 0;
  static float _prev_delta_val = 999.0f;
  static uint32_t _delta_flash_start_ms = 0;

  float delta_val = telemetry.predictive_delta_s;
  if ((telemetry.current_sector != _prev_sector && _prev_sector != 0) ||
      (telemetry.lap_number != _prev_lap && _prev_lap != 0) ||
      (telemetry.best_lap_time_ms != _prev_best_lap && _prev_best_lap != 0) ||
      (fabs(delta_val - _prev_delta_val) > 0.001f && _prev_delta_val < 900.0f)) {
    _delta_flash_start_ms = millis();
  }
  _prev_sector = telemetry.current_sector;
  _prev_lap = telemetry.lap_number;
  _prev_best_lap = telemetry.best_lap_time_ms;
  _prev_delta_val = delta_val;

  char delta_buf[32];
  if (telemetry.best_lap_time_ms > 0 || fabs(delta_val) > 0.001f) {
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

  // Inverted color flash when best lap delta gets updated
  uint32_t flash_elapsed = millis() - _delta_flash_start_ms;
  bool is_flashing = (flash_elapsed < 1500 && _delta_flash_start_ms > 0);
  bool is_inverted = is_flashing && (((flash_elapsed / 250) % 2) == 0);

  if (is_inverted) {
    u8g2->drawRBox(10, 162, 185, 52, 4);
    u8g2->setDrawColor(0);
    u8g2->drawStr(10 + (185 - d_w) / 2, 198, delta_buf);
    u8g2->setDrawColor(1);
  } else {
    u8g2->drawRFrame(10, 162, 185, 52, 4);
    u8g2->drawStr(10 + (185 - d_w) / 2, 198, delta_buf);
  }

  // ==========================================
  // 4 & 5. RIGHT UNIFIED WARNING / STATUS PANEL (185 x 110 px)
  // ==========================================
  // Evaluate Base Alarm Conditions
  bool alm_active[5];
  alm_active[ALARM_WATER] = (telemetry.water_temp_c >= settings.water_temp_alarm_c && settings.water_temp_alarm_c > 0);
  alm_active[ALARM_EGT]   = (telemetry.exhaust_temp_c >= settings.exhaust_temp_alarm_c && settings.exhaust_temp_alarm_c > 0);
  alm_active[ALARM_REV]   = (telemetry.rpm >= settings.over_rev_rpm && settings.over_rev_rpm > 0);
  alm_active[ALARM_BAT]   = (telemetry.battery_voltage < settings.low_bat_alarm_v && telemetry.battery_voltage > 1.0f);
  alm_active[ALARM_LINK]  = (!telemetry.track_module_connected);

  // Evaluate which alarms trigger the blinking WARN alert
  bool alm_warn[5];
  alm_warn[ALARM_WATER] = alm_active[ALARM_WATER] && settings.warn_trigger_water;
  alm_warn[ALARM_EGT]   = alm_active[ALARM_EGT]   && settings.warn_trigger_egt;
  alm_warn[ALARM_REV]   = alm_active[ALARM_REV]   && settings.warn_trigger_rev;
  alm_warn[ALARM_BAT]   = alm_active[ALARM_BAT]   && settings.warn_trigger_battery;
  alm_warn[ALARM_LINK]  = alm_active[ALARM_LINK]  && settings.warn_trigger_link;

  // Find most severe active warning based on configurable priority order
  int8_t top_alarm_id = -1;
  for (int i = 0; i < 5; i++) {
    uint8_t aid = settings.alarm_priority[i];
    if (aid < 5 && alm_warn[aid]) {
      top_alarm_id = aid;
      break;
    }
  }

  // ==========================================
  // 5. BOTTOM ENGINE (LEFT)
  // ==========================================
  // Left: Water, EGT & Engine Runtime Pane (185 px width)
  u8g2->drawRFrame(10, 222, 185, 50, 4);

  float w_temp = settings.use_celsius ? telemetry.water_temp_c : (telemetry.water_temp_c * 1.8f + 32.0f);
  float e_temp = settings.use_celsius ? telemetry.exhaust_temp_c : (telemetry.exhaust_temp_c * 1.8f + 32.0f);
  const char *t_unit = settings.use_celsius ? "\xb0\x43" : "\xb0\x46";

  // Col 1: Water & EGT temp sensors
  u8g2->drawXBMP(14, 228, 16, 16, icon_water_16x16);
  u8g2->setFont(u8g2_font_helvB10_tr);
  snprintf(buf, sizeof(buf), "%.1f%s", w_temp, t_unit);
  u8g2->drawStr(34, 241, buf);

  u8g2->drawXBMP(14, 249, 16, 16, icon_egt_16x16);
  snprintf(buf, sizeof(buf), "%d%s", (int)e_temp, t_unit);
  u8g2->drawStr(34, 263, buf);

  // Vertical Separator
  u8g2->drawVLine(96, 226, 42);

  // Col 2: Total Engine Runtime (Row 1) & Current Session Time (Row 2)
  uint32_t eng_hrs = telemetry.engine_total_hours_sec / 3600;
  uint32_t eng_min = (telemetry.engine_total_hours_sec % 3600) / 60;
  u8g2->drawXBMP(104, 228, 16, 16, icon_engine_16x16);
  snprintf(buf, sizeof(buf), "%02luh%02lu", (unsigned long)eng_hrs, (unsigned long)eng_min);
  u8g2->drawStr(124, 241, buf);

  uint32_t sess_hrs = telemetry.session_time_sec / 3600;
  uint32_t sess_min = (telemetry.session_time_sec % 3600) / 60;
  u8g2->drawXBMP(104, 249, 16, 16, icon_stopwatch_16x16);
  snprintf(buf, sizeof(buf), "%02luh%02lu", (unsigned long)sess_hrs, (unsigned long)sess_min);
  u8g2->drawStr(124, 263, buf);

  // Right: Unified Flashing Warning / Status Panel (185 x 110 px)
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
      // Phase B: Actual triggering alarm icon + short text
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
    // Normal System Status (Outlined Box)
    u8g2->drawRFrame(205, 162, 185, 110, 6);
    u8g2->setFont(u8g2_font_helvB14_tr);
    int ok_w = u8g2->getStrWidth("SYSTEM OK");
    u8g2->drawStr(205 + (185 - ok_w) / 2, 222, "SYSTEM OK");
  }

  // ==========================================
  // 6. BOTTOM LINE (TRACK INFO & STATUS)
  // ==========================================
  u8g2->drawHLine(0, 276, 400);
  u8g2->setFont(u8g2_font_helvB10_tr);
  if (telemetry.track_error_code != 0) {
    static const char* const err_strings[] = {
      "ERR: NO GPS FIX",
      "ERR: IMU FAULT",
      "ERR: SD CARD FAIL",
      "ERR: EGT SENSOR",
      "ERR: H2O SENSOR",
      "ERR: CAN BUS FAIL",
      "ERR: LOW MEMORY"
    };
    if (telemetry.track_error_code >= 1 && telemetry.track_error_code <= 7) {
      snprintf(buf, sizeof(buf), "%s", err_strings[telemetry.track_error_code - 1]);
    } else {
      snprintf(buf, sizeof(buf), "ERR: CODE #%u", telemetry.track_error_code);
    }
  } else {
    snprintf(buf, sizeof(buf), "TRACK: %s", telemetry.current_track_name);
  }
  u8g2->drawStr(8, 292, buf);

  bool blink_1hz = ((millis() / 500) % 2) == 0;
  bool show_bat = (telemetry.battery_percent >= 10) || blink_1hz;
  bool show_link = telemetry.track_module_connected || blink_1hz;

  char bat_str[32];
  snprintf(bat_str, sizeof(bat_str), "BAT: %.1fV (%d%%)", telemetry.battery_voltage, telemetry.battery_percent);
  const char *sep_str = " | ";
  const char *link_str = telemetry.track_module_connected ? "LINK OK" : "NO LINK";

  int bat_w = u8g2->getStrWidth(bat_str);
  int sep_w = u8g2->getStrWidth(sep_str);
  int link_w = u8g2->getStrWidth(link_str);
  int total_w = bat_w + sep_w + link_w;
  int start_x = 392 - total_w;

  if (show_bat) {
    u8g2->drawStr(start_x, 292, bat_str);
  }
  u8g2->drawStr(start_x + bat_w, 292, sep_str);
  if (show_link) {
    u8g2->drawStr(start_x + bat_w + sep_w, 292, link_str);
  }
}




