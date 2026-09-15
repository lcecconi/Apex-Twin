"""
Live Serial / ESP-NOW Bridge for Apex-Dash Emulator
Reads 25 Hz telemetry packets directly from Apex-Track on USB.
"""

import threading
import time
from typing import Callable, List, Optional
from emu.core.telemetry_model import TelemetrySnapshot

try:
    import serial
    import serial.tools.list_ports
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False


class SerialBridge:
    def __init__(self, port: Optional[str] = None, baud: int = 115200, callback: Optional[Callable] = None):
        self.port = port
        self.baud = baud
        self.callback = callback
        self.running = False
        self.thread: Optional[threading.Thread] = None
        self.ser = None
        self.connected = False
        self.total_packets_received = 0
        self.latest_snapshot = TelemetrySnapshot()

    @property
    def is_connected(self) -> bool:
        return self.connected

    @staticmethod
    def get_available_ports() -> List[str]:
        if not HAS_SERIAL:
            return []
        return [p.device for p in serial.tools.list_ports.comports()]

    @staticmethod
    def list_ports() -> List[str]:
        return SerialBridge.get_available_ports()

    def connect(self, port: str, baud: int = 115200) -> bool:
        self.port = port
        self.baud = baud
        return self.start()

    def start(self) -> bool:
        if not HAS_SERIAL or not self.port:
            return False
        self.running = True
        self.thread = threading.Thread(target=self._read_loop, daemon=True)
        self.thread.start()
        return True

    def stop(self):
        self.running = False
        if self.ser and self.ser.is_open:
            try:
                self.ser.close()
            except Exception:
                pass
        self.connected = False

    def get_latest_snapshot(self) -> TelemetrySnapshot:
        return self.latest_snapshot

    def _read_loop(self):
        while self.running:
            try:
                if not self.ser or not self.ser.is_open:
                    self.ser = serial.Serial(self.port, self.baud, timeout=0.5)
                    self.connected = True
                    print(f"[SerialBridge] Connected to {self.port} @ {self.baud}")

                line = self.ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    self.total_packets_received += 1
                    if line.startswith("[TELEMETRY]"):
                        # Parse telemetry values
                        parts = line.split("|")
                        for part in parts:
                            part = part.strip()
                            if part.startswith("RPM:"):
                                self.latest_snapshot.rpm = int(part.replace("RPM:", "").strip())
                            elif part.startswith("Spd:"):
                                val = part.replace("Spd:", "").replace("km/h", "").strip()
                                self.latest_snapshot.speed_kmh = float(val)
                            elif part.startswith("Gear:"):
                                self.latest_snapshot.gear = int(part.replace("Gear:", "").strip())
                            elif part.startswith("H2O:"):
                                val = part.replace("H2O:", "").replace("C", "").strip()
                                self.latest_snapshot.water_temp_c = float(val)
                            elif part.startswith("EGT:"):
                                val = part.replace("EGT:", "").replace("C", "").strip()
                                self.latest_snapshot.exhaust_temp_c = float(val)
                            elif part.startswith("Delta:"):
                                val = part.replace("Delta:", "").replace("s", "").strip()
                                self.latest_snapshot.predictive_delta_s = float(val)
                    if self.callback:
                        self.callback(line)
            except Exception as e:
                self.connected = False
                time.sleep(1.0)
