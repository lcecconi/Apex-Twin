#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "telemetry_data.h"

#define APEX_PACKET_MAGIC 0x41504558 // "APEX" in ASCII

// Binary packet structure broadcasted by Apex-Track at 25 Hz
struct __attribute__((packed)) ApexTrackTelemetryPacket {
  uint32_t magic;
  uint32_t packet_seq;
  uint32_t timestamp_ms;

  // Engine
  uint16_t rpm;
  float speed_kmh;
  uint8_t gear;
  float water_temp_c;
  float exhaust_temp_c;

  // Dynamics (IMU)
  float lateral_g;
  float longitudinal_g;

  // GNSS
  double latitude;
  double longitude;
  float altitude_m;
  uint8_t satellites;
  uint8_t fix_type;
  float hdop;

  // Lapping
  uint16_t lap_number;
  uint32_t current_lap_time_ms;
  uint32_t last_lap_time_ms;
  uint32_t best_lap_time_ms;
  float predictive_delta_s;
  uint8_t sector;
  uint16_t error_code;
};

class EspNowReceiver {
public:
  void begin();
  bool isConnected() const;
  int8_t getRssi() const;
  bool applyLatestTelemetry(TelemetrySnapshot &target);

  static void onDataRecv(const uint8_t *mac, const uint8_t *data, int len);

private:
  static ApexTrackTelemetryPacket _latest_packet;
  static uint32_t _last_packet_ms;
  static bool _new_packet_ready;
  static int8_t _last_rssi;
};
