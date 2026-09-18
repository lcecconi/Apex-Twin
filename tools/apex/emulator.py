"""
Apex-Dash Desktop Emulator Launcher Submodule
Supports both native SDL2 LVGL v9 binary and fallback PySide6 emulator.
"""

import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent


def run_emulator(args):
    """Launch the native SDL2 or PySide6 Desktop Hardware & Telemetry Emulator."""
    use_python = getattr(args, "python", False)
    native_bin = REPO_ROOT / "emu/native/build/apex_emulator_native"

    if not use_python and native_bin.exists():
        print(f"[apex] Launching Native SDL2 LVGL v9 Emulator: {native_bin}")
        cmd = [str(native_bin)]
        try:
            return subprocess.run(cmd).returncode
        except KeyboardInterrupt:
            return 0

    print("[apex] Launching PySide6 Desktop Telemetry Emulator...")
    from emu.main import MainWindow, QApplication

    app = QApplication.instance() or QApplication(sys.argv[:1])
    app.setStyle("Fusion")

    port = getattr(args, "serial_port", None)
    baud = getattr(args, "baud", 115200)
    replay = getattr(args, "replay", None)
    if replay:
        replay = Path(replay)

    window = MainWindow(port=port, baud=baud, replay_file=replay)
    window.show()

    return app.exec()
