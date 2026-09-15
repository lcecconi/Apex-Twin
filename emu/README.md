# Apex-Dash Desktop Hardware & Telemetry Emulator

A high-fidelity Python/PySide6 desktop emulator for **Apex-Dash**, reproducing the 4.2" Reflective LCD (Sitronix ST7305), 7x WS2812 RGB LED strip, physical buttons, and multi-source telemetry engine.

---

## Features

### 🖥️ Display & Hardware Emulation
* **Pixel-accurate $400 \times 300$ Reflective LCD**:
  * Emulates the Sitronix ST7305 reflective display with true monochrome dot rendering and high-contrast dark/silver polarity inversion.
  * **4 Racing HUD Pages**:
    1. **Live Race HUD**: Giant rolling speed, curved progressive RPM tachometer, gear indicator, lap time, predictive time delta ($\pm\Delta$), and split delta.
    2. **Detailed Telemetry**: RPM, Speed, Water Temp, Exhaust Gas Temp (EGT), Lateral G-Force, Battery Voltage, GPS Satellites & RSSI.
    3. **Paddock & GPS Status**: Coordinates (Lat/Lon), HDOP, Fix Type, Current Track detection, Total Engine Hours, and Piston Run Time.
    4. **Data Recall**: Lap summary table with sector splits (S1, S2, S3), maximum speed, max RPM, max water temperature, and best lap highlights.
  * **7 Interactive Setup Submenus**:
    1. Race Setup (Drive type KZ/Rotax/Direct, Over-rev limit, Lap Hold time).
    2. LEDs & Alarms (Shift light threshold, Water temp alarm, EGT alarm, LED brightness, Test pattern).
    3. Track & GPS (Detection mode Auto/Manual, Track file selector, Track learning mode).
    4. Storage & PC (SD Card status, Free space, USB Mass Storage MSC mode).
    5. Display & Backlight (Silver/Black polarity inversion, Backlight PWM brightness).
    6. System & Language (Language selection EN/IT/FR/DE, Units Metric/Imperial, Firmware version).
    7. Hardware Diagnostics (Real-time I2C sensor bus scan, GPS status, RTC clock check).

### 💡 7-LED WS2812 RGB Light Bar
* 5 Progressive RPM Shift LEDs (Green $\rightarrow$ Yellow $\rightarrow$ Red) + Strobe flash at shift point.
* 2 Multi-color Alarm LEDs:
  * Overheat (Flashing Red)
  * High EGT (Flashing Magenta)
  * Low Battery (Flashing Amber)
  * Sector Split Time Delta (Flashing Cyan / Purple)
* Radial light glow simulation matching physical diffused polycarbonate light pipe.

### 🏎️ Multi-Source Telemetry Engine
1. **Autonomous 25 Hz Kart Physics Simulator**: Real-time simulation around Lonato circuit with acceleration, braking, gear shifts, tyre slip angle, and temperature dynamics.
2. **Live Apex-Track Link**: Real-time binary packet reception over USB-serial / ESP-NOW bridge directly from physical track hardware.
3. **Session Log Replayer**: Timeline scrubbing, play/pause, and $0.5\times$ to $10\times$ playback of recorded CSV / GPX race sessions.
4. **Fault & Parameter Injector**: Live sliders for RPM, Water Temp, EGT, G-Force, Battery Voltage, Satellites, and manual S/F & split gate triggers.

---

## Getting Started

### Prerequisites
Make sure Python 3.10+ and dependencies (`PySide6`, `pyserial`) are available.

```bash
# Using uv (fastest, zero-install setup):
uv run --with PySide6,pyserial python3 emu/main.py

# Or using pip / venv:
pip install PySide6 pyserial
python3 emu/main.py
```

### Command Line Options
```bash
# Launch with default physics simulation
python3 emu/main.py

# Launch connected to a physical Apex-Track serial receiver on /dev/ttyUSB0
python3 emu/main.py --serial-port /dev/ttyUSB0 --baud 115200

# Launch and replay a recorded race log
python3 emu/main.py --replay session_lonato_kz.csv
```

---

## Controls & Keyboard Shortcuts

| Button / Hotkey | Action in Race HUD | Action in Setup Menu |
|---|---|---|
| **`Space`** or **`Up`** / BOOT Click | Previous HUD Page | Move Cursor Up |
| **`Esc`** or **`Shift+Space`** / BOOT Hold (>0.5s) | Enter Setup Menu | Exit / Back to Previous Menu |
| **`Enter`** or **`Down`** / KEY Click | Next HUD Page | Move Cursor Down |
| **`I`** or **`Shift+Enter`** / KEY Hold (>0.5s) | Invert Silver/Black Screen | Select / Toggle Item |
| **`1`**, **`2`**, **`3`**, **`4`** | Direct Jump to HUD Page 1..4 | - |
| **`Tab`** | Trigger Start/Finish Gate (New Lap) | - |
| **Right-Click** on Button | Instant Long-Press Trigger | Instant Select / Enter |

---

## Project Structure

```
emu/
├── core/
│   ├── i18n.py             # Multilingual dictionary (EN, IT, FR, DE)
│   ├── log_player.py       # CSV / GPX session log player
│   ├── physics_sim.py      # 25 Hz autonomous kart dynamics sim
│   ├── serial_bridge.py    # USB-serial Apex-Track packet receiver
│   └── telemetry_model.py  # Data models matching firmware telemetry_data.h
├── ui/
│   ├── bezel_widget.py     # CNC steering wheel enclosure & push buttons
│   ├── fault_injector_panel.py # Sliders & fault simulation panel
│   ├── led_bar_widget.py   # WS2812 7-LED shift & alarm strip
│   ├── replayer_panel.py   # Session replayer timeline scrubber
│   └── rlcd_renderer.py    # 400x300 Reflective LCD rendering engine
├── main.py                 # Main application entry point
└── README.md               # User manual & documentation
```
