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

    if (telemetry.rpm >= settings.shift_rpm) {
      u8g2->drawRBox(300, 22, 90, 14, 2);
      u8g2->setDrawColor(0);
      u8g2->drawStr(305, 33, I18n::get(STR_WARN_SHIFT));
      u8g2->setDrawColor(1);
    } else {
      snprintf(buf, sizeof(buf), "MAX %u", settings.max_rpm);
      u8g2->drawStr(328, 32, buf);
    }
  }


  // ==========================================
  // 2 & 3. CENTER AREA (SPEED/GEAR & LAP TIME)
  // ==========================================
  bool has_left_pane = settings.show_speed || (settings.drive_type == DRIVE_SHIFTER_6SPEED);

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
        u8g2->drawRFrame(116, 82, 46, 66, 4);
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawStr(122, 94, I18n::get(STR_LABEL_GEAR));
        u8g2->setFont(u8g2_font_logisoso32_tn);
        if (telemetry.gear == 0) {
          u8g2->setFont(u8g2_font_helvB18_tr);
          u8g2->drawStr(132, 134, "N");
        } else {
          snprintf(buf, sizeof(buf), "%d", telemetry.gear);
          u8g2->drawStr(130, 136, buf);
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
      u8g2->setFont(u8g2_font_helvB10_tr);
      u8g2->drawStr(66, 60, I18n::get(STR_LABEL_GEAR));

      if (telemetry.gear == 0) {
        u8g2->setFont(u8g2_font_helvB24_tr);
        u8g2->drawStr(78, 120, "N");
      } else {
        u8g2->setFont(u8g2_font_logisoso58_tn);
        snprintf(buf, sizeof(buf), "%d", telemetry.gear);
        u8g2->drawStr(72, 126, buf);
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

    // Best Lap reference (Inverted Badge)
    char b_buf[32], l_buf[32];
    if (telemetry.best_lap_time_ms > 0) {
      uint32_t b_sec = (telemetry.best_lap_time_ms % 60000) / 1000;
      uint32_t b_cen = (telemetry.best_lap_time_ms % 1000) / 10;
      snprintf(b_buf, sizeof(b_buf), " %s %02lu.%02lus ", I18n::get(STR_LABEL_BEST), (unsigned long)b_sec, (unsigned long)b_cen);
    } else {
      snprintf(b_buf, sizeof(b_buf), " %s --.--s ", I18n::get(STR_LABEL_BEST));
    }

    u8g2->setFont(u8g2_font_6x10_tr);
    int b_w = u8g2->getStrWidth(b_buf);
    u8g2->drawRBox(182, 128, b_w, 16, 2);
    u8g2->setDrawColor(0);
    u8g2->drawStr(182, 140, b_buf);
    u8g2->setDrawColor(1);

    // Last Lap reference (Normal text)
    if (telemetry.last_lap_time_ms > 0) {
      uint32_t l_sec = (telemetry.last_lap_time_ms % 60000) / 1000;
      uint32_t l_cen = (telemetry.last_lap_time_ms % 1000) / 10;
      snprintf(l_buf, sizeof(l_buf), "%s %02lu.%02lus", I18n::get(STR_LABEL_LAST), (unsigned long)l_sec, (unsigned long)l_cen);
    } else {
      snprintf(l_buf, sizeof(l_buf), "%s --.--s", I18n::get(STR_LABEL_LAST));
    }
    int l_w = u8g2->getStrWidth(l_buf);
    u8g2->drawStr(380 - l_w, 140, l_buf);
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

    // Best Lap reference on bottom-left (Inverted Badge)
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
    u8g2->drawRBox(22, 127, b_w, 20, 3);
    u8g2->setDrawColor(0);
    u8g2->drawStr(22, 142, b_buf);
    u8g2->setDrawColor(1);

    // Last Lap reference on bottom-right (Normal text)
    if (telemetry.last_lap_time_ms > 0) {
      uint32_t l_sec = (telemetry.last_lap_time_ms % 60000) / 1000;
      uint32_t l_cen = (telemetry.last_lap_time_ms % 1000) / 10;
      snprintf(l_buf, sizeof(l_buf), "%s: %02lu.%02lus", I18n::get(STR_LABEL_LAST), (unsigned long)l_sec, (unsigned long)l_cen);
    } else {
      snprintf(l_buf, sizeof(l_buf), "%s: --.--s", I18n::get(STR_LABEL_LAST));
    }
    int last_w = u8g2->getStrWidth(l_buf);
    u8g2->drawStr(376 - last_w, 142, l_buf);
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

  // Right Pane: Alarms Grid (185 px width)
  u8g2->drawRFrame(205, 162, 185, 52, 4);

  // Evaluate Alarm Conditions
  bool alm_active[5];
  alm_active[0] = (telemetry.water_temp_c >= settings.water_temp_alarm_c && settings.water_temp_alarm_c > 0);
  alm_active[1] = (telemetry.exhaust_temp_c >= settings.exhaust_temp_alarm_c && settings.exhaust_temp_alarm_c > 0);
  alm_active[2] = (telemetry.rpm >= settings.over_rev_rpm && settings.over_rev_rpm > 0);
  alm_active[3] = (telemetry.battery_voltage < settings.low_bat_alarm_v && telemetry.battery_voltage > 1.0f);
  alm_active[4] = (!telemetry.track_module_connected);

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

  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(212, 172, "SYSTEM ALARMS");

  for (int i = 0; i < 5; i++) {
    int tx = 211 + (i * 35);
    int ty = 175;
    int tw = 32;
    int th = 34;

    if (alm_active[i]) {
      // Lit Up Alarm (Inverted Solid Fill)
      u8g2->drawRBox(tx, ty, tw, th, 2);
      u8g2->setDrawColor(0);

      u8g2->drawXBMP(tx + 8, ty + 3, 16, 16, tiles[i].xbm);

      u8g2->setFont(u8g2_font_5x8_tr);
      int lw = u8g2->getStrWidth(tiles[i].label);
      u8g2->drawStr(tx + (tw - lw) / 2, ty + 30, tiles[i].label);

      u8g2->setDrawColor(1);
    } else {
      // Normally OFF (Dim Outline Box)
      u8g2->drawRFrame(tx, ty, tw, th, 2);

      u8g2->drawXBMP(tx + 8, ty + 3, 16, 16, tiles[i].xbm);

      u8g2->setFont(u8g2_font_5x8_tr);
      int lw = u8g2->getStrWidth(tiles[i].label);
      u8g2->drawStr(tx + (tw - lw) / 2, ty + 30, tiles[i].label);
    }
  }

  // ==========================================
  // 5. BOTTOM ENGINE & SENSOR STATUS BAR
  // ==========================================
  u8g2->drawHLine(10, 222, 380);

  u8g2->setFont(u8g2_font_helvB10_tr);
  // Water Temp
  snprintf(buf, sizeof(buf), "%s: %.1f\xb0\x43", I18n::get(STR_LABEL_WATER), telemetry.water_temp_c);
  u8g2->drawStr(14, 244, buf);
  if (telemetry.water_temp_c >= settings.water_temp_alarm_c) {
    u8g2->drawRBox(120, 232, 42, 16, 2);
    u8g2->setDrawColor(0);
    u8g2->drawStr(124, 244, "WARN");
    u8g2->setDrawColor(1);
  }

  // Exhaust Temp (EGT)
  snprintf(buf, sizeof(buf), "%s: %d\xb0\x43", I18n::get(STR_LABEL_EGT), (int)telemetry.exhaust_temp_c);
  u8g2->drawStr(175, 244, buf);

  // Track Name
  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "%s", telemetry.current_track_name);
  u8g2->drawStr(275, 244, buf);

  // Battery & Mode
  snprintf(buf, sizeof(buf), "%s: %.2fV (%d%%)  |  Apex-Track: %s",
           I18n::get(STR_LABEL_BAT), telemetry.battery_voltage, telemetry.battery_percent,
           telemetry.track_module_connected ? I18n::get(STR_STATUS_WIRELESS_OK) : I18n::get(STR_STATUS_SIMULATION));
  u8g2->drawStr(14, 264, buf);
}


