"""
Apex-Twin: Firmware Builder, Flasher, and Live Serial Monitor Tool
Targets both Apex-Dash (Steering Wheel unit) and Apex-Track (Chassis unit).
Provides both a modern graphical interface (PySide6) and a command-line interface.
"""

import argparse
import os
import subprocess
import sys
import threading
import time
from pathlib import Path

# Try importing serial
try:
    import serial
    import serial.tools.list_ports
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False


def find_repo_root() -> Path:
    """Find repository root by walking up until platformio or .git is found."""
    p = Path(__file__).resolve().parent
    for _ in range(5):
        if (p / "apex-dash").exists() or (p / ".git").exists():
            return p
        p = p.parent
    return Path.cwd()


REPO_ROOT = find_repo_root()


def resolve_target(target: str) -> tuple[str, Path]:
    """Resolve shorthand target ('dash', 'track') to name and directory."""
    t = (target or "dash").lower().strip()
    if t in ["dash", "apex-dash", "dashboard"]:
        return "Apex-Dash (Display Unit)", REPO_ROOT / "apex-dash"
    elif t in ["track", "apex-track", "chassis"]:
        track_dir = REPO_ROOT / "apex-track"
        # If apex-track folder doesn't have platformio.ini yet, fall back to repo root if configured
        if not (track_dir / "platformio.ini").exists() and (REPO_ROOT / "platformio.ini").exists():
            return "Apex-Track (Chassis Unit)", REPO_ROOT
        return "Apex-Track (Chassis Unit)", track_dir
    else:
        return t, REPO_ROOT / t


def find_platformio_cmd() -> list:
    """Find the best available PlatformIO executable."""
    if subprocess.run(["which", "pio"], capture_output=True).returncode == 0:
        return ["pio"]
    if subprocess.run(["which", "platformio"], capture_output=True).returncode == 0:
        return ["platformio"]
    if subprocess.run(["which", "uvx"], capture_output=True).returncode == 0:
        return ["uvx", "platformio"]
    return ["pio"]


def list_serial_ports() -> list:
    """Return list of available serial port device paths and descriptions."""
    ports = []
    if HAS_SERIAL:
        for p in serial.tools.list_ports.comports():
            desc = f"{p.device} ({p.description})" if p.description else p.device
            ports.append((p.device, desc))
    else:
        # Fallback for Linux /dev/ttyACM* and /dev/ttyUSB*
        for dev in sorted(list(Path("/dev").glob("ttyACM*")) + list(Path("/dev").glob("ttyUSB*"))):
            ports.append((str(dev), str(dev)))

    return ports


def run_command_stream(cmd: list, cwd: Path, output_callback=None, cancel_event=None) -> int:
    """Run a subprocess command and stream its stdout/stderr line by line."""
    if output_callback:
        output_callback(f"\n[RUN] {' '.join(cmd)}\n")

    try:
        process = subprocess.Popen(
            cmd,
            cwd=str(cwd),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            universal_newlines=True
        )

        for line in process.stdout:
            if cancel_event and cancel_event.is_set():
                process.terminate()
                if output_callback:
                    output_callback("\n[CANCELLED] Operation stopped by user.\n")
                return -1
            if output_callback:
                output_callback(line)

        process.wait()
        return process.returncode
    except Exception as e:
        if output_callback:
            output_callback(f"\n[ERROR] Command failed to start: {e}\n")
        return 1


