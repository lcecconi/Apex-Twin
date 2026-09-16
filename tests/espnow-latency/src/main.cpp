#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_timer.h>
#include <math.h>
#include "espnow_protocol.h"

// =============================================================================
// Operating Roles & Defaults
// =============================================================================
enum DeviceRole {
  ROLE_UNSET = 0,
  ROLE_INITIATOR = 1,
  ROLE_RESPONDER = 2
};

#if defined(ROLE_INITIATOR_DEFAULT)
static DeviceRole current_role = ROLE_INITIATOR;
#elif defined(ROLE_RESPONDER_DEFAULT)
static DeviceRole current_role = ROLE_RESPONDER;
#else
static DeviceRole current_role = ROLE_UNSET;
#endif

// Configuration parameters
static uint8_t  wifi_channel = 1;
static uint16_t current_payload_size = 64;   // Default payload size in bytes
static uint32_t current_rate_hz = 25;        // Default 25 Hz (matching Apex-Twin telemetry rate)
static bool     use_broadcast = false;       // Unicast (hardware ACK) vs Broadcast
static bool     long_range_mode = false;     // WiFi Long Range (LR) mode
static bool     csv_stream_mode = false;     // Stream every sample as CSV line
static uint32_t summary_interval_ms = 2000;  // Periodic summary print interval

// Auto Benchmark state
static bool     auto_benchmark_active = false;
static uint8_t  bench_rate_idx = 0;
static uint8_t  bench_payload_idx = 0;
static uint32_t bench_packets_collected = 0;
static const uint32_t BENCH_PACKETS_PER_STEP = 200;
static const uint32_t BENCH_RATES[] = { 10, 25, 50, 100, 200, 500 };
static const uint16_t BENCH_PAYLOADS[] = { 32, 64, 128, 240 };
#define NUM_BENCH_RATES (sizeof(BENCH_RATES) / sizeof(BENCH_RATES[0]))
#define NUM_BENCH_PAYLOADS (sizeof(BENCH_PAYLOADS) / sizeof(BENCH_PAYLOADS[0]))

struct BenchmarkResult {
  uint32_t rate_hz;
  uint16_t payload_bytes;
  uint32_t sent;
  uint32_t received;
  float loss_pct;
  float min_rtt_ms;
  float avg_rtt_ms;
  float max_rtt_ms;
  float jitter_ms;
  float throughput_kb_s;
};
static BenchmarkResult bench_results[NUM_BENCH_RATES * NUM_BENCH_PAYLOADS];
static uint16_t bench_results_count = 0;

// Communication & Peers
static uint8_t broadcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t peer_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static bool    peer_paired = false;
static uint8_t my_mac[6];

// Send & Receive State
static EspNowLatencyPacket tx_packet;
static EspNowLatencyPacket rx_packet;
static LatencyStats stats;
static uint16_t current_seq = 0;
static uint64_t last_ping_sent_us = 0;
static bool     waiting_for_pong = false;
static uint64_t last_summary_ms = 0;
static volatile bool tx_callback_status = true;
static volatile uint64_t last_tx_cb_us = 0;

// =============================================================================
// Helper Functions: Peer Management & Radio Control
// =============================================================================
static bool add_or_update_peer(const uint8_t *mac, uint8_t channel) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = channel;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;

  if (esp_now_is_peer_exist(mac)) {
    esp_now_del_peer(mac);
  }
  return (esp_now_add_peer(&peerInfo) == ESP_OK);
}

static void apply_wifi_protocol_and_channel() {
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (long_range_mode) {
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
  } else {
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  }

  // Ensure peer is re-registered on the active channel
  if (peer_paired) {
    add_or_update_peer(peer_mac, wifi_channel);
  }
  add_or_update_peer(broadcast_mac, wifi_channel);
}

