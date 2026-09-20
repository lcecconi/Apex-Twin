"""
Apex-Twin Unified Command Line Interface
Main console script entry point for the 'apex' command.
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

# Find repository root
REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

# ANSI Colors
C_CYAN = "\033[96m"
C_GREEN = "\033[92m"
C_YELLOW = "\033[93m"
C_RED = "\033[91m"
C_BOLD = "\033[1m"
C_DIM = "\033[2m"
C_RESET = "\033[0m"


def print_banner():
    banner = rf"""{C_CYAN}{C_BOLD}
    ___    ____  _______ _  __      _______       _____ _   __
   /   |  / __ \/ ____/| |/ /     /_  __/ |     / /  _/ | / /
  / /| | / /_/ / __/   |   / ____  / /  | | /| / // / /  |/ / 
 / ___ |/ ____/ /___  /   | /___/ / /   | |/ |/ // / / /|  /  
/_/  |_/_/   /_____/ /_/|_|      /_/    |__/|__/___//_/ |_/   
{C_RESET}{C_DIM}   Dual-Module Karting Telemetry & Racing Display Ecosystem{C_RESET}
"""
    print(banner)


def handle_emu(args):
    """Launch Desktop Telemetry & Display Emulator"""
    from apex.emulator import run_emulator
    return run_emulator(args)


def handle_flash(args):
    """Flash firmware to connected ESP32-S3 module (dash or track)"""
    from apex.flash_monitor import run_cli
    args.flash = True
    return run_cli(args)


def handle_build(args):
    """Compile module firmware with PlatformIO (dash or track)"""
    from apex.flash_monitor import run_cli
    args.build = True
    return run_cli(args)


def handle_monitor(args):
    """Open live serial telemetry monitor for module (dash or track)"""
    from apex.flash_monitor import run_cli
    args.monitor = True
    return run_cli(args)


def handle_util(args):
    """Launch Desktop Utility GUI (Flasher, Monitor & Diagnostics)"""
    from apex.flash_monitor import run_pyside_gui
    target = getattr(args, "target", "dash")
    return run_pyside_gui(initial_target=target)


def handle_test(args):
    """Run syntax checks and offscreen emulator test"""
    print(f"{C_CYAN}{C_BOLD}▶ Verifying Python modules syntax...{C_RESET}")
    files_to_check = [
        "emu/main.py",
        "tools/apex/cli.py",
        "tools/apex/flash_monitor.py",
        "tools/apex/emulator.py",
        "scripts/download_tracks.py",
    ]
    res = subprocess.run([sys.executable, "-m", "py_compile"] + files_to_check, cwd=str(REPO_ROOT))
    if res.returncode != 0:
        print(f"{C_RED}✖ Syntax check failed!{C_RESET}")
        return res.returncode
    print(f"{C_GREEN}✔ Syntax validation passed!{C_RESET}")

    print(f"{C_CYAN}{C_BOLD}▶ Running offscreen emulator smoke test...{C_RESET}")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    smoke_script = "from emu.main import MainWindow, QApplication; import sys; a=QApplication(sys.argv); w=MainWindow(); w._on_tick(); print('✔ Offscreen smoke test OK')"
    res2 = subprocess.run([sys.executable, "-c", smoke_script], env=env, cwd=str(REPO_ROOT))
    return res2.returncode


def handle_setup(args):
    """Install/sync dependencies in editable mode"""
    req_file = REPO_ROOT / "py-requirements.txt"
    tools_dir = REPO_ROOT / "tools"
    has_uv = shutil.which("uv") is not None

    if has_uv:
        print(f"{C_GREEN}{C_BOLD}▶ Syncing dependencies with uv...{C_RESET}")
        subprocess.run(["uv", "pip", "install", "-r", str(req_file)], cwd=str(REPO_ROOT))
        print(f"{C_GREEN}{C_BOLD}▶ Installing apex-tools in editable mode (-e tools)...{C_RESET}")
        return subprocess.run(["uv", "pip", "install", "-e", str(tools_dir)], cwd=str(REPO_ROOT)).returncode
    else:
        print(f"{C_GREEN}{C_BOLD}▶ Syncing dependencies with pip...{C_RESET}")
        subprocess.run([sys.executable, "-m", "pip", "install", "-r", str(req_file)], cwd=str(REPO_ROOT))
        print(f"{C_GREEN}{C_BOLD}▶ Installing apex-tools in editable mode (-e tools)...{C_RESET}")
        return subprocess.run([sys.executable, "-m", "pip", "install", "-e", str(tools_dir)], cwd=str(REPO_ROOT)).returncode


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="apex",
        description="Apex-Twin Unified Command Center",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    subparsers = parser.add_subparsers(dest="command", help="Target modules or actions")

    # =========================================================================
    # 1. Target Subcommand: 'dash' (apex dash <action>)
    # =========================================================================
    p_dash = subparsers.add_parser("dash", help="Manage Apex-Dash display unit (build, flash, monitor, util, emu)")
    dash_subs = p_dash.add_subparsers(dest="action", help="Apex-Dash action")

    # apex dash build
    p_db = dash_subs.add_parser("build", help="Compile Apex-Dash firmware using PlatformIO")
    p_db.set_defaults(func=handle_build, target="dash")

    # apex dash flash
    p_df = dash_subs.add_parser("flash", help="Build & flash firmware to Apex-Dash (USB Serial or Wi-Fi OTA)")
    p_df.add_argument("-p", "--port", type=str, help="Serial port (e.g. /dev/ttyACM0)")
    p_df.add_argument("-b", "--build", action="store_true", default=True, help="Compile before flashing")
    p_df.add_argument("--erase", action="store_true", help="Erase flash memory before upload")
    p_df.add_argument("--ota", action="store_true", help="Flash firmware wirelessly over Wi-Fi in Maintenance Mode")
    p_df.add_argument("--ip", type=str, default=None, help="Target IP address of dash module connected to Wi-Fi")
    p_df.set_defaults(func=handle_flash, target="dash")

    # apex dash monitor
    p_dm = dash_subs.add_parser("monitor", help="Open real-time serial monitor for Apex-Dash")
    p_dm.add_argument("-p", "--port", type=str, help="Serial port (e.g. /dev/ttyACM0)")
    p_dm.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    p_dm.set_defaults(func=handle_monitor, target="dash")

    # apex dash util
    p_du = dash_subs.add_parser("util", help="Launch desktop GUI flasher & monitor utility for Apex-Dash")
    p_du.set_defaults(func=handle_util, target="dash")

    # apex dash emu
    p_de = dash_subs.add_parser("emu", help="Launch desktop hardware & telemetry emulator for Apex-Dash")
    p_de.add_argument("-p", "--serial-port", type=str, default=None, help="Serial port for live Apex-Track link")
    p_de.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default 115200)")
    p_de.add_argument("-r", "--replay", type=Path, default=None, help="Path to CSV/GPX session log to replay")
    p_de.set_defaults(func=handle_emu)

    # =========================================================================
    # 2. Target Subcommand: 'track' (apex track <action>)
    # =========================================================================
    p_track = subparsers.add_parser("track", help="Manage Apex-Track acquisition module (build, flash, monitor, util)")
    track_subs = p_track.add_subparsers(dest="action", help="Apex-Track action")

    # apex track build
    p_tb = track_subs.add_parser("build", help="Compile Apex-Track acquisition firmware using PlatformIO")
    p_tb.set_defaults(func=handle_build, target="track")

    # apex track flash
    p_tf = track_subs.add_parser("flash", help="Build & flash firmware to Apex-Track")
    p_tf.add_argument("-p", "--port", type=str, help="Serial port (e.g. /dev/ttyUSB0)")
    p_tf.add_argument("-b", "--build", action="store_true", default=True, help="Compile before flashing")
    p_tf.add_argument("--erase", action="store_true", help="Erase flash memory before upload")
    p_tf.set_defaults(func=handle_flash, target="track")

    # apex track monitor
    p_tm = track_subs.add_parser("monitor", help="Open real-time serial monitor for Apex-Track")
    p_tm.add_argument("-p", "--port", type=str, help="Serial port (e.g. /dev/ttyUSB0)")
    p_tm.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    p_tm.set_defaults(func=handle_monitor, target="track")

    # apex track util
    p_tu = track_subs.add_parser("util", help="Launch desktop GUI flasher & monitor utility for Apex-Track")
    p_tu.set_defaults(func=handle_util, target="track")

    # =========================================================================
    # 3. Top-Level Standalone Actions (apex <action>)
    # =========================================================================
    # apex emu
    p_emu = subparsers.add_parser("emu", help="Launch desktop hardware & telemetry emulator")
    p_emu.add_argument("-p", "--serial-port", type=str, default=None, help="Serial port for live Apex-Track link")
    p_emu.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default 115200)")
    p_emu.add_argument("-r", "--replay", type=Path, default=None, help="Path to CSV/GPX session log to replay")
    p_emu.set_defaults(func=handle_emu)

    # apex util (GUI)
    p_util_gui = subparsers.add_parser("util", help="Launch desktop GUI flasher & live serial monitor")
    p_util_gui.add_argument("target", nargs="?", default="dash", choices=["dash", "track"], help="Initial target module (dash or track)")
    p_util_gui.set_defaults(func=handle_util)

    # apex test
    p_tst = subparsers.add_parser("test", help="Run unit tests, syntax checks & smoke tests")
    p_tst.set_defaults(func=handle_test)

    # apex setup
    p_set = subparsers.add_parser("setup", help="Install/sync dependencies in editable mode (-e tools)")
    p_set.set_defaults(func=handle_setup)

    return parser


def main():
    parser = build_parser()
    if len(sys.argv) < 2 or sys.argv[1] in ["-h", "--help"]:
        print_banner()
        parser.print_help()
        sys.exit(0)

    # Handle 'apex dash' or 'apex track' without sub-actions by showing sub-help
    if len(sys.argv) == 2 and sys.argv[1] in ["dash", "track"]:
        print_banner()
        sub_help = parser.parse_args([sys.argv[1], "--help"])
        sys.exit(0)

    args = parser.parse_args()
    if hasattr(args, "func"):
        sys.exit(args.func(args) or 0)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()