# =========================================================================
# Command Line Interface (CLI Mode)
# =========================================================================
def run_cli(args):
    pio_cmd = find_platformio_cmd()
    raw_target = getattr(args, "target", "dash")
    target_display_name, target_dir = resolve_target(raw_target)

    if not target_dir.exists():
        print(f"[ERROR] Target directory '{target_dir}' does not exist.")
        sys.exit(1)

    print("=======================================================")
    print("   APEX-TWIN: Build, Flash & Serial Monitor           ")
    print("=======================================================")
    print(f"Target:   {target_display_name} ({target_dir.name})")
    print(f"Platform: {' '.join(pio_cmd)}")

    # 1. Port detection if flashing or monitoring
    port = getattr(args, "port", None)
    need_port = getattr(args, "flash", False) or getattr(args, "monitor", False) or getattr(args, "erase", False)
    if need_port and not port:
        ports = list_serial_ports()
        if ports:
            port = ports[0][0]
            print(f"Auto-detected port: {port}")
        else:
            print("[WARN] No serial ports detected! Defaulting to /dev/ttyACM0")
            port = "/dev/ttyACM0"

    # 2. Erase Flash
    if getattr(args, "erase", False):
        print(f"\n[1] Erasing flash memory on {port}...")
        cmd = pio_cmd + ["run", "-d", str(target_dir), "-t", "erase", "--upload-port", port]
        ret = subprocess.run(cmd, cwd=str(REPO_ROOT)).returncode
        if ret != 0:
            print("[ERROR] Flash erase failed.")
            sys.exit(ret)

    # 3. Build Firmware
    do_build = getattr(args, "build", False)
    do_flash = getattr(args, "flash", False)
    do_monitor = getattr(args, "monitor", False)
    do_erase = getattr(args, "erase", False)

    if do_build or (not do_flash and not do_monitor and not do_erase):
        print(f"\n[BUILD] Compiling firmware for {target_display_name}...")
        cmd = pio_cmd + ["run", "-d", str(target_dir)]
        ret = subprocess.run(cmd, cwd=str(REPO_ROOT)).returncode
        if ret != 0:
            print("[ERROR] Build failed.")
            sys.exit(ret)
        print("[SUCCESS] Build completed.")

    # 4. Flash / Upload
    if do_flash:
        print(f"\n[FLASH] Uploading firmware to {port} ({target_display_name})...")
        cmd = pio_cmd + ["run", "-d", str(target_dir), "-t", "upload", "--upload-port", port]
        ret = subprocess.run(cmd, cwd=str(REPO_ROOT)).returncode
        if ret != 0:
            print("[ERROR] Flash upload failed.")
            sys.exit(ret)
        print("[SUCCESS] Firmware successfully flashed!")

    # 5. Serial Monitor
    if do_monitor:
        baud = getattr(args, "baud", 115200)
        print(f"\n[MONITOR] Connecting to {port} @ {baud} baud (Ctrl+C to exit)...")
        time.sleep(1)
        if HAS_SERIAL:
            try:
                ser = serial.Serial(port, baud, timeout=1)
                while True:
                    line = ser.readline()
                    if line:
                        print(line.decode("utf-8", errors="replace"), end="")
            except KeyboardInterrupt:
                print("\n[MONITOR] Disconnected.")
            except Exception as e:
                print(f"[ERROR] Serial connection failed: {e}")
        else:
            cmd = pio_cmd + ["device", "monitor", "-d", str(target_dir), "--port", port, "--baud", str(baud)]
            subprocess.run(cmd, cwd=str(REPO_ROOT))