// =============================================================================
// ESP-NOW Callbacks
// =============================================================================
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status)
#else
static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status)
#endif
{
  last_tx_cb_us = esp_timer_get_time();
  tx_callback_status = (status == ESP_NOW_SEND_SUCCESS);
  if (status != ESP_NOW_SEND_SUCCESS && current_role == ROLE_INITIATOR) {
    stats.send_errors++;
  }
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void on_data_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *mac_addr = info->src_addr;
#else
static void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) {
#endif
  uint64_t now_us = esp_timer_get_time();

  if (len < 4) return;
  if (data[0] != ESPNOW_MAGIC_BYTE) return;

  uint8_t pkt_type = data[1];

  // ---------------------------------------------------------------------------
  // RESPONDER BEHAVIOR
  // ---------------------------------------------------------------------------
  if (current_role == ROLE_RESPONDER || current_role == ROLE_UNSET) {
    if (pkt_type == PKT_DISCOVERY_PING || pkt_type == PKT_LATENCY_PING) {
      if (current_role == ROLE_UNSET) {
        current_role = ROLE_RESPONDER;
        Serial.printf("[AUTO] Configured as RESPONDER after receiving packet from %02X:%02X:%02X:%02X:%02X:%02X\n",
                      mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
      }

      // Automatically register the sender as our peer if not present
      if (!peer_paired || memcmp(peer_mac, mac_addr, 6) != 0) {
        memcpy(peer_mac, mac_addr, 6);
        peer_paired = true;
        add_or_update_peer(peer_mac, wifi_channel);
      }

      if (pkt_type == PKT_DISCOVERY_PING) {
        // Reply with discovery pong
        EspNowLatencyPacket disc_pong = {};
        disc_pong.magic = ESPNOW_MAGIC_BYTE;
        disc_pong.type = PKT_DISCOVERY_PONG;
        disc_pong.t_recv_us = now_us;
        disc_pong.t_echo_us = esp_timer_get_time();
        disc_pong.payload_len = sizeof(EspNowLatencyPacket);
        esp_now_send(peer_mac, (uint8_t *)&disc_pong, sizeof(EspNowLatencyPacket));
        return;
      }

      if (pkt_type == PKT_LATENCY_PING) {
        const EspNowLatencyPacket *req = (const EspNowLatencyPacket *)data;
        EspNowLatencyPacket resp = {};
        resp.magic = ESPNOW_MAGIC_BYTE;
        resp.type = PKT_LATENCY_PONG;
        resp.seq = req->seq;
        resp.t_send_us = req->t_send_us;
        resp.t_recv_us = now_us;
        resp.t_echo_us = esp_timer_get_time();
        resp.payload_len = (uint16_t)len;

        // Echo back with exact same length to measure symmetric payload latency
        esp_now_send(peer_mac, (uint8_t *)&resp, len);
        stats.packets_received++;
      }
    }
    return;
  }

  // ---------------------------------------------------------------------------
  // INITIATOR BEHAVIOR
  // ---------------------------------------------------------------------------
  if (current_role == ROLE_INITIATOR) {
    if (pkt_type == PKT_DISCOVERY_PONG) {
      if (!peer_paired || memcmp(peer_mac, mac_addr, 6) != 0) {
        memcpy(peer_mac, mac_addr, 6);
        peer_paired = true;
        add_or_update_peer(peer_mac, wifi_channel);
        Serial.printf("\n\033[92m[INITIATOR] Successfully paired with Responder MAC: %02X:%02X:%02X:%02X:%02X:%02X\033[0m\n",
                      peer_mac[0], peer_mac[1], peer_mac[2], peer_mac[3], peer_mac[4], peer_mac[5]);
      }
      return;
    }

    if (pkt_type == PKT_LATENCY_PONG) {
      const EspNowLatencyPacket *pong = (const EspNowLatencyPacket *)data;
      if (pong->seq != current_seq) {
        // Out of order or old echo
        return;
      }

      waiting_for_pong = false;
      stats.packets_received++;

      uint64_t rtt_us = (now_us > pong->t_send_us) ? (now_us - pong->t_send_us) : 0;
      uint64_t resp_proc_us = (pong->t_echo_us >= pong->t_recv_us) ? (pong->t_echo_us - pong->t_recv_us) : 0;
      uint64_t ota_rtt_us = (rtt_us > resp_proc_us) ? (rtt_us - resp_proc_us) : rtt_us;
      uint64_t one_way_us = ota_rtt_us / 2;

      // Update RTT statistics
      if (rtt_us < stats.rtt_min_us) stats.rtt_min_us = (uint32_t)rtt_us;
      if (rtt_us > stats.rtt_max_us) stats.rtt_max_us = (uint32_t)rtt_us;
      stats.rtt_sum_us += rtt_us;
      stats.rtt_sq_sum_us += (rtt_us * rtt_us);

      // Update OTA statistics
      if (ota_rtt_us < stats.ota_min_us) stats.ota_min_us = (uint32_t)ota_rtt_us;
      if (ota_rtt_us > stats.ota_max_us) stats.ota_max_us = (uint32_t)ota_rtt_us;
      stats.ota_sum_us += ota_rtt_us;

      // Update Responder processing overhead stats
      if (resp_proc_us < stats.responder_proc_min_us) stats.responder_proc_min_us = (uint32_t)resp_proc_us;
      if (resp_proc_us > stats.responder_proc_max_us) stats.responder_proc_max_us = (uint32_t)resp_proc_us;
      stats.responder_proc_sum_us += resp_proc_us;

      // Histogram bins
      if (rtt_us < 500) {
        stats.bucket_under_500us++;
      } else if (rtt_us < 1000) {
        stats.bucket_500_to_1000us++;
      } else if (rtt_us < 2000) {
        stats.bucket_1_to_2ms++;
      } else if (rtt_us < 3000) {
        stats.bucket_2_to_3ms++;
      } else if (rtt_us < 5000) {
        stats.bucket_3_to_5ms++;
      } else if (rtt_us < 10000) {
        stats.bucket_5_to_10ms++;
      } else {
        stats.bucket_over_10ms++;
      }

      if (csv_stream_mode) {
        Serial.printf("%u,%u,%llu,%llu,%llu,%llu,%u\n",
                      pong->seq, len, (unsigned long long)rtt_us,
                      (unsigned long long)ota_rtt_us, (unsigned long long)one_way_us,
                      (unsigned long long)resp_proc_us, tx_callback_status ? 1 : 0);
      }

      if (auto_benchmark_active) {
        bench_packets_collected++;
      }
    }
  }
}

// =============================================================================
// Transmit Ping Engine
// =============================================================================
static void send_latency_ping() {
  if (waiting_for_pong) {
    stats.packets_lost++;
    if (auto_benchmark_active) {
      bench_packets_collected++;
    }
  }

  current_seq++;
  tx_packet.magic = ESPNOW_MAGIC_BYTE;
  tx_packet.type = PKT_LATENCY_PING;
  tx_packet.seq = current_seq;
  tx_packet.t_send_us = esp_timer_get_time();
  tx_packet.t_recv_us = 0;
  tx_packet.t_echo_us = 0;
  tx_packet.payload_len = current_payload_size;

  // Fill dummy payload pattern
  for (uint16_t i = 0; i < sizeof(tx_packet.dummy_payload); i++) {
    tx_packet.dummy_payload[i] = (uint8_t)(i ^ 0x5A);
  }

  const uint8_t *target = (use_broadcast || !peer_paired) ? broadcast_mac : peer_mac;
  esp_err_t res = esp_now_send(target, (uint8_t *)&tx_packet, current_payload_size);

  stats.packets_sent++;
  if (res == ESP_OK) {
    waiting_for_pong = true;
    last_ping_sent_us = tx_packet.t_send_us;
  } else {
    stats.send_errors++;
    waiting_for_pong = false;
  }
}

static void send_discovery_ping() {
  EspNowLatencyPacket disc = {};
  disc.magic = ESPNOW_MAGIC_BYTE;
  disc.type = PKT_DISCOVERY_PING;
  disc.seq = 0;
  disc.t_send_us = esp_timer_get_time();
  disc.payload_len = sizeof(EspNowLatencyPacket);
  esp_now_send(broadcast_mac, (uint8_t *)&disc, sizeof(EspNowLatencyPacket));
}

// =============================================================================
// Output Reporting & Histograms
// =============================================================================
static void print_banner_and_status() {
  Serial.println("\033[2J\033[H"); // Clear screen & home cursor
  Serial.println("\033[96m================================================================================");
  Serial.println("         APEX-TWIN ESP-NOW LATENCY & JITTER BENCHMARK SUITE");
  Serial.println("================================================================================\033[0m");
  Serial.printf("Device MAC:        \033[1m%02X:%02X:%02X:%02X:%02X:%02X\033[0m\n",
                my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]);
  Serial.printf("Operating Role:    \033[1;93m%s\033[0m\n",
                current_role == ROLE_INITIATOR ? "INITIATOR (Ping Master)" :
                current_role == ROLE_RESPONDER ? "RESPONDER (Echo Slave)" : "AUTO-DETECT (Listening)");
  Serial.printf("WiFi Channel:      %u | Long-Range (LR): %s\n", wifi_channel, long_range_mode ? "ENABLED" : "DISABLED");
  Serial.printf("Transmission Mode: %s (%s)\n",
                use_broadcast ? "BROADCAST" : "UNICAST (with 802.11 MAC ACK)",
                peer_paired ? "Peer Paired" : "Waiting for Peer");
  if (peer_paired) {
    Serial.printf("Paired Peer MAC:   %02X:%02X:%02X:%02X:%02X:%02X\n",
                  peer_mac[0], peer_mac[1], peer_mac[2], peer_mac[3], peer_mac[4], peer_mac[5]);
  }
  Serial.printf("Test Frequency:    \033[1m%u Hz\033[0m (Interval: %.1f ms)\n", current_rate_hz, 1000.0f / current_rate_hz);
  Serial.printf("Frame Payload:     \033[1m%u Bytes\033[0m (Max ESP-NOW: 250 Bytes)\n", current_payload_size);
  Serial.println("--------------------------------------------------------------------------------");
  Serial.println("Interactive Commands: [m] Initiator | [s] Responder | [a] Auto-Sweep | [f <hz>] Freq");
  Serial.println("                      [p <bytes>] Payload | [c <ch>] Channel | [b] Broadcast | [z] Reset");
  Serial.println("================================================================================\n");
}

