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


class AlarmType(IntEnum):
    WATER = 0
    EGT = 1
    REV = 2
    BAT = 3
    LINK = 4
    COUNT = 5


class CanMessageId(IntEnum):
    FAST_DYNAMICS = 0x100
    IMU_DYNAMICS = 0x110
    SECTOR_EVENT = 0x120
    ENGINE_THERMAL = 0x200
    GNSS_STATUS = 0x210
    CHASSIS_HEALTH = 0x220
    DASH_COMMAND = 0x300
    DASH_STATUS = 0x310


@dataclass
class CanFdFrame:
    can_id: int
    data: bytes
    len: int = 0
    flags: int = 0
    timestamp_us: int = 0

    def __post_init__(self):
        if not self.len:
            self.len = len(self.data)


MAX_TRACK_SECTORS = 5


@dataclass(init=False)
class LapRecord:
    lap_number: int = 1
    lap_time_ms: int = 48520
    sector_count: int = 3
    sector_times_ms: List[int] = field(default_factory=lambda: [16120, 16250, 16150])
    max_speed_kmh: float = 124.5
    max_rpm: int = 15820
    min_rpm: int = 5620
    max_water_temp: float = 58.4
    is_best_lap: bool = False

    def __init__(
        self,
        lap_number: int = 1,
        lap_time_ms: int = 48520,
        *args,
        sector_count: Optional[int] = None,
        sector_times_ms: Optional[List[int]] = None,
        max_speed_kmh: float = 124.5,
        max_rpm: int = 15820,
        min_rpm: int = 5620,
        max_water_temp: float = 58.4,
        is_best_lap: bool = False,
        **kwargs,
    ):
        self.lap_number = lap_number
        self.lap_time_ms = lap_time_ms
        self.max_speed_kmh = max_speed_kmh
        self.max_rpm = max_rpm
        self.min_rpm = min_rpm
        self.max_water_temp = max_water_temp
        self.is_best_lap = is_best_lap

        if len(args) >= 3 and isinstance(args[0], (int, float)) and isinstance(args[1], (int, float)) and isinstance(args[2], (int, float)):
            self.sector_count = 3
            self.sector_times_ms = [int(args[0]), int(args[1]), int(args[2])]
            if len(args) > 3:
                self.max_speed_kmh = float(args[3])
            if len(args) > 4:
                self.max_rpm = int(args[4])
            if len(args) > 5:
                self.min_rpm = int(args[5])
            if len(args) > 6:
                self.max_water_temp = float(args[6])
            if len(args) > 7:
                self.is_best_lap = bool(args[7])
        elif len(args) == 2 and isinstance(args[1], list):
            self.sector_count = int(args[0])
            self.sector_times_ms = list(args[1])
        else:
            self.sector_count = sector_count if sector_count is not None else (len(sector_times_ms) if sector_times_ms else 3)
            self.sector_times_ms = list(sector_times_ms) if sector_times_ms is not None else [16120, 16250, 16150]

    @property
    def split1_ms(self) -> int:
        return self.sector_times_ms[0] if len(self.sector_times_ms) > 0 else 0

    @property
    def split2_ms(self) -> int:
        return self.sector_times_ms[1] if len(self.sector_times_ms) > 1 else 0

    @property
    def split3_ms(self) -> int:
        return self.sector_times_ms[2] if len(self.sector_times_ms) > 2 else 0


@dataclass
class ChassisTelemetry:
    rpm: int = 5800
    speed_kmh: float = 0.0
    gear: int = 0
    status_flags: int = 0
    current_sector: int = 1
    total_sectors: int = 3
    lap_number: int = 1
    current_lap_time_ms: int = 0
    last_lap_time_ms: int = 0
    best_lap_time_ms: int = 0
    predictive_delta_s: float = 0.0
    last_split_delta_ms: int = 0
    lateral_g: float = 0.0
    longitudinal_g: float = 0.0
    vertical_g: float = 0.0
    yaw_rate_dps: float = 0.0
    water_temp_c: float = 56.5
    exhaust_temp_c: float = 520.0
    head_temp_c: float = 65.0
    latitude: float = 45.388712
    longitude: float = 10.479521
    altitude_m: float = 120.0
    heading_deg: float = 88.5
    satellites_visible: int = 14
    gps_fix: int = 3
    hdop: float = 0.85
    chassis_battery_voltage: float = 12.6
    engine_total_hours_sec: int = 52400
    piston_hours_sec: int = 18600
    track_error_code: int = 0
    connected: bool = True
    link_rssi: int = -54


@dataclass
class DashLocalState:
    ambient_temp_c: float = 24.5
    ambient_humidity_pct: float = 48.0
    battery_voltage: float = 4.05
    battery_percent: int = 92
    rtc_epoch_s: int = 0
    session_time_sec: int = 1122
    session_active: bool = True
    current_track_name: str = "South Garda (Lonato)"


@dataclass
class TelemetrySnapshot:
    timestamp_ms: int = 0

    # Domain models
    chassis: ChassisTelemetry = field(default_factory=ChassisTelemetry)
    local: DashLocalState = field(default_factory=DashLocalState)

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
    current_sector: int = 1         # 1 to total_sectors
    total_sectors: int = 3          # 1 to 5
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
    session_time_sec: int = 1122          # 18m 42s
    session_active: bool = True
    track_error_code: int = 0             # 0 = OK, >0 = Apex-Track error code

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
    alarm_priority: list[int] = field(default_factory=lambda: [0, 1, 2, 3, 4])

