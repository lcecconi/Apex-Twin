# Apex-Twin

> [!WARNING]
> ### ⚠️ WORK IN PROGRESS — NOT YET FUNCTIONAL
> **Apex-Twin is currently under active development and is NOT yet fully functional or ready for on-track racing use.**  
> Hardware pinouts, communication protocols, firmware interfaces, and telemetry schemas are subject to ongoing changes.

**Apex-Twin** is an open-source, dual-module telemetry, data logging, and live racing display ecosystem for go-karts.

The system is split into two specialized modules:
* **Apex-Track:** Chassis-mounted data acquisition and high-rate sensor logging module.
* **Apex-Dash:** Wireless steering-wheel live display and analysis unit.

---

## 1. System Architecture

The twin-module topology isolates high-vibration sensor wiring and high-voltage ignition leads on the chassis, allowing the steering-wheel unit to operate completely wire-free:

```text
       ┌────────────────────────┐                   ┌────────────────────────┐
       │      APEX-TRACK        │                   │       APEX-DASH        │
       │   (Chassis Module)     │                   │ (Steering Wheel Unit)  │
       │                        │   ESP-NOW 2.4GHz  │                        │
       │  • High-Rate GNSS      │ ════════════════> │  • 4.2" Reflective LCD │
       │  • 6-Axis IMU          │    (Sub-5ms,      │  • WS2812 Shift LEDs   │
       │  • Sparkplug RPM Lead  │     25 Hz)        │  • MicroSD Track DB    │
       │  • Water & EGT Temps   │                   │  • Multilingual UI     │
       └────────────────────────┘                   └────────────────────────┘
```