static void print_periodic_stats() {
  if (stats.packets_sent == 0) return;

  uint32_t rcv = stats.packets_received;
  float loss_pct = (stats.packets_sent > 0) ? ((float)stats.packets_lost * 100.0f / (float)stats.packets_sent) : 0.0f;
  float avg_rtt_us = (rcv > 0) ? ((float)stats.rtt_sum_us / (float)rcv) : 0.0f;
  float avg_ota_us = (rcv > 0) ? ((float)stats.ota_sum_us / (float)rcv) : 0.0f;
  float avg_proc_us = (rcv > 0) ? ((float)stats.responder_proc_sum_us / (float)rcv) : 0.0f;

  // Jitter (Standard Deviation of RTT in microseconds)
  float variance = 0.0f;
  if (rcv > 1) {
    variance = ((float)stats.rtt_sq_sum_us - ((float)stats.rtt_sum_us * stats.rtt_sum_us / rcv)) / (rcv - 1);
    if (variance < 0.0f) variance = 0.0f;
  }
  float jitter_us = sqrtf(variance);

  // Bandwidth calculation (payload data rate in KB/s)
  float throughput_kb_s = (rcv > 0) ? (rcv * (float)current_payload_size / (summary_interval_ms / 1000.0f) / 1024.0f) : 0.0f;

  Serial.println("\n\033[96m+------------------------------------------------------------------------------+");
  Serial.printf("| LATENCY & PACKET STATISTICS SUMMARY (%u Hz, %u Bytes Payload)                 |\n", current_rate_hz, current_payload_size);
  Serial.println("+------------------------------------------------------------------------------+\033[0m");
  Serial.printf("| Sent: %-6u | Received: %-6u | Lost: %-6u (Loss: \033[1;%sm%.2f%%\033[0m) | Errors: %-4u |\n",
                stats.packets_sent, rcv, stats.packets_lost,
                loss_pct > 1.0f ? "91" : "92", loss_pct, stats.send_errors);
  Serial.println("+------------------------------------------------------------------------------+");
  Serial.printf("| \033[1mMetric\033[0m                   | \033[1mMin\033[0m           | \033[1mAverage\033[0m       | \033[1mMax\033[0m           | \033[1mJitter (StdDev)\033[0m |\n");
  Serial.println("+--------------------------+---------------+---------------+---------------+---------------+");
  Serial.printf("| Full Round-Trip (RTT)    | %7.2f ms    | %7.2f ms    | %7.2f ms    | %7.2f ms     |\n",
                stats.rtt_min_us / 1000.0f, avg_rtt_us / 1000.0f, stats.rtt_max_us / 1000.0f, jitter_us / 1000.0f);
  Serial.printf("| Over-The-Air (OTA) RTT   | %7.2f ms    | %7.2f ms    | %7.2f ms    |       -       |\n",
                stats.ota_min_us / 1000.0f, avg_ota_us / 1000.0f, stats.ota_max_us / 1000.0f);
  Serial.printf("| One-Way OTA (Estimated)  | %7.2f ms    | %7.2f ms    | %7.2f ms    |       -       |\n",
                stats.ota_min_us / 2000.0f, avg_ota_us / 2000.0f, stats.ota_max_us / 2000.0f);
  Serial.printf("| Responder Processing     | %7.1f us    | %7.1f us    | %7.1f us    |       -       |\n",
                (float)stats.responder_proc_min_us, avg_proc_us, (float)stats.responder_proc_max_us);
  Serial.println("+------------------------------------------------------------------------------+");
  Serial.printf("| Throughput: %-6.2f KB/s (%.1f packets/sec)                                    |\n",
                throughput_kb_s, (float)rcv / (summary_interval_ms / 1000.0f));
  Serial.println("+------------------------------------------------------------------------------+");

  // Latency Distribution Histogram
  Serial.println("| LATENCY DISTRIBUTION HISTOGRAM                                               |");
  Serial.printf("|   < 500 us   : %-6u  [%-40s] |\n", stats.bucket_under_500us,
                rcv ? String(std::string(stats.bucket_under_500us * 40 / rcv, '#').c_str()).c_str() : "");
  Serial.printf("|  0.5-1.0 ms  : %-6u  [%-40s] |\n", stats.bucket_500_to_1000us,
                rcv ? String(std::string(stats.bucket_500_to_1000us * 40 / rcv, '#').c_str()).c_str() : "");
  Serial.printf("|  1.0-2.0 ms  : %-6u  [%-40s] |\n", stats.bucket_1_to_2ms,
                rcv ? String(std::string(stats.bucket_1_to_2ms * 40 / rcv, '#').c_str()).c_str() : "");
  Serial.printf("|  2.0-3.0 ms  : %-6u  [%-40s] |\n", stats.bucket_2_to_3ms,
                rcv ? String(std::string(stats.bucket_2_to_3ms * 40 / rcv, '#').c_str()).c_str() : "");
  Serial.printf("|  3.0-5.0 ms  : %-6u  [%-40s] |\n", stats.bucket_3_to_5ms,
                rcv ? String(std::string(stats.bucket_3_to_5ms * 40 / rcv, '#').c_str()).c_str() : "");
  Serial.printf("|  5.0-10.0 ms : %-6u  [%-40s] |\n", stats.bucket_5_to_10ms,
                rcv ? String(std::string(stats.bucket_5_to_10ms * 40 / rcv, '#').c_str()).c_str() : "");
  Serial.printf("|  > 10.0 ms   : %-6u  [%-40s] |\n", stats.bucket_over_10ms,
                rcv ? String(std::string(stats.bucket_over_10ms * 40 / rcv, '#').c_str()).c_str() : "");
  Serial.println("\033[96m+------------------------------------------------------------------------------+\033[0m\n");
}

