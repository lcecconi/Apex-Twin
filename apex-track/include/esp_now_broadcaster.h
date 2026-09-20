#pragma once

#include <stdint.h>
#include <esp_now.h>
#include "telemetry_can.h"
#include "config.h"

// Container for up to 3 Virtual CAN-FD frames bundled in a single ESP-NOW radio burst
struct __attribute__((packed)) ApexCanFdPacket {
  uint32_t magic;       // APEX_CAN_FD_MAGIC (0x41434644)
  uint16_t seq;         // Sequence counter
  uint8_t  frame_count; // Number of valid CAN-FD frames (1 - 3)
  uint8_t  reserved;
  CanFdFrame frames[3]; // Up to 3 frames (~210 bytes)
};

class EspNowBroadcaster {
public:
  EspNowBroadcaster();
  bool begin();

  bool sendFastAndImuDynamics(const CanFastDynamicsPayload &fast, const CanImuDynamicsPayload &imu);
  bool sendSectorEvent(const CanSectorEventPayload &sector);
  bool sendEngineThermal(const CanEngineThermalPayload &thermal);
  bool sendGnssStatus(const CanGnssStatusPayload &gnss);
  bool sendChassisHealth(const CanChassisHealthPayload &health);

  uint32_t getPacketsSent() const { return _packets_sent; }
  uint32_t getPacketsFailed() const { return _packets_failed; }

  // Dash uplink telemetry status
  bool isDashConnected() const;
  uint16_t getDashBatteryMv() const { return _dash_battery_mv; }
  uint8_t getDashBatteryPct() const { return _dash_battery_pct; }
  float getDashAmbientTemp() const { return _dash_ambient_temp_c; }
  uint8_t getDashAmbientHum() const { return _dash_ambient_hum_pct; }

  // Static ESP-NOW callback handlers
  static void onDataSent(const uint8_t *mac, esp_now_send_status_t status);
  static void onDataRecv(const uint8_t *mac, const uint8_t *data, int len);

private:
  bool sendPacket(const ApexCanFdPacket &pkt, uint8_t frame_count);

  static uint8_t _broadcast_mac[6];
  static uint16_t _seq;
  static uint32_t _packets_sent;
  static uint32_t _packets_failed;

  // Dash uplink state
  static uint32_t _last_dash_packet_ms;
  static uint16_t _dash_battery_mv;
  static uint8_t  _dash_battery_pct;
  static float    _dash_ambient_temp_c;
  static uint8_t  _dash_ambient_hum_pct;
  static uint32_t _dash_rtc_epoch_s;
};
