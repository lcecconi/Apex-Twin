#!/usr/bin/env bash
# ==============================================================================
# Apex-Twin Virtual Environment Setup & Activation Script
# Usage:
#     source setup_env.sh
#     . setup_env.sh
# ==============================================================================

# Ensure script is being sourced
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo -e "\033[93m[!] Warning: This script should be sourced so that the virtual environment remains active in your shell.\033[0m"
    echo -e "    Please run: \033[1;96msource setup_env.sh\033[0m or \033[1;96m. setup_env.sh\033[0m\n"
fi

# Locate repository root directory
if [ -n "${BASH_SOURCE[0]}" ]; then
    SCRIPT_PATH="${BASH_SOURCE[0]}"
else
    SCRIPT_PATH="$0"
fi
REPO_ROOT="$(cd "$(dirname "${SCRIPT_PATH}")" && pwd)"
VENV_DIR="${REPO_ROOT}/.venv"
REQ_FILE="${REPO_ROOT}/py-requirements.txt"
TOOLS_DIR="${REPO_ROOT}/tools"

echo -e "\033[96m\033[1m=== Apex-Twin Virtual Environment Setup ===\033[0m"

# 1. Create virtual environment if it doesn't exist
if [ ! -d "${VENV_DIR}" ]; then
    echo -e "\033[92m[+] Creating virtual environment in ${VENV_DIR}...\033[0m"
    if command -v uv >/dev/null 2>&1; then
        uv venv "${VENV_DIR}"
    else
        python3 -m venv "${VENV_DIR}"
    fi
else
    echo -e "\033[90m[*] Existing virtual environment detected at ${VENV_DIR}\033[0m"
fi

# 2. Activate virtual environment
if [ -f "${VENV_DIR}/bin/activate" ]; then
    # shellcheck disable=SC1090
    source "${VENV_DIR}/bin/activate"
    echo -e "\033[92m[+] Virtual environment activated:\033[0m $(which python3)"
else
    echo -e "\033[91m[!] Error: Failed to locate activation script at ${VENV_DIR}/bin/activate\033[0m"
    return 1 2>/dev/null || exit 1
fi

# 3. Install / Sync requirements and install apex-tools in editable mode
if [ -f "${REQ_FILE}" ]; then
    echo -e "\033[92m[+] Installing/syncing dependencies from py-requirements.txt...\033[0m"
    if command -v uv >/dev/null 2>&1; then
        uv pip install -r "${REQ_FILE}"
        echo -e "\033[92m[+] Installing apex package in editable mode (-e tools)...\033[0m"
        uv pip install -e "${TOOLS_DIR}"
    else
        pip install --upgrade pip
        pip install -r "${REQ_FILE}"
        echo -e "\033[92m[+] Installing apex package in editable mode (-e tools)...\033[0m"
        pip install -e "${TOOLS_DIR}"
    fi
else
    echo -e "\033[93m[!] Warning: ${REQ_FILE} not found. Skipping dependency installation.\033[0m"
fi

echo -e "\n\033[96m\033[1m=== Environment Ready! ===\033[0m"
echo -e "You can now run directly from anywhere in your shell:"
echo -e "  \033[92mapex emu\033[0m              - Launch Desktop Display & Telemetry Emulator"
echo -e "  \033[92mapex flash [dash|track]\033[0m - Build & Flash Firmware"
echo -e "  \033[92mapex build [dash|track]\033[0m - Compile Firmware"
echo -e "  \033[92mapex monitor [dash|track]\033[0m - Open Live Serial Terminal Monitor"
echo -e "  \033[92mapex mon [dash|track]\033[0m   - Launch Desktop Flasher & Monitor GUI"
echo -e "  \033[92mapex help\033[0m             - View all available commands"
echo -e "\nTo deactivate later, type: \033[90mdeactivate\033[0m\n"
