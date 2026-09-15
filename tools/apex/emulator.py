"""
Apex-Dash Desktop Emulator Launcher Submodule
"""

import sys
from pathlib import Path


def run_emulator(args):
    """Launch the PySide6 Desktop Hardware & Telemetry Emulator."""
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
