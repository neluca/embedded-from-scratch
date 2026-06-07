# ==============================================================================
# ARM Cortex-M0 bare-metal CMake toolchain file
# ==============================================================================
# Usage:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
#
# Toolchain auto-detection:
#   1. ARM_GCC_PREFIX env var (e.g. /opt/arm-gcc/bin/arm-none-eabi-)
#   2. PATH lookup (arm-none-eabi-gcc must be in PATH)
#   3. Windows default: D:/Bin/arm/gcc-arm-none-eabi-10.3-2021.10/bin/
# ==============================================================================

set(CMAKE_SYSTEM_NAME  Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# --- Toolchain prefix detection ---
if(NOT TOOLCHAIN_PREFIX)
    if(DEFINED ENV{ARM_GCC_PREFIX})
        set(TOOLCHAIN_PREFIX "$ENV{ARM_GCC_PREFIX}")
    elseif(WIN32)
        # Windows default path (author's environment)
        set(TOOLCHAIN_PREFIX "D:/Bin/arm/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-")
    else()
        # Linux/macOS: assume arm-none-eabi-gcc is in PATH
        set(TOOLCHAIN_PREFIX "arm-none-eabi-")
    endif()
    set(TOOLCHAIN_PREFIX "${TOOLCHAIN_PREFIX}" CACHE STRING "ARM GCC toolchain prefix")
endif()

# --- Compiler and tools ---
if(WIN32 AND NOT TOOLCHAIN_PREFIX MATCHES "/bin/")
    # On Windows, PATH-based lookup needs .exe
    set(EXE_SUFFIX ".exe")
elseif(TOOLCHAIN_PREFIX MATCHES "\\.exe$")
    set(EXE_SUFFIX "")
elseif(WIN32 AND EXISTS "${TOOLCHAIN_PREFIX}gcc.exe")
    set(EXE_SUFFIX ".exe")
else()
    set(EXE_SUFFIX "")
endif()

set(CMAKE_C_COMPILER   "${TOOLCHAIN_PREFIX}gcc${EXE_SUFFIX}")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++${EXE_SUFFIX}")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}gcc${EXE_SUFFIX}")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_PREFIX}objcopy${EXE_SUFFIX}")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_PREFIX}objdump${EXE_SUFFIX}")
set(CMAKE_SIZE         "${TOOLCHAIN_PREFIX}size${EXE_SUFFIX}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# --- Flags ---
set(ARCH_FLAGS "-mcpu=cortex-m0 -mthumb")
set(WARN_FLAGS "-Wall -Wextra")

set(CMAKE_C_FLAGS_DEBUG          "-O0 -g3" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE        "-Os -g0" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL     "-Os -g0" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g3" CACHE STRING "" FORCE)

set(CMAKE_C_FLAGS
    "${ARCH_FLAGS} ${WARN_FLAGS} -ffunction-sections -fdata-sections"
    CACHE STRING "C compiler flags" FORCE)
set(CMAKE_ASM_FLAGS
    "${ARCH_FLAGS}"
    CACHE STRING "ASM compiler flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS
    "${ARCH_FLAGS} -Wl,--gc-sections,-Map=${CMAKE_PROJECT_NAME}.map,--print-memory-usage"
    CACHE STRING "Linker flags" FORCE)

message(STATUS "ARM Cortex-M0 Toolchain:")
message(STATUS "  Prefix:  ${TOOLCHAIN_PREFIX}")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER}")