# =========================================================================
# Graphical User Interface (GUI Mode via PySide6) — 'apex mon'
# =========================================================================
def run_pyside_gui(initial_target: str = "dash"):
    from PySide6.QtWidgets import (
        QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
        QPushButton, QComboBox, QTextEdit, QLabel, QGroupBox,
        QSplitter, QStatusBar, QLineEdit
    )
    from PySide6.QtCore import Qt, QThread, Signal
    from PySide6.QtGui import QTextCursor

    class WorkerThread(QThread):
        output_signal = Signal(str)
        finished_signal = Signal(int)

        def __init__(self, cmd, cwd):
            super().__init__()
            self.cmd = cmd
            self.cwd = cwd
            self.cancel_event = threading.Event()

        def run(self):
            ret = run_command_stream(self.cmd, self.cwd, self.output_signal.emit, self.cancel_event)
            self.finished_signal.emit(ret)

        def cancel(self):
            self.cancel_event.set()

    class SerialThread(QThread):
        line_signal = Signal(str)

        def __init__(self, port, baud):
            super().__init__()
            self.port = port
            self.baud = baud
            self.running = False
            self.ser = None

        def run(self):
            self.running = True
            try:
                self.ser = serial.Serial(self.port, self.baud, timeout=0.1)
                self.line_signal.emit(f"[SERIAL] Connected to {self.port} @ {self.baud} baud\n")
                while self.running:
                    line = self.ser.readline()
                    if line:
                        self.line_signal.emit(line.decode("utf-8", errors="replace"))
            except Exception as e:
                self.line_signal.emit(f"[SERIAL ERROR] {e}\n")
            finally:
                if self.ser and self.ser.is_open:
                    self.ser.close()
                self.line_signal.emit("[SERIAL] Disconnected.\n")

        def stop(self):
            self.running = False

        def send(self, data: str):
            if self.ser and self.ser.is_open:
                self.ser.write(data.encode("utf-8"))

    class ApexMonWindow(QMainWindow):
        def __init__(self, default_target: str):
            super().__init__()
            self.setWindowTitle("Apex-Twin — Firmware Manager & Live Serial Monitor")
            self.resize(980, 680)
            self.pio_cmd = find_platformio_cmd()
            self.worker = None
            self.serial_thread = None
            self.default_target = default_target

            self.init_ui()
            self.refresh_ports()

        def init_ui(self):
            self.setStyleSheet("""
                QMainWindow { background-color: #1e1e24; color: #f0f0f0; }
                QGroupBox { font-weight: bold; border: 1px solid #3d3d4a; border-radius: 6px; margin-top: 10px; padding-top: 10px; color: #61afef; }
                QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
                QPushButton { background-color: #2b2b36; color: #ffffff; border: 1px solid #4d4d5e; padding: 7px 12px; border-radius: 4px; font-weight: bold; font-size: 11px; }
                QPushButton:hover { background-color: #3b3b4a; border-color: #61afef; }
                QPushButton:pressed { background-color: #1e1e24; }
                QPushButton#btn_flash { background-color: #98c379; color: #1e1e24; }
                QPushButton#btn_flash:hover { background-color: #a8d389; }
                QPushButton#btn_build_flash { background-color: #61afef; color: #1e1e24; }
                QPushButton#btn_build_flash:hover { background-color: #71bfef; }
                QComboBox, QLineEdit { background-color: #2b2b36; color: #f0f0f0; border: 1px solid #4d4d5e; padding: 5px; border-radius: 4px; }
                QTextEdit { background-color: #15151a; color: #98c379; font-family: monospace; font-size: 12px; border: 1px solid #333340; border-radius: 4px; }
                QStatusBar { background-color: #18181f; color: #abb2bf; }
            """)

            main_widget = QWidget()
            self.setCentralWidget(main_widget)
            layout = QVBoxLayout(main_widget)

            # Top Header / Port & Target Selection Bar
            top_group = QGroupBox("Target Module & Connection Settings")
            top_layout = QHBoxLayout(top_group)

            top_layout.addWidget(QLabel("Target Module:"))
            self.combo_target = QComboBox()
            self.combo_target.addItem("Apex-Dash (Display Unit)", "apex-dash")
            self.combo_target.addItem("Apex-Track (Chassis Unit)", "apex-track")
            if "track" in self.default_target.lower():
                self.combo_target.setCurrentIndex(1)
            else:
                self.combo_target.setCurrentIndex(0)
            top_layout.addWidget(self.combo_target)

            top_layout.addWidget(QLabel("Serial Port:"))
            self.combo_port = QComboBox()
            self.combo_port.setMinimumWidth(220)
            top_layout.addWidget(self.combo_port)

            btn_refresh = QPushButton("🔄 Refresh")
            btn_refresh.clicked.connect(self.refresh_ports)
            top_layout.addWidget(btn_refresh)

            top_layout.addWidget(QLabel("Baud Rate:"))
            self.combo_baud = QComboBox()
            self.combo_baud.addItems(["115200", "921600", "460800", "230400", "57600"])
            top_layout.addWidget(self.combo_baud)

            layout.addWidget(top_group)

            # Action Buttons Bar
            action_layout = QHBoxLayout()

            self.btn_build = QPushButton("🔨 Build Firmware")
            self.btn_build.clicked.connect(self.action_build)
            action_layout.addWidget(self.btn_build)

            self.btn_flash = QPushButton("⚡ Flash to Device")
            self.btn_flash.setObjectName("btn_flash")
            self.btn_flash.clicked.connect(self.action_flash)
            action_layout.addWidget(self.btn_flash)

            self.btn_build_flash = QPushButton("🚀 Build & Flash")
            self.btn_build_flash.setObjectName("btn_build_flash")
            self.btn_build_flash.clicked.connect(self.action_build_flash)
            action_layout.addWidget(self.btn_build_flash)

            self.btn_erase = QPushButton("🧹 Erase Flash")
            self.btn_erase.clicked.connect(self.action_erase)
            action_layout.addWidget(self.btn_erase)

            layout.addLayout(action_layout)

            # Splitter with Build Console & Serial Monitor
            splitter = QSplitter(Qt.Vertical)

            # Build / Action Console Box
            console_box = QGroupBox("Build & Flash Console Output")
            console_layout = QVBoxLayout(console_box)
            self.txt_console = QTextEdit()
            self.txt_console.setReadOnly(True)
            console_layout.addWidget(self.txt_console)
            splitter.addWidget(console_box)

            # Live Serial Monitor Box
            monitor_box = QGroupBox("Live Serial Monitor (Telemetry & Diagnostic Stream)")
            monitor_layout = QVBoxLayout(monitor_box)

            serial_ctrl_layout = QHBoxLayout()
            self.btn_serial_toggle = QPushButton("▶ Connect Monitor")
            self.btn_serial_toggle.clicked.connect(self.toggle_serial)
            serial_ctrl_layout.addWidget(self.btn_serial_toggle)

            btn_clear_serial = QPushButton("Clear Monitor")
            btn_clear_serial.clicked.connect(lambda: self.txt_serial.clear())
            serial_ctrl_layout.addWidget(btn_clear_serial)

            self.input_serial = QLineEdit()
            self.input_serial.setPlaceholderText("Send serial command...")
            self.input_serial.returnPressed.connect(self.send_serial_data)
            serial_ctrl_layout.addWidget(self.input_serial)

            btn_send = QPushButton("Send")
            btn_send.clicked.connect(self.send_serial_data)
            serial_ctrl_layout.addWidget(btn_send)

            monitor_layout.addLayout(serial_ctrl_layout)

            self.txt_serial = QTextEdit()
            self.txt_serial.setReadOnly(True)
            monitor_layout.addWidget(self.txt_serial)

            splitter.addWidget(monitor_box)
            layout.addWidget(splitter)

            # Status Bar
            self.status_bar = QStatusBar()
            self.setStatusBar(self.status_bar)
            self.status_bar.showMessage("Ready — Select module, port, and click Build & Flash")

        def refresh_ports(self):
            self.combo_port.clear()
            ports = list_serial_ports()
            if ports:
                for dev, desc in ports:
                    self.combo_port.addItem(desc, dev)
                self.status_bar.showMessage(f"Found {len(ports)} serial port(s)")
            else:
                self.combo_port.addItem("No ports detected (/dev/ttyACM0)", "/dev/ttyACM0")
                self.status_bar.showMessage("No serial ports detected.")

        def get_selected_port(self) -> str:
            return self.combo_port.currentData() or "/dev/ttyACM0"

        def get_target_path(self) -> Path:
            raw_target = self.combo_target.currentData() or "apex-dash"
            _, target_path = resolve_target(raw_target)
            return target_path

        def append_console(self, text: str):
            self.txt_console.moveCursor(QTextCursor.End)
            self.txt_console.insertPlainText(text)
            self.txt_console.moveCursor(QTextCursor.End)

        def append_serial(self, text: str):
            self.txt_serial.moveCursor(QTextCursor.End)
            self.txt_serial.insertPlainText(text)
            self.txt_serial.moveCursor(QTextCursor.End)

        def run_pio_task(self, extra_args: list, status_msg: str):
            if self.worker and self.worker.isRunning():
                self.append_console("\n[WARN] Another task is already running!\n")
                return

            target = self.get_target_path()
            if not target.exists():
                self.append_console(f"\n[ERROR] Target directory '{target}' does not exist yet.\n")
                return

            cmd = self.pio_cmd + ["run", "-d", str(target)] + extra_args

            self.status_bar.showMessage(status_msg)
            self.append_console(f"\n=======================================================\n{status_msg}\n")

            self.worker = WorkerThread(cmd, REPO_ROOT)
            self.worker.output_signal.connect(self.append_console)
            self.worker.finished_signal.connect(lambda ret: self.status_bar.showMessage(
                "Task Succeeded!" if ret == 0 else f"Task Failed with exit code {ret}"
            ))
            self.worker.start()

        def action_build(self):
            target_name = self.combo_target.currentText()
            self.run_pio_task([], f"Building Firmware for {target_name}...")

        def action_flash(self):
            port = self.get_selected_port()
            target_name = self.combo_target.currentText()
            self.run_pio_task(["-t", "upload", "--upload-port", port], f"Flashing Firmware to {port} ({target_name})...")

        def action_build_flash(self):
            port = self.get_selected_port()
            target_name = self.combo_target.currentText()
            self.run_pio_task(["-t", "upload", "--upload-port", port], f"Building & Flashing to {port} ({target_name})...")

        def action_erase(self):
            port = self.get_selected_port()
            self.run_pio_task(["-t", "erase", "--upload-port", port], f"Erasing Flash on {port}...")

        def toggle_serial(self):
            if self.serial_thread and self.serial_thread.isRunning():
                self.serial_thread.stop()
                self.btn_serial_toggle.setText("▶ Connect Monitor")
                self.status_bar.showMessage("Serial monitor disconnected.")
            else:
                port = self.get_selected_port()
                baud = int(self.combo_baud.currentText())
                self.serial_thread = SerialThread(port, baud)
                self.serial_thread.line_signal.connect(self.append_serial)
                self.serial_thread.start()
                self.btn_serial_toggle.setText("⏹ Disconnect")
                self.status_bar.showMessage(f"Monitoring {port} @ {baud} baud")

        def send_serial_data(self):
            text = self.input_serial.text()
            if text and self.serial_thread and self.serial_thread.isRunning():
                self.serial_thread.send(text + "\n")
                self.input_serial.clear()

    app = QApplication.instance() or QApplication(sys.argv[:1])
    window = ApexMonWindow(default_target=initial_target)
    window.show()
    sys.exit(app.exec())
