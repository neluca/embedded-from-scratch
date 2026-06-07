#!/usr/bin/env bash
# ==============================================================================
# QEMU runner for ARM Cortex-M0 (microbit)
# Usage:
#   ./scripts/run_qemu.sh <elf_file> [--gdb] [--no-semihosting]
#   ./scripts/run_qemu.sh lesson_01_bare_metal/build/Debug/lesson_01_bare_metal.elf
#   ./scripts/run_qemu.sh lesson_01_bare_metal/build/Debug/lesson_01_bare_metal.elf --gdb-pause
#
# Environment variables:
#   QEMU   path to qemu-system-arm (default: auto-detect)
# ==============================================================================
set -euo pipefail

# --- Detect QEMU ---
QEMU="${QEMU:-}"
if [ -z "${QEMU}" ]; then
    for candidate in qemu-system-arm qemu-system-arm.exe \
        "/c/Program Files/qemu/qemu-system-arm.exe"; do
        if command -v "$candidate" &>/dev/null || [ -x "$candidate" ]; then
            QEMU="$(command -v "$candidate" || echo "$candidate")"
            break
        fi
    done
fi
if [ -z "${QEMU}" ] || [ ! -x "${QEMU}" ] && ! command -v "${QEMU}" &>/dev/null; then
    echo "Error: qemu-system-arm not found. Install QEMU or set QEMU env var."
    exit 1
fi

# --- Parameters ---
ELF_FILE=""
USE_SEMIHOSTING="true"
ENABLE_GDB=""
GDB_PAUSE=""

for arg in "$@"; do
    case "$arg" in
        --no-semihosting) USE_SEMIHOSTING="false" ;;
        --gdb)            ENABLE_GDB="-s" ;;
        --gdb-pause)      ENABLE_GDB="-s"; GDB_PAUSE="-S" ;;
        *.elf|*.axf)      ELF_FILE="$arg" ;;
    esac
done

if [ -z "${ELF_FILE}" ]; then
    echo "Usage: $0 <elf_file> [--gdb|--gdb-pause] [--no-semihosting]"
    exit 1
fi

# --- Build QEMU command ---
QEMU_CMD=("${QEMU}" -M microbit -nographic -serial null -monitor none)

if [ "${USE_SEMIHOSTING}" = "true" ]; then
    QEMU_CMD+=(-semihosting)
fi
[ -n "${ENABLE_GDB}" ] && QEMU_CMD+=(${ENABLE_GDB})
[ -n "${GDB_PAUSE}" ] && QEMU_CMD+=(${GDB_PAUSE})
QEMU_CMD+=(-kernel "${ELF_FILE}")

echo "============================================"
echo " QEMU ARM Cortex-M0 (microbit)"
echo "============================================"
echo " ELF: ${ELF_FILE}"
echo " SH:  ${USE_SEMIHOSTING}"
echo " GDB: ${ENABLE_GDB:-off}"
echo "============================================"
echo ""

exec "${QEMU_CMD[@]}"
