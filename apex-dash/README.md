# Apex-Dash: Open-Source Kart Racing Display Module

**Apex-Dash** is the wireless steering-wheel display module of the **Apex-Twin** telemetry ecosystem, engineered around the **Waveshare ESP32-S3-RLCD-4.2** development board.

---

## 1. System Architecture & Features

| Feature | Traditional Monolithic Dash | Apex-Dash (Apex-Twin) |
| :--- | :--- | :--- |
| **System Topology** | Single unit on steering wheel with all sensor wiring harnesses attached directly to the wheel. | **Twin Architecture:** Chassis-mounted **Apex-Track** acquires high-rate GNSS, IMU, RPM, and temps. Steering-wheel **Apex-Dash** receives live data wirelessly via **ESP-NOW (2.4 GHz)**. |
| **Display Panel** | Custom multi-segment / low-res greyscale LCD with RGB backlight. | **4.2" Reflective LCD (Sitronix ST7305):** Full dot-matrix $400 \times 300$ resolution, zero backlight power draw, sunlight readable, ~24 FPS. |
| **Compute / SoC** | Proprietary low-power microcontroller. | **Espressif ESP32-S3-WROOM-1-N16R8:** Dual-core Xtensa LX7 @ 240 MHz, 16 MB Flash, 8 MB Octal PSRAM. |
| **Offline Operation** | None (requires sensors attached). | **Physics Simulation Stub:** Provides realistic kart racing telemetry (accelerations, gear shifts, lap deltas) when unlinked. |

---

## 2. Displayed Racing Pages

The driver can cycle through 5 dedicated pages by pressing the **KEY Button (GPIO 18)**:

1. **Live Race HUD (Flagship View):**
   * Top full-width tachometer bar ($0 - 16{,}000$ RPM) with shift point marker.
   * Prominent digital Speedometer and KZ Shifter gear indicator ($1 - 6$ or $N$).
   * Active lap time readout with best lap and last lap reference badges.
   * **Predictive Best Lap Delta:** Large bold $+/-$ delta readout (e.g. `+ 0.22`, `- 0.56`) with a 3-pulse inverted flash animation on delta updates and sector checkpoints.
   * **Unified Alarm Panel:** Alternates at $\sim 350\text{ ms}$ between the warning triangle / `WARN` and the highest-priority active alarm icon + text (`H2O HIGH`, `EGT HIGH`, `OVER-REV`, `LOW BATT`, `NO LINK`), or displays `SYSTEM OK`.
   * **Engine & Status Line:** Dual temperatures (Water & EGT), total engine runtime and current session time formatted as `HHhMM`, track name (overwritten with error codes on fault), and low-battery/link status indicator.

2. **Schumacher 3-Speedometer HUD (`PageShumacher`):**
   * **Historical Background:** Faithful recreation of the 3-speedometer setup designed by Michael Schumacher and Benetton Chief Aerodynamicist Willem Toet during the early 1990s (Benetton B192–B194). Schumacher requested this setup to analyze cornering entry/exit performance and evaluate whether different gear ratios or driving lines improved corner apex speed and straight acceleration.
   * **Three Dedicated Speedometer Dials:**
     * **Left Dial ($V_{min}$ / Corner Apex Speed):** Tracks the minimum speed reached in a corner. Holds and displays that speed through the apex and along the entire subsequent straight until the driver hits the brakes again for the next corner.
     * **Center Dial ($V_{act}$ / Real-Time Speed):** Real-time instantaneous speed readout.
     * **Right Dial ($V_{max}$ / Previous Straight Top Speed):** Holds the maximum speed reached on the preceding straight throughout the braking zone, cornering phase, and early exit until the driver has been flat on the throttle for $\ge 1.8\text{ seconds}$ on the new straight.
   * **Bottom Telemetry & Alarm Section:**
     * Left: Active lap time (`LAP 03 [S2]`, `00:48.42`) and predictive best lap delta (`+ 0.22` / `- 0.56`) with inverted flash animation.
     * Right: Unified dynamic alarm panel (`WARN` / `SYSTEM OK`) prioritized by user-defined severity.
     * Bottom Line: Live sensor bar with Water temp, EGT temp, Total Engine Hours (`HHhMM`), Session Time (`HHhMM`), synchronized low-battery blinking, and link status.

