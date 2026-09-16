# Apex-Twin: ESP-NOW Latency & Throughput Benchmark Suite

A standalone, microsecond-accurate test harness designed to explore and characterize **ESP-NOW communication latencies, jitter, packet loss, and throughput** between two ESP32 modules (ESP32-S3 or classic ESP32).

---

## 1. Overview & Architecture

ESP-NOW is a connectionless, peer-to-peer 2.4 GHz protocol developed by Espressif that bypasses the TCP/IP stack to achieve sub-millisecond transmission latencies. In **Apex-Twin**, it serves as the wireless bridge transmitting live 25 Hz telemetry packets from the chassis acquisition module (**Apex-Track**) to the steering display (**Apex-Dash**).

```
+---------------------+                       +---------------------+
|      INITIATOR      |      ESP-NOW Ping     |      RESPONDER      |
|    (Ping Master)    | --------------------> |    (Echo Slave)     |
|                     |     [t_send_us]       |                     |
|  • Timestamps t_send|                       |  • Timestamps t_recv|
|  • Tracks Sequence  |      ESP-NOW Pong     |  • Timestamps t_echo|
|  • Measures RTT     | <-------------------- |  • Echoes payload   |
|  • Calculates Jitter|      [t_echo_us]      |                     |
+---------------------+                       +---------------------+
           │
           │  Round-Trip Time (RTT)  = t_now - t_send
           │  Slave Processing Delay = t_echo - t_recv
           │  Over-The-Air (OTA) RTT = RTT - Slave Delay
           │  Estimated 1-Way Latency = OTA_RTT / 2
           v
  Microsecond ANSI Stats & Histograms
```

### Metrics Measured
* **Full Round-Trip Time (RTT)** ($\mu\text{s}$ and $\text{ms}$): Complete end-to-end cycle including radio stack and processing.
* **Over-The-Air (OTA) Latency** ($\mu\text{s}$): Pure wireless transmission time with slave turnaround delay subtracted.
* **Estimated One-Way Latency** ($\mu\text{s}$): $RTT_{\text{OTA}} / 2$.
* **Jitter ($\sigma$, Standard Deviation)**: Latency consistency and variation.
* **Packet Loss & Error Rate (%)**: Hardware transmission errors and missing sequence IDs.
* **Latency Distribution Histogram**: Microsecond-binned histograms ($<500\mu\text{s}$, $0.5-1\text{ms}$, $1-2\text{ms}$, $2-3\text{ms}$, $3-5\text{ms}$, $5-10\text{ms}$, $>10\text{ms}$).
* **Throughput (KB/s and packets/sec)**: Effective data bandwidth.

---

## 2. Quick Start & Flashing

### Flash 2 ESP32-S3 Devices:

```bash
# Option A: Auto-Dual Mode (Flash identical firmware to both devices; roles auto-negotiate):
uvx platformio run -d tests/espnow-latency -e auto_dual_s3 --target upload --upload-port /dev/ttyACM0
uvx platformio run -d tests/espnow-latency -e auto_dual_s3 --target upload --upload-port /dev/ttyACM1

# Option B: Dedicated Roles:
# 1. Flash Responder (Slave):
uvx platformio run -d tests/espnow-latency -e responder_s3 --target upload --upload-port /dev/ttyACM0

# 2. Flash Initiator (Master):
uvx platformio run -d tests/espnow-latency -e initiator_s3 --target upload --upload-port /dev/ttyACM1
```

*(For classic ESP32-WROOM/DevKit devices, replace `_s3` with `_esp32`, e.g. `-e auto_dual_esp32`).*

---

## 3. Real-Time Interactive Terminal Output

Open the serial monitor on the **Initiator**:
```bash
uvx platformio device monitor -d tests/espnow-latency --port /dev/ttyACM1 --baud 115200
```

The device renders live ANSI tables and histograms every 2 seconds:

