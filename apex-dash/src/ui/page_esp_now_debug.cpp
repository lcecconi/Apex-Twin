#include "ui/page_esp_now_debug.h"

void PageEspNowDebug::render(U8G2 *u8g2, const TelemetrySnapshot &telemetry,
                             const EspNowDebugStats &stats) {
  char line[64];
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(10, 20, "ESP-NOW DEBUG");
  u8g2->drawHLine(10, 26, 380);

  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(line, sizeof(line), "INIT: %s   LINK: %s   RSSI: %d dBm",
           stats.initialized ? "OK" : "FAIL",
           stats.receiving && telemetry.track_module_connected ? "RX" : "WAIT",
           stats.rssi);
  u8g2->drawStr(10, 45, line);
  snprintf(line, sizeof(line), "Packets: %lu   Invalid: %lu",
           (unsigned long)stats.packets_received,
           (unsigned long)stats.invalid_packets);
  u8g2->drawStr(10, 62, line);
  snprintf(line, sizeof(line), "Packet rate: %0.1f Hz", stats.packet_rate_hz);
  u8g2->drawStr(10, 79, line);
  snprintf(line, sizeof(line), "Last seq: %u   Frames: %u   Length: %u",
           stats.last_sequence, stats.last_frame_count, stats.last_packet_length);
  u8g2->drawStr(10, 96, line);
  snprintf(line, sizeof(line), "Track time: %lu us",
           (unsigned long)stats.last_frame_timestamp_us);
  u8g2->drawStr(10, 113, line);
  snprintf(line, sizeof(line), "Last packet: %lu ms ago",
           stats.last_packet_ms ? (unsigned long)(millis() - stats.last_packet_ms) : 0UL);
  u8g2->drawStr(10, 130, line);

  u8g2->drawFrame(10, 146, 380, 78);
  snprintf(line, sizeof(line), "RPM: %u   Speed: %0.1f km/h   Gear: %u",
           telemetry.rpm, telemetry.speed_kmh, telemetry.gear);
  u8g2->drawStr(18, 164, line);
  snprintf(line, sizeof(line), "GPS: fix=%u sats=%u HDOP=%0.2f",
           telemetry.gps_fix, telemetry.satellites_visible, telemetry.hdop);
  u8g2->drawStr(18, 182, line);
  snprintf(line, sizeof(line), "Lat: %0.6f", telemetry.chassis.latitude);
  u8g2->drawStr(18, 200, line);
  snprintf(line, sizeof(line), "Lon: %0.6f   Alt: %0.1f m",
           telemetry.chassis.longitude, telemetry.chassis.altitude_m);
  u8g2->drawStr(18, 218, line);
  snprintf(line, sizeof(line), "Water: %0.1f C   EGT: %0.1f C",
           telemetry.water_temp_c, telemetry.exhaust_temp_c);
  u8g2->drawStr(18, 236, line);
  u8g2->drawStr(10, 250, "KEY: next page   BOOT: previous page");
}