// =============================================================================
// Automated Full Benchmark Sweep
// =============================================================================
static void start_auto_benchmark() {
  auto_benchmark_active = true;
  bench_rate_idx = 0;
  bench_payload_idx = 0;
  bench_packets_collected = 0;
  bench_results_count = 0;
  current_rate_hz = BENCH_RATES[0];
  current_payload_size = BENCH_PAYLOADS[0];
  stats.reset();

  Serial.println("\n\033[93m================================================================================");
  Serial.println("           STARTING AUTOMATED ESP-NOW LATENCY MATRIX SWEEP");
  Serial.println("================================================================================\033[0m");
  Serial.printf("Rates (%d): 10Hz, 25Hz, 50Hz, 100Hz, 200Hz, 500Hz\n", NUM_BENCH_RATES);
  Serial.printf("Payloads (%d): 32B, 64B, 128B, 240B\n", NUM_BENCH_PAYLOADS);
  Serial.printf("Collecting %u samples per step...\n\n", BENCH_PACKETS_PER_STEP);
}

static void print_benchmark_final_report() {
  Serial.println("\n\n\033[92m======================================================================================================");
  Serial.println("                            AUTOMATED BENCHMARK RESULTS REPORT");
  Serial.println("======================================================================================================\033[0m");
  Serial.println("| Rate (Hz) | Payload (B) | Sent | Recv | Loss (%) | Min RTT (ms) | Avg RTT (ms) | Max RTT (ms) | Jitter (ms) | Throughput (KB/s) |");
  Serial.println("|-----------|-------------|------|------|----------|--------------|--------------|--------------|-------------|-------------------|");

  for (uint16_t i = 0; i < bench_results_count; i++) {
    const BenchmarkResult &r = bench_results[i];
    Serial.printf("| %-9u | %-11u | %-4u | %-4u | %7.2f%% | %12.2f | %12.2f | %12.2f | %11.2f | %17.2f |\n",
                  r.rate_hz, r.payload_bytes, r.sent, r.received, r.loss_pct,
                  r.min_rtt_ms, r.avg_rtt_ms, r.max_rtt_ms, r.jitter_ms, r.throughput_kb_s);
  }
  Serial.println("\033[92m======================================================================================================\033[0m\n");
}

