"""
Session Log Replayer (CSV / GPX) for Apex-Dash Emulator
Enables timeline scrubbing and playback of real on-track sessions.
"""

import csv
import math
from pathlib import Path
from typing import List, Optional
from emu.core.telemetry_model import TelemetrySnapshot


class LogPlayer:
    def __init__(self):
        self.records: List[TelemetrySnapshot] = []
        self.current_idx = 0
        self.is_playing = False
        self.speed_multiplier = 1.0
        self.loaded_file: Optional[Path] = None

    def load_csv(self, filepath: Path) -> bool:
        self.records.clear()
        self.current_idx = 0
        self.loaded_file = filepath

        try:
            with open(filepath, "r", encoding="utf-8") as f:
                reader = csv.DictReader(f)
                for row in reader:
                    snap = TelemetrySnapshot()
                    snap.rpm = int(float(row.get("rpm", 6000)))
                    snap.speed_kmh = float(row.get("speed_kmh", 0.0))
                    snap.gear = int(float(row.get("gear", 0)))
                    snap.water_temp_c = float(row.get("water_temp_c", 55.0))
                    snap.exhaust_temp_c = float(row.get("exhaust_temp_c", 500.0))
                    snap.lap_number = int(float(row.get("lap_number", 1)))
                    snap.current_lap_time_ms = int(float(row.get("current_lap_time_ms", 0)))
                    snap.predictive_delta_s = float(row.get("predictive_delta_s", 0.0))
                    snap.lateral_g = float(row.get("lateral_g", 0.0))
                    snap.longitudinal_g = float(row.get("longitudinal_g", 0.0))
                    self.records.append(snap)
            return len(self.records) > 0
        except Exception as e:
            print(f"[LogPlayer] Failed to load CSV {filepath}: {e}")
            return False

    def get_total_frames(self) -> int:
        return len(self.records)

    def get_progress_pct(self) -> float:
        if not self.records:
            return 0.0
        return (self.current_idx / len(self.records)) * 100.0

    def seek_pct(self, pct: float):
        if not self.records:
            return
        self.current_idx = max(0, min(len(self.records) - 1, int((pct / 100.0) * len(self.records))))

    def step(self, step_count: int = 1) -> Optional[TelemetrySnapshot]:
        if not self.records:
            return None
        self.current_idx = (self.current_idx + step_count) % len(self.records)
        return self.records[self.current_idx]
