#include "ui/page_gps_paddock.h"

static const char *gpsFixName(uint8_t fix) {
  switch (fix) {
    case 0: return "No Fix";
    case 1: return "GPS";
    case 2: return "DGPS";
    case 3: return "PPS";
    case 4: return "RTK Fixed";
    case 5: return "RTK Float";
    case 6: return "Estimated";
    case 7: return "Manual";
    case 8: return "Simulation";
    default: return "Unknown";
  }
}

void PageGpsPaddock::render(U8G2 *u8g2, const TelemetrySnapshot &telemetry, const SystemSettings &settings) {
  char buf[64];

  // Header Title
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(10, 20, "PADDOCK & PRE-RACE STATUS");
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(290, 20, "[STANDBY MODE]");
  u8g2->drawHLine(10, 26, 380);

  // ==========================================
  // CARD 1: GNSS SATELLITE CONSTELLATION (Left)
  // ==========================================
  u8g2->drawRFrame(10, 32, 185, 130, 4);
  u8g2->drawBox(10, 32, 185, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(16, 44, "GNSS SATELLITE RADAR");
  u8g2->setDrawColor(1);

  // Satellite Signal Bar Chart (up to 10 channels, refreshed every 10 seconds)
  for (int i = 0; i < 10; i++) {
    int bx = 18 + (i * 17);
    uint8_t snr = i < telemetry.satellite_snr_count ? telemetry.satellite_snr[i] : 0;
    int bar_h = (snr * 38) / 50;
    u8g2->drawFrame(bx, 96 - bar_h, 12, bar_h);
    u8g2->drawBox(bx, 96 - bar_h, 12, bar_h);
  }
  u8g2->drawHLine(16, 97, 172);

  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "Sats Visible: %d  (GPS/Gal)", telemetry.satellites_visible);
  u8g2->drawStr(16, 114, buf);

  snprintf(buf, sizeof(buf), "Fix: %s  |  HDOP: %0.2f",
           gpsFixName(telemetry.gps_fix), telemetry.hdop);
  u8g2->drawStr(16, 132, buf);

  u8g2->drawStr(16, 150, "Antenna: Active Helix L1/L5");

  // ==========================================
  // CARD 2: TRACK RECOGNITION (Right)
  // ==========================================
  u8g2->drawRFrame(205, 32, 185, 130, 4);
  u8g2->drawBox(205, 32, 185, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(211, 44, "TRACK AUTO-DETECTION");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(214, 70, telemetry.current_track_name);

  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(214, 92, "Distance to S/F: 12 m");
  u8g2->drawStr(214, 110, "Status: Ready to Race");
  u8g2->drawStr(214, 128, "Auto-Trip: Speed > 15 km/h");
  u8g2->drawStr(214, 148, "Start Line: 45.3882N 10.4918E");

  // ==========================================
  // CARD 3: AMBIENT & ENGINE MAINTENANCE (Bottom)
  // ==========================================
  u8g2->drawRFrame(10, 168, 380, 96, 4);
  u8g2->drawBox(10, 168, 380, 16);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(16, 180, "AMBIENT WEATHER & ENGINE MAINTENANCE");
  u8g2->setDrawColor(1);

  // Left col: Weather
  u8g2->setFont(u8g2_font_6x12_tr);
  float amb_temp = settings.use_celsius ? telemetry.ambient_temp_c : (telemetry.ambient_temp_c * 1.8f + 32.0f);
  snprintf(buf, sizeof(buf), "Track Ambient Temp:    %+d", (int)roundf(amb_temp));
  u8g2->drawStr(20, 204, buf);

  snprintf(buf, sizeof(buf), "Relative Humidity:     %.1f %% RH", telemetry.ambient_humidity_pct);
  u8g2->drawStr(20, 224, buf);

  snprintf(buf, sizeof(buf), "Steering Battery:      %.2f V  (%d%%)", telemetry.battery_voltage, telemetry.battery_percent);
  u8g2->drawStr(20, 244, buf);

  // Right col: Maintenance
  uint32_t eng_hrs = telemetry.engine_total_hours_sec / 3600;
  uint32_t eng_min = (telemetry.engine_total_hours_sec % 3600) / 60;
  snprintf(buf, sizeof(buf), "Engine Total:  %lu h %02lu m", (unsigned long)eng_hrs, (unsigned long)eng_min);
  u8g2->drawStr(240, 204, buf);

  uint32_t pis_hrs = telemetry.piston_hours_sec / 3600;
  uint32_t pis_min = (telemetry.piston_hours_sec % 3600) / 60;
  snprintf(buf, sizeof(buf), "Piston Run:    %lu h %02lu m", (unsigned long)pis_hrs, (unsigned long)pis_min);
  u8g2->drawStr(240, 224, buf);

  u8g2->drawStr(240, 244, "Apex-Track:    2.4GHz Link OK");
}
