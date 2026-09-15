# Apex-Dash: Open-Source Kart Racing Display Module

**Apex-Dash** is the wireless steering-wheel display module of the **Apex-Twin** telemetry ecosystem, engineered around the **Waveshare ESP32-S3-RLCD-4.2** development board and inspired by the industry-standard ** Apex-Dash**.

---

## 1. Key Architectural Differences from Apex-Dash

| Feature |  Apex-Dash | Apex-Dash (Apex-Twin) |
| :--- | :--- | :--- |
| **System Topology** | Single unit on steering wheel with all sensor wiring harnesses attached directly to the wheel. | **Twin Architecture:** Chassis-mounted **Apex-Track** acquires high-rate GNSS, IMU, RPM, and temps. Steering-wheel **Apex-Dash** receives live data wirelessly via **ESP-NOW (2.4 GHz)**. |
| **Display Panel** | Custom multi-segment / low-res greyscale LCD with RGB backlight. | **4.2" Reflective LCD (Sitronix ST7305):** Full dot-matrix $400 \times 300$ resolution, zero backlight power draw, sunlight readable, ~24 FPS. |
| **Compute / SoC** | Proprietary low-power microcontroller. | **Espressif ESP32-S3-WROOM-1-N16R8:** Dual-core Xtensa LX7 @ 240 MHz, 16 MB Flash, 8 MB Octal PSRAM. |
| **Offline Operation** | None (requires sensors attached). | **Physics Simulation Stub:** Provides realistic kart racing telemetry (accelerations, gear shifts, lap deltas) when unlinked. |

---

## 2. Displayed Racing Pages

The driver can cycle through 4 dedicated pages by pressing the **KEY Button (GPIO 18)**:

1. **Live Race HUD (Flagship View):**
   * Top full-width tachometer bar ($0 - 16{,}000$ RPM) with shift point marker and `** SHIFT **` alert.
   * Giant $50\text{ pt}$ digital Speedometer readout (km/h or mph).
   * Shifter gear indicator box ($1 - 6$ or $N$).
   * Massive current lap time readout and session best lap reference.
   * **Predictive Lap Time Delta Bar:** Center horizontal $+/-$ delta bar extending left when faster and right when slower (with numerical $+0.25\text{s}$ readout).
   * Engine status: Radiator Water Temp (with alarm flag if $>65^\circ\text{C}$), EGT Exhaust Temp, Battery voltage & level.

2. **Telemetry & Dynamics Monitor:**
   * Segmented tachometer with min/max RPM range.
   * Dual Temperatures: Water Temp & Exhaust Gas Temp (EGT) with configurable thresholds.
   * 2D **G-G Diagram** showing real-time lateral and longitudinal cornering acceleration dots.
   * Sector 1 and Sector 2 split delta times.

3. **Paddock & Pre-Race Status:**
   * 10-channel GNSS satellite signal radar (GPS/Galileo constellation bars, fix type, HDOP).
   * Automatic track detection (e.g. `South Garda Karting - Lonato`).
   * Ambient track weather from onboard Sensirion SHTC3 (temperature & relative humidity).
   * Maintenance timers: Total engine hours and piston rebuild countdown.

4. **Data Recall (Post-Session Review):**
   * Top 3 Best Laps comparison table (Lap #, Lap Time, Sector splits, Peak Speed, Peak RPM).
   * Optimal theoretical lap time calculation ($S_1 + S_2 + S_3$).
   * Session consistency index and peak cornering G-forces.

---

## 3. On-Screen Configuration Menu

Holding down the **BOOT Button (GPIO 0)** opens the hierarchical setup menu:

1. **Track Management:** Select active track (Lonato, Salbris, Genk, Wackersdorf, Castelletto, Custom).
2. **Drive & Engine Setup:** Select kart drive type (KZ 6-Speed Shifter, Clutch, Direct Drive), Max RPM scale, Shift Light RPM.
3. **Sensors & Alarms:** Water temp alarm threshold, EGT alarm threshold, Low battery cutoff warning.
4. **Display & Units:** Speed unit (km/h vs mph), Temperature (°C vs °F), Inverted color scheme.
5. **Wireless & Apex-Track:** ESP-NOW 2.4 GHz link status, signal RSSI meter, Simulation mode toggle.
6. **Maintenance Counters:** View total engine hours, piston running hours, session odometer.
7. **System Diagnostics:** ESP32-S3 CPU/Flash/PSRAM metrics, SHTC3 ambient status, RTC sync.

---

## 4. Hardware Controls

| Control | Action | Function in Race Views | Function in Menu |
| :--- | :--- | :--- | :--- |
| **KEY (GPIO 18)** | Short Press | Next Page ($1 \to 2 \to 3 \to 4 \to 1$) | Scroll Down / Next Item |
| **KEY (GPIO 18)** | Long Press | Invert Display Color Polarity | Select / Confirm / Change Value |
| **BOOT (GPIO 0)** | Short Press | Previous Page | Scroll Up / Previous Item |
| **BOOT (GPIO 0)** | Long Press | **Open Setup Menu** | **Back / Exit Menu** |

---

## 5. Building & Flashing

```bash
# Build
uvx platformio run -d apex-dash

# Flash to Device
uvx platformio run -d apex-dash --target upload --upload-port /dev/ttyACM0

# Live Serial Monitor (115200 baud)
uvx platformio device monitor -d apex-dash --port /dev/ttyACM0 --baud 115200
```