static void progress_auto_benchmark() {
  if (!auto_benchmark_active) return;

  if (bench_packets_collected >= BENCH_PACKETS_PER_STEP) {
    // Record results for current step
    uint32_t rcv = stats.packets_received;
    float avg_rtt_ms = (rcv > 0) ? ((float)stats.rtt_sum_us / (float)rcv / 1000.0f) : 0.0f;
    float variance = 0.0f;
    if (rcv > 1) {
      variance = ((float)stats.rtt_sq_sum_us - ((float)stats.rtt_sum_us * stats.rtt_sum_us / rcv)) / (rcv - 1);
      if (variance < 0.0f) variance = 0.0f;
    }
    float jitter_ms = sqrtf(variance) / 1000.0f;
    float duration_s = (float)stats.packets_sent / (float)current_rate_hz;
    float throughput_kb_s = (rcv > 0 && duration_s > 0) ? (rcv * (float)current_payload_size / duration_s / 1024.0f) : 0.0f;

    BenchmarkResult &res = bench_results[bench_results_count++];
    res.rate_hz = current_rate_hz;
    res.payload_bytes = current_payload_size;
    res.sent = stats.packets_sent;
    res.received = rcv;
    res.loss_pct = (stats.packets_sent > 0) ? ((float)stats.packets_lost * 100.0f / (float)stats.packets_sent) : 0.0f;
    res.min_rtt_ms = stats.rtt_min_us / 1000.0f;
    res.avg_rtt_ms = avg_rtt_ms;
    res.max_rtt_ms = stats.rtt_max_us / 1000.0f;
    res.jitter_ms = jitter_ms;
    res.throughput_kb_s = throughput_kb_s;

    Serial.printf("Step [%u/%u] Complete: %u Hz | %u Bytes -> Avg RTT: %.2f ms | Jitter: %.2f ms | Loss: %.2f%%\n",
                  bench_results_count, NUM_BENCH_RATES * NUM_BENCH_PAYLOADS,
                  current_rate_hz, current_payload_size, avg_rtt_ms, jitter_ms, res.loss_pct);

    // Advance to next step
    bench_payload_idx++;
    if (bench_payload_idx >= NUM_BENCH_PAYLOADS) {
      bench_payload_idx = 0;
      bench_rate_idx++;
      if (bench_rate_idx >= NUM_BENCH_RATES) {
        // Benchmark completed!
        auto_benchmark_active = false;
        print_benchmark_final_report();
        current_rate_hz = 25;
        current_payload_size = 64;
        stats.reset();
        return;
      }
    }

    current_rate_hz = BENCH_RATES[bench_rate_idx];
    current_payload_size = BENCH_PAYLOADS[bench_payload_idx];
    bench_packets_collected = 0;
    stats.reset();
  }
}

