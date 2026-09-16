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


def handle_mon(args):
    """Launch Desktop Graphical Serial Monitor & Flasher"""
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
    subparsers = parser.add_subparsers(dest="command", help="Available subcommands")

    # 1. emu
    p_emu = subparsers.add_parser("emu", help="Launch desktop hardware & telemetry emulator")
    p_emu.add_argument("-p", "--serial-port", type=str, default=None, help="Serial port for live Apex-Track link")
    p_emu.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default 115200)")
    p_emu.add_argument("-r", "--replay", type=Path, default=None, help="Path to CSV/GPX session log to replay")
    p_emu.set_defaults(func=handle_emu)

    # 2. flash [dash|track]
    p_flash = subparsers.add_parser("flash", help="Build & flash firmware to target module (dash or track)")
    p_flash.add_argument("target", nargs="?", default="dash", choices=["dash", "track"], help="Target module (dash or track, default: dash)")
    p_flash.add_argument("-p", "--port", type=str, help="Serial port (e.g. /dev/ttyACM0)")
    p_flash.add_argument("-b", "--build", action="store_true", default=True, help="Compile before flashing")
    p_flash.add_argument("--erase", action="store_true", help="Erase flash memory before upload")
    p_flash.set_defaults(func=handle_flash)

    # 3. build [dash|track]
    p_build = subparsers.add_parser("build", help="Compile firmware using PlatformIO (dash or track)")
    p_build.add_argument("target", nargs="?", default="dash", choices=["dash", "track"], help="Target module (dash or track, default: dash)")
    p_build.set_defaults(func=handle_build)

    # 4. monitor [dash|track]
    p_mon = subparsers.add_parser("monitor", help="Open real-time terminal serial monitor (dash or track)")
    p_mon.add_argument("target", nargs="?", default="dash", choices=["dash", "track"], help="Target module (dash or track, default: dash)")
    p_mon.add_argument("-p", "--port", type=str, help="Serial port (e.g. /dev/ttyACM0)")
    p_mon.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    p_mon.set_defaults(func=handle_monitor)

    # 5. mon [dash|track] (Desktop GUI Flasher & Monitor)
    p_mon_gui = subparsers.add_parser("mon", help="Launch desktop GUI flasher & live serial monitor")
    p_mon_gui.add_argument("target", nargs="?", default="dash", choices=["dash", "track"], help="Initial target module (dash or track)")
    p_mon_gui.set_defaults(func=handle_mon)

    # 6. test
    p_tst = subparsers.add_parser("test", help="Run unit tests, syntax checks & smoke tests")
    p_tst.set_defaults(func=handle_test)

    # 7. setup
    p_set = subparsers.add_parser("setup", help="Install/sync dependencies in editable mode (-e tools)")
    p_set.set_defaults(func=handle_setup)

    return parser


def main():
    parser = build_parser()
    if len(sys.argv) < 2 or sys.argv[1] in ["-h", "--help"]:
        print_banner()
        parser.print_help()
        sys.exit(0)

    args = parser.parse_args()
    if hasattr(args, "func"):
        sys.exit(args.func(args) or 0)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
