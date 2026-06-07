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
# Priority: 1) ARM_GCC_PREFIX env var, 2) PATH lookup, 3) common install paths
if(NOT TOOLCHAIN_PREFIX)
    if(DEFINED ENV{ARM_GCC_PREFIX})
        set(TOOLCHAIN_PREFIX "$ENV{ARM_GCC_PREFIX}")
        message(STATUS "Using ARM_GCC_PREFIX from environment: ${TOOLCHAIN_PREFIX}")
    else()
        # Try PATH lookup first
        find_program(ARM_GCC_FOUND NAMES arm-none-eabi-gcc arm-none-eabi-gcc.exe)
        if(ARM_GCC_FOUND)
            get_filename_component(ARM_GCC_DIR "${ARM_GCC_FOUND}" DIRECTORY)
            set(TOOLCHAIN_PREFIX "${ARM_GCC_DIR}/arm-none-eabi-")
            message(STATUS "Found ARM GCC in PATH: ${ARM_GCC_FOUND}")
        else()
            # Fallback: common install locations
            set(ARM_GCC_CANDIDATES
                "D:/Bin/arm/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-"
                "C:/Program Files (x86)/GNU Arm Embedded Toolchain/*/bin/arm-none-eabi-"
                "/usr/bin/arm-none-eabi-"
                "/opt/arm-gcc/bin/arm-none-eabi-"
            )
            foreach(CANDIDATE ${ARM_GCC_CANDIDATES})
                file(GLOB MATCHED "${CANDIDATE}*")
                if(MATCHED OR EXISTS "${CANDIDATE}gcc" OR EXISTS "${CANDIDATE}gcc.exe")
                    set(TOOLCHAIN_PREFIX "${CANDIDATE}")
                    message(STATUS "Found ARM GCC at: ${CANDIDATE}")
                    break()
                endif()
            endforeach()
            if(NOT TOOLCHAIN_PREFIX)
                message(FATAL_ERROR
                    "ARM GCC not found! Install gcc-arm-none-eabi or set ARM_GCC_PREFIX env var.\n"
                    "  Linux:   sudo apt install gcc-arm-none-eabi\n"
                    "  macOS:   brew install arm-none-eabi-gcc\n"
                    "  Windows: export ARM_GCC_PREFIX=D:/path/to/bin/arm-none-eabi-")
            endif()
        endif()
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
