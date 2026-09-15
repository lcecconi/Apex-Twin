"""
Telemetry Data Model & System Settings for Apex-Dash Emulator
Matches telemetry_data.h in the embedded firmware.
"""

from dataclasses import dataclass, field
from enum import IntEnum
from typing import List, Optional


class DriveType(IntEnum):
    DIRECT = 0        # Single gear (Direct Drive)
    CLUTCH = 1        # Centrifugal clutch (OKJ, Rotax, IAME X30)
    SHIFTER_6SPEED = 2  # KZ / Shifter (1-6 gears)


class RpmDisplayMode(IntEnum):
    BOTH = 0          # Both LCD screen and external LED strip
    DISPLAY_ONLY = 1  # Only on LCD screen
    LEDS_ONLY = 2     # Only on external LED strip


class TrackDetectionMode(IntEnum):
    AUTO = 0
    MANUAL = 1
    LEARNING = 2


class Language(IntEnum):
    LANG_EN = 0
    LANG_IT = 1
    LANG_FR = 2
    LANG_DE = 3


@dataclass
class LapRecord:
    lap_number: int = 1
    lap_time_ms: int = 48520
    split1_ms: int = 16120
    split2_ms: int = 16250
    split3_ms: int = 16150
    max_speed_kmh: float = 124.5
    max_rpm: int = 15820
    min_rpm: int = 5620
    max_water_temp: float = 58.4
    is_best_lap: bool = False


@dataclass
class TelemetrySnapshot:
    timestamp_ms: int = 0

    # Engine metrics
    rpm: int = 5800
    speed_kmh: float = 0.0
    gear: int = 0          # 0 = Neutral, 1-6 = Gears
    water_temp_c: float = 56.5
    exhaust_temp_c: float = 520.0

    # Lapping & Timing
    lap_number: int = 1
    current_lap_time_ms: int = 0
    last_lap_time_ms: int = 0
    best_lap_time_ms: int = 0
    predictive_delta_s: float = 0.0  # Negative = faster, Positive = slower
    current_sector: int = 1         # 1, 2, or 3
    last_split_delta_ms: int = 0

    # GPS & Dynamics
    satellites_visible: int = 14
    gps_fix: int = 3                # 0 = None, 1 = 2D, 2 = 3D, 3 = DGPS/RTK
    hdop: float = 0.85
    lateral_g: float = 0.0
    longitudinal_g: float = 0.0
    latitude: float = 45.388712
    longitude: float = 10.479521
    current_track_name: str = "South Garda (Lonato)"

    # Device & Environmental
    battery_voltage: float = 4.05
    battery_percent: int = 92
    ambient_temp_c: float = 24.5
    ambient_humidity_pct: float = 48.0
    track_module_connected: bool = True
    link_rssi: int = -54
    engine_total_hours_sec: int = 52400   # ~14.5 hours
    piston_hours_sec: int = 18600         # ~5.1 hours

    # Lap History buffer for Data Recall
    lap_history: List[LapRecord] = field(default_factory=list)


@dataclass
class SystemSettings:
    drive_type: DriveType = DriveType.SHIFTER_6SPEED
    max_rpm: int = 16000
    shift_rpm: int = 14000
    over_rev_rpm: int = 15500
    water_temp_alarm_c: float = 65.0
    exhaust_temp_alarm_c: float = 640.0
    low_bat_alarm_v: float = 3.40
    use_kmh: bool = True
    use_celsius: bool = True
    show_speed: bool = True
    inverted_display: bool = True  # High-contrast Black on Silver
    lap_hold_seconds: int = 5
    simulation_mode: bool = True
    track_mode: TrackDetectionMode = TrackDetectionMode.AUTO
    selected_track: str = "South Garda (Lonato)"
    selected_track_file: str = "lonato.json"
    language: Language = Language.LANG_EN
    led_brightness: int = 80       # 0-100%
    rpm_display_mode: RpmDisplayMode = RpmDisplayMode.BOTH
    led_shift_enable: bool = True
    led_alarm_enable: bool = True
    backlight_percent: int = 0     # 0-100%
    warn_trigger_water: bool = True
    warn_trigger_egt: bool = True
    warn_trigger_rev: bool = True
    warn_trigger_battery: bool = True
    warn_trigger_link: bool = True

