#include "ui/page_live_race.h"
#include "i18n.h"

void PageLiveRace::render(U8G2 *u8g2, const TelemetrySnapshot &telemetry, const SystemSettings &settings) {
  char buf[48];

  // ==========================================
  // 1. TOP TACHOMETER (RPM BAR GRAPH)
  // ==========================================
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

  // ==========================================
  // 2. SPEED & GEAR (CENTER-LEFT)
  // ==========================================
  u8g2->drawRFrame(10, 38, 160, 118, 6);

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

  // ==========================================
  // 3. LAP TIME (CENTER-RIGHT)
  // ==========================================
  u8g2->drawRFrame(176, 38, 214, 118, 6);

  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "%s %02u  [%s %d]", 
           I18n::get(STR_LABEL_LAP), telemetry.lap_number, 
           I18n::get(STR_LABEL_SECTOR), telemetry.current_sector);
  u8g2->drawStr(186, 54, buf);

  // Active or last lap time
  uint32_t active_lap_time = telemetry.current_lap_time_ms;
  uint32_t lap_min = (active_lap_time / 60000);
  uint32_t lap_sec = (active_lap_time % 60000) / 1000;
  uint32_t lap_cen = (active_lap_time % 1000) / 10;

  u8g2->setFont(u8g2_font_logisoso38_tn);
  snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", (unsigned long)lap_min, (unsigned long)lap_sec, (unsigned long)lap_cen);
  u8g2->drawStr(184, 102, buf);

  // Best Lap reference
  u8g2->setFont(u8g2_font_helvB10_tr);
  if (telemetry.best_lap_time_ms > 0) {
    uint32_t b_sec = (telemetry.best_lap_time_ms % 60000) / 1000;
    uint32_t b_cen = (telemetry.best_lap_time_ms % 1000) / 10;
    snprintf(buf, sizeof(buf), "%s: %02lu.%02lus", I18n::get(STR_LABEL_BEST), (unsigned long)b_sec, (unsigned long)b_cen);
  } else {
    snprintf(buf, sizeof(buf), "%s: --.--s", I18n::get(STR_LABEL_BEST));
  }
  u8g2->drawStr(186, 138, buf);

  // ==========================================
  // 4. PREDICTIVE LAP TIME DELTA BAR
  // ==========================================
  u8g2->drawRFrame(10, 162, 380, 52, 4);

  // Header & Numerical Delta
  u8g2->setFont(u8g2_font_helvB10_tr);
  snprintf(buf, sizeof(buf), "%s %s", I18n::get(STR_LABEL_PRED), I18n::get(STR_LABEL_DELTA));
  u8g2->drawStr(18, 180, buf);

  u8g2->setFont(u8g2_font_helvB14_tr);
  snprintf(buf, sizeof(buf), "%+0.2f s", telemetry.predictive_delta_s);
  int delta_w = u8g2->getStrWidth(buf);
  u8g2->drawStr(376 - delta_w, 182, buf);

  // Center Zero Marker Line
  int center_x = 200;
  u8g2->drawFrame(20, 192, 360, 14);
  u8g2->drawVLine(center_x, 188, 22);

  // Graphical delta bar (+/- 1.0 second range = 170 pixels each side)
  int bar_px = (int)(telemetry.predictive_delta_s * 170.0f);
  if (bar_px > 170) bar_px = 170;
  if (bar_px < -170) bar_px = -170;

  if (bar_px < 0) {
    // Faster -> Bar extends LEFT from center
    u8g2->drawBox(center_x + bar_px, 194, -bar_px, 10);
  } else if (bar_px > 0) {
    // Slower -> Bar extends RIGHT from center
    u8g2->drawBox(center_x, 194, bar_px, 10);
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
