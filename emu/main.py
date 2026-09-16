#!/usr/bin/env python3
"""
Apex-Dash Desktop Hardware & Telemetry Emulator
High-fidelity desktop emulator for Apex-Dash 4.2" Reflective LCD and WS2812 RGB LED strip.
Top-view dashboard layout with bottom-docked control deck.
"""

import sys
import argparse
from pathlib import Path

# Ensure repository root is in sys.path so 'emu' package is discoverable
REPO_ROOT = Path(__file__).resolve().parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QIcon, QKeySequence, QAction, QFont
from PySide6.QtWidgets import (
    QApplication,
    QMainWindow,
    QWidget,
    QHBoxLayout,
    QVBoxLayout,
    QTabWidget,
    QLabel,
    QStatusBar,
    QPushButton,
    QSplitter,
    QGroupBox,
    QFormLayout,
)

from emu.core.telemetry_model import TelemetrySnapshot, SystemSettings, DriveType, Language
from emu.core.physics_sim import PhysicsSim
from emu.core.serial_bridge import SerialBridge
from emu.core.log_player import LogPlayer
from emu.ui.bezel_widget import BezelWidget
from emu.ui.fault_injector_panel import FaultInjectorPanel
from emu.ui.replayer_panel import ReplayerPanel


class LiveBridgePanel(QWidget):
    """
    Compact horizontal panel displaying live Apex-Track USB-serial / ESP-NOW link status
    """
    def __init__(self, serial_bridge: SerialBridge, parent=None):
        super().__init__(parent)
        self.bridge = serial_bridge

        layout = QHBoxLayout(self)
        layout.setContentsMargins(10, 8, 10, 8)
        layout.setSpacing(14)

        # Status cards
        self.lbl_status = QLabel("⚪ Disconnected (Physics Sim Active)")
        self.lbl_status.setStyleSheet("color: #8b949e; font-weight: bold; font-size: 11px;")
        layout.addWidget(self.lbl_status)

        self.lbl_port = QLabel("Port: None")
        self.lbl_port.setStyleSheet("color: #c9d1d9; font-size: 11px;")
        layout.addWidget(self.lbl_port)

        self.lbl_packets = QLabel("Packets: 0")
        self.lbl_packets.setStyleSheet("color: #c9d1d9; font-size: 11px;")
        layout.addWidget(self.lbl_packets)

        layout.addStretch()

        self.btn_reconnect = QPushButton("🔄 Refresh & Scan Ports")
        self.btn_reconnect.setStyleSheet("""
            QPushButton {
                background: #21262d;
                color: #c9d1d9;
                border: 1px solid #30363d;
                border-radius: 4px;
                padding: 4px 10px;
                font-weight: bold;
                font-size: 10px;
            }
            QPushButton:hover {
                background: #30363d;
                color: #58a6ff;
            }
        """)
        self.btn_reconnect.clicked.connect(self._scan_ports)
        layout.addWidget(self.btn_reconnect)

    def _scan_ports(self):
        ports = self.bridge.get_available_ports()
        if ports:
            self.lbl_port.setText(f"Ports: {', '.join(ports)}")
        else:
            self.lbl_port.setText("Ports: No serial devices detected")

    def update_stats(self):
        if self.bridge.is_connected:
            self.lbl_status.setText("🟢 Connected (Live Apex-Track Link)")
            self.lbl_status.setStyleSheet("color: #3fb950; font-weight: bold; font-size: 11px;")
        else:
            self.lbl_status.setText("⚪ Disconnected (Physics Sim Active)")
            self.lbl_status.setStyleSheet("color: #8b949e; font-weight: bold; font-size: 11px;")
        self.lbl_packets.setText(f"Packets: {self.bridge.total_packets_received}")


