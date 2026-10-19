# Apex-Track: Chassis Acquisition & High-Rate Telemetry Module

**Apex-Track** is the chassis-mounted data acquisition, GNSS receiver, and high-rate MicroSD logging module of the **Apex-Twin** ecosystem.

---

## 1. Overview & Hardware Features

Rigidly mounted to the kart chassis or seat stay, Apex-Track consolidates all physical sensor wiring and broadcasts live telemetry frames wirelessly to **Apex-Dash** on the steering wheel via low-latency **ESP-NOW (2.4 GHz @ 25 Hz)**.

| Subsystem | Specification / Component |
| :--- | :--- |
| **Microcontroller** | Espressif ESP32-S3 / ESP32-WROOM |
| **GNSS Positioning** | Quectel LC29HEA Dual-Band RTK GNSS ($10 - 25\text{ Hz}$ Multi-Constellation) |
| **Inertial Measurement** | 6-Axis High-G IMU (Lateral & Longitudinal Accelerations, Yaw Gyro) |
| **Engine RPM Sensing** | Inductive Sparkplug Lead Signal Conditioning |
| **Thermal Sensing** | Dual Thermocouple / Analog Front-Ends (Water Coolant & Exhaust Gas Temp EGT) |
| **Local Storage** | MicroSD FAT32 High-Rate Binary Telemetry Logger |
| **Power Supply** | LiFePO4 / Kart 12V Battery Input with Low-Dropout Voltage Regulation |

---

## 2. Wireless Telemetry Link (ESP-NOW)

Apex-Track broadcasts binary virtual CAN-FD packets at $25\text{ Hz}$ using peer-to-peer ESP-NOW:
* **Over-the-Air Latency:** Sub-$2\text{ ms}$ transmission time.
* **Zero Wires to Steering Wheel:** Eliminates cable fatigue, coil entanglement, and ignition interference across the steering column.

The sender broadcasts the existing virtual CAN-FD packet format on ESP-NOW
using the channel selected by the track board's Wi-Fi access point. `Apex-Dash`
must listen on that same channel and consumes the
GNSS and fast-dynamics frames. The current track firmware has GNSS as its
available live telemetry source; additional sensor producers can add frames to
the same packet without changing the dash transport.

Each transmitted frame carries the GPS measurement timestamp in
`timestamp_us`. It is derived from the GPS UTC time in milliseconds and
converted to microseconds.

---

## 3. Related Documentation

* **[Main System Documentation](../README.md)**
* **[Apex-Dash Steering Wheel Display](../apex-dash/README.md)**
* **[Connector & Pinout Specification](../docs/connector_pinout.md)**
* **[ESP-NOW Latency Benchmark Test](../tests/espnow-latency/README.md)**

### Building and uploading on Windows

The `test_idf` environment is configured for the GPS/track board on `COM8`:

```powershell
platformio run -e test_idf --target upload
platformio device monitor -e test_idf
```

The dash project is configured for `COM9`:

```powershell
platformio run -d ../apex-dash -e apex_dash --target upload
platformio device monitor -d ../apex-dash -e apex_dash
```

If the port changes, override it for a single command with
`--upload-port COMx` or `--monitor-port COMx`.
