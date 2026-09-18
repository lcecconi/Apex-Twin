"""
Autonomous Kart Physics Simulator for Apex-Dash Emulator
Generates realistic 25 Hz telemetry curves (RPM, speed, gears, predictive deltas, G-forces).
"""

import math
import time
from emu.core.telemetry_model import DriveType, LapRecord, SystemSettings, TelemetrySnapshot


class PhysicsSimulator:
    def __init__(self):
        self.lap_start_time = time.time()
        self.lap_duration_s = 48.50  # Typical Lonato lap ~48.5 seconds
        self.current_lap = 1
        self.best_lap_ms = 48420
        self.last_lap_ms = 48680
        self.gear_ratios = [0.0, 3.2, 2.4, 1.85, 1.50, 1.25, 1.05]
        self.final_drive = 4.2
        self.tire_radius_m = 0.14
        self.snapshot = TelemetrySnapshot()
        self.current_sector = 1
        self.lap_history = [
            LapRecord(1, 48680, 16180, 16300, 16200, 122.4, 15600, 5800, 57.2, False),
            LapRecord(2, 48420, 16080, 16150, 16190, 125.1, 15850, 5620, 58.0, True),
            LapRecord(3, 48750, 16220, 16290, 16240, 121.8, 15500, 5900, 58.4, False),
        ]

    def trigger_finish_line(self):
        self.last_lap_ms = int((time.time() - self.lap_start_time) * 1000)
        if self.last_lap_ms > 10000:
            if self.best_lap_ms == 0 or self.last_lap_ms < self.best_lap_ms:
                self.best_lap_ms = self.last_lap_ms
            n_sec = max(1, min(5, getattr(self.snapshot, "total_sectors", 3)))
            sec_time = int(self.last_lap_ms / n_sec)
            sec_times = [sec_time] * n_sec
            self.lap_history.append(
                LapRecord(
                    lap_number=self.current_lap,
                    lap_time_ms=self.last_lap_ms,
                    sector_count=n_sec,
                    sector_times_ms=sec_times,
                    max_speed_kmh=self.snapshot.speed_kmh,
                    max_rpm=self.snapshot.rpm,
                    min_rpm=5600,
                    max_water_temp=self.snapshot.water_temp_c,
                    is_best_lap=(self.last_lap_ms == self.best_lap_ms),
                )
            )
            self.current_lap += 1
        self.lap_start_time = time.time()

    def step(self, settings: SystemSettings = None, dt_s: float = 0.04):
        self.update(self.snapshot, settings or SystemSettings(), dt_s)

    def get_snapshot(self) -> TelemetrySnapshot:
        return self.snapshot

    def update(self, snapshot: TelemetrySnapshot, settings: SystemSettings, dt_s: float = 0.04):
        now = time.time()
        elapsed = (now - self.lap_start_time) % self.lap_duration_s
        snapshot.current_lap_time_ms = int(elapsed * 1000)

        # Handle Lap Completion rollover
        if elapsed < dt_s * 1.5 and snapshot.current_lap_time_ms > 0:
            self.last_lap_ms = int(self.lap_duration_s * 1000)
            snapshot.last_lap_time_ms = self.last_lap_ms
            if self.best_lap_ms == 0 or self.last_lap_ms < self.best_lap_ms:
                self.best_lap_ms = self.last_lap_ms
            snapshot.best_lap_time_ms = self.best_lap_ms
            self.current_lap += 1
            snapshot.lap_number = self.current_lap

        snapshot.last_lap_time_ms = self.last_lap_ms
        snapshot.best_lap_time_ms = self.best_lap_ms
        snapshot.lap_number = self.current_lap

        # Dynamic Sector calculation (1 to total_sectors, capped at 5)
        total_sectors = max(1, min(5, getattr(snapshot, "total_sectors", 3)))
        sector_len = self.lap_duration_s / float(total_sectors)
        sec_idx = int(elapsed / sector_len) + 1
        snapshot.current_sector = min(total_sectors, max(1, sec_idx))

        # Realistic Lonato Circuit Profile Simulation
        norm_lap = elapsed / self.lap_duration_s
        omega = 2.0 * math.pi * norm_lap

        # Cornering vs Straight simulation
        is_braking = math.sin(omega * 4.0) < -0.4
        is_cornering = abs(math.sin(omega * 3.0)) > 0.5

        if is_braking:
            snapshot.longitudinal_g = -1.65 - 0.2 * math.sin(omega * 12.0)
            snapshot.speed_kmh = max(42.0, snapshot.speed_kmh - 85.0 * dt_s)
            snapshot.gear = max(2, int(snapshot.speed_kmh / 22.0))
            snapshot.rpm = int(5800 + (snapshot.speed_kmh * 75.0))
            snapshot.lateral_g = 0.3 * math.sin(omega * 5.0)
        elif is_cornering:
            snapshot.longitudinal_g = 0.15
            snapshot.speed_kmh = 68.0 + 15.0 * math.sin(omega * 6.0)
            snapshot.gear = 3
            snapshot.rpm = int(8500 + 2000.0 * math.sin(omega * 8.0))
            snapshot.lateral_g = 1.85 * math.sin(omega * 3.0)
        else:
            # Full Acceleration Straight
            snapshot.longitudinal_g = 0.95 - (snapshot.speed_kmh / 200.0)
            snapshot.speed_kmh = min(128.0, snapshot.speed_kmh + 45.0 * dt_s)
            snapshot.gear = min(6, max(1, int(snapshot.speed_kmh / 21.0) + 1))
            snapshot.rpm = int(7200 + (snapshot.speed_kmh % 22.0) * 410.0)
            if snapshot.speed_kmh > 120.0:
                snapshot.rpm = min(settings.max_rpm, int(14000 + (snapshot.speed_kmh - 120.0) * 220.0))
            snapshot.lateral_g = 0.1 * math.sin(omega * 7.0)

        # Predictive Delta (updates every 3.0 seconds so animations and values are clearly visible)
        if not hasattr(self, "_last_delta_update_s"):
            self._last_delta_update_s = 0.0
            self._current_sim_delta = -0.22

        if (now - self._last_delta_update_s) >= 3.0:
            self._last_delta_update_s = now
            self._current_sim_delta = round(-0.35 * math.sin(omega * 2.0 + 0.8) + 0.12 * math.cos(omega * 5.0), 2)

        snapshot.predictive_delta_s = self._current_sim_delta

        # Engine & Environmental
        snapshot.water_temp_c = round(56.2 + 2.5 * math.sin(omega * 1.5), 1)
        snapshot.exhaust_temp_c = round(510.0 + (snapshot.rpm / settings.max_rpm) * 140.0, 0)
        snapshot.battery_voltage = 4.05
        snapshot.battery_percent = 92
        snapshot.ambient_temp_c = 24.5
        snapshot.ambient_humidity_pct = 48.0
        snapshot.satellites_visible = 14
        snapshot.hdop = 0.82
        snapshot.track_module_connected = True
        snapshot.link_rssi = -54
        snapshot.lap_history = self.lap_history

        # Accumulate engine runtime when engine is running (rpm > 0)
        if snapshot.rpm > 0:
            if not hasattr(self, "_eng_accum_s"):
                self._eng_accum_s = 0.0
            self._eng_accum_s += dt_s
            if self._eng_accum_s >= 1.0:
                add_sec = int(self._eng_accum_s)
                snapshot.engine_total_hours_sec += add_sec
                self._eng_accum_s -= add_sec

        # Track current session time
        if not hasattr(self, "_session_accum_s"):
            self._session_accum_s = float(snapshot.session_time_sec)
            self._speed_low_s = 0.0

        if snapshot.session_active:
            self._session_accum_s += dt_s
            snapshot.session_time_sec = int(self._session_accum_s)
            if snapshot.speed_kmh < 5.0:
                self._speed_low_s += dt_s
                if self._speed_low_s >= 60.0:
                    snapshot.session_active = False
            else:
                self._speed_low_s = 0.0
        elif snapshot.speed_kmh >= 5.0 and snapshot.lap_number >= 1:
            snapshot.session_active = True
            self._speed_low_s = 0.0


# Compatibility alias
PhysicsSim = PhysicsSimulator