* **[Apex-Dash Detailed Documentation](file:///home/leonardo/Dev/Apex-Twin/apex-dash/README.md)**
* **[Connector & Pinout Specification](file:///home/leonardo/Dev/Apex-Twin/docs/connector_pinout.md)**
* **[Implementation & Enhancement Plan](file:///home/leonardo/Dev/Apex-Twin/docs/apex_dash_implementation_plan.md)**

---

## 2. Quickstart & Environment Setup

To create/activate a dedicated Python virtual environment and install all dependencies (including the `apex` developer tools package in editable `-e` mode), simply source the setup script:

```bash
# Set up .venv and install all tools & dependencies
source setup_env.sh
```

*(To exit the environment when done, type `deactivate`)*

---

## 3. Apex Command Center (`apex`)

Once the environment is active, the `apex` command is globally available in your shell:

```bash
# Display all available commands and help
apex help

# 1. Desktop Hardware & Telemetry Emulator
apex emu                               # 25 Hz autonomous kart simulation
apex emu --replay session_kz.csv       # Replay recorded session log
apex emu --serial-port /dev/ttyUSB0    # Live link to physical Apex-Track

# 2. Firmware Building & Flashing (Target: dash or track)
apex flash dash                        # Build & flash Apex-Dash (display unit)
apex flash track                       # Build & flash Apex-Track (chassis unit)
apex flash --port /dev/ttyACM0         # Flash to specific serial port

apex build dash                        # Compile Apex-Dash with PlatformIO
apex build track                       # Compile Apex-Track with PlatformIO

# 3. Live Serial Terminal Monitor
apex monitor dash                      # Connect to live serial stream for Dash
apex monitor track                     # Connect to live serial stream for Track

# 4. Graphical Desktop Flasher & Monitor (GUI)
apex mon                               # Launch Desktop Monitor & Flasher GUI (default: Dash)
apex mon track                         # Launch GUI targeting Track module

# 5. Test Suite & Validation
apex test                              # Run unit tests and emulator smoke tests
```

*(You can also use standard `make` targets such as `make emu`, `make flash`, `make build`, `make monitor`, `make mon`, `make test`, or run `uv run apex <command>`).*

---

## 4. Track Management & GPS Database

Apex-Dash uses an open JSON circuit format to define track coordinates, start/finish line gates, and sector splits.

### A. Downloading & Generating Track Files

Generate circuit files using the standalone script in `scripts/`:

```bash
# 1. Generate 15 curated international championship tracks (Lonato, Sarno, Genk, Salbris, Zuera, etc.)
python3 scripts/download_tracks.py --builtin

# 2. Query OpenStreetMap live for all karting circuits in a specific country
python3 scripts/download_tracks.py --country IT
python3 scripts/download_tracks.py --country FR
python3 scripts/download_tracks.py --country DE

# 3. Search OpenStreetMap for a specific circuit by name
python3 scripts/download_tracks.py --search "South Garda"
```

The script saves `.json` circuit files into the local `tracks/` directory.

---

### B. Loading Tracks onto the MicroSD Card

There are two methods to load tracks onto the dashboard:

#### Method 1: Direct SD Card Copy (PC / Card Reader)
1. Insert a FAT32-formatted MicroSD card into your computer.
2. Copy the `.json` files from `tracks/` into the `/tracks/` folder on the root of the SD card.
3. Insert the MicroSD card into the TF slot on the Waveshare ESP32-S3-RLCD board.

#### Method 2: On-Board USB Mass Storage Mode (No Card Removal)
1. Turn on the dashboard and open the Setup Menu (**Long Press BOOT**).
2. Navigate to `4. Storage & PC Sync` $\rightarrow$ `Start PC USB Drive`.
3. Connect the dashboard to your PC using a USB-C cable.
4. The MicroSD card will mount as a standard USB removable drive. Drop your `.json` files into `/tracks/`.
5. Press **BOOT** or **KEY** to safely exit USB mode and return to the dashboard.

---

### C. Track Retrieval & Fallback Hierarchy

When Apex-Dash boots or opens the **Track & GPS Database** menu, it retrieves tracks according to the following hierarchy:

```mermaid
flowchart TD
    Boot([Apex-Dash Boot]) --> CheckSD{Is MicroSD Card<br/>Inserted & Valid?}
    
    CheckSD -- Yes --> ScanSD["Scan /tracks/*.json<br/>on MicroSD"]
    ScanSD --> HasFiles{Are there valid<br/>JSON tracks on SD?}
    
    HasFiles -- Yes --> LoadAll["Load SD Tracks [SD]<br/>+ Merge Built-in Presets [ROM]"]
    HasFiles -- No --> LoadFlash["Fallback to Built-in Flash Presets<br/>(Lonato, Castelletto, Genk, Salbris, Wackersdorf)"]
    
    CheckSD -- No --> LoadFlash
    
    LoadAll --> ActiveTrack["Active Track Ready<br/>(Auto-detect by GPS or Manual Selection)"]
    LoadFlash --> ActiveTrack
```

1. **If the SD Card is missing or empty:** Apex-Dash automatically falls back to its internal ROM track library stored in Flash memory (*South Garda Lonato, Circuito 7 Laghi, Karting Genk, Salbris International, Prokart Wackersdorf*). The display remains 100% operational without requiring an SD card.
2. **If the SD Card is present:** Apex-Dash parses all custom `/tracks/*.json` files and presents them in the track selection menu with an `[SD]` badge.
3. **Live Refresh:** Drivers can trigger **"Reload Tracks from SD"** directly from the menu at the track without restarting the device.

---

## 5. Desktop Hardware & Telemetry Emulator (`emu/`)

A high-fidelity Python/PySide6 desktop emulator is available under [`emu/`](file:///home/leonardo/Dev/Apex-Twin/emu/README.md) for rapid development and testing without physical hardware:

```bash
# Launch emulator via apex command
apex emu

# Or launch connected to a physical Apex-Track serial receiver
apex emu --serial-port /dev/ttyUSB0

# Or replay a recorded race log
apex emu --replay session_lonato_kz.csv
```

* **Pixel-accurate Display:** $400 \times 300$ Reflective LCD with all 4 racing pages and 7 setup menus.
* **RGB LED Strip:** 7-LED WS2812 shift lights and multi-color alarm strobe effects.
* **Multi-Source Telemetry:** Autonomous physics sim around Lonato, live USB-serial bridge, CSV/GPX session replayer, and manual fault injection sliders.
* See **[Emulator Documentation](file:///home/leonardo/Dev/Apex-Twin/emu/README.md)** for full details.


