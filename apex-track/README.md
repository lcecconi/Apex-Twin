# Apex-Track: Chassis Acquisition & Telemetry Module

**Apex-Track** is the chassis-mounted data acquisition, GNSS receiver, and high-rate MicroSD logging module of the **Apex-Twin** ecosystem.

It also functions as an **autonomous hardware telemetry emulator** on standard ESP32 boards (such as the classic **ESP32-WROOM-32**), generating 25 Hz kart dynamics, engine telemetry, and split timing over **ESP-NOW** directly to the **Apex-Dash** steering wheel unit.

---

## 1. Overview & Hardware Architecture

Rigidly mounted to the kart chassis or seat stay, Apex-Track consolidates all physical sensor wiring and broadcasts live Virtual CAN-FD telemetry frames wirelessly to **Apex-Dash** on the steering wheel via low-latency **ESP-NOW (2.4 GHz @ 25 Hz)**:

| Subsystem | Specification / Component |
| :--- | :--- |
| **Microcontroller** | Espressif ESP32-WROOM-32 (Xtensa Dual-Core LX6) or ESP32-S3 |
| **Wireless Protocol** | ESP-NOW 2.4 GHz (Broadcast MAC `FF:FF:FF:FF:FF:FF`, Sub-2ms Latency) |
| **Telemetry Format** | Virtual CAN-FD (`0x100`, `0x110`, `0x120`, `0x200`, `0x210`, `0x220`) packed in `ApexCanFdPacket` (`magic = 0x41434644`) |
| **GNSS Positioning** | Quectel LC29HEA Dual-Band RTK GNSS ($10 - 25\text{ Hz}$ Multi-Constellation) or Simulated Lonato GPS |
| **Inertial Measurement** | 6-Axis High-G IMU (Lateral & Longitudinal Accelerations, Yaw Gyro) |
| **Engine Sensing** | Inductive Sparkplug Lead / KZ 6-Speed Shifter dynamic simulation |
| **Thermal Sensing** | Dual Coolant & Exhaust Gas Temperature (EGT) profiles |

---

## 2. Multi-Rate Virtual CAN-FD Broadcasts

Apex-Track uses a multi-rate transmission engine over ESP-NOW:

* **25 Hz (every 40 ms):** `CAN_ID_FAST_DYNAMICS` (`0x100`) + `CAN_ID_IMU_DYNAMICS` (`0x110`)
  * Engine RPM, ground speed, KZ shifter gear (1–6), status flags, lap counter, lap time, and predictive best lap delta ($\pm\Delta$).
  * Lateral G, Longitudinal G, Vertical G, and Yaw velocity.
* **Event-Driven (Instantaneous):** `CAN_ID_SECTOR_EVENT` (`0x120`)
  * Broadcasted immediately when crossing an intermediate split gate (Sector 1, Sector 2) or the Start/Finish line (New Lap + Best Lap flag).
* **2 Hz (every 500 ms):** `CAN_ID_ENGINE_THERMAL` (`0x200`)
  * Radiator water temperature and exhaust gas temperature (EGT).
* **1 Hz (every 1000 ms):** `CAN_ID_GNSS_STATUS` (`0x210`) + `CAN_ID_CHASSIS_HEALTH` (`0x220`)
  * Coordinates (Lat/Lon), altitude, heading, satellite constellation count, fix type, 12V battery voltage, and engine run hours.
* **Uplink Listener:**
  * Receives `CAN_ID_DASH_STATUS` (`0x310`) and `CAN_ID_DASH_COMMAND` (`0x300`) from Apex-Dash to track steering battery voltage and ambient weather.

---

## 3. Quickstart & Flashing to ESP32-WROOM

### Compile & Flash with the Unified `apex` CLI:

```bash
# 1. Compile firmware
apex track build

# 2. Flash to connected ESP32-WROOM board
apex track flash

# 3. Monitor live serial diagnostics
apex track monitor
```

### Alternatively, using PlatformIO directly:

```bash
# Build
.venv/bin/pio run -d apex-track

# Flash to device on /dev/ttyACM0 or /dev/ttyUSB0
.venv/bin/pio run -d apex-track --target upload --upload-port /dev/ttyUSB0

# Serial monitor (115200 baud)
.venv/bin/pio device monitor -d apex-track --port /dev/ttyUSB0 --baud 115200
```

---

## 4. Related Documentation

* **[Main System Documentation](../README.md)**
* **[Virtual CAN-FD Telemetry Protocol Specification](../docs/telemetry_protocol.md)**
* **[Apex-Dash Steering Wheel Display Documentation](../apex-dash/README.md)**
* **[Connector & Pinout Specification](../docs/connector_pinout.md)**