// =============================================================================
// Interactive Serial Command Processing
// =============================================================================
static void handle_serial_command(const String &cmd_str) {
  String cmd = cmd_str;
  cmd.trim();
  if (cmd.length() == 0) return;

  char c = cmd[0];
  if (c == 'h' || c == '?') {
    print_banner_and_status();
  } else if (c == 'm' || c == 'i') {
    current_role = ROLE_INITIATOR;
    stats.reset();
    Serial.println("\n\033[92mSwitched to ROLE: INITIATOR (Ping Master)\033[0m");
    send_discovery_ping();
  } else if (c == 's' || c == 'r') {
    current_role = ROLE_RESPONDER;
    stats.reset();
    Serial.println("\n\033[93mSwitched to ROLE: RESPONDER (Echo Slave)\033[0m");
  } else if (c == 'a') {
    current_role = ROLE_INITIATOR;
    start_auto_benchmark();
  } else if (c == 'f') {
    int hz = cmd.substring(1).toInt();
    if (hz > 0 && hz <= 1000) {
      current_rate_hz = hz;
      Serial.printf("Rate updated to %u Hz\n", current_rate_hz);
    }
  } else if (c == 'p') {
    int sz = cmd.substring(1).toInt();
    if (sz >= 16 && sz <= ESPNOW_MAX_TOTAL_PAYLOAD) {
      current_payload_size = sz;
      Serial.printf("Payload size updated to %u Bytes\n", current_payload_size);
    }
  } else if (c == 'c') {
    int ch = cmd.substring(1).toInt();
    if (ch >= 1 && ch <= 13) {
      wifi_channel = ch;
      apply_wifi_protocol_and_channel();
      Serial.printf("WiFi channel set to %u\n", wifi_channel);
    }
  } else if (c == 'b') {
    use_broadcast = !use_broadcast;
    Serial.printf("Transmission mode: %s\n", use_broadcast ? "BROADCAST" : "UNICAST");
  } else if (c == 'l') {
    long_range_mode = !long_range_mode;
    apply_wifi_protocol_and_channel();
    Serial.printf("Long Range (LR) mode: %s\n", long_range_mode ? "ENABLED" : "DISABLED");
  } else if (c == 'v') {
    csv_stream_mode = !csv_stream_mode;
    if (csv_stream_mode) {
      Serial.println("\nseq,len,rtt_us,ota_rtt_us,one_way_us,proc_us,tx_ok");
    } else {
      Serial.println("\nDisabled CSV streaming mode.");
    }
  } else if (c == 'z') {
    stats.reset();
    Serial.println("Statistics reset.");
  }
}

