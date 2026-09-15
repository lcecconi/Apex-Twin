#include "ui/page_live_race.h"
#include "ui/icons_xbm.h"
#include "i18n.h"

void PageLiveRace::render(U8G2 *u8g2, const TelemetrySnapshot &telemetry, const SystemSettings &settings) {
  char buf[48];

  // ==========================================
  // 1. TOP TACHOMETER (RPM BAR GRAPH)
  // ==========================================
  if (settings.rpm_display_mode != RPM_DISP_LEDS_ONLY) {
    // Outline bar: 380 px wide, 16 px tall
    u8g2->drawFrame(10, 4, 380, 16);

    // Shift light marker line at shift_rpm
    int shift_x = 10 + (int)((uint32_t)settings.shift_rpm * 376 / settings.max_rpm);
    if (shift_x < 386) {
      u8g2->drawVLine(shift_x, 2, 20);
      u8g2->drawVLine(shift_x + 1, 2, 20);
    }

    // Fill current RPM
    int rpm_fill = (int)((uint32_t)telemetry.rpm * 376 / settings.max_rpm);
    if (rpm_fill > 376) rpm_fill = 376;
    if (rpm_fill > 0) {
      u8g2->drawBox(12, 6, rpm_fill, 12);
    }

    // Numerical RPM label below bar
    u8g2->setFont(u8g2_font_6x10_tr);
    snprintf(buf, sizeof(buf), "%s: %u", I18n::get(STR_LABEL_RPM), telemetry.rpm);
    u8g2->drawStr(14, 32, buf);

    snprintf(buf, sizeof(buf), "MAX %u", settings.max_rpm);
    u8g2->drawStr(328, 32, buf);
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
        // 6-Speed Shifter Kart (Speed + Gear Panel)
        u8g2->setFont(u8g2_font_logisoso50_tn);
        snprintf(buf, sizeof(buf), "%03d", (int)disp_speed);
        u8g2->drawStr(18, 114, buf);

        u8g2->setFont(u8g2_font_helvB10_tr);
        u8g2->drawStr(118, 70, unit_str);

        // Gear Box
        if (shift_blink) {
          u8g2->drawRBox(116, 82, 46, 66, 4);
          u8g2->setDrawColor(0);
        } else {
          u8g2->drawRFrame(116, 82, 46, 66, 4);
        }

        if (telemetry.gear == 0) {
          u8g2->setFont(u8g2_font_helvB24_tr);
          int nw = u8g2->getStrWidth("N");
          u8g2->drawStr(116 + (46 - nw) / 2, 126, "N");
        } else {
          u8g2->setFont(u8g2_font_logisoso50_tn);
          snprintf(buf, sizeof(buf), "%d", telemetry.gear);
          int gw = u8g2->getStrWidth(buf);
          u8g2->drawStr(116 + (46 - gw) / 2, 136, buf);
        }

        if (shift_blink) {
          u8g2->setDrawColor(1);
        }
      } else {
        // Single Speed (Direct Drive / Clutch) — Centered Large Speed Display
        u8g2->setFont(u8g2_font_logisoso58_tn);
        snprintf(buf, sizeof(buf), "%03d", (int)disp_speed);
        u8g2->drawStr(32, 110, buf);

        u8g2->setFont(u8g2_font_helvB10_tr);
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

    u8g2->setFont(u8g2_font_6x10_tr);
    snprintf(buf, sizeof(buf), "%s %02u  [%s %d]", 
             I18n::get(STR_LABEL_LAP), telemetry.lap_number, 
             I18n::get(STR_LABEL_SECTOR), telemetry.current_sector);
    u8g2->drawStr(186, 54, buf);

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
      snprintf(b_buf, sizeof(b_buf), " %s %02lu.%02lus ", I18n::get(STR_LABEL_BEST), (unsigned long)b_sec, (unsigned long)b_cen);
    } else {
      snprintf(b_buf, sizeof(b_buf), " %s --.--s ", I18n::get(STR_LABEL_BEST));
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
      snprintf(l_buf, sizeof(l_buf), "%s: %02lu.%02lus", I18n::get(STR_LABEL_LAST), (unsigned long)l_sec, (unsigned long)l_cen);
    } else {
      snprintf(l_buf, sizeof(l_buf), "%s: --.--s", I18n::get(STR_LABEL_LAST));
    }
    int l_w = u8g2->getStrWidth(l_buf);
    u8g2->drawStr(384 - l_w, 138, l_buf);
  } else {
    // Full-Width Lap Time Pane (380 px width)
    u8g2->drawRFrame(10, 38, 380, 118, 6);

    u8g2->setFont(u8g2_font_helvB10_tr);
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
      snprintf(b_buf, sizeof(b_buf), " %s %02lu.%02lus ", I18n::get(STR_LABEL_BEST), (unsigned long)b_sec, (unsigned long)b_cen);
    } else {
      snprintf(b_buf, sizeof(b_buf), " %s --.--s ", I18n::get(STR_LABEL_BEST));
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
      snprintf(l_buf, sizeof(l_buf), "%s: %02lu.%02lus", I18n::get(STR_LABEL_LAST), (unsigned long)l_sec, (unsigned long)l_cen);
    } else {
      snprintf(l_buf, sizeof(l_buf), "%s: --.--s", I18n::get(STR_LABEL_LAST));
    }
    int last_w = u8g2->getStrWidth(l_buf);
    u8g2->drawStr(376 - last_w, 140, l_buf);
  }

  // ==========================================
  // 4. PREDICTIVE DELTA (LEFT) & ALARMS (RIGHT)
  // ==========================================
  // Left Pane: Predictive Lap Time Delta Bar (185 px width)
  u8g2->drawRFrame(10, 162, 185, 52, 4);

  // Header & Numerical Delta
  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "%s %s", I18n::get(STR_LABEL_PRED), I18n::get(STR_LABEL_DELTA));
  u8g2->drawStr(16, 178, buf);

  u8g2->setFont(u8g2_font_helvB10_tr);
  snprintf(buf, sizeof(buf), "%+0.2f s", telemetry.predictive_delta_s);
  int delta_w = u8g2->getStrWidth(buf);
  u8g2->drawStr(190 - delta_w, 178, buf);

  // Center Zero Marker Line & Delta Bar
  int center_x = 102;
  u8g2->drawFrame(18, 188, 169, 14);
  u8g2->drawVLine(center_x, 184, 22);

  int bar_px = (int)(telemetry.predictive_delta_s * 80.0f);
  if (bar_px > 80) bar_px = 80;
  if (bar_px < -80) bar_px = -80;

  if (bar_px < 0) {
    u8g2->drawBox(center_x + bar_px, 190, -bar_px, 10);
  } else if (bar_px > 0) {
    u8g2->drawBox(center_x, 190, bar_px, 10);
  }

  // Right Pane: Alarms Grid (185 px width, no header text)
  u8g2->drawRFrame(205, 162, 185, 52, 4);

  // Evaluate Base Alarm Conditions
  bool alm_active[5];
  alm_active[0] = (telemetry.water_temp_c >= settings.water_temp_alarm_c && settings.water_temp_alarm_c > 0);
  alm_active[1] = (telemetry.exhaust_temp_c >= settings.exhaust_temp_alarm_c && settings.exhaust_temp_alarm_c > 0);
  alm_active[2] = (telemetry.rpm >= settings.over_rev_rpm && settings.over_rev_rpm > 0);
  alm_active[3] = (telemetry.battery_voltage < settings.low_bat_alarm_v && telemetry.battery_voltage > 1.0f);
  alm_active[4] = (!telemetry.track_module_connected);

  // Evaluate which alarms trigger the blinking WARN alert
  bool alm_warn[5];
  alm_warn[0] = alm_active[0] && settings.warn_trigger_water;
  alm_warn[1] = alm_active[1] && settings.warn_trigger_egt;
  alm_warn[2] = alm_active[2] && settings.warn_trigger_rev;
  alm_warn[3] = alm_active[3] && settings.warn_trigger_battery;
  alm_warn[4] = alm_active[4] && settings.warn_trigger_link;

  bool any_warn = alm_warn[0] || alm_warn[1] || alm_warn[2] || alm_warn[3] || alm_warn[4];

  struct AlarmTile {
    const char *label;
    const uint8_t *xbm;
  };

  static const AlarmTile tiles[5] = {
    { "H2O",  icon_water_16x16 }, // Water / Droplet
    { "EGT",  icon_egt_16x16 },   // Exhaust / Flame
    { "REV",  icon_rev_16x16 },   // Tach / Over-rev
    { "BAT",  icon_bat_16x16 },   // Battery
    { "LINK", icon_link_16x16 }   // Telemetry Wireless Link
  };

  for (int i = 0; i < 5; i++) {
    int tx = 211 + (i * 35);
    int ty = 169;
    int tw = 32;
    int th = 38;

    if (alm_active[i]) {
      // Lit Up Alarm (Inverted Solid Fill)
      u8g2->drawRBox(tx, ty, tw, th, 3);
      u8g2->setDrawColor(0);

      u8g2->drawXBMP(tx + 8, ty + 4, 16, 16, tiles[i].xbm);

      u8g2->setFont(u8g2_font_5x8_tr);
      int lw = u8g2->getStrWidth(tiles[i].label);
      u8g2->drawStr(tx + (tw - lw) / 2, ty + 32, tiles[i].label);

      u8g2->setDrawColor(1);
    } else {
      // Normally OFF (Dim Outline Box)
      u8g2->drawRFrame(tx, ty, tw, th, 3);

      u8g2->drawXBMP(tx + 8, ty + 4, 16, 16, tiles[i].xbm);

      u8g2->setFont(u8g2_font_5x8_tr);
      int lw = u8g2->getStrWidth(tiles[i].label);
      u8g2->drawStr(tx + (tw - lw) / 2, ty + 32, tiles[i].label);
    }
  }

  // ==========================================
  // 5. BOTTOM ENGINE (LEFT) & ALARM BANNER (RIGHT)
  // ==========================================
  // Left: Water & EGT Temp Pane (185 px width)
  u8g2->drawRFrame(10, 222, 185, 50, 4);

  float w_temp = settings.use_celsius ? telemetry.water_temp_c : (telemetry.water_temp_c * 1.8f + 32.0f);
  float e_temp = settings.use_celsius ? telemetry.exhaust_temp_c : (telemetry.exhaust_temp_c * 1.8f + 32.0f);
  const char *t_unit = settings.use_celsius ? "\xb0\x43" : "\xb0\x46";

  u8g2->setFont(u8g2_font_helvB10_tr);
  snprintf(buf, sizeof(buf), "%s: %.1f%s", I18n::get(STR_LABEL_WATER), w_temp, t_unit);
  u8g2->drawStr(18, 242, buf);

  snprintf(buf, sizeof(buf), "%s: %d%s", I18n::get(STR_LABEL_EGT), (int)e_temp, t_unit);
  u8g2->drawStr(18, 262, buf);

  // Right: Flashing WARN Alert or System Status (185 px width)
  if (any_warn) {
    bool flash_state = ((millis() / 300) % 2) == 0;

    u8g2->setFont(u8g2_font_helvB24_tr);
    int w_warn = u8g2->getStrWidth("WARN");
    int total_w = 24 + 10 + w_warn;
    int start_x = 205 + (185 - total_w) / 2;
    int icon_y = 222 + (50 - 24) / 2;

    if (flash_state) {
      // Solid Inverted Fill (Active Flashing Warning)
      u8g2->drawRBox(205, 222, 185, 50, 4);
      u8g2->setDrawColor(0);

      u8g2->drawXBMP(start_x, icon_y, 24, 24, icon_warn_24x24);
      u8g2->drawStr(start_x + 24 + 10, 222 + 37, "WARN");

      u8g2->setDrawColor(1);
    } else {
      // Outlined Frame (Flash Alternate Phase)
      u8g2->drawRFrame(205, 222, 185, 50, 4);

      u8g2->drawXBMP(start_x, icon_y, 24, 24, icon_warn_24x24);
      u8g2->drawStr(start_x + 24 + 10, 222 + 37, "WARN");
    }
  } else {
    // Normal System Status (Dim Outline Box)
    u8g2->drawRFrame(205, 222, 185, 50, 4);
    u8g2->setFont(u8g2_font_helvB10_tr);
    int ok_w = u8g2->getStrWidth("[ ALL SYSTEMS OK ]");
    u8g2->drawStr(205 + (185 - ok_w) / 2, 252, "[ ALL SYSTEMS OK ]");
  }

  // ==========================================
  // 6. BOTTOM LINE (TRACK INFO & STATUS)
  // ==========================================
  u8g2->drawHLine(0, 276, 400);
  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "TRACK: %s", telemetry.current_track_name);
  u8g2->drawStr(8, 292, buf);

  snprintf(buf, sizeof(buf), "BAT: %.1fV (%d%%) | %s",
           telemetry.battery_voltage, telemetry.battery_percent,
           telemetry.track_module_connected ? "LINK OK" : "SIM");
  int bw = u8g2->getStrWidth(buf);
  u8g2->drawStr(392 - bw, 292, buf);
}




