# Apex-Twin System Architecture

Apex-Twin is a distributed open-source telemetry and display system designed specifically for go-karting.

---

## 1. The Distributed Twin-Module Concept

Commercial kart data loggers are traditionally designed as monolithic units mounted on the steering wheel. This requires running high-voltage inductive sparkplug leads, water/exhaust thermocouple wiring, power leads, and optical/magnetic lap receivers through the steering column. These cables are subject to extreme vibration, mechanical wear, and ignition electromagnetic interference (EMI).

Apex-Twin tries to decouple **Acquisition & Logging** from **Visualization**:

```
+-------------------------------------------------------------+
|                     APEX-TRACK (Chassis)                    |
|                                                             |
|  • Quectel LC29HEA Dual-Band RTK GNSS (10-25 Hz)           |
|  • 6-Axis IMU (Acc / Gyro)                                 |
|  • Inductive Sparkplug RPM Conditioning                     |
|  • Thermocouple / Water Temp Front-End                      |
|  • MicroSD FAT32 High-Rate Logger                          |
|  • LiFePO4 / Kart 12V Battery Powered                      |
+-------------------------------------------------------------+
                              |
                     ESP-NOW 2.4 GHz
                 (Sub-5ms Ultra-Low Latency)
                              v
+-------------------------------------------------------------+
|                      APEX-DASH (Steering)                   |
|                                                             |
|  • Waveshare 4.2" Reflective LCD (Sitronix ST7305, 400x300)|
|  • ESP32-S3 (Dual-Core LX7 @ 240 MHz, 16MB Flash, 8MB PSRAM)|
|  • Sensirion SHTC3 Ambient Temp & Humidity Sensor          |
|  • NXP PCF85063A Real-Time Clock                           |
|  • 18650 Li-Ion Battery (Zero Wires to Chassis)            |
|  • High-Contrast Racing HUD, Lap Timing & Menu System       |
+-------------------------------------------------------------+
```

---

## 2. Module Responsibilities

### Apex-Track (Acquisition Engine)

* **Location:** Rigidly mounted to the kart chassis or seat stay.
* **Function:** High-frequency acquisition of GNSS ($25\text{ Hz}$ RTK-capable), IMU dynamics, engine RPM pulses, and temperatures.
* **Logging:** Writes raw and calibrated binary telemetry frames to MicroSD.
* **Transmission:** Broadcasts live telemetry frames at $25\text{ Hz}$ via peer-to-peer **ESP-NOW**.

### Apex-Dash (Steering Display)

* **Location:** Mounted on the steering wheel via standard 3-hole / 6-hole kart bracket.
* **Function:** Renders instantaneous driver feedback (RPM bar, digital speed, gear, predictive lap delta, alarms).
* **Power:** Onboard 18650 lithium cell, delivering $>20\text{ hours}$ of continuous operation.
* **Fallback:** Features an internal physics simulation engine for offline demonstration, menu configuration, and testing when unlinked from the chassis.

---

## 3. Related Documentation

* [Apex-Dash Implementation & User Guide](../apex-dash/README.md)
* [Connector & Pinout Specification](connector_pinout.md)
