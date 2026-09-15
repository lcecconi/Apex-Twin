#include "ui/page_data_recall.h"

void PageDataRecall::render(U8G2 *u8g2, const TelemetryProvider &provider, const SystemSettings &settings) {
  char buf[64];

  // Header Title
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(10, 20, "SESSION DATA RECALL");
  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "TOTAL LAPS: %u", provider.getCompletedLapCount());
  u8g2->drawStr(290, 20, buf);
  u8g2->drawHLine(10, 26, 380);

  // ==========================================
  // TOP 3 BEST LAPS TABLE
  // ==========================================
  u8g2->drawRFrame(10, 32, 380, 116, 4);
  u8g2->drawBox(10, 32, 380, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(16, 44, "TOP 3 BEST LAPS COMPARISON");
  u8g2->setDrawColor(1);

  // Table Column Headers
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(20, 62, "RANK");
  u8g2->drawStr(65, 62, "LAP #");
  u8g2->drawStr(115, 62, "LAP TIME");
  u8g2->drawStr(190, 62, "S1 / S2 / S3");
  u8g2->drawStr(280, 62, "TOP SPD");
  u8g2->drawStr(340, 62, "MAX RPM");
  u8g2->drawHLine(16, 66, 368);

  // Render top laps
  uint16_t total = provider.getCompletedLapCount();
  for (int i = 0; i < 3; i++) {
    int y = 84 + (i * 20);
    const LapRecord *lap = provider.getLapRecord(i);
    if (lap) {
      uint32_t sec = (lap->lap_time_ms % 60000) / 1000;
      uint32_t cen = (lap->lap_time_ms % 1000) / 10;

      snprintf(buf, sizeof(buf), "#%d", i + 1);
      u8g2->drawStr(24, y, buf);

      snprintf(buf, sizeof(buf), "L%02d", lap->lap_number);
      u8g2->drawStr(70, y, buf);

      snprintf(buf, sizeof(buf), "%02lu.%02lus", (unsigned long)sec, (unsigned long)cen);
      u8g2->drawStr(115, y, buf);

      snprintf(buf, sizeof(buf), "16.0 / 16.0 / 15.9");
      u8g2->drawStr(186, y, buf);

      snprintf(buf, sizeof(buf), "%.1f", lap->max_speed_kmh);
      u8g2->drawStr(284, y, buf);

      snprintf(buf, sizeof(buf), "%u", lap->max_rpm);
      u8g2->drawStr(340, y, buf);
    } else {
      snprintf(buf, sizeof(buf), "#%d   --      --.--s        -- / -- / --      ---      -----", i + 1);
      u8g2->drawStr(24, y, buf);
    }
  }

  // ==========================================
  // SESSION SUMMARY & THEORETICAL BEST
  // ==========================================
  u8g2->drawRFrame(10, 156, 380, 108, 4);
  u8g2->drawBox(10, 156, 380, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(16, 168, "SESSION TELEMETRY STATS & THEORETICAL BEST");
  u8g2->setDrawColor(1);

  const LapRecord *best = provider.getBestLap();
  u8g2->setFont(u8g2_font_helvB10_tr);
  if (best) {
    uint32_t b_sec = (best->lap_time_ms % 60000) / 1000;
    uint32_t b_cen = (best->lap_time_ms % 1000) / 10;
    snprintf(buf, sizeof(buf), "Session Best Lap:  L%02d  (%02lu.%02lu s)", 
             best->lap_number, (unsigned long)b_sec, (unsigned long)b_cen);
  } else {
    snprintf(buf, sizeof(buf), "Session Best Lap:  None");
  }
  u8g2->drawStr(20, 194, buf);

  u8g2->setFont(u8g2_font_6x12_tr);
  u8g2->drawStr(20, 216, "Optimal Theoretical Lap:   47.85 s (Sum of Best Sectors S1+S2+S3)");
  u8g2->drawStr(20, 234, "Peak Cornering G-Force:    1.85 G (Turn 4 Chicane entry)");
  u8g2->drawStr(20, 252, "Session Consistency Index: 98.4 % (Laps within 0.5s of best)");
}
