#pragma once

#include <stdint.h>
#include <stddef.h>

#define ESPNOW_MAGIC_BYTE 0xAE
#define ESPNOW_MAX_TOTAL_PAYLOAD 250

// Packet Types
enum EspNowPacketType : uint8_t {
  PKT_DISCOVERY_PING = 0x01,
  PKT_DISCOVERY_PONG = 0x02,
  PKT_LATENCY_PING   = 0x10,
  PKT_LATENCY_PONG   = 0x11,
  PKT_CONFIG_CHANGE  = 0x20,
  PKT_CONFIG_ACK     = 0x21
};

// Packed Latency Measurement Frame
struct __attribute__((packed)) EspNowLatencyPacket {
  uint8_t magic;           // 0xAE
  uint8_t type;            // EspNowPacketType
  uint16_t seq;            // Monotonic sequence number
  uint64_t t_send_us;      // Timestamp when Initiator transmits ping
  uint64_t t_recv_us;      // Timestamp when Responder receives ping
  uint64_t t_echo_us;      // Timestamp when Responder transmits echo
  uint16_t payload_len;    // Total frame length in bytes
  uint8_t dummy_payload[228]; // Variable padding to test various packet sizes
};

// Packet Config Command Frame (used to synchronize channel or settings)
struct __attribute__((packed)) EspNowConfigPacket {
  uint8_t magic;
  uint8_t type;
  uint8_t target_channel;
  uint8_t long_range_mode;
  uint16_t test_payload_size;
  uint16_t target_hz;
};

// Summary Statistics Model
struct LatencyStats {
  uint32_t packets_sent = 0;
  uint32_t packets_received = 0;
  uint32_t packets_lost = 0;
  uint32_t send_errors = 0;

  uint32_t rtt_min_us = 0xFFFFFFFF;
  uint32_t rtt_max_us = 0;
  uint64_t rtt_sum_us = 0;
  uint64_t rtt_sq_sum_us = 0;

  uint32_t ota_min_us = 0xFFFFFFFF;
  uint32_t ota_max_us = 0;
  uint64_t ota_sum_us = 0;

  uint32_t responder_proc_min_us = 0xFFFFFFFF;
  uint32_t responder_proc_max_us = 0;
  uint64_t responder_proc_sum_us = 0;

  // Histogram buckets (in microseconds)
  uint32_t bucket_under_500us = 0;   // < 500 us
  uint32_t bucket_500_to_1000us = 0; // 0.5 - 1.0 ms
  uint32_t bucket_1_to_2ms = 0;      // 1.0 - 2.0 ms
  uint32_t bucket_2_to_3ms = 0;      // 2.0 - 3.0 ms
  uint32_t bucket_3_to_5ms = 0;      // 3.0 - 5.0 ms
  uint32_t bucket_5_to_10ms = 0;     // 5.0 - 10.0 ms
  uint32_t bucket_over_10ms = 0;     // > 10.0 ms

  void reset() {
    packets_sent = 0;
    packets_received = 0;
    packets_lost = 0;
    send_errors = 0;
    rtt_min_us = 0xFFFFFFFF;
    rtt_max_us = 0;
    rtt_sum_us = 0;
    rtt_sq_sum_us = 0;
    ota_min_us = 0xFFFFFFFF;
    ota_max_us = 0;
    ota_sum_us = 0;
    responder_proc_min_us = 0xFFFFFFFF;
    responder_proc_max_us = 0;
    responder_proc_sum_us = 0;
    bucket_under_500us = 0;
    bucket_500_to_1000us = 0;
    bucket_1_to_2ms = 0;
    bucket_2_to_3ms = 0;
    bucket_3_to_5ms = 0;
    bucket_5_to_10ms = 0;
    bucket_over_10ms = 0;
  }
};