```text
+------------------------------------------------------------------------------+
| LATENCY & PACKET STATISTICS SUMMARY (25 Hz, 64 Bytes Payload)                |
+------------------------------------------------------------------------------+
| Sent: 500    | Received: 500    | Lost: 0      (Loss: 0.00%) | Errors: 0     |
+------------------------------------------------------------------------------+
| Metric                   | Min           | Average       | Max           | Jitter (StdDev) |
+--------------------------+---------------+---------------+---------------+---------------+
| Full Round-Trip (RTT)    |    1.12 ms    |    1.38 ms    |    2.45 ms    |    0.18 ms     |
| Over-The-Air (OTA) RTT   |    1.08 ms    |    1.32 ms    |    2.39 ms    |       -       |
| One-Way OTA (Estimated)  |    0.54 ms    |    0.66 ms    |    1.20 ms    |       -       |
| Responder Processing     |    48.0 us    |    54.2 us    |    76.0 us    |       -       |
+------------------------------------------------------------------------------+
| Throughput: 1.56 KB/s (25.0 packets/sec)                                     |
+------------------------------------------------------------------------------+
| LATENCY DISTRIBUTION HISTOGRAM                                               |
|   < 500 us   : 0       [                                        ] |
|  0.5-1.0 ms  : 12      [#                                       ] |
|  1.0-2.0 ms  : 472     [######################################  ] |
|  2.0-3.0 ms  : 16      [#                                       ] |
|  3.0-5.0 ms  : 0       [                                        ] |
|  5.0-10.0 ms : 0       [                                        ] |
|  > 10.0 ms   : 0       [                                        ] |
+------------------------------------------------------------------------------+
```

---

## 4. Interactive Serial Commands

You can send single-character commands to the serial port in real time:

| Command | Action | Description |
| :--- | :--- | :--- |
| **`a`** | **Automated Sweep** | Automatically sweeps 6 rates ($10 \dots 500\text{ Hz}$) $\times$ 4 payload sizes ($32 \dots 240\text{ B}$) and prints a final comparison report. |
| **`m`** / **`i`** | **Set Initiator** | Promotes device to Ping Master role. |
| **`s`** / **`r`** | **Set Responder** | Demotes device to Echo Slave role. |
| **`f <hz>`** | **Set Frequency** | Changes ping rate (e.g. `f 25`, `f 100`, `f 500`). |
| **`p <bytes>`** | **Set Payload** | Changes frame length in bytes (e.g. `p 32`, `p 64`, `p 128`, `p 240`). |
| **`c <channel>`**| **Set WiFi Channel** | Changes 2.4 GHz channel from $1 \dots 13$ to test interference. |
| **`b`** | **Toggle Broadcast** | Toggles Unicast (with 802.11 MAC ACK) vs Broadcast (`FF:FF:FF:FF:FF:FF`). |
| **`l`** | **Toggle Long Range** | Toggles Espressif WiFi Long Range (`LR`) mode. |
| **`v`** | **Toggle CSV Output** | Streams raw samples as CSV (`seq,len,rtt_us,ota_rtt_us,one_way_us,proc_us,tx_ok`). |
| **`z`** | **Reset Counters** | Resets all statistical min/max/average registers. |
| **`h`** / **`?`** | **Help Banner** | Redisplays configuration banner and menu options. |

---

## 5. Python Automation & Plotting Tool

Under `tests/espnow-latency/tools/`, a dedicated host script is provided to capture runs and generate graphs:

```bash
# 1. Run automated test sweep:
uv run python3 tests/espnow-latency/tools/latency_bench.py --port /dev/ttyACM1 --auto-sweep

# 2. Capture a 60-second live test and generate matplotlib plots:
uv run python3 tests/espnow-latency/tools/latency_bench.py --port /dev/ttyACM1 --duration 60 --plot --output run_25hz_64b.csv
```

The plotting engine generates a 4-panel figure:
1. **Latency Timeline**: RTT and OTA vs sequence number.
2. **Distribution Histogram**: Density histogram with Mean and Median markers.
3. **Cumulative CDF**: Latency distribution with 95th and 99th percentile markers.
4. **Summary Box**: Formatted statistics table.
