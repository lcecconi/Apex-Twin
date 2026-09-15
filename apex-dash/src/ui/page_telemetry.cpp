#include "ui/page_telemetry.h"

void PageTelemetry::render(U8G2 *u8g2, const TelemetrySnapshot &telemetry, const SystemSettings &settings) {
  char buf[48];

  // Header Title
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(10, 20, "TELEMETRY & SENSOR MONITOR");
  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "LAP %02d  |  SEC %d", telemetry.lap_number, telemetry.current_sector);
  u8g2->drawStr(300, 20, buf);
  u8g2->drawHLine(10, 26, 380);

  // ==========================================
  // CARD 1: ENGINE TACHO & GEAR (Top-Left)
  // ==========================================
  u8g2->drawRFrame(10, 32, 185, 110, 4);
  u8g2->drawBox(10, 32, 185, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(16, 44, "ENGINE RPM & GEAR");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_logisoso32_tn);
  snprintf(buf, sizeof(buf), "%u", telemetry.rpm);
  u8g2->drawStr(16, 84, buf);

  u8g2->setFont(u8g2_font_helvB10_tr);
  snprintf(buf, sizeof(buf), "Gear: %d", telemetry.gear);
  u8g2->drawStr(124, 72, buf);

  // Segmented Bar
  u8g2->drawFrame(16, 96, 172, 10);
  int fill = (int)((uint32_t)telemetry.rpm * 168 / settings.max_rpm);
  if (fill > 168) fill = 168;
  if (fill > 0) u8g2->drawBox(18, 98, fill, 6);

  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "Min: 5,620  Max: %u", settings.max_rpm);
  u8g2->drawStr(16, 134, buf);

  // ==========================================
  // CARD 2: DUAL TEMPERATURES (Top-Right)
  // ==========================================
  u8g2->drawRFrame(205, 32, 185, 110, 4);
  u8g2->drawBox(205, 32, 185, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(211, 44, "COOLANT & EXHAUST (EGT)");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_helvB14_tr);
  snprintf(buf, sizeof(buf), "H2O:  %.1f \xb0\x43", telemetry.water_temp_c);
  u8g2->drawStr(214, 76, buf);

  snprintf(buf, sizeof(buf), "EGT:  %d \xb0\x43", (int)telemetry.exhaust_temp_c);
  u8g2->drawStr(214, 106, buf);

  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "Alerts: H2O>%.0f\xb0 / EGT>%.0f\xb0", 
           settings.water_temp_alarm_c, settings.exhaust_temp_alarm_c);
  u8g2->drawStr(214, 134, buf);

  // ==========================================
  // CARD 3: G-FORCE & VEHICLE DYNAMICS (Bottom-Left)
  // ==========================================
  u8g2->drawRFrame(10, 148, 185, 118, 4);
  u8g2->drawBox(10, 148, 185, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(16, 160, "ACCELERATION & G-FORCE");
  u8g2->setDrawColor(1);

  // G-G Diagram crosshair
  int gx = 55;
  int gy = 208;
  u8g2->drawCircle(gx, gy, 28);
  u8g2->drawHLine(gx - 32, gy, 64);
  u8g2->drawVLine(gx, gy - 32, 64);

  // Current G dot
  int dot_x = gx + (int)(telemetry.lateral_g * 14.0f);
  int dot_y = gy - (int)(telemetry.longitudinal_g * 14.0f);
  if (dot_x < gx - 27) dot_x = gx - 27;
  if (dot_x > gx + 27) dot_x = gx + 27;
  if (dot_y < gy - 27) dot_y = gy - 27;
  if (dot_y > gy + 27) dot_y = gy + 27;
  u8g2->drawDisc(dot_x, dot_y, 3);

  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "Lat: %+0.2f G", telemetry.lateral_g);
  u8g2->drawStr(100, 195, buf);
  snprintf(buf, sizeof(buf), "Lon: %+0.2f G", telemetry.longitudinal_g);
  u8g2->drawStr(100, 220, buf);
  snprintf(buf, sizeof(buf), "Peak: 1.85 G");
  u8g2->drawStr(100, 245, buf);

  // ==========================================
  // CARD 4: TIMING & SPLITS (Bottom-Right)
  // ==========================================
  u8g2->drawRFrame(205, 148, 185, 118, 4);
  u8g2->drawBox(205, 148, 185, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(211, 160, "TIMING & SECTOR SPLITS");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_helvB10_tr);
  if (telemetry.best_lap_time_ms > 0) {
    uint32_t b_sec = (telemetry.best_lap_time_ms % 60000) / 1000;
    uint32_t b_cen = (telemetry.best_lap_time_ms % 1000) / 10;
    snprintf(buf, sizeof(buf), "Best: %02lu.%02lu s", (unsigned long)b_sec, (unsigned long)b_cen);
  } else {
    snprintf(buf, sizeof(buf), "Best: --.-- s");
  }
  u8g2->drawStr(214, 186, buf);

  if (telemetry.last_lap_time_ms > 0) {
    uint32_t l_sec = (telemetry.last_lap_time_ms % 60000) / 1000;
    uint32_t l_cen = (telemetry.last_lap_time_ms % 1000) / 10;
    snprintf(buf, sizeof(buf), "Last: %02lu.%02lu s", (unsigned long)l_sec, (unsigned long)l_cen);
  } else {
    snprintf(buf, sizeof(buf), "Last: --.-- s");
  }
  u8g2->drawStr(214, 210, buf);

  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "Sector 1: 15.98s  [-0.12s]");
  u8g2->drawStr(214, 232, buf);
  snprintf(buf, sizeof(buf), "Sector 2: 16.04s  [+0.05s]");
  u8g2->drawStr(214, 252, buf);
}
