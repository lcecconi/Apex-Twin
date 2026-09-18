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

Apex-Track broadcasts binary `ApexTrackTelemetryPacket` frames at $25\text{ Hz}$ using peer-to-peer ESP-NOW:
* **Over-the-Air Latency:** Sub-$2\text{ ms}$ transmission time.
* **Zero Wires to Steering Wheel:** Eliminates cable fatigue, coil entanglement, and ignition interference across the steering column.

---

## 3. Related Documentation

* **[Main System Documentation](file:///home/leonardo/Dev/Apex-Twin/README.md)**
* **[Apex-Dash Steering Wheel Display](file:///home/leonardo/Dev/Apex-Twin/apex-dash/README.md)**
* **[Connector & Pinout Specification](file:///home/leonardo/Dev/Apex-Twin/docs/connector_pinout.md)**
* **[ESP-NOW Latency Benchmark Test](file:///home/leonardo/Dev/Apex-Twin/tests/espnow-latency/README.md)**
