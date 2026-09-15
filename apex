#!/usr/bin/env python3
"""
Apex-Twin Unified Command Launcher Wrapper
Dispatches to the apex-tools package.
"""

import os
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent
VENV_APEX = REPO_ROOT / ".venv" / "bin" / "apex"

# If running inside or beside a virtualenv where apex console script is installed, prefer it
if VENV_APEX.exists() and os.environ.get("VIRTUAL_ENV"):
    import subprocess
    sys.exit(subprocess.run([str(VENV_APEX)] + sys.argv[1:]).returncode)

# Otherwise ensure tools/ is in sys.path and invoke apex.cli directly
TOOLS_DIR = REPO_ROOT / "tools"
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from apex.cli import main

if __name__ == "__main__":
    main()
