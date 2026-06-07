#!/usr/bin/env bash
# ==============================================================================
# Build script for ARM Cortex-M0 embedded learning project
# Usage:
#   ./scripts/build.sh [phase_dir] [build_type] [-j N]
#   ./scripts/build.sh lesson_01_bare_metal Debug
#   ./scripts/build.sh lesson_01_bare_metal Release -j4
#
# Environment variables:
#   ARM_GCC_PREFIX   ARM GCC prefix (default: auto-detect)
#   CMAKE_GENERATOR  CMake generator (default: Ninja or Unix Makefiles)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TOOLCHAIN="${PROJECT_ROOT}/cmake/arm-none-eabi-gcc.cmake"

# --- Detect cmake ---
CMAKE=""
for candidate in cmake cmake.exe \
    "/d/Program Files/JetBrains/CLion 2025.2.2/bin/cmake/win/x64/bin/cmake.exe" \
    "/c/Program Files/CMake/bin/cmake.exe"; do
    if command -v "$candidate" &>/dev/null || [ -x "$candidate" ]; then
        CMAKE="$(command -v "$candidate" || echo "$candidate")"
        break
    fi
done
if [ -z "${CMAKE}" ]; then
    echo "Error: cmake not found. Install CMake or set PATH."
    exit 1
fi

# --- Detect generator ---
GENERATOR="${CMAKE_GENERATOR:-}"
if [ -z "${GENERATOR}" ]; then
    if command -v ninja &>/dev/null; then
        GENERATOR="Ninja"
    else
        GENERATOR="Unix Makefiles"
    fi
fi

# --- Parameters ---
PHASE_DIR="${1:-lesson_01_bare_metal}"
BUILD_TYPE="${2:-Debug}"
PHASE_PATH="${PROJECT_ROOT}/${PHASE_DIR}"
BUILD_DIR="${PHASE_PATH}/build/${BUILD_TYPE}"

echo "============================================"
echo " ARM Cortex-M0 Build"
echo "============================================"
echo " Phase:       ${PHASE_DIR}"
echo " Build type:  ${BUILD_TYPE}"
echo " Generator:   ${GENERATOR}"
echo " GCC prefix:  ${ARM_GCC_PREFIX:-auto}"
echo "============================================"

if [ ! -d "${PHASE_PATH}" ]; then
    echo "Error: Phase directory not found: ${PHASE_PATH}"
    exit 1
fi

echo ""
echo "[1/2] Configuring..."
"${CMAKE}" -B "${BUILD_DIR}" -S "${PHASE_PATH}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -G "${GENERATOR}"

echo ""
echo "[2/2] Building..."
"${CMAKE}" --build "${BUILD_DIR}"

echo ""
echo "Build: ${BUILD_DIR}/"
echo "Done."