class MainWindow(QMainWindow):
    def __init__(self, port: str = None, baud: int = 115200, replay_file: Path = None):
        super().__init__()
        self.setWindowTitle("Apex-Dash Telemetry & Hardware Emulator")
        self.setMinimumSize(780, 620)
        self.resize(880, 720)

        # Core Engines
        self.settings = SystemSettings()
        self.sim = PhysicsSim()
        self.bridge = SerialBridge(port, baud) if port else SerialBridge()
        self.player = LogPlayer()

        if port:
            self.bridge.connect(port, baud)
        if replay_file and replay_file.exists():
            self.player.load_csv(replay_file)
            self.player.is_playing = True

        self.current_telemetry = TelemetrySnapshot()

        self._setup_ui()
        self._setup_shortcuts()

        # 25 Hz (40 ms) Main Update Timer
        self.main_timer = QTimer(self)
        self.main_timer.setInterval(40)
        self.main_timer.timeout.connect(self._on_tick)
        self.main_timer.start()

    def _setup_ui(self):
        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)

        root_layout = QVBoxLayout(central_widget)
        root_layout.setContentsMargins(10, 10, 10, 10)
        root_layout.setSpacing(6)

        # Vertical Splitter: View on TOP, Controls on BOTTOM
        self.splitter = QSplitter(Qt.Vertical)
        self.splitter.setStyleSheet("""
            QSplitter::handle {
                background: #21262d;
                height: 4px;
                border-radius: 2px;
            }
            QSplitter::handle:hover {
                background: #58a6ff;
            }
        """)

        # --- TOP: Steering Wheel Bezel & RLCD Display ---
        top_container = QWidget()
        top_layout = QVBoxLayout(top_container)
        top_layout.setContentsMargins(0, 0, 0, 0)
        top_layout.setSpacing(0)

        self.bezel = BezelWidget(self)
        self.bezel.btn_aux_l.clicked.connect(self._trigger_aux_lap_mark)
        self.bezel.btn_aux_r.clicked.connect(self.bezel.screen.handle_key_short)
        self.bezel.btn_reload.clicked.connect(self._hot_reload)
        self.bezel.btn_reset.clicked.connect(self._reset_sim)
        top_layout.addWidget(self.bezel)

        self.splitter.addWidget(top_container)

        # --- BOTTOM: Tool & Control Tabs ---
        bottom_container = QWidget()
        bottom_layout = QVBoxLayout(bottom_container)
        bottom_layout.setContentsMargins(0, 0, 0, 0)
        bottom_layout.setSpacing(0)

        self.tabs = QTabWidget()
        self.tabs.setMaximumHeight(260)
        self.tabs.setStyleSheet("""
            QTabWidget::pane {
                border: 1px solid #30363d;
                background: #0d1117;
                border-radius: 6px;
            }
            QTabBar::tab {
                background: #161b22;
                color: #8b949e;
                padding: 6px 14px;
                border-top-left-radius: 6px;
                border-top-right-radius: 6px;
                font-size: 11px;
                font-weight: bold;
            }
            QTabBar::tab:selected {
                background: #21262d;
                color: #58a6ff;
                border-bottom: 2px solid #58a6ff;
            }
        """)

        # Tab 1: Fault & Parameter Injector
        self.fault_panel = FaultInjectorPanel(self)
        self.fault_panel.trigger_gate.connect(self._on_gate_triggered)
        self.tabs.addTab(self.fault_panel, "🛠 Parameter Injector")

        # Tab 2: Session Log Replayer
        self.replayer_panel = ReplayerPanel(self.player, self)
        self.tabs.addTab(self.replayer_panel, "📼 Session Replayer")

        # Tab 3: Live Serial Link
        self.bridge_panel = LiveBridgePanel(self.bridge, self)
        self.tabs.addTab(self.bridge_panel, "📡 Live Track Bridge")

        bottom_layout.addWidget(self.tabs)
        self.splitter.addWidget(bottom_container)

        # Proportions: 70% Top Dashboard View, 30% Bottom Controls
        self.splitter.setStretchFactor(0, 4)
        self.splitter.setStretchFactor(1, 1)

        root_layout.addWidget(self.splitter)

        # Status Bar
        self.statusBar = QStatusBar(self)
        self.statusBar.setStyleSheet("background: #161b22; color: #8b949e; font-size: 11px;")
        self.setStatusBar(self.statusBar)

    def _setup_shortcuts(self):
        """Keyboard hotkeys for easy desktop control"""
        # Page Navigation / Up / Down
        act_boot_short = QAction(self)
        act_boot_short.setShortcut(QKeySequence("Space"))
        act_boot_short.triggered.connect(self.bezel.screen.handle_boot_short)
        self.addAction(act_boot_short)

        act_boot_long = QAction(self)
        act_boot_long.setShortcut(QKeySequence("Esc"))
        act_boot_long.triggered.connect(self.bezel.screen.handle_boot_long)
        self.addAction(act_boot_long)

        act_key_short = QAction(self)
        act_key_short.setShortcut(QKeySequence("Return"))
        act_key_short.triggered.connect(self.bezel.screen.handle_key_short)
        self.addAction(act_key_short)

        act_key_long = QAction(self)
        act_key_long.setShortcut(QKeySequence("I"))
        act_key_long.triggered.connect(self.bezel.screen.handle_key_long)
        self.addAction(act_key_long)

        # Hot Reload (F5 or Ctrl+R)
        act_f5 = QAction(self)
        act_f5.setShortcut(QKeySequence("F5"))
        act_f5.triggered.connect(self._hot_reload)
        self.addAction(act_f5)

        act_reload = QAction(self)
        act_reload.setShortcut(QKeySequence("Ctrl+R"))
        act_reload.triggered.connect(self._hot_reload)
        self.addAction(act_reload)

        # Reset Simulation (Ctrl+Shift+R)
        act_reset = QAction(self)
        act_reset.setShortcut(QKeySequence("Ctrl+Shift+R"))
        act_reset.triggered.connect(self._reset_sim)
        self.addAction(act_reset)

        # Direct View Selectors (1, 2, 3, 4, 5)
        for i in range(5):
            act = QAction(self)
            act.setShortcut(QKeySequence(str(i + 1)))
            act.triggered.connect(lambda checked=False, view_idx=i: self._jump_view(view_idx))
            self.addAction(act)

        # Gate trigger (Tab)
        act_gate = QAction(self)
        act_gate.setShortcut(QKeySequence("Tab"))
        act_gate.triggered.connect(lambda: self._on_gate_triggered("sf"))
        self.addAction(act_gate)

    def _hot_reload(self):
        """Hot reload UI modules and repaint without restarting the process"""
        try:
            self.bezel.reload_screen()
            self.statusBar.showMessage("⚡ Hot-reloaded UI & Renderer modules successfully [F5]", 4000)
        except Exception as e:
            self.statusBar.showMessage(f"❌ Hot-reload error: {e}", 6000)

    def _reset_sim(self):
        """Reset physics sim lap counter, sectors, and dynamics"""
        self.sim = PhysicsSim()
        self.statusBar.showMessage("🔄 Simulation session reset", 3000)

    def _jump_view(self, view_idx: int):
        self.bezel.screen.menu_active = False
        self.bezel.screen.current_view = view_idx
        self.bezel.screen.update()

    def _trigger_aux_lap_mark(self):
        self._on_gate_triggered("sf")

    def _on_gate_triggered(self, gate_type: str):
        if gate_type == "sf":
            self.sim.trigger_finish_line()
        elif gate_type == "s1":
            self.sim.current_sector = 2
        elif gate_type == "s2":
            self.sim.current_sector = 3

    def _on_tick(self):
        """Main 25 Hz Engine Tick"""
        mode_str = "PHYSICS SIM"

        if self.fault_panel.manual_override:
            mode_str = "MANUAL OVERRIDE"
            self.current_telemetry = self.fault_panel.get_injected_snapshot()
        elif self.replayer_panel.is_active and self.player.is_playing:
            mode_str = "LOG REPLAY"
            step_count = max(1, int(self.player.speed_multiplier))
            snap = self.player.step(step_count)
            if snap:
                self.current_telemetry = snap
            self.replayer_panel.sync_ui_progress()
        elif self.bridge.is_connected:
            mode_str = "LIVE APEX-TRACK LINK"
            snap = self.bridge.get_latest_snapshot()
            if snap:
                self.current_telemetry = snap
        else:
            mode_str = "AUTONOMOUS PHYSICS SIM"
            self.sim.step(self.settings)
            self.current_telemetry = self.sim.get_snapshot()

        # Update Hardware UI (LEDs + Screen)
        self.bezel.update_hardware(self.current_telemetry, self.settings)

        # Update Live Bridge Stats
        self.bridge_panel.update_stats()

        # Update Status Bar
        lap_ms = self.current_telemetry.current_lap_time_ms
        sec = (lap_ms // 1000) % 60
        mins = (lap_ms // 60000)
        ms = (lap_ms % 1000) // 10
        lap_str = f"L{self.current_telemetry.lap_number} {mins:02d}:{sec:02d}.{ms:02d}"

        self.statusBar.showMessage(
            f"Mode: {mode_str} | 25 Hz Engine | Speed: {self.current_telemetry.speed_kmh:.1f} km/h | "
            f"RPM: {self.current_telemetry.rpm} | Lap: {lap_str} | Track: {self.current_telemetry.current_track_name}"
        )


def main():
    parser = argparse.ArgumentParser(description="Apex-Dash Desktop Telemetry & Hardware Emulator")
    parser.add_argument("-p", "--serial-port", type=str, default=None, help="Serial port for live Apex-Track link")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default 115200)")
    parser.add_argument("-r", "--replay", type=Path, default=None, help="Path to CSV/GPX session log to replay")
    args = parser.parse_args()

    app = QApplication.instance() or QApplication(sys.argv)
    app.setStyle("Fusion")

    window = MainWindow(port=args.serial_port, baud=args.baud, replay_file=args.replay)
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