// =============================================================================
// Arduino Setup & Main Loop
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // Initialize WiFi in Station Mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.macAddress(my_mac);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("\033[91m[ERROR] Failed to initialize ESP-NOW!\033[0m");
    while (1) delay(1000);
  }

  // Register Callbacks
  esp_now_register_send_cb(on_data_sent);
  esp_now_register_recv_cb(on_data_recv);

  apply_wifi_protocol_and_channel();
  print_banner_and_status();

  if (current_role == ROLE_INITIATOR) {
    send_discovery_ping();
  }
}

void loop() {
  uint32_t now_ms = millis();
  uint64_t now_us = esp_timer_get_time();

  // Read serial commands
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handle_serial_command(line);
  }

  // Initiator periodic transmission
  if (current_role == ROLE_INITIATOR) {
    uint32_t interval_us = 1000000 / current_rate_hz;
    if (now_us - last_ping_sent_us >= interval_us) {
      // If not yet paired, periodically broadcast discovery ping
      if (!peer_paired && !use_broadcast) {
        send_discovery_ping();
        last_ping_sent_us = now_us;
      } else {
        send_latency_ping();
      }
    }

    // Auto benchmark progression
    progress_auto_benchmark();

    // Periodic statistics printout (if not streaming CSV and not during auto-sweep)
    if (!csv_stream_mode && !auto_benchmark_active && (now_ms - last_summary_ms >= summary_interval_ms)) {
      last_summary_ms = now_ms;
      print_periodic_stats();
    }
  }

  // Auto discovery beacon if unset
  if (current_role == ROLE_UNSET && (now_ms - last_summary_ms >= 1000)) {
    last_summary_ms = now_ms;
    send_discovery_ping();
  }

  yield();
}