3. **Telemetry & Dynamics Monitor:**
   * Segmented tachometer with min/max RPM range.
   * Dual Temperatures: Water Temp & Exhaust Gas Temp (EGT) with configurable alert thresholds.
   * 2D **G-G Diagram** showing real-time lateral and longitudinal cornering acceleration dots.
   * Sector 1 and Sector 2 split delta times.

4. **Paddock & Pre-Race Status:**
   * 10-channel GNSS satellite signal radar (GPS/Galileo constellation bars, fix type, HDOP).
   * Automatic track detection (e.g. `South Garda Karting - Lonato`).
   * Ambient track weather from onboard Sensirion SHTC3 (temperature & relative humidity).
   * Maintenance timers: Total engine hours and piston rebuild countdown.

5. **Data Recall (Post-Session Review):**
   * Top 3 Best Laps comparison table (Lap #, Lap Time, Sector splits, Peak Speed, Peak RPM).
   * Optimal theoretical lap time calculation ($S_1 + S_2 + S_3$).
   * Session consistency index and peak cornering G-forces.

---

## 3. On-Screen Configuration Menu

Holding down the **BOOT Button (GPIO 0)** opens the hierarchical setup menu:

1. **Track Management:** Select active track (Lonato, Salbris, Genk, Wackersdorf, Castelletto, Custom).
2. **Drive & Engine Setup:** Select kart drive type (KZ 6-Speed Shifter, Clutch, Direct Drive), Max RPM scale, Shift Light RPM.
3. **Sensors & Alarms:** Water temp alarm threshold, EGT alarm threshold, Low battery cutoff warning, Alarm priority order configuration.
4. **Display & Units:** Speed unit (km/h vs mph), Temperature (°C vs °F), Inverted color scheme.
5. **Wireless & Apex-Track:** ESP-NOW 2.4 GHz link status, signal RSSI meter, Simulation mode toggle.
6. **Maintenance Counters:** View total engine hours, piston running hours, session odometer.
7. **System Diagnostics:** ESP32-S3 CPU/Flash/PSRAM metrics, SHTC3 ambient status, RTC sync.

---

## 4. Hardware Controls

| Control | Action | Function in Race Views | Function in Menu |
| :--- | :--- | :--- | :--- |
| **KEY (GPIO 18)** | Short Press | Next Page ($1 \to 2 \to 3 \to 4 \to 5 \to 1$) | Scroll Down / Next Item |
| **KEY (GPIO 18)** | Long Press | Invert Display Color Polarity | Select / Confirm / Change Value |
| **BOOT (GPIO 0)** | Short Press | Previous Page ($1 \leftarrow 2 \leftarrow 3 \leftarrow 4 \leftarrow 5 \leftarrow 1$) | Scroll Up / Previous Item |
| **BOOT (GPIO 0)** | Long Press | **Open Setup Menu** | **Back / Exit Menu** |

---

## 5. Building, Flashing & Monitoring
 
### Option A: Using the Unified Apex Command Center (`apex`)
```bash
# Build & Flash firmware (auto-detects serial port)
apex flash dash

# Live Serial Terminal Monitor
apex monitor dash

# Launch Graphical Desktop Flasher & Monitor GUI
apex mon dash
```

### Option B: Using PlatformIO CLI Directly
```bash
# Build
uvx platformio run -d apex-dash

# Flash to Device
uvx platformio run -d apex-dash --target upload --upload-port /dev/ttyACM0

# Live Serial Monitor (115200 baud)
uvx platformio device monitor -d apex-dash --port /dev/ttyACM0 --baud 115200
```
