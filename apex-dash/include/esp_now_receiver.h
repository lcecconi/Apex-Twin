#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "telemetry_can.h"
#include "telemetry_data.h"
#include "telemetry_transport.h"

#define APEX_CAN_FD_MAGIC 0x41434644 // "ACFD"
#define APEX_LEGACY_MAGIC 0x41504558 // "APEX"

// Container for up to 3 Virtual CAN-FD frames bundled in a single ESP-NOW radio burst
struct __attribute__((packed)) ApexCanFdPacket {
  uint32_t magic;       // APEX_CAN_FD_MAGIC
  uint16_t seq;         // Sequence counter
  uint8_t  frame_count; // Number of valid CAN-FD frames (1 - 3)
  uint8_t  reserved;
  CanFdFrame frames[3]; // Up to 3 frames (~210 bytes, well within 250 byte ESP-NOW limit)
};

class EspNowReceiver : public ITelemetryTransport {
public:
  EspNowReceiver();
  virtual ~EspNowReceiver() {}

  // ITelemetryTransport interface
  bool begin() override;
  bool isConnected() const override;
  int8_t getRssi() const override;
  bool sendFrame(const CanFdFrame &frame) override;
  void setReceiveCallback(CanFrameReceiveCallback cb, void *user_arg = nullptr) override;

  // Telemetry application
  bool applyLatestTelemetry(TelemetrySnapshot &target);
  const ChassisTelemetry &getChassisTelemetry() const { return _chassis; }

  // Send command from Dash to Track
  bool sendDashCommand(uint8_t cmd_id, uint8_t sub_param = 0, uint32_t param = 0, const char *track_id = nullptr);
  bool sendDashStatus(uint16_t battery_mv, uint8_t battery_pct, float amb_temp_c, uint8_t amb_hum_pct, uint32_t epoch_s);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  static void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);
#else
  static void onDataRecv(const uint8_t *mac, const uint8_t *data, int len);
#endif

private:
  static void processIncomingCanFrame(const CanFdFrame &frame);

  static ChassisTelemetry _chassis;
  static uint32_t _last_packet_ms;
  static bool _new_packet_ready;
  static int8_t _last_rssi;
  static uint8_t _peer_mac[6];
  static bool _peer_registered;
  static uint16_t _tx_seq;

  static CanFrameReceiveCallback _rx_callback;
  static void *_rx_callback_arg;
};
